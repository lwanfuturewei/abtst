/*
 * See COPYRIGHT in top-level directory.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "env.h"
#include "mapping.h"
#include "load.h"
#include "abtst_stream.h"
#include "sched.h"


static int init_load(abtst_load *load, uint32_t rank)
{
	int ret;

	load->pkt_started = load->pkt_ended = 0;
	load->init_rank = load->curr_rank = rank;
	load->dest_rank = rank;
	//load->migrating = false;

	INIT_LIST_HEAD(&load->list);

	ret = ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPSC,
				ABT_TRUE, &load->pool);
	if (ret)
	{
		printf("ABT_pool_create_basic error %d\n", ret);
		return ret;
	}

	//ret = ABT_sched_create_basic(ABT_SCHED_DEFAULT, 1, &load->pool,
	//                         ABT_SCHED_CONFIG_NULL, &load->sched);
	ret = abtst_init_sub_sched(&load->sched, &load->pool, (void *)load);
	if (ret)
	{
		printf("abtst_init_sub_sched error %d\n", ret);
		return ret;
	}

	return 0;
}

static abtst_stream * find_next_stream(abtst_streams *streams, int partition_id, int start)
{
	int s = start % streams->max_xstreams;
	int i;

	for (i = s; i < streams->max_xstreams; i++)
	{
		if (streams->streams[i].used && (partition_id == streams->streams[i].part_id))
		{
			return &streams->streams[i];
		}
	}

	for (i = 0; i < s; i++)
	{
		if (streams->streams[i].used && (partition_id == streams->streams[i].part_id))
		{
			return &streams->streams[i];
		}
	}

	return NULL;
}

int abtst_init_loads(abtst_loads *loads, abtst_streams *streams)
{
	int i;
	int start = 0;
	abtst_stream *stream;
	int ret;
	abtst_load *load;

	//loads->nr_loads = env.nr_cores * LOADS_PER_XSTREAM;
	loads->loads  = calloc(loads->nr_loads, sizeof(abtst_load));
	if (!loads->loads) {
		return -1;
	}

	for (i = 0; i < loads->nr_loads; i++)
	{
		stream = find_next_stream(streams, loads->partition_id, start);
		if (!stream)
		{
			printf("No stream in use!\n");
			return -1;
		}
		start = stream->rank + 1;
		
		load = &loads->loads[i];
		load->load_id = i;
		ret = init_load(load, stream->rank);
		if (ret) 
		{
			return -1;
		}

		ret = ABT_pool_add_sched(stream->pool, load->sched);
		if (ret)
		{
			return -1;
		}

		abtst_add_load_to_stream(stream, &load->list);
	}

	return 0;
}

void abtst_free_loads(abtst_loads *loads)
{
	int i;
	abtst_load *load;

	if (!loads->loads)
	{
		return;
	}
		
	load = loads->loads;
	for (i = 0; i < loads->nr_loads; i++, load++)
	{
		if (load->sched)
		{
			abtst_free_sub_sched(&load->sched);
		}
	}

	free(loads->loads);
}

int map_load_to_stream(abtst_loads *loads, uint32_t load)
{
	abtst_load *p_load;

	p_load = (abtst_load *)loads->loads;
	p_load += load;

	return (p_load->curr_rank);
}

