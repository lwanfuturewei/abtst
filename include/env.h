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

typedef struct abtst_env_struct
{
	uint32_t nr_cores;
	uint32_t nr_numas;

	abtst_numa numa_info[MAX_NUMAS];

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


#endif
