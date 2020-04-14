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

#include "env.h"
#include "abt.h"
#include "abt_st.h"
#include "st.h"
#include "hash.h"


#define HASH_TABLE_BUCKETS      (1024 * 8 * 16)
#define HASH_BLOCK_SIZE         (64 * 1024)

#define PARALLEL_IOS            (128)
#define LOOPS                   (1000)

//extern int map_key_to_xstream(abtst_global *global, void *key);
extern int map_key_to_load(abtst_global *global, int mapping_id, void *key);

abtst_global global;

int main(int argc, char *argv[])
{
	int ret;
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
	
	/* Initialize abt-st */
	ret = abtst_init(&global, NULL);
	if (ret)
	{
		printf("abtst_init error %d", ret);
		goto EXIT1;
	}

	/* Add mappings */
	/* We can add multiple mappings */
	struct hash_info hash;
	set_hash_info(&hash, HASH_TABLE_BUCKETS, HASH_BLOCK_SIZE, abtst_hash_func);
	ret = abtst_create_mapping(&global, MAPPING_TYPE_HASHING, &hash, 0);
	if (ret < 0)
	{
		printf("abtst_create_mapping error %d", ret);
		goto EXIT1;
	}
	int mapping_id = ret;

	/* Initialize storage system */
	ret = st_init();
	if (ret)
	{
		printf("st_init error %d", ret);
		goto EXIT1;
	}


	/* Create threads */
	//abtst_stream *stream = global.streams.streams;
	abtst_load *load = global.mappings[mapping_id].loads.loads;

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

			int l = map_key_to_load(&global, mapping_id, &params[i].key);
			params[i].load = &load[l];
			//printf("loop %d lba 0x%lx es %d load %d\n", j, params[i].key.lba, load[l].curr_rank, l);
			ret = ABT_thread_create(load[l].pool,
											st_thread_func, (void *)&params[i], ABT_THREAD_ATTR_NULL, NULL);
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

	/* Storage system exit */
	ret = st_finalize();
	if (ret)
	{
		printf("st_finalize error %d", ret);
	}

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

