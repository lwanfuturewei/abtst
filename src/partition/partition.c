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

void abtst_reset_partition_stats(abtst_partitions *partitions)
{
	memset(partitions->numa_stats, 0, sizeof(abtst_partition_stat) * MAX_NUMAS * MAX_PARTITIONS);
	memset(partitions->stats, 0, sizeof(abtst_partition_stat) * MAX_PARTITIONS);

}

void abtst_update_partition_stats(abtst_global *global)
{
	int i, j;
	abtst_numa *numa_info;
	abtst_stream *stream;
	int pid;
	abtst_partitions *partitions = &global->partitions;

	abtst_reset_partition_stats(partitions);

	for (i = 0; i < env.nr_numas; i++)
	{
		numa_info = abtst_get_numa_info(i);
		for (j = numa_info->start_core; j <= numa_info->end_core; j++)
		{
			stream = &global->streams.streams[j];
			pid = stream->part_id;
			if (!stream->used)
			{
				continue;
			}
			partitions->numa_stats[pid][i].qdepth += abtst_stream_get_qdepth(stream);
			partitions->numa_stats[pid][i].used_cores++;

			partitions->stats[pid].qdepth += abtst_stream_get_qdepth(stream);
			partitions->stats[pid].used_cores++;
		}
	}
	//print_partitions(partitions);
}

void print_partitions(abtst_partitions *partitions)
{
	int i, j;

	for (i = 0; i < partitions->nr_partitions; i++)
	{
		printf("part %d, cores/qdepth %4d/%4d, ", i, partitions->stats[i].used_cores, partitions->stats[i].qdepth);
		for (j = 0; j < env.nr_numas; j++)
		{
			printf("%4d/%4d ", partitions->numa_stats[i][j].used_cores, partitions->numa_stats[i][j].qdepth);
		}
		printf("\n");
	}
}
