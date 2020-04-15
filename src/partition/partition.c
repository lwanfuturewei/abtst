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

#include "abt_st.h"
#include "partition.h"
#include "env.h"


static int init_partition_map(abtst_partitions *partitions)
{
	partitions->partition_map = malloc(env.nr_cores * sizeof(int));
	if (!partitions->partition_map)
	{
		printf("init_partition_map error\n");
		return -1;
	}

	memset(partitions->partition_map, 0xff, env.nr_cores * sizeof(int));
	return 0;
}

static void free_partition_map(abtst_partitions *partitions)
{
	if (partitions->partition_map)
	{
		free(partitions->partition_map);
	}
}

int abtst_init_partitions(abtst_partitions *partitions, int num, abtst_partition *init_partitions)
{
	abtst_partition *partition = init_partitions;
	int i, j;
	int core;
	int ret;

	partitions->nr_partitions = num;
	partitions->partitions = init_partitions;

	ret = init_partition_map(partitions);
	if (ret)
	{
		partitions->partition_map = NULL;
		return -1;
	}

	for (j = 0; j < num; j++, partition++)
	{
		for (i = 0; i < partition->nr_cores; i++)
		{
			core = partition->partition_cores[i];
			if (partitions->partition_map[core] >= 0)
			{
				printf("abtst_init_partitions core %d already in partition %d\n",
						core, partitions->partition_map[core]);
				return -1;
			}

			partitions->partition_map[core] = partition->partition_id;
		}
	}

	return 0;
}

void abtst_free_partitions(abtst_partitions *partitions)
{
	free_partition_map(partitions);
}
