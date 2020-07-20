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

#define IOS_PER_LOAD            (128)
#define LOOPS                   (1000)

extern int map_key_to_xstream(abtst_global *global, void *key);
extern int map_key_to_load(abtst_global *global, int mapping_id, void *key);


int main(int argc, char *argv[])
{
	int ret;
	abtst_global global;
	//struct timespec t;
	abtst_stream *stream;
	abtst_load *load;
	uint64_t lba = 0;
	int i, j = 0;
	io_param *params;
	int first = -1;
	struct list_head *pos;

	params = (io_param *)calloc(IOS_PER_LOAD * 8, sizeof(io_param));
	if (!params)
	{
		return -1;
	}
	//t.tv_sec = 0;
	//t.tv_nsec = 1000;
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

	/* Initialize storage system */
	ret = st_init();
	if (ret)
	{
		printf("st_init error %d", ret);
		goto EXIT1;
	}


	/* Block streams */
	for (i = 0; i < env.nr_cores; i++)
	{
		stream = &global.streams.streams[i];
		if (!stream->used)
		{
			continue;
		}
		if (first < 0)
		{
			first = i;
		}
		abtst_stream_set_blocking(stream, true);
	}

	stream = &global.streams.streams[first];

	/* Create threads for the first stream */
	list_for_each(pos, &stream->load_q)
	{
		load = list_entry(pos, abtst_load, list);
		for (i = 0; i < IOS_PER_LOAD; i++)
		{
			params[j].used = true;
	 		params[j].key.lba = lba;
			params[j].key.volumeId = 1;
			params[j].load = load;
			//printf("loop %d lba 0x%lx es %d load %d\n", j, params[j].key.lba, load[l].curr_rank, l);
			ret = ABT_thread_create(load->pool,
	                                st_thread_func, (void *)&params[j], ABT_THREAD_ATTR_NULL, NULL);
			if (ret) {
				printf("ABT_thread_create error %d\n", ret);
				goto EXIT1;
			}
			abtst_load_inc_started(load);

			lba += HASH_BLOCK_SIZE;
			j++;
		}
		if (j >= (IOS_PER_LOAD * 8))
		{
			break;
		}
	}

	print_streams(&global.streams);
	/* Wait for rebalance to kick in */
	for (i = 0; i < 10; i++)
	{
		sleep(1);
		print_streams(&global.streams);
	}

	/* Unblock streams */
	for (i = 0; i < env.nr_cores; i++)
	{
		stream = &global.streams.streams[i];
		if (!stream->used)
		{
			continue;
		}
		abtst_stream_set_blocking(stream, false);
	}

	sleep(2);

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

