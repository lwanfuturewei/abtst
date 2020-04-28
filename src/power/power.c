/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <strings.h>

#include "env.h"
#include "stream.h"
#include "abt_st.h"
#include "power.h"


abtst_power power;

void abtst_init_power(void)
{
	int i;

	for (i = 0; i < env.nr_numas; i++)
	{
		power.ps_state[i] = false;
		power.ps_pct[i] = 0;
	}

	power.ps_enabled = false;
	power.nr_ps_numas = 0;
}

static void update_power_stat(abtst_global *global, int numa_id)
{
	abtst_stream *stream;
	uint32_t msec = 0;
	int j;
	abtst_numa *numa_info;
	uint32_t cnt = 0;
	int pct;

	numa_info = abtst_get_numa_info(numa_id);
	for (j = numa_info->start_core; j <= numa_info->end_core; j++)
	{
		stream = &global->streams.streams[j];
		if (!stream->used)
		{
			continue;
		}
		msec += abtst_stream_get_sleep_time(stream) / 1000000;
		abtst_stream_reset_sleep_time(stream);
		cnt++;
	}

	if (cnt)
	{
		pct = ((msec / cnt) * 100) / (1000 * abtst_get_rebalance_interval(&global->rebalance)) ;
		if (pct > 100)
		{
			pct = 100;
		}
	}
	else
	{
		/* This numa hasn't been used for xstreams */
		pct = 100;
	}

	abtst_set_numa_ps_pct(numa_id, pct);
}

void abtst_update_power_stats(abtst_global *global)
{
	int i;

	for (i = 0; i < env.nr_numas; i++)
	{
		update_power_stat(global, i);
	}

}

#define WORK_BALANCE_LEVEL 0.5
/*
 * Estimate how many NUMAs should be in PS state
 */
int abtst_estimate_ps_numas(void)
{
	int i;
	int pct = 0;

	for (i = 0; i < env.nr_numas; i++)
	{
		pct += abtst_get_numa_ps_pct(i);
	}

	for (i = 1; i < env.nr_numas; i++)
	{
		if ((env.nr_numas * 100 - pct) < (i * 100 * WORK_BALANCE_LEVEL))
		{
			return (env.nr_numas - i);
		}
	}

	return 0;
}
