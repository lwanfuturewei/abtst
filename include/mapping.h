#ifndef ABTST_MAPPING_H
#define ABTST_MAPPING_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>


enum mapping_type {
    MAPPING_TYPE_HASHING,
};

typedef struct abtst_mapping_struct
{
	uint32_t type;
	void *param;
	int (*init_mapping)(void *mapping);
	uint32_t (*mapping_to_load)(void *mapping, void *loads, void *key);

} abtst_mapping;


int abtst_init_mapping(int type, abtst_mapping *mapping, void *param);

	
#endif
