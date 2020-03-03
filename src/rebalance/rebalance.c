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
#include "rebalance.h"
#include "env.h"



void reb_in_numa(int numa_id)
{
}

void reb_between_numas(void)
{
}

void *reb_func(void *arg)
{
	int i;

	printf("rebalance started\n");
	for (i = 0; i < env.nr_numas; i++)
	{
		reb_in_numa(i);
	}

	reb_between_numas();

	sleep(1);
	printf("rebalance ended\n");
	return NULL;
}

int abtst_init_rebalance(abtst_rebalance *reb, void *param)
{
	int ret;

	atomic_init(&reb->in_use, 0);
	reb->param = param;

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
