/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

//#include "abt.h"
#include "abt_st.h"
#include "rebalance.h"
#include "env.h"
#include "list.h"


void abtst_reb_process_add_core(abtst_global *global, int core);
void abtst_reb_process_remove_core(abtst_global *global, int core);

int abtst_reb_req_add_core(void *p_global, int core)
{
	abtst_global *global = (abtst_global *)p_global;
	abtst_reb_request *req;

	if ((core <= 0) || (core >= env.nr_cores))
	{
		printf("abtst_reb_add_core error core %d\n", core);
		return -1;
	}

	req = (abtst_reb_request *)malloc(sizeof(abtst_reb_request));
	if (!req)
	{
                printf("abtst_reb_add_core malloc failed\n");
		return -1;
	}

	INIT_LIST_HEAD(&req->list);
	req->type = REB_REQUEST_ADD_CORE;
	req->core = core;
	spin_lock(&global->rebalance.lock);
	list_add_tail(&req->list, &global->rebalance.req_q);
        spin_unlock(&global->rebalance.lock);

	return 0;
}

int abtst_reb_req_remove_core(void *p_global, int core)
{
        abtst_global *global = (abtst_global *)p_global;
	abtst_reb_request *req;

	if (core >= (int)env.nr_cores)
        {
               	printf("abtst_reb_add_core error core %d\n", core);
               	return -1;
        }	

        req = (abtst_reb_request *)malloc(sizeof(abtst_reb_request));
        if (!req)
        {
                printf("abtst_reb_add_core malloc failed\n");
                return -1;
        }

        INIT_LIST_HEAD(&req->list);
        req->type = REB_REQUEST_REM_CORE;
        req->core = core;
	spin_lock(&global->rebalance.lock);
	list_add_tail(&req->list, &global->rebalance.req_q);
	spin_unlock(&global->rebalance.lock);

	return 0;
}

int abtst_reb_process_requests(void *p_global)
{
        abtst_global *global = (abtst_global *)p_global;
	abtst_reb_request *req;
        struct list_head *pos, *n;

	spin_lock(&global->rebalance.lock);
        list_for_each_safe(pos, n, &global->rebalance.req_q)
        {
		list_del(pos);
		spin_unlock(&global->rebalance.lock);

                req = list_entry(pos, abtst_reb_request, list);
      		switch(req->type)
		{
			case REB_REQUEST_ADD_CORE:
				abtst_reb_process_add_core(global, req->core);
				break;
			case REB_REQUEST_REM_CORE:
				abtst_reb_process_remove_core(global, req->core);
				break;
			default:
				break;
		}
		
		free(req);
	 	spin_lock(&global->rebalance.lock);
	}
	spin_unlock(&global->rebalance.lock);

	return 0;	
}
