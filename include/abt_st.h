/*
 * See COPYRIGHT in top-level directory.
 */
#ifndef ABT_ST_H
#define ABT_ST_H

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "mapping.h"
#include "load.h"
#include "stream.h"
#include "rebalance.h"


#define MAX_MAPPINGS 8

typedef struct abtst_partition_struct
{
	int partition_id;
	int nr_cores;
	int *partition_cores;

} abtst_partition;

typedef struct abtst_global_struct
{
	abtst_streams streams;

	int nr_mappings;
	abtst_mapping mappings[MAX_MAPPINGS];

	abtst_rebalance rebalance;

	int nr_partitions;
	abtst_partition *partitions;

} abtst_global;


/* This function should be called sequentially */
static inline int get_mapping_id(abtst_global *global)
{
	int id = -1;

	if (global->nr_mappings < MAX_MAPPINGS)
	{
		id = global->nr_mappings;
		global->nr_mappings++;
	}

	return id;
}


int abtst_init(abtst_global *global, void *param);
int abtst_finalize(abtst_global *global);
void abtst_free(abtst_global *global);


#endif

