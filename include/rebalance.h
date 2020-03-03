#ifndef ABTST_REBALANCE_H
#define ABTST_REBALANCE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>
#include <stdatomic.h>
#include <pthread.h>


typedef struct abtst_rebalance_struct
{
        atomic_int in_use;

	pthread_t thread;
	void *param;

} abtst_rebalance;


int abtst_init_rebalance(abtst_rebalance *reb, void *param);
void abtst_free_rebalance(abtst_rebalance *reb);


#endif
