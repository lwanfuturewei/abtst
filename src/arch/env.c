/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <strings.h>
#include "env.h"


abtst_env env;

void abtst_init_env(void)
{
	int i;

	/* Get the number of available cores in the system */
	env.nr_cores = sysconf(_SC_NPROCESSORS_ONLN);

	/* Get NUMA info from the system */
	env.nr_numas = 4;
	for (i = 0; i < env.nr_numas; i++)
	{
		env.numa_info[i].start_core = (env.nr_cores / env.nr_numas) * i;
		env.numa_info[i].end_core = (env.nr_cores / env.nr_numas) * (i + 1) - 1;
	
		env.numa_stat[i].used_cores = 0;
		env.numa_stat[i].avg_qdepth = 0;
	}

	env.stopping = false;
	env.rebalance_enabled = false;
}

int abtst_env_get_numa_id(uint32_t core)
{
	int i;

	for (i = 0; i < env.nr_numas; i++)
        {
		if ((core >= env.numa_info[i].start_core) 
			&& (core <= env.numa_info[i].end_core))
		{
			return i;
		}
	}

	return -1;
}
