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


static int apply_stream_config(abtst_streams *streams, int *map)
{
	abtst_stream *stream;
	int i;
	int cnt = 0;

	stream = streams->streams;
	for (i = 0; i < env.nr_cores; i++)
	{
		if (map[i] >= 0)
		{
			stream[i].used = true;
			stream[i].part_id = map[i];
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

int abtst_init_streams(abtst_streams *streams, void *p_global)
{
	abtst_global *global = (abtst_global *)p_global;
	int i;
	int ret;
	abtst_stream *stream;

	streams->max_xstreams = env.nr_cores;
	streams->init_xstreams = streams->nr_xstreams = 1;

	streams->streams = calloc(streams->max_xstreams, sizeof(abtst_stream));
	if (!streams->streams) {
		return -1;
	}

	ret = apply_stream_config(streams, get_partition_map(&global->partitions));
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

int abtst_combine_streams(abtst_stream *from, abtst_stream *to)
{
	struct list_head *pos, *n;
	abtst_load *load;
	int count = 0;
	int i;

	if (!from->nr_loads)
	{
		return 0;
	}

	abtst_load **mloads = (abtst_load **)malloc(from->nr_loads * sizeof(abtst_load *));
	list_for_each_safe(pos, n, &from->load_q)
	{
		load = list_entry(pos, abtst_load, list);
//		if (abtst_load_is_migrating(load))
//		{
//			return -1;
//		}
		mloads[count++] = load;
	}

	for (i = 0; i < count; i++)
	{
		abtst_remove_load_from_stream(from, &mloads[i]->list);
		abtst_add_load_to_stream(to, &mloads[i]->list);

		abtst_load_set_migrating(mloads[i], true, to->rank);
	}

	abtst_stream_update_qdepth(to, abtst_stream_get_qdepth(from) + abtst_stream_get_qdepth(to));
	abtst_stream_update_qdepth(from, 0);
	free(mloads);
	return 0;
}

int abtst_rebalance_streams(sort_param *p1, sort_param *p2, uint32_t avg)
{
	abtst_stream *from, *to;
	int ios = p1->ios;
	int count = 0;
	int i;
	struct list_head *pos, *n;
	abtst_load *load, *min_load = NULL;
	int lsize, min_lsize = 0;

	from = p2->stream;
	to = p1->stream;
	//printf("Rebalance streams from %d to %d qdepth %d\n", from->rank, to->rank, avg);

	/* Check whether rebalance is needed */
	if ((p1->ios >= avg) || (p2->ios <= avg))
	{
		return -1;
	}

//	if (p2->ios < MIN_IOS_TO_START_REB)
//	{
//		return -1;
//	}

	if (p2->ios < (p1->ios * 2))
	{
		return -1;
	}

	abtst_load **mloads = (abtst_load **)malloc(from->nr_loads * sizeof(abtst_load *));
	list_for_each_safe(pos, n, &from->load_q)
	{
		load = list_entry(pos, abtst_load, list);
//		if (abtst_load_is_migrating(load))
//		{
//			continue;
//		}

		lsize = abtst_get_load_size(load);
		if (!lsize)
		{
			continue;
		}
		if (!min_lsize || (lsize < min_lsize))
		{
			min_lsize = lsize;
			min_load = load;
		}

		if (lsize && ((ios + lsize) <= avg))
		{
			/* the load is chosen for migration */
			ios += lsize;
			mloads[count++] = load;
		}

		if (ios == avg)
		{
			break;
		}
	}

	if (!count && min_lsize)
	{
		/* Need to consider the case when from stream have a small
		 * number of large loads.
		 */
		if ((p1->ios + min_lsize) < (p2->ios - min_lsize) * 1.5)
		{
			ios += min_lsize;
			mloads[count++] = min_load;
		}

	}

	if (!count)
	{
		return -1;
	}

	for (i = 0; i < count; i++)
	{
		abtst_remove_load_from_stream(from, &mloads[i]->list);
		abtst_add_load_to_stream(to, &mloads[i]->list);

		abtst_load_set_migrating(mloads[i], true, to->rank);
	}

	abtst_stream_update_qdepth(from, p2->ios - (ios - p1->ios));
	abtst_stream_update_qdepth(to, ios);
	free(mloads);

	return 0;
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

static int get_stream_ios(abtst_stream *stream)
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

/*
 * We use moving average algorithm here to calculate qdepth of a stream.
 * This count historical data in our formula.
 */
static uint32_t calc_stream_qdepth(abtst_stream *stream, uint32_t ios)
{
	//return ((ios / 2) + (abtst_stream_get_qdepth(stream) / 2));
	return ios;
}

void abtst_update_streams_stat(abtst_streams *streams)
{
	int i;
	uint32_t ios;
	abtst_stream *stream;
	uint32_t qdepth;

	for (i = 0; i < env.nr_cores; i++)
	{
		stream = &streams->streams[i];
		if (!stream->used)
		{
			continue;
		}

		ios = get_stream_ios(stream);
		qdepth = calc_stream_qdepth(stream, ios);
		abtst_stream_update_qdepth(stream, qdepth);
	}

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

		printf("stream %4d, part %4d, loads %4d, qdepth %8d\n",
	        	i, stream->part_id, stream->nr_loads, stream->stat.total_qdepth);
	}
}
