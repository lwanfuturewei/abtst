#ifndef ABTST_MAPPING_H
#define ABTST_MAPPING_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>

#include "load.h"

#define MAX_MAPPINGS 8

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

typedef struct abtst_mappings_struct
{
	int nr_mappings;
	abtst_mapping mappings[MAX_MAPPINGS];

} abtst_mappings;

/* This function should be called sequentially */
static inline int get_mapping_id(abtst_mappings *mappings)
{
	int id = -1;

	if (mappings->nr_mappings < MAX_MAPPINGS)
	{
		id = mappings->nr_mappings;
		mappings->nr_mappings++;
	}

	return id;
}


int abtst_create_mapping(void *pglobal, int type, void *params, int partid);
void abtst_free_mappings(abtst_mappings *mappings);

	
#endif
