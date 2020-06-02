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
#include "abtst_stream.h"
#include "rebalance.h"
#include "partition.h"


typedef struct abtst_global_struct
{
	abtst_streams streams;

	abtst_mappings mappings;

	abtst_rebalance rebalance;

	abtst_partitions partitions;

} abtst_global;

typedef struct create_param_struct
{
	int nr_partitions;
	abtst_partition *init_partitions;

} create_params;


int abtst_init(abtst_global *global, void *param);
int abtst_finalize(abtst_global *global);
void abtst_free(abtst_global *global);


#endif

