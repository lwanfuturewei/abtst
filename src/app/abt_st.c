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

#include "abt.h"
#include "abt_st.h"
#include "env.h"
#include "hash.h"


#define HASH_TABLE_BUCKETS      (1024 * 8 * 16)
#define HASH_BLOCK_SIZE         (64 * 1024)

#define PARALLEL_IOS            (128)
#define LOOPS                   (1000)

extern int map_key_to_xstream(abtst_global *global, void *key);
extern int map_key_to_load(abtst_global *global, void *key);

typedef struct io_param_struct
{
	bool used;
	//uint32_t load;
	//uint32_t stream;
	struct USER_KEY_S key;
	void *packet;
	abtst_load *load;

} io_param;

int abtst_init(abtst_global *global, void *param)
{
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

void thread_func(void *arg)
{
	io_param *param = (io_param *)arg;
	
	int i, v = 0;
	for (i = 0; i < 100000; i++) {
		v += 1;
		v *= 2;
	}
	param->used = false;

	abtst_load_inc_ended(param->load);
	printf("[TH%u]: s: %lu e: %lu\n", param->load->curr_rank,
		param->load->pkt_started,  param->load->pkt_ended);
}

int main(int argc, char *argv[])
{
	int ret;
	abtst_global global;
	struct timespec t;

        int i, j;
	io_param *params;

	params = (io_param *)calloc(PARALLEL_IOS, sizeof(io_param));
	if (!params)
	{
		return -1;
	}
	t.tv_sec = 0;
	t.tv_nsec = 1000;
	memset(&global, 0, sizeof(abtst_global));

        /* Initialize Argobots */
        ret = ABT_init(argc, argv);
	if (ret)
	{
        	printf("ABT_init error %d", ret);
		free(params);
		return ret;
	}
	
	/* Init env */
	abtst_init_env();

	/* Init structures */
        struct hash_info hash;
        set_hash_info(&hash, HASH_TABLE_BUCKETS, HASH_BLOCK_SIZE, abtst_hash_func);

	ret = abtst_init(&global, &hash);
        if (ret)
        {
                printf("abtst_init error %d", ret);
                goto EXIT1;
        }

	/* Create threads */
	//abtst_stream *stream = global.streams.streams;
	abtst_load *load = global.loads.loads;

	uint64_t lba = 0;
	for (j = 0; j < LOOPS; j++) {
	for (i = 0; i < PARALLEL_IOS; i++) {
		if (params[i].used) {
			continue;
		}
		params[i].used = true;
 		params[i].key.lba = lba;
		params[i].key.volumeId = 1;

		//int s = map_key_to_xstream(&global, &params[i].key);
		//params[i].stream = s;
		//printf("loop %d lba 0x%lx es %d\n", j, params[i].key.lba, s);
		//ret = ABT_thread_create(stream[s].pool,
		//		thread_func, (void *)&params[i], ABT_THREAD_ATTR_NULL,
		//		NULL);
		
		int l = map_key_to_load(&global, &params[i].key);
		params[i].load = &load[l];
		printf("loop %d lba 0x%lx es %d load %d\n", j, params[i].key.lba, load[l].curr_rank, l);
		ret = ABT_thread_create(load[l].pool,
                              thread_func, (void *)&params[i], ABT_THREAD_ATTR_NULL, NULL);
		if (ret) {
			printf("ABT_thread_create error %d\n", ret);
			goto EXIT1;
		}
		abtst_load_inc_started(&load[l]);
			
		lba += HASH_BLOCK_SIZE;
	}
	nanosleep(&t, NULL);
	}

EXIT1:
	abtst_set_stopping();

	/* Wait all work to be done */
	ret = abtst_finalize(&global);
        if (ret)
        {
                printf("abtst_finalize error %d", ret);
        }

	/* Finalize Argobots */
	ret = ABT_finalize();
	if (ret)
	{
		printf("ABT_finalize error %d", ret);
	}

	abtst_free(&global);

	free(params);
	return 0;
}

