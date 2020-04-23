/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <strings.h>
#include "env.h"
#include "abt_st.h"


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

static void update_numa_stat(abtst_global *global, int numa_id)
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

void abtst_update_numa_stats(abtst_global *global)
{
	int i;

	for (i = 0; i < env.nr_numas; i++)
	{
		update_numa_stat(global, i);
	}

}

uint32_t abtst_get_average_qdepth(void)
{
	int i;
	abtst_numa_stat *numa_stat;
	uint32_t cores = 0, qdepth = 0;

	for (i = 0; i < env.nr_numas; i++)
	{
		numa_stat = abtst_get_numa_stat(i);
		if (!numa_stat->used_cores)
		{
			continue;
		}
		cores += numa_stat->used_cores;
		qdepth += numa_stat->avg_qdepth * numa_stat->used_cores;
	}

	if (!cores)
	{
		/* Not initialized */
		return 0;
	}

	return (qdepth / cores);
}

void print_numas(void)
{
	int i;

	for (i = 0; i < env.nr_numas; i++)
        {
		printf("numa %d, cores %4d - %4d, used %4d, qdepth %4d\n",
			i, env.numa_info[i].start_core, env.numa_info[i].end_core,
			env.numa_stat[i].used_cores, env.numa_stat[i].avg_qdepth);
        }
}

