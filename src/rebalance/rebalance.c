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


static int cmp_func(const void *p1, const void *p2);
extern int init_xstream(abtst_stream *stream, uint32_t init_rank);
extern void abtst_update_partition_stats(abtst_global *global);
extern void abtst_update_numa_stats(abtst_global *global);


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
	qsort(params, cnt, sizeof(sort_param), cmp_func);

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
	ret = abtst_combine_streams(from, to);
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

static int cmp_func(const void *p1, const void *p2)
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

static sort_param * sort_streams(abtst_global *global, int numa_id, int partition_id)
{
	abtst_numa *numa_info;
	int i;
	int start, end;
	abtst_stream *stream;
	int nr_cores;
	int cnt = 0;

	if (numa_id == -1)
	{
		start = 0;
		end = env.nr_cores - 1;
	}
	else
	{
		numa_info = abtst_get_numa_info(numa_id);
		if (!numa_info)
		{
			return NULL;
		}
		start = numa_info->start_core;
		end = numa_info->end_core;
	}

	nr_cores = end - start + 1;

	sort_param *params = (sort_param *)calloc(nr_cores, sizeof(sort_param));
	if (!params)
	{
		return NULL;
	}

	for (i = start; i <= end; i++)
	{
		stream = &global->streams.streams[i];
		if (!stream->used)
		{
			continue;
		}
		if ((partition_id != -1) && (partition_id != stream->part_id))
		{
			continue;
		}
		params[cnt].ios = abtst_stream_get_qdepth(stream);
		params[cnt].stream = stream;
		cnt++;
	}

	if (!cnt)
	{
		free(params);
		return NULL;
	}

	/* Sort params */
	qsort(params, cnt, sizeof(sort_param), cmp_func);

	return params;
}

static sort_param * sort_partitions(abtst_partitions *partitions, int numa_id)
{
	int i;

	sort_param *params = (sort_param *)calloc(partitions->nr_partitions, sizeof(sort_param));
	if (!params)
	{
		return NULL;
	}

	for (i = 0; i < partitions->nr_partitions; i++)
	{
		if (numa_id == -1)
		{
			params[i].ios = partitions->stats[i].qdepth / partitions->stats[i].used_cores;
		}
		else
		{
			params[i].ios = partitions->numa_stats[i][numa_id].qdepth / partitions->numa_stats[i][numa_id].used_cores;
		}
		params[i].stream = (abtst_stream *)(uint64_t)i;
	}

	/* Sort params */
	qsort(params, partitions->nr_partitions, sizeof(sort_param), cmp_func);

	return params;
}


static void reb_in_partition(abtst_global *global, int numa_id, int partition_id)
{
	abtst_partition_stat *stat = abtst_get_partition_stat(&global->partitions, numa_id, partition_id);
	uint32_t loop = 0;
	int ret;
	sort_param * params;

	while (++loop < stat->used_cores)
	{
		params = sort_streams(global, numa_id, partition_id);
		if (!params)
		{
			return;
		}

		if (params[stat->used_cores - 1].ios < MIN_IOS_TO_START_REB)
		{
			free(params);
			return;
		}

		ret = abtst_rebalance_streams(&params[0], &params[stat->used_cores - 1], stat->qdepth/stat->used_cores);
		free(params);
		if (ret < 0)
		{
			break;
		}
	}
}

/*
 * Level 1 rebalance: inside NUMA, inside partition
 */
void abtst_reb_in_partitions(abtst_global *global)
{
	int i, j;
	abtst_partition_stat *stat;

	for (i = 0; i < env.nr_numas; i++)
	{
		for (j = 0; j < global->partitions.nr_partitions; j++)
		{
			stat = abtst_get_partition_stat(&global->partitions, i, j);
			if ((stat->used_cores <= 1) || !stat->qdepth)
			{
				continue;
			}

			reb_in_partition(global, i, j);
		}
	}
}

static void reb_in_numa(abtst_global *global, int numa_id)
{
	sort_param *params;
	abtst_numa_stat *numa_stat = abtst_get_numa_stat(numa_id);
	abtst_partitions *partitions = &global->partitions;
	abtst_partition_stat *stat;
	int part_id, from_pid = -1, to_pid = -1;
	int from, to;
	int ret;

	params = sort_partitions(partitions, numa_id);
	if (!params)
	{
		return;
	}

	/* Need to move one core from a partitions with lowest qdepth to a partition with highest qdepth */
	from = 0;
	to = partitions->nr_partitions - 1;
	while (from < to)
	{
		part_id = (int)(uint64_t)params[from].stream;
		stat = &partitions->numa_stats[part_id][numa_id];

		if ((stat->used_cores > 1) &&
			(stat->qdepth / (stat->used_cores - 1) <= numa_stat->avg_qdepth))
		{
			from_pid = part_id;
			break;
		}

		from++;
	}
	if (from >= to)
	{
		free(params);
		return;
	}

	/* Find to partition */
	while (from < to)
	{
		part_id = (int)(uint64_t)params[to].stream;
		stat = &partitions->numa_stats[part_id][numa_id];

		if ((stat->qdepth / (stat->used_cores + 1) > numa_stat->avg_qdepth) &&
			(stat->qdepth / stat->used_cores > MIN_IOS_TO_START_REB))
		{
			to_pid = part_id;
			break;
		}

		to--;
	}
	if (from >= to)
	{
		free(params);
		return;
	}
	free(params);

	params = sort_streams(global, numa_id, from_pid);
	if (!params)
	{
		return;
	}

	/* Combine two streams with lowest qdepth */
	ret = abtst_combine_streams(params[0].stream, params[1].stream);
	if (ret)
	{
		free(params);
		return;
	}

	/* Move a stream to destination partition */
	printf("NUMA %d: move stream %d from partition %d to partition %d\n",
			numa_id, params[0].stream->rank, from_pid, to_pid);
	abtst_stream_set_partition_id(params[0].stream, to_pid);

	abtst_update_partition_stats(global);
	free(params);
}

/*
 * Level 2 rebalance: inside NUMA, between partitions
 */
void abtst_reb_in_numas(abtst_global *global)
{
	abtst_numa_stat *numa_stat;
	abtst_partitions *partitions = &global->partitions;
	int i;

	if (partitions->nr_partitions <= 1)
	{
		return;
	}

	for (i = 0; i < env.nr_numas; i++)
	{
		numa_stat = abtst_get_numa_stat(i);
		if (!numa_stat->used_cores || !numa_stat->avg_qdepth)
		{
			continue;
		}

		reb_in_numa(global, i);
	}
}

#if 0
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

		ret = abtst_rebalance_streams(&params_to[0], &params_from[numa_stat_from->used_cores - 1], avg_qdepth);
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

/*
 * Level 3 rebalance: between NUMAs, inside partition
 */
static void reb_between_numas(abtst_global *global)
{
	int i;
	int from = -1, to = -1;
	uint32_t max_qdepth = 0, min_qdepth;
	uint32_t total_cores = 0;
	uint32_t total_qdepth = 0;
	uint32_t avg_qdepth;
	abtst_numa_stat *numa_stat;

	if (abtst_get_rebalance_level(&global->rebalance) < REBALANCE_LEVEL_BETWEEN_NUMA)
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
#endif

static void abtst_reb_update_stat(abtst_global *global)
{

	abtst_update_streams_stat(&global->streams);

	abtst_update_partition_stats(global);

	abtst_update_numa_stats(global);

}

static void *reb_func(void *arg)
{
	abtst_global *global = (abtst_global *)arg;

	printf("rebalance started\n");
	while (!abtst_is_system_stopping())
	{
		abtst_reb_update_stat(global);

#if 0
		/* First process requests */
		abtst_reb_process_requests(global);
#endif


		if (abtst_get_rebalance_level(&global->rebalance) >= REBALANCE_LEVEL_IN_NUMA)
		{
			abtst_reb_in_numas(global);
		}

		if (abtst_get_rebalance_level(&global->rebalance) >= REBALANCE_LEVEL_IN_PARTITION)
		{
			abtst_reb_in_partitions(global);
		}

		sleep(1);
	}

	printf("rebalance ended\n");
	return NULL;
}

int abtst_init_rebalance(abtst_rebalance *reb, void *param)
{
	int ret;

	//atomic_init(&reb->in_use, 0);
	reb->param = param;
	INIT_LIST_HEAD(&reb->req_q);
	spinlock_init(&reb->lock);
	reb->rebalance_level = REBALANCE_LEVEL_IN_PARTITION;

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
