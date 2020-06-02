/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "mapping.h"
#include "load.h"
#include "env.h"


int random_mapping_init(void *map);
uint32_t random_mapping_to_load(void *map, void *loads, void *key);


static abtst_mapping random_mapping = {
    .type = MAPPING_TYPE_RANDOM,
    .init_mapping = random_mapping_init,
    .mapping_to_load = random_mapping_to_load,
};

abtst_mapping * get_random_mapping_def(void)
{
	return (&random_mapping);
}

int random_mapping_init(void *map)
{
	return 0;
}

uint32_t random_mapping_to_load(void *map, void *ploads, void *key)
{
	//abtst_mapping *mapping = (abtst_mapping *)map;
	abtst_loads *loads = (abtst_loads *)ploads;

	return (rand() % loads->nr_loads);
}
