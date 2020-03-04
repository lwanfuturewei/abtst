/*
 * See COPYRIGHT in top-level directory.
 */
#ifndef ST_H
#define ST_H

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "hash.h"
#include "load.h"


typedef struct io_param_struct
{
        bool used;
        //uint32_t load;
        //uint32_t stream;
        struct USER_KEY_S key;
        void *packet;
        abtst_load *load;

} io_param;


int st_init(void);
int st_finalize(void);
void st_thread_func(void *arg);


#endif

