#ifndef ABTST_MAPPING_H
#define ABTST_MAPPING_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>

#include "load.h"

enum mapping_type {
    MAPPING_TYPE_HASHING,
};

typedef struct abtst_mapping_struct
{
	int mapping_id;
	int partition_id;
	uint32_t type;
	void *param;
	int (*init_mapping)(void *mapping);
	uint32_t (*mapping_to_load)(void *mapping, void *loads, void *key);

	abtst_loads   loads;

} abtst_mapping;



int abtst_create_mapping(void *pglobal, int type, void *params, int partid);

	
#endif
