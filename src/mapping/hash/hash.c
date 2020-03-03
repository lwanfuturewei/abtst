/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "mapping.h"
#include "load.h"
#include "hash.h"
#include "env.h"


extern abtst_env env;
int hash_mapping_init(void *map);
uint32_t hash_mapping_to_load(void *map, void *loads, void *key);


static abtst_mapping hash_mapping = {
    .type = MAPPING_TYPE_HASHING,
    .init_mapping = hash_mapping_init,
    .mapping_to_load = hash_mapping_to_load,
};

abtst_mapping * get_hash_mapping_def(void)
{
	return (&hash_mapping);
}

void set_hash_info(struct hash_info *hash_info, uint32_t bucketNum, uint32_t blockSz, HASH_FN hashFunc)
{
	hash_info->total_buckets = bucketNum;
	hash_info->block_size = blockSz;
	hash_info->hash_fn = hashFunc;
}

int hash_mapping_init(void *map)
{
	return 0;
}


int32_t get_hash_key(HASH_FN hash_fn, void *key, int32_t bucketNum)
{
    return (int32_t)(uint64_t)(hash_fn(key) % ((uint32_t)bucketNum));
}

uint32_t hash_mapping_to_load(void *map, void *ploads, void *key)
{
	abtst_mapping *mapping = (abtst_mapping *)map;
	abtst_loads *loads = (abtst_loads *)ploads;
	int32_t hash;
	struct hash_info *hash_info = (struct hash_info *)mapping->param;
	uint32_t nr_loads = loads->nr_loads;
	uint32_t buckets_per_loads;
	struct USER_KEY_S *pKey = (struct USER_KEY_S *)key;
	struct USER_KEY_S user_key;

	user_key.lba = (pKey->lba / hash_info->block_size) * hash_info->block_size;
	user_key.volumeId = pKey->volumeId;
	hash = get_hash_key(hash_info->hash_fn, &user_key, hash_info->total_buckets);
	buckets_per_loads = (hash_info->total_buckets + nr_loads -1) / nr_loads;

	return ((uint32_t)hash / buckets_per_loads);
}

uint64_t abtst_hash_func(void *key)
{
    struct USER_KEY_S *pKey = (struct USER_KEY_S *)key;
    uint64_t priLoc = 0;
    uint64_t secLoc = 0;

    priLoc = pKey->lba >> 9;

    /*Thomas Wang's 64 bit mix function*/
    priLoc = (~priLoc) + (priLoc << 21); // key = (key << 21) - key - 1;
    priLoc = priLoc ^ (priLoc >> 24);
    priLoc = (priLoc + (priLoc << 3)) + (priLoc << 8); // key * 265
    priLoc = priLoc ^ (priLoc >> 14);
    priLoc = (priLoc + (priLoc << 2)) + (priLoc << 4); // key * 21
    priLoc = priLoc ^ (priLoc >> 28);
    priLoc = priLoc + (priLoc << 31);

    secLoc = pKey->volumeId;

    return priLoc + secLoc;
}

