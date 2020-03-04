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
        abtst_stream *stream = NULL;
        abtst_stream *to;
        int numa_id = abtst_env_get_numa_id(core);
        abtst_numa *numa_info = abtst_get_numa_info(numa_id);
        int i;
        int nr_cores;
        int cnt = 0;
	int ret;

        if (core >= 0)
        {
                stream = &global->streams.streams[core];
                if (!stream->used)
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
                params[cnt].ios = abtst_get_stream_ios(stream);
                params[cnt].stream = stream;
                cnt++;
        }
        /* Sort params */
        qsort(params, cnt, sizeof(sort_param), cmp_streams);

        if (stream)
        {
                to = params[0].stream;
        }
        else
        {
                stream = params[0].stream;
                to = params[1].stream;
        }

        /* Move all the loads from stream to to_stream */
        ret = combine_streams(stream, to);
        if (ret)
        {
                printf("combine_streams error %d\n", ret);
        }
        else
        {
                ret = ABT_xstream_join(stream->xstream);
                ABT_xstream_free(&stream->xstream);
                abtst_free_sched(&stream->sched);
        }

        free(params);
}


/* We may need to find a better policy to chose loads for migration:
 * 1) consider original stream;
 * 2) consider the history of stream ios;
 * 3) whether to_stream is a totally new stream;
 * 4) etc.
 */
static void reb_streams(sort_param *p1, sort_param *p2)
{
	abtst_stream *from, *to;
	int avg = (p1->ios + p2->ios)/2;
	int ios = p1->ios;
	int count = 0;
	int i;
        struct list_head *pos, *n;
        abtst_load *load;
	int lsize;

        from = p2->stream;
        to = p1->stream;

	/* Check whether rebalance is needed */
	if (to->nr_loads && ((p2->ios - p1->ios) < MIN_IOS_TO_START_REB))
	{
		return;
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
		if (lsize && (ios + lsize < avg))
		{
			/* the load is chosen for migration */
			ios += lsize;
			mloads[count++] = load;
		}
	}

	for (i = 0; i < count; i++)
	{
		abtst_load_set_migrating(mloads[i], true, to->rank);
	}

	free(mloads);
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

static void reb_in_numa(abtst_global *global, int numa_id)
{
	abtst_numa *numa_info = abtst_get_numa_info(numa_id);
	int i;
	abtst_stream *stream;
	int nr_cores;
	int cnt = 0;

	if (!numa_info)
	{
		return;
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
		params[cnt].ios = abtst_get_stream_ios(stream);
		params[cnt].stream = stream;
		cnt++;
	}
	/* Sort params */
	qsort(params, cnt, sizeof(sort_param), cmp_streams);

	for (i = 0; i < min(cnt/2, REB_STREAMS); i++)
	{
		reb_streams(&params[i], &params[cnt - 1 - i]);
	}

	free(params);
}

static void reb_between_numas(abtst_global *global)
{
}

static void *reb_func(void *arg)
{
	int i;
	abtst_global *global = (abtst_global *)arg;

	printf("rebalance started\n");
	while (!abtst_is_system_stopping())
	{
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
