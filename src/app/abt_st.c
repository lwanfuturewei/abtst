/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>

#include "abt_st.h"
#include "env.h"

static int partitions_0_cores[] =
{
	 2, 3, 4, 5, 6, 7, 8, 9,		/* Numa 0 */
	//34,35,36,37,38,39,40,41,		/* Numa 1 */
	//66,67,68,69,70,71,72,73,		/* Numa 2 */
	//98,99,100,101,102,103,104,105,		/* Numa 3 */
};

static int partitions_1_cores[] =
{
	 10,11,12,13,14,15,16,17,		/* Numa 0 */
	//42,43,44,45,46,47,48,49,		/* Numa 1 */
	//74,75,76,77,78,79,80,81,		/* Numa 2 */
	//106,107,108,109,110,111,112,113,		/* Numa 3 */
};

abtst_partition init_partitions[] =
{
	{0, 8, partitions_0_cores},
	{1, 8, partitions_1_cores},
};


int abtst_init(abtst_global *global, void *param)
{
	/* Init env */
	abtst_init_env();

	/* Init partitions */
	abtst_init_partitions(&global->partitions, sizeof(init_partitions)/sizeof(abtst_partition), init_partitions);

	/* Init stream */
	abtst_init_streams(&global->streams, global);

	/* Init loads */
	//abtst_init_loads(&global->mapping.loads, &global->streams);

	/* Init mapping */
	//abtst_init_mapping(global, param);

	/* Init rebalance */
	abtst_init_rebalance(&global->rebalance, global);
	
	return 0;
}

int abtst_finalize(abtst_global *global)
{
	int ret;

	/* Stop and free rebalance */
	abtst_free_rebalance(&global->rebalance);

	/* Wait xstreams to join and free */
	ret = abtst_finalize_streams(&global->streams);

	/* Free mappings */
	abtst_free_mappings(&global->mappings);

	return ret;
}

void abtst_free(abtst_global *global)
{
	/* Now it is safe to free streams */
	abtst_free_streams(&global->streams);

	abtst_free_partitions(&global->partitions);
}

