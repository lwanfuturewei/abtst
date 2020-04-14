/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>

#include "abt_st.h"
#include "mapping.h"
#include "load.h"
#include "env.h"
#include "hash.h"


abtst_mapping * get_hash_mapping_def(void);

int abtst_create_mapping(void *pglobal, int type, void *params, int partid)
{
	abtst_global *global = (abtst_global *)pglobal;
	int mapping_id = get_mapping_id(global);
	abtst_mapping *mapping = &global->mappings[mapping_id];
	abtst_mapping *def;
	int ret;

	mapping->mapping_id = mapping_id;
	mapping->partition_id = partid;
	mapping->type = type;
	mapping->param = params;

	switch (mapping->type)
	{
	case MAPPING_TYPE_HASHING:
		def = get_hash_mapping_def();
		mapping->init_mapping = def->init_mapping;
		mapping->mapping_to_load = def->mapping_to_load;
		//mapping->init_mapping(mapping);
		mapping->loads.nr_loads = env.nr_cores * LOADS_PER_XSTREAM;
		mapping->loads.partition_id = mapping->partition_id;
		break;

	default:
        return -1;

	}

	/* Init loads */
	ret = abtst_init_loads(&mapping->loads, &global->streams);
	if (ret)
	{
		return -1;
	}

	ret = mapping->init_mapping(mapping);
	if (ret)
	{
		return -1;
	}

	return mapping_id;
}

int map_key_to_load(abtst_global *global, int mapping_id, void *key)
{
	abtst_mapping * mapping = &global->mappings[mapping_id];
	abtst_loads *loads = &mapping->loads;
	uint32_t load;

	/* key -> load */
	load = mapping->mapping_to_load(mapping, loads, key);

	return load;
}

/* Map a key to a xstream.
 */
int map_key_to_xstream(abtst_global *global, int mapping_id, void *key)
{
	abtst_mapping * mapping = &global->mappings[mapping_id];
	abtst_loads *loads = &mapping->loads;
	uint32_t load;

	/* key -> load */
	load = mapping->mapping_to_load(mapping, loads, key);

	/* load -> xsteram */
	return map_load_to_stream(loads, load);
}

