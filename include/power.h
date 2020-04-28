#ifndef ABTST_POWER_H
#define ABTST_POWER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>

#include "env.h"


typedef struct abtst_power_struct
{
	bool   ps_state[MAX_NUMAS];
	uint32_t ps_pct[MAX_NUMAS];

	bool ps_enabled;
	int nr_ps_numas;

} abtst_power;

extern abtst_power power;


static inline bool abtst_is_power_saving_enabled(void)
{
	return power.ps_enabled;
}

static inline void abtst_set_power_saving_enabled(bool ena)
{
	power.ps_enabled = ena;
}

static inline int abtst_get_numas_in_ps_state(void)
{
	return power.nr_ps_numas;
}

static inline bool abtst_is_numa_in_ps_state(int numa_id)
{
	return power.ps_state[numa_id];
}

static inline void abtst_set_numa_ps_state(int numa_id, bool state)
{
	power.ps_state[numa_id] = state;
	if (state == true) {
		power.nr_ps_numas++;
	} else {
		power.nr_ps_numas--;
	}
}

static inline uint32_t abtst_get_numa_ps_pct(int numa_id)
{
	return power.ps_pct[numa_id];
}

static inline void abtst_set_numa_ps_pct(int numa_id, uint32_t pct)
{
	power.ps_pct[numa_id] = pct;
}


void abtst_init_power(void);
int abtst_estimate_ps_numas(void);

#endif
