/*
 * See COPYRIGHT in top-level directory.
 */
#ifndef ABTST_HASH_H
#define ABTST_HASH_H

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>


typedef uint64_t (*HASH_FN)(void *key);

struct hash_info {
	uint32_t total_buckets;
	uint32_t block_size;
	HASH_FN hash_fn;
};

struct USER_KEY_S {
	uint64_t lba;
	uint64_t volumeId;
};

void set_hash_info(struct hash_info *hash, uint32_t bucketNum, uint32_t blockSz, HASH_FN hashFunc);

uint64_t abtst_hash_func(void *key);

#endif

