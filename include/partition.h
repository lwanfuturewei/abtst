#ifndef ABTST_PARTITION_H
#define ABTST_PARTITION_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "env.h"


#define MAX_PARTITIONS 8

typedef struct abtst_partition_stat_struct
{
	uint32_t used_cores;
	uint32_t qdepth;
} abtst_partition_stat;

typedef struct abtst_partition_struct
{
	int partition_id;
	int nr_cores;
	int *partition_cores;

} abtst_partition;

typedef struct abtst_partitions_struct
{
	int nr_partitions;
	abtst_partition *partitions;

	int *partition_map;

	abtst_partition_stat numa_stats[MAX_PARTITIONS][MAX_NUMAS];
	abtst_partition_stat stats[MAX_PARTITIONS];


} abtst_partitions;


static inline int *get_partition_map(abtst_partitions *partitons)
{
	return partitons->partition_map;
}

static inline abtst_partition_stat * abtst_get_partition_stat(abtst_partitions *partitions, int numa_id, int p_id)
{
	if (numa_id < 0) {
		return &partitions->stats[p_id];
	}
	else {
		return &partitions->numa_stats[p_id][numa_id];
	}
}

int abtst_init_partitions(abtst_partitions *partitions, int num, abtst_partition *init_partitions);
void abtst_free_partitions(abtst_partitions *partitions);
void print_partitions(abtst_partitions *partitions);


#endif
