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

int abtst_init_mapping(int type, abtst_mapping *mapping, void *param)
{
	abtst_mapping *def;

	switch (type)
	{
	case MAPPING_TYPE_HASHING:
		def = get_hash_mapping_def();
		*mapping = *def;
		mapping->init_mapping(mapping);
		break;

	default:
        break;

	}

        mapping->param = param;

	return 0;
}

int map_key_to_load(abtst_global *global, void *key)
{
        abtst_mapping * mapping = &global->mapping;
        abtst_loads *loads = &global->loads;
        uint32_t load;

        /* key -> load */
        load = mapping->mapping_to_load(mapping, loads, key);

	return load;
}

/* Map a key to a xstream.
 */
int map_key_to_xstream(abtst_global *global, void *key)
{
	abtst_mapping * mapping = &global->mapping;
	abtst_loads *loads = &global->loads;
        uint32_t load;

	/* key -> load */
        load = mapping->mapping_to_load(mapping, loads, key);

	/* load -> xsteram */
	return map_load_to_stream(loads, load);
}

