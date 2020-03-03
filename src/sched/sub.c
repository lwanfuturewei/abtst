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

#include "abt.h"


#define SCHED_EVENT_FREQ           50

static int  sub_sched_init(ABT_sched sched, ABT_sched_config config);
static void sub_sched_run(ABT_sched sched);
static int  sub_sched_free(ABT_sched);

static ABT_sched_def abtst_sub_sched_def = {
    .type = ABT_SCHED_TYPE_TASK,
    .init = sub_sched_init,
    .run = sub_sched_run,
    .free = sub_sched_free,
    .get_migr_pool = NULL,
};

typedef struct {
    uint32_t event_freq;
    int num_pools;
    ABT_pool *pools;
    void *load;
} sched_data;

ABT_sched_config_var abtst_sub_sched_basic_freq = {
    .idx = 0,
    .type = ABT_SCHED_CONFIG_INT
};

ABT_sched_config_var abtst_sub_sched_load = {
    .idx = 1,
    .type = ABT_SCHED_CONFIG_PTR
};


ABT_sched_def *abtst_sub_sched_get_def(void)
{
    return &abtst_sub_sched_def;
}

static inline sched_data *sub_sched_data_get_ptr(void *data)
{
    return (sched_data *)data;
}

void *sub_sched_get_load(ABT_sched sched)
{
    void *data;
    sched_data *p_data;

    ABT_sched_get_data(sched, &data);
    p_data = sub_sched_data_get_ptr(data);

    return (p_data->load);
}

static int sub_sched_init(ABT_sched sched, ABT_sched_config config)
{
    int abt_errno = ABT_SUCCESS;
    int num_pools;

    /* Default settings */
    sched_data *p_data = (sched_data *)malloc(sizeof(sched_data));
    p_data->event_freq = SCHED_EVENT_FREQ;

    /* Set the variables from the config */
    ABT_sched_config_read(config, 2, &p_data->event_freq, &p_data->load);
    //printf("sub_sched_init freq %d load %p\n", p_data->event_freq, p_data->load);

    /* Save the list of pools */
    ABT_sched_get_num_pools(sched, &num_pools);
    p_data->num_pools = num_pools;
    p_data->pools = (ABT_pool *)malloc(num_pools * sizeof(ABT_pool));
    abt_errno = ABT_sched_get_pools(sched, num_pools, 0, p_data->pools);
    //ABTI_CHECK_ERROR(abt_errno);

    /* Sort pools according to their access mode so the scheduler can execute
       work units from the private pools. */
    //if (num_pools > 1) {
    //    sched_sort_pools(num_pools, p_data->pools);
    //}

    abt_errno = ABT_sched_set_data(sched, (void *)p_data);

    return abt_errno;
}

#define MAX_BATCH_COUNT 5
static void sub_sched_run(ABT_sched sched)
{
    void *data;
    sched_data *p_data;
    int num_pools;
    ABT_pool *pools;
    int i;
    int run_cnt;

    //ABTI_xstream *p_xstream = ABTI_local_get_xstream();
    //ABTI_sched *p_sched = ABTI_sched_get_ptr(sched);

    ABT_sched_get_data(sched, &data);
    p_data = sub_sched_data_get_ptr(data);
    //event_freq = p_data->event_freq;
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
            while (unit != ABT_UNIT_NULL) {
                ABT_xstream_run_unit(unit, pool);
                run_cnt++;
		if (run_cnt >= MAX_BATCH_COUNT) {
			break;
		}
		ABT_pool_pop(pool, &unit);
    	    }
        }

	//if (run_cnt > 1) printf("run_cnt %d\n", run_cnt);

	if (!run_cnt || (run_cnt >= MAX_BATCH_COUNT)) {
		break;
		//ABT_thread_yield();
	}
    }
}

static int sub_sched_free(ABT_sched sched)
{
    int abt_errno = ABT_SUCCESS;

    void *data;

    ABT_sched_get_data(sched, &data);
    sched_data *p_data = sub_sched_data_get_ptr(data);
    free(p_data->pools);
    free(p_data);
    return abt_errno;
}

