#ifndef ABTST_ENV_H
#define ABTST_ENV_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>


typedef struct abtst_env_struct
{
	uint32_t nr_cores;
	uint32_t nr_numas;

	bool stopping;
} abtst_env;

extern abtst_env env;

void abtst_init_env(void);
void abtst_set_stopping(void);
bool abtst_is_system_stopping(void);

#endif
