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

#include "list.h"
#include "spinlock.h"

typedef struct abtst_rebalance_struct
{
        atomic_int in_use;

	pthread_t thread;
	void *param;

	struct list_head req_q;
	spinlock_t lock;
} abtst_rebalance;

enum abtst_reb_req_type
{
	REB_REQUEST_ADD_CORE,
	REB_REQUEST_REM_CORE,
};

typedef struct abtst_reb_req_struct
{
	struct list_head list;
	int type;
	int core;
} abtst_reb_request;

int abtst_init_rebalance(abtst_rebalance *reb, void *param);
void abtst_free_rebalance(abtst_rebalance *reb);

int abtst_reb_req_add_core(void *global, int core);
int abtst_reb_req_remove_core(void *global, int core);
int abtst_reb_process_requests(void *global);


#endif
