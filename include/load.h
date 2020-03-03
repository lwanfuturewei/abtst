#ifndef ABTST_LOAD_H
#define ABTST_LOAD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>

#include "list.h"
#include "stream.h"
#include "abt.h"


#define LOADS_PER_XSTREAM 16

typedef struct abtst_load_struct 
{
	struct list_head list;
	
	uint64_t pkt_started;
	uint64_t pkt_ended;

	uint32_t init_rank;
	uint32_t curr_rank;

	bool migrating;
	
	ABT_pool pool;
	ABT_sched sched;

} abtst_load;

typedef struct abtst_loads_struct 
{
        uint32_t nr_loads;
        abtst_load *loads;

} abtst_loads;


static inline void abtst_load_update_curr_rank(abtst_load *load, int rank)
{
	load->curr_rank = rank;
}

static inline void abtst_load_set_migrating(abtst_load *load, bool migrate)
{
	load->migrating = migrate;
}

static inline bool abtst_load_is_migrating(abtst_load *load)
{
	return load->migrating;
}

static inline void abtst_load_inc_started(abtst_load *load)
{
	load->pkt_started++;
}

static inline void abtst_load_inc_ended(abtst_load *load)
{
	load->pkt_ended++;
}

int abtst_init_loads(abtst_loads *loads, abtst_streams *streams);
void abtst_free_loads(abtst_loads *loads);
int map_load_to_stream(abtst_loads *loads, uint32_t load);


#endif
