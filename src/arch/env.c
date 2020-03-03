/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <strings.h>
#include "env.h"


abtst_env env;

void abtst_init_env(void)
{

    /* Get the number of available cores in the system */
    env.nr_cores = sysconf(_SC_NPROCESSORS_ONLN);

    /* Get NUMA info of the system */
    env.nr_numas = 4;

    env.stopping = false;
}

void abtst_set_stopping(void)
{
	env.stopping = true;
}

bool abtst_is_system_stopping(void)
{
	return (env.stopping);
}

