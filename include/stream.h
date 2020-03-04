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


typedef struct abtst_stream_struct
{
	uint32_t rank;
	bool used;
	ABT_xstream xstream;
	ABT_pool pool;	
	ABT_sched sched;	

	struct list_head load_q;
	uint32_t nr_loads;
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


int abtst_init_streams(abtst_streams *streams);
int abtst_finalize_streams(abtst_streams *streams);
void abtst_free_streams(abtst_streams *streams);
void abtst_add_load_to_stream(abtst_stream *stream, struct list_head *entry);
void abtst_remove_load_from_stream(abtst_stream *stream, struct list_head *entry);
int abtst_get_stream_ios(abtst_stream *stream);


#endif
