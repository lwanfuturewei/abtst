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
#include <assert.h>

#include "abt.h"
#include "abt_st.h"
#include "rebalance.h"
#include "env.h"
#include "list.h"


#define REB_STREAMS 1
#define MIN_IOS_TO_START_REB 100

#ifndef max
#define max(a,b) (((a) (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif


typedef struct sort_param_s
{
	int ios;
	abtst_stream *stream;
} sort_param;

static int cmp_streams(const void *p1, const void *p2);
extern int init_xstream(abtst_stream *stream, uint32_t init_rank);
static void abtst_reb_update_numa_stat(abtst_global *global, int numa_id);


void abtst_reb_process_add_core(abtst_global *global, int core)
{
	abtst_stream *stream;

	stream = &global->streams.streams[core];
	if (stream->used)
	{
		printf("abtst_reb_add_core core %d already used\n", core);
		return;
	}

	assert(stream->rank == (uint32_t)core);
	stream->used = true;
	init_xstream(stream, core);
	/* The rebalancer will do migration later */
}

static int combine_streams(abtst_stream *from, abtst_stream *to)
{
	struct list_head *pos, *n;
	abtst_load *load;
	int count = 0;
	int i;

	abtst_load **mloads = (abtst_load **)malloc(from->nr_loads * sizeof(abtst_load *));
	list_for_each_safe(pos, n, &from->load_q)
	{
		load = list_entry(pos, abtst_load, list);
		if (abtst_load_is_migrating(load))
		{
			continue;
		}
		mloads[count++] = load;
	}

	for (i = 0; i < count; i++)
	{
		abtst_load_set_migrating(mloads[i], true, to->rank);
	}

	free(mloads);
	return 0;
}

void abtst_reb_process_remove_core(abtst_global *global, int core)
{
	abtst_stream *from = NULL;
	abtst_stream *to;
	abtst_stream *stream;
	int numa_id = abtst_env_get_numa_id(core);
	abtst_numa *numa_info = abtst_get_numa_info(numa_id);
	int i;
	int nr_cores;
	int cnt = 0;
	int ret;

	if (core >= env.nr_cores)
	{
		printf("abtst_reb_add_core core %d invalid\n", core);
		return;
	}

	if (core >= 0)
	{
		from = &global->streams.streams[core];
		if (!from->used)
		{
			printf("abtst_reb_add_core core %d already free\n", core);
			return;
		}
	}

	nr_cores = numa_info->end_core - numa_info->start_core + 1;
	sort_param *params = (sort_param *)calloc(nr_cores, sizeof(sort_param));
	if (!params)
	{
		return;
	}

	for (i = numa_info->start_core; i <= numa_info->end_core; i++)
	{
		stream = &global->streams.streams[i];
		if (!stream->used)
		{
			continue;
		}
		if (stream == from)
		{
			continue;
		}
		params[cnt].ios = abtst_stream_get_qdepth(stream);
		params[cnt].stream = stream;
		cnt++;
	}

	if (!cnt)
	{
		/* Need to find a stream from other numa */
		free(params);
		return;
	}

	/* Sort params */
	qsort(params, cnt, sizeof(sort_param), cmp_streams);

	if (from)
	{
		to = params[0].stream;
	}
	else
	{
		from = params[0].stream;
		to = params[1].stream;
	}

	/* Move all the loads from stream to to_stream */
	ret = combine_streams(from, to);
	if (ret)
	{
		printf("combine_streams error %d\n", ret);
	}
	else
	{
		ret = ABT_xstream_join(stream->xstream);
		ABT_xstream_free(&stream->xstream);
		abtst_free_sched(&stream->sched);

		stream->used = false;
	}

	free(params);
}


/* We may need to find a better policy to chose loads for migration:
 * 1) consider original stream;
 * 2) consider the history of stream ios;
 * 3) whether to_stream is a totally new stream;
 * 4) etc.
 */
static int reb_streams(sort_param *p1, sort_param *p2, uint32_t avg)
{
	abtst_stream *from, *to;
	int ios = p1->ios;
	int count = 0;
	int i;
	struct list_head *pos, *n;
	abtst_load *load;
	int lsize;

	from = p2->stream;
	to = p1->stream;

	/* Check whether rebalance is needed */
	if (p1->ios >= avg || p2->ios <= avg)
	{
		return -1;
	}

	if (p2->ios < MIN_IOS_TO_START_REB)
	{
		return -1;
	}

	abtst_load **mloads = (abtst_load **)malloc(from->nr_loads * sizeof(abtst_load *));
	list_for_each_safe(pos, n, &from->load_q)
	{
		load = list_entry(pos, abtst_load, list);
		if (abtst_load_is_migrating(load))
		{
			continue;
		}
		lsize = abtst_get_load_size(load);
		if (lsize && (ios + lsize <= avg))
		{
			/* the load is chosen for migration */
			ios += lsize;
			mloads[count++] = load;
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

static int cmp_streams(const void *p1, const void *p2)
{
	sort_param *s1 = (sort_param *)p1;
	sort_param *s2 = (sort_param *)p2;

	if (s1->ios > s2->ios) {
		return 1;
	} else if (s1->ios < s2->ios) {
		return -1;
	} else {
		return 0;
	}
}

static sort_param * sort_streams(abtst_global *global, int numa_id)
{
	abtst_numa *numa_info = abtst_get_numa_info(numa_id);
	int i;
	abtst_stream *stream;
	int nr_cores;
	int cnt = 0;

	if (!numa_info)
	{
		return NULL;
	}

	nr_cores = numa_info->end_core - numa_info->start_core + 1;

	sort_param *params = (sort_param *)calloc(nr_cores, sizeof(sort_param));
	if (!params)
	{
		return NULL;
	}

	for (i = numa_info->start_core; i <= numa_info->end_core; i++)
	{
		stream = &global->streams.streams[i];
		if (!stream->used)
		{
			continue;
		}
		params[cnt].ios = abtst_stream_get_qdepth(stream);
		params[cnt].stream = stream;
		cnt++;
	}

	/* Sort params */
	qsort(params, cnt, sizeof(sort_param), cmp_streams);

	return params;
}

static void reb_in_numa(abtst_global *global, int numa_id)
{
	sort_param *params;
	abtst_numa_stat *numa_stat = abtst_get_numa_stat(numa_id);
	uint32_t loop = 0;
	int ret;

	while (++loop < numa_stat->used_cores)
	{
		params = sort_streams(global, numa_id);
		if (!params)
		{
			return;
		}

		ret = reb_streams(&params[0], &params[numa_stat->used_cores - 1], numa_stat->avg_qdepth);
		free(params);
		if (ret < 0)
		{
			break;
		}
	}
}

static void reb_numas(abtst_global *global, int from, int to, uint32_t avg_qdepth)
{
	sort_param *params_from, *params_to;
	abtst_numa_stat *numa_stat_from = abtst_get_numa_stat(from);
	abtst_numa_stat *numa_stat_to = abtst_get_numa_stat(to);
	uint32_t loop = 0;
	int ret;

	while (++loop < min(numa_stat_from->used_cores, numa_stat_to->used_cores))
	{
		params_from = sort_streams(global, from);
		if (!params_from)
		{
			return;
		}

		params_to = sort_streams(global, to);
		if (!params_to)
		{
			free(params_from);
			return;
		}

		ret = reb_streams(&params_to[0], &params_from[numa_stat_from->used_cores - 1], avg_qdepth);
		free(params_from);
		free(params_to);
		if (ret < 0)
		{
			break;
		}

		abtst_reb_update_numa_stat(global, from);
		abtst_reb_update_numa_stat(global, to);
	}
}

static void reb_between_numas(abtst_global *global)
{
	int i;
	int from = -1, to = -1;
	uint32_t max_qdepth = 0, min_qdepth;
	uint32_t total_cores = 0;
	uint32_t total_qdepth = 0;
	uint32_t avg_qdepth;
	abtst_numa_stat *numa_stat;

	if (!abtst_is_numa_rebalance_enabled())
	{
		return;
	}


	/* We choose the numa with highest/lowest qdepth to rebalance from/to */
	for (i = 0; i < env.nr_numas; i++)
	{
		numa_stat = abtst_get_numa_stat(i);
		if (!max_qdepth)
		{
			max_qdepth = min_qdepth = numa_stat->avg_qdepth;
		}
		else if (numa_stat->avg_qdepth > max_qdepth)
		{
			max_qdepth = numa_stat->avg_qdepth;
			from = i;
		}
		else if (numa_stat->avg_qdepth < min_qdepth)
		{
			min_qdepth = numa_stat->avg_qdepth;
			to = i;
		}
		total_cores += numa_stat->used_cores;
		total_qdepth += numa_stat->used_cores * numa_stat->avg_qdepth;
	}
	avg_qdepth = total_qdepth / total_cores;

	/* Check whether the max qdepth qualifies to rebalance from */
	if ((max_qdepth < MIN_IOS_TO_START_REB) || (max_qdepth < 2 * avg_qdepth))
	{
		return;
	}

	reb_numas(global, from, to, avg_qdepth);
}

/*
 * We use moving average algorithm here to calculate qdepth of a stream.
 * This count historical data in our formula.
 */
static inline uint32_t reb_calc_qdepth(abtst_stream *stream, uint32_t ios)
{
	//return ((ios / 2) + (abtst_stream_get_qdepth(stream) / 2));
	return ios;
}

static void abtst_reb_update_numa_stat(abtst_global *global, int numa_id)
{
	abtst_stream *stream;
	uint32_t qdepth = 0;
	int j;
	abtst_numa *numa_info;
	uint32_t cnt = 0;

	numa_info = abtst_get_numa_info(numa_id);

	for (j = numa_info->start_core; j <= numa_info->end_core; j++)
	{
		stream = &global->streams.streams[j];
		if (!stream->used)
		{
			continue;
		}
		qdepth += abtst_stream_get_qdepth(stream);
		cnt++;
	}

	abtst_set_numa_stat(numa_id, cnt, qdepth/cnt);
}

static void abtst_reb_update_stat(abtst_global *global)
{
	int i;
	uint32_t ios;
	abtst_stream *stream;
	uint32_t qdepth;

	for (i = 0; i < env.nr_cores; i++)
	{
		stream = &global->streams.streams[i];
		if (!stream->used)
		{
			continue;
		}

		ios = abtst_get_stream_ios(stream);
		qdepth = reb_calc_qdepth(stream, ios);
		abtst_stream_update_qdepth(stream, qdepth);
	}

	for (i = 0; i < env.nr_numas; i++)
	{
		abtst_reb_update_numa_stat(global, i);
	}

}

static void *reb_func(void *arg)
{
	int i;
	abtst_global *global = (abtst_global *)arg;

	printf("rebalance started\n");
	while (!abtst_is_system_stopping())
	{
		abtst_reb_update_stat(global);

		/* First process requests */
		abtst_reb_process_requests(global);

		for (i = 0; i < env.nr_numas; i++)
		{
			reb_in_numa(global, i);
		}

		reb_between_numas(global);

		sleep(1);
	}

	printf("rebalance ended\n");
	return NULL;
}

int abtst_init_rebalance(abtst_rebalance *reb, void *param)
{
	int ret;

	atomic_init(&reb->in_use, 0);
	reb->param = param;
	INIT_LIST_HEAD(&reb->req_q);
	spinlock_init(&reb->lock);

	ret = pthread_create(&reb->thread, NULL, reb_func, param);
	if (ret)
	{
		printf("abtst_init_rebalance error %d\n", ret);
		return -1;
	}

	return 0;
}

void abtst_free_rebalance(abtst_rebalance *reb)
{
	int ret;

	ret = pthread_join(reb->thread, NULL);
	if (ret)
	{
		printf("abtst_free_rebalance error %d\n", ret);
	}
}
