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

#include "abt.h"
#include "sched.h"


int abtst_init_sched(ABT_sched *sched, ABT_pool *pool, void *stream)
{
	int ret;
	ABT_sched_config config;

	ret = ABT_sched_config_create(&config,
                                  abtst_sched_basic_freq, 10,
				  abtst_sched_stream, stream,
                                  ABT_sched_config_var_end);
	if (ret)
	{
		printf("ABT_sched_config_create error %d\n", ret);
		return -1;
	}

	ret = ABT_sched_create(abtst_sched_get_def(), 1, pool, config, sched);
	if (ret) 
	{
		printf("ABT_sched_create error %d\n", ret);
		ABT_sched_config_free(&config);
		return -1;
	}

	ABT_sched_config_free(&config);

	return 0;
}

void abtst_free_sched(ABT_sched *sched)
{
	int ret;

	ret = ABT_sched_free(sched);
	if (ret)
	{
		printf("ABT_sched_free error %d\n", ret);
	}

}

int abtst_init_sub_sched(ABT_sched *sched, ABT_pool *pool, void *load)
{
        int ret;
        ABT_sched_config config;

        ret = ABT_sched_config_create(&config,
                                  abtst_sub_sched_basic_freq, 10,
                                  abtst_sub_sched_load, load,
				  ABT_sched_config_var_end);
        if (ret)
        {
                printf("ABT_sched_config_create error %d\n", ret);
                return -1;
        }

        ret = ABT_sched_create(abtst_sub_sched_get_def(), 1, pool, config, sched);
        if (ret)
        {
                printf("ABT_sched_create error %d\n", ret);
        }

        ABT_sched_config_free(&config);

        return ret;
}

void abtst_free_sub_sched(ABT_sched *sched)
{
        int ret;

        ret = ABT_sched_free(sched);
        if (ret)
        {
                printf("ABT_sched_free error %d\n", ret);
        }

}

