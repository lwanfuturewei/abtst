#ifndef ABTST_ENV_H
#define ABTST_ENV_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>


#define MAX_NUMAS 32

typedef struct abtst_numa_struct
{
        uint32_t start_core;
        uint32_t end_core;
} abtst_numa;

typedef struct abtst_numa_stat_struct
{
        uint32_t used_cores;
        uint32_t avg_qdepth;
} abtst_numa_stat;


typedef struct abtst_env_struct
{
	uint32_t nr_cores;
	uint32_t nr_numas;

	abtst_numa numa_info[MAX_NUMAS];
	abtst_numa_stat numa_stat[MAX_NUMAS];

	bool rebalance_enabled;
	bool stopping;

} abtst_env;

extern abtst_env env;


static inline abtst_numa *abtst_get_numa_info(uint32_t id)
{
	if (id >= env.nr_numas) {
		return NULL;
	} else {
		return (&env.numa_info[id]);
	}
}

static inline abtst_numa_stat *abtst_get_numa_stat(uint32_t id)
{
        if (id >= env.nr_numas) {
                return NULL;
        } else {
                return (&env.numa_stat[id]);
        }
}

static inline void abtst_set_numa_stat(uint32_t id, uint32_t cores, uint32_t qd)
{
        if (id >= env.nr_numas) {
                return;
        }
 
	env.numa_stat[id].used_cores = cores;
	env.numa_stat[id].avg_qdepth = qd;
}

static inline void abtst_set_numa_rebalance(bool enabled)
{
	env.rebalance_enabled = enabled;
}

static inline bool abtst_is_numa_rebalance_enabled(void)
{
	return env.rebalance_enabled;
}

static inline void abtst_set_stopping(void)
{
        env.stopping = true;
}

static inline bool abtst_is_system_stopping(void)
{
        return (env.stopping);
}

void abtst_init_env(void);
void abtst_set_stopping(void);
bool abtst_is_system_stopping(void);
int abtst_env_get_numa_id(uint32_t core);
void print_numas(void);


#endif
