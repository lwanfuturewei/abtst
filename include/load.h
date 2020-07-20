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
#include "abt.h"
#include "abtst_stream.h"


#define LOADS_PER_XSTREAM 32


typedef struct abtst_load_struct 
{
	struct list_head list;
	
	int load_id;
	bool used;

	uint64_t pkt_started;
	uint64_t pkt_ended;

	uint32_t init_rank;
	uint32_t curr_rank;

	//bool migrating;
	int dest_rank;

	ABT_pool pool;
	ABT_sched sched;

} abtst_load;

typedef struct abtst_loads_struct 
{
	int partition_id;
	uint32_t nr_loads;
	abtst_load *loads;

} abtst_loads;


static inline uint32_t abtst_load_get_curr_rank(abtst_load *load)
{
	return load->curr_rank;
}

static inline void abtst_load_update_curr_rank(abtst_load *load, int rank)
{
	load->curr_rank = rank;
}

static inline void abtst_load_set_migrating(abtst_load *load, bool migrate, int dest)
{
	load->dest_rank = dest;
	//load->migrating = migrate;
}

static inline bool abtst_load_is_migrating(abtst_load *load)
{
	//return load->migrating;
	return (load->curr_rank != load->dest_rank);
}

static inline void abtst_load_inc_started(abtst_load *load)
{
	load->pkt_started++;
}

static inline void abtst_load_inc_ended(abtst_load *load)
{
	load->pkt_ended++;
}

static inline int abtst_get_load_size(abtst_load *load)
{
        if (load->pkt_started > load->pkt_ended)
        {
                return (load->pkt_started - load->pkt_ended);
        }
        return 0;
}

int abtst_init_loads(abtst_loads *loads, abtst_streams *streams);
void abtst_free_loads(abtst_loads *loads);
int map_load_to_stream(abtst_loads *loads, uint32_t load);


#endif
