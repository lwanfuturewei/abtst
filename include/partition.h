#ifndef ABTST_PARTITION_H
#define ABTST_PARTITION_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>


#define MAX_PARTITIONS 8

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

} abtst_partitions;


static inline int *get_partition_map(abtst_partitions *partitons)
{
	return partitons->partition_map;
}

int abtst_init_partitions(abtst_partitions *partitions, int num, abtst_partition *init_partitions);
void abtst_free_partitions(abtst_partitions *partitions);


#endif
