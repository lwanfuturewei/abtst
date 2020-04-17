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

#include "hash.h"
#include "st.h"


/* Strage application initialize.
 */
int st_init(void)
{
	/* Initialize data structures here */

	return 0;
}

/* Storage application finalize.
 */
int st_finalize(void)
{
	return 0;
}

void st_thread_func(void *arg)
{
        io_param *param = (io_param *)arg;

	/* This is simulated work */
	/* We need to do:
	 * 1) obtain access right;
	 * 2) read/write ops;
	 * 3) update metadata (hash table/tree.
	 */
        int i, v = 0;
        for (i = 0; i < 100000; i++) {
                v += 1;
                v *= 2;
        }
        param->used = false;

        abtst_load_inc_ended(param->load);
        //printf("[TH%u]: s: %lu e: %lu\n", param->load->curr_rank,
        //        param->load->pkt_started,  param->load->pkt_ended);
}

