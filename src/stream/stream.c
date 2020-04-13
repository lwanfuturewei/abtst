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


static int init_partitions[] =
{
	 2, 3, 4, 5, 6, 7, 8, 9,		/* Numa 0 */
	//34,35,36,37,38,39,40,41,		/* Numa 1 */
	//66,67,68,69,70,71,72,73,		/* Numa 2 */
	//98,99,100,101,102,103,104,105,		/* Numa 3 */
};

static void apply_stream_config(abtst_streams *streams)
{
	abtst_stream *stream;
	int i;

	streams->init_xstreams = streams->nr_xstreams = sizeof(init_partitions) / sizeof(int);

	stream = streams->streams;
	for (i = 0; i < streams->init_xstreams; i++)
	{
		stream[init_partitions[i]].used = true;
	}
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

int abtst_init_streams(abtst_streams *streams)
{
	int i;
	int ret;
	abtst_stream *stream;

        streams->max_xstreams = env.nr_cores;
        streams->init_xstreams = streams->nr_xstreams = 1;

	streams->streams = calloc(streams->max_xstreams, sizeof(abtst_stream));
	if (!streams->streams) {
		return -1;
	}

	apply_stream_config(streams);

        /* Create pools */
        //streams->pools = create_pools(streams->max_xstreams);
	//if (!streams->pools)
	//{
	//	goto error;
	//}

	//streams->scheds = abtst_init_sched(streams->max_xstreams);
        //if (!streams->scheds)
        //{
        //        goto error;
        //}

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
