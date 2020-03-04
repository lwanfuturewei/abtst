#ifndef ABTST_SCHED_H
#define ABTST_SCHED_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>

#include "abt.h"


extern ABT_sched_config_var abtst_sched_basic_freq;
extern ABT_sched_config_var abtst_sched_stream;
ABT_sched_def *abtst_sched_get_def(void);

int abtst_init_sched(ABT_sched *sched, ABT_pool *pool, void *stream);
void abtst_free_sched(ABT_sched *sched);

extern ABT_sched_config_var abtst_sub_sched_basic_freq;
extern ABT_sched_config_var abtst_sub_sched_load;
ABT_sched_def *abtst_sub_sched_get_def(void);

int abtst_init_sub_sched(ABT_sched *sched, ABT_pool *pool, void *load);
void abtst_free_sub_sched(ABT_sched *sched);
void *sub_sched_get_load(ABT_sched sched);
ABT_pool sub_sched_get_pool(ABT_sched sched);


#endif
