#ifndef ABTST_STREAM_H
#define ABTST_STREAM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>

#include "abt.h"
#include "list.h"


typedef struct abtst_stream_stat_s
{
	uint32_t total_qdepth;
	uint32_t sleep_nsec;
} abtst_stream_stat;

typedef struct abtst_stream_struct
{
	uint32_t rank;
	bool used;
	int part_id;
	ABT_xstream xstream;
	ABT_pool pool;	
	ABT_sched sched;	

	struct list_head load_q;
	uint32_t nr_loads;

	bool blocking;

	abtst_stream_stat stat;
} abtst_stream;

typedef struct abtst_streams_struct 
{
	uint32_t nr_xstreams;
	uint32_t init_xstreams;
	uint32_t max_xstreams;

	abtst_stream * streams;
	//ABT_sched *scheds;
	//ABT_pool *pools;

} abtst_streams;

typedef struct sort_param_s
{
	int ios;
	abtst_stream *stream;
} sort_param;


static inline void abtst_stream_init_stat(abtst_stream *stream)
{
	stream->stat.total_qdepth = 0;
	stream->stat.sleep_nsec = 0;
}

static inline uint32_t abtst_stream_get_rank(abtst_stream *stream)
{
	return stream->rank;
}
static inline uint32_t abtst_stream_get_qdepth(abtst_stream *stream)
{
	return stream->stat.total_qdepth;
}

static inline void abtst_stream_update_qdepth(abtst_stream *stream, uint32_t qdepth)
{
	stream->stat.total_qdepth = qdepth;
}

static inline uint32_t abtst_stream_get_sleep_time(abtst_stream *stream)
{
	return stream->stat.sleep_nsec;
}

static inline void abtst_stream_update_sleep_time(abtst_stream *stream, uint32_t sleep_nsec)
{
	stream->stat.sleep_nsec += sleep_nsec;
}

static inline void abtst_stream_reset_sleep_time(abtst_stream *stream)
{
	stream->stat.sleep_nsec = 0;
}

static inline bool abtst_stream_is_blocking(abtst_stream *stream)
{
	return stream->blocking;
}

static inline void abtst_stream_set_blocking(abtst_stream *stream, bool blocking)
{
	stream->blocking = blocking;
}

static inline int abtst_stream_get_partition_id(abtst_stream *stream)
{
	return stream->part_id;
}

static inline void abtst_stream_set_partition_id(abtst_stream *stream, int part_id)
{
	stream->part_id = part_id;
}


int abtst_init_streams(abtst_streams *streams, void *global);
int abtst_finalize_streams(abtst_streams *streams);
void abtst_free_streams(abtst_streams *streams);
int abtst_combine_streams(abtst_stream *from, abtst_stream *to);
void abtst_add_load_to_stream(abtst_stream *stream, struct list_head *entry);
void abtst_remove_load_from_stream(abtst_stream *stream, struct list_head *entry);
int abtst_rebalance_streams(sort_param *p1, sort_param *p2, uint32_t avg);
void abtst_update_streams_stat(abtst_streams *streams);
void abtst_reset_streams_stat(abtst_streams *streams);
void print_streams(abtst_streams *streams);
void print_stream_qdepth(abtst_streams *streams);
void print_stream_sleeptime(abtst_streams *streams, int interval);


#endif
