/*
 * See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>

#include "env.h"
#include "abt.h"


#define SCHED_EVENT_FREQ           50

static int  sched_init(ABT_sched sched, ABT_sched_config config);
static void sched_run(ABT_sched sched);
static int  sched_free(ABT_sched);
static void sched_sort_pools(int num_pools, ABT_pool *pools);

static ABT_sched_def abtst_sched_def = {
    .type = ABT_SCHED_TYPE_ULT,
    .init = sched_init,
    .run = sched_run,
    .free = sched_free,
    .get_migr_pool = NULL,
};

typedef struct {
    uint32_t event_freq;
    int num_pools;
    ABT_pool *pools;
    struct timespec sleep_time;

    void *stream;
} sched_data;

ABT_sched_config_var abtst_sched_basic_freq = {
    .idx = 0,
    .type = ABT_SCHED_CONFIG_INT
};

ABT_sched_config_var abtst_sched_stream = {
    .idx = 1,
    .type = ABT_SCHED_CONFIG_PTR
};

ABT_sched_def *abtst_sched_get_def(void)
{
    return &abtst_sched_def;
}

static inline sched_data *sched_data_get_ptr(void *data)
{
    return (sched_data *)data;
}

static int sched_init(ABT_sched sched, ABT_sched_config config)
{
    int abt_errno = ABT_SUCCESS;
    int num_pools;

    /* Default settings */
    sched_data *p_data = (sched_data *)malloc(sizeof(sched_data));
    p_data->event_freq = SCHED_EVENT_FREQ;
    p_data->sleep_time.tv_sec = 0;
    p_data->sleep_time.tv_nsec = 100;

    /* Set the variables from the config */
    ABT_sched_config_read(config, 2, &p_data->event_freq, &p_data->stream);

    /* Save the list of pools */
    ABT_sched_get_num_pools(sched, &num_pools);
    p_data->num_pools = num_pools;
    p_data->pools = (ABT_pool *)malloc(num_pools * sizeof(ABT_pool));
    abt_errno = ABT_sched_get_pools(sched, num_pools, 0, p_data->pools);
    //ABTI_CHECK_ERROR(abt_errno);

    /* Sort pools according to their access mode so the scheduler can execute
       work units from the private pools. */
    if (num_pools > 1) {
        sched_sort_pools(num_pools, p_data->pools);
    }

    abt_errno = ABT_sched_set_data(sched, (void *)p_data);

    return abt_errno;
}

static void sched_run(ABT_sched sched)
{
    uint32_t work_count = 0;
    void *data;
    sched_data *p_data;
    uint32_t event_freq;
    int num_pools;
    ABT_pool *pools;
    int i;
    int run_cnt;
    ABT_bool stop = ABT_FALSE;

    //ABTI_xstream *p_xstream = ABTI_local_get_xstream();
    //ABTI_sched *p_sched = ABTI_sched_get_ptr(sched);

    ABT_sched_get_data(sched, &data);
    p_data = sched_data_get_ptr(data);
    event_freq = p_data->event_freq;
    num_pools  = p_data->num_pools;
    pools      = p_data->pools;

    while (1) {
        run_cnt = 0;

        /* Execute one work unit from the scheduler's pool */
        for (i = 0; i < num_pools; i++) {
            ABT_pool pool = pools[i];
            //ABTI_pool *p_pool = ABTI_pool_get_ptr(pool);
            /* Pop one work unit */
            ABT_unit unit;
            
	    ABT_pool_pop(pool, &unit);
            if (unit != ABT_UNIT_NULL) {
                ABT_xstream_run_unit(unit, pool);
                run_cnt++;
		if (!abtst_is_system_stopping()) {
			/* Check whether the unit should be migrated */
			/* Currently all units are sub_sched */
			ABT_pool_push(pool, unit);
		}
		break;
            }
        }

	//if (run_cnt) printf("run_cnt %d\n", run_cnt);

        if (++work_count >= event_freq) {
            ABT_xstream_check_events(sched);
	    if (stop == ABT_FALSE) {
            	ABT_sched_has_to_stop(sched, &stop);
	    }
            if (!run_cnt && stop == ABT_TRUE)
                break;
            work_count = 0;
	    
	    if (run_cnt) {
		    p_data->sleep_time.tv_nsec = 100;
	    }
	    else if (p_data->sleep_time.tv_nsec < 10000 * 100) {
		    p_data->sleep_time.tv_nsec *= 2;
	    }

            if (run_cnt == 0) nanosleep(&p_data->sleep_time, NULL);	    
	    //SCHED_SLEEP(run_cnt, p_data->sleep_time);
        }
    }
    printf("abtst basic sched exit\n");
}

static int sched_free(ABT_sched sched)
{
    int abt_errno = ABT_SUCCESS;

    void *data;

    ABT_sched_get_data(sched, &data);
    sched_data *p_data = sched_data_get_ptr(data);
    free(p_data->pools);
    free(p_data);
    return abt_errno;
}

static int pool_get_access_num(ABT_pool *p_pool)
{
    ABT_pool_access access;
    int num = 0;

    ABT_pool_get_access(*p_pool, &access);
    switch (access) {
        case ABT_POOL_ACCESS_PRIV: num = 0; break;
        case ABT_POOL_ACCESS_SPSC:
        case ABT_POOL_ACCESS_MPSC: num = 1; break;
        case ABT_POOL_ACCESS_SPMC:
        case ABT_POOL_ACCESS_MPMC: num = 2; break;
        default: assert(0); break;
    }

    return num;
}

static int sched_cmp_pools(const void *p1, const void *p2)
{
    int p1_access, p2_access;

    p1_access = pool_get_access_num((ABT_pool *)p1);
    p2_access = pool_get_access_num((ABT_pool *)p2);

    if (p1_access > p2_access) {
        return 1;
    } else if (p1_access < p2_access) {
        return -1;
    } else {
        return 0;
    }
}

static void sched_sort_pools(int num_pools, ABT_pool *pools)
{
    qsort(pools, num_pools, sizeof(ABT_pool), sched_cmp_pools);
}

