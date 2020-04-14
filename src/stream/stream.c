/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>

#include "mapping.h"
#include "load.h"
#include "env.h"
#include "stream.h"
#include "sched.h"
#include "abt_st.h"


static int apply_stream_config(abtst_streams *streams, int nr_partitions, abtst_partition *partitions)
{
	abtst_stream *stream;
	int i, j;
	int cnt = 0;
	abtst_partition *partition = partitions;
	int core;

	for (j = 0; j < nr_partitions; j++, partition++)
	{
		stream = streams->streams;
		for (i = 0; i < partition->nr_cores; i++)
		{
			core = partition->partition_cores[i];
			if (streams->partition_map[core] >= 0)
			{
				printf("apply_stream_config core %d already in partition %d\n",
						core, streams->partition_map[core]);
				return -1;
			}

			streams->partition_map[core] = partition->partition_id;
			stream[core].used = true;
			stream[core].part_id = partition->partition_id;
			cnt++;
		}
	}

	streams->init_xstreams = streams->nr_xstreams = cnt;

	return 0;
}

int init_xstream(abtst_stream *stream, uint32_t init_rank)
{
	int ret = 0;

	stream->rank = init_rank;
	INIT_LIST_HEAD(&stream->load_q);
	stream->nr_loads = 0;
	abtst_stream_init_stat(stream);

	/* For the main stream */
	if (init_rank == 0)
	{
		ret = ABT_xstream_self(&stream->xstream);
		if (ret)
		{
			return -1;
		}
		ret = ABT_xstream_get_main_pools(stream->xstream, 1, &stream->pool);
		if (ret)
		{
			printf("ABT_xstream_get_main_pools error %d\n", ret);
			return -1;
		}
		return 0;
	}

	if (!stream->used)
	{
		return 0;
	}

	/* For all used streams */
	ret =  ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPSC,
                                      ABT_TRUE, &stream->pool);
	if (ret)
	{
		printf("ABT_pool_create error %d\n", ret);
		return -1;
	}

	ret = abtst_init_sched(&stream->sched, &stream->pool, (void *)stream);
	if (ret)
	{
		printf("abtst_init_sched error %d\n", ret);
		return -1;
	}

	ret = ABT_xstream_create_with_rank(stream->sched, stream->rank, &stream->xstream);
	if (ret)
	{
		printf("ABT_xstream_create error %d\n", ret);
		return -1;
	}

	return 0;
}

int init_partition_map(abtst_streams *streams)
{
	streams->partition_map = malloc(streams->max_xstreams * sizeof(int));
	if (!streams->partition_map)
	{
		printf("init_partition_map error\n");
		return -1;
	}

	memset(streams->partition_map, 0xff, streams->max_xstreams * sizeof(int));
	return 0;
}

int abtst_init_streams(abtst_streams *streams, void *p_global)
{
	abtst_global *global = (abtst_global *)p_global;
	int i;
	int ret;
	abtst_stream *stream;

	streams->max_xstreams = env.nr_cores;
	streams->init_xstreams = streams->nr_xstreams = 1;

	ret = init_partition_map(streams);
	if (ret)
	{
		return -1;
	}

	streams->streams = calloc(streams->max_xstreams, sizeof(abtst_stream));
	if (!streams->streams) {
		free(streams->partition_map);
		return -1;
	}

	ret = apply_stream_config(streams, global->nr_partitions, global->partitions);
	if (ret)
	{
		goto error;
	}

	stream = streams->streams;
	for (i = 0; i < streams->max_xstreams; i++, stream++)
	{
		ret = init_xstream(stream, i);
		if (ret) {
			goto error;
		}
	}

	return 0;

error:
	free(streams->streams);
	free(streams->partition_map);
	return -1;
}

int abtst_finalize_streams(abtst_streams *streams)
{
	int i;
	int ret = 0;
        abtst_stream *stream;

	if (!streams->streams) {
		return 0;
	}

	/* Join xstreams */
	stream = &streams->streams[1];
	for (i = 1; i < streams->max_xstreams; i++, stream++)
	{
		if (stream->xstream) {
			ret = ABT_xstream_join(stream->xstream);
		}
	}

	/* Free xstreams */
	stream = &streams->streams[1];
	for (i = 1; i < streams->max_xstreams; i++, stream++)
	{
		if (stream->xstream) {
			ret = ABT_xstream_free(&stream->xstream);
			abtst_free_sched(&stream->sched);
		}
	}
	
	return ret;
}

void abtst_free_streams(abtst_streams *streams)
{
	if (streams->streams) {
		free(streams->streams);
		free(streams->partition_map);
	}
}

void abtst_add_load_to_stream(abtst_stream *stream, struct list_head *entry)
{
	list_add(entry, &stream->load_q);
	stream->nr_loads++;
}

void abtst_remove_load_from_stream(abtst_stream *stream, struct list_head *entry)
{
	list_del_init(entry);
	stream->nr_loads--;
}

int abtst_get_stream_ios(abtst_stream *stream)
{
	int count = 0;
	struct list_head *pos, *n;
	abtst_load *load;

	list_for_each_safe(pos, n, &stream->load_q)
	{
		load = list_entry(pos, abtst_load, list);
		count += abtst_get_load_size(load);
	}

	return count;
}

void print_streams(abtst_streams *streams)
{
	int i;
	abtst_stream *stream;

	stream = &streams->streams[0];
	for (i = 0; i < streams->max_xstreams; i++, stream++)
	{
		if (!stream->used) {
			continue;
		}
	        printf("stream %4d, loads %4d, qdepth %8d\n", i, stream->nr_loads, stream->stat.total_qdepth);
	}
}
