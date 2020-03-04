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


int abtst_init(abtst_global *global, void *param)
{
        /* Init env */
        abtst_init_env();

	/* Init stream */
	abtst_init_streams(&global->streams);

	/* Init loads */
	abtst_init_loads(&global->loads, &global->streams);

	/* Init mapping */
	abtst_init_mapping(MAPPING_TYPE_HASHING, &global->mapping, param);

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

        /* Free loads */
        abtst_free_loads(&global->loads);

	return ret;
}

void abtst_free(abtst_global *global)
{
	/* Now it is safe to free streams */
        abtst_free_streams(&global->streams);
}

