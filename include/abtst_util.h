/*
 * See COPYRIGHT in top-level directory.
 */
#ifndef ABTST_UTIL_H
#define ABTST_UTIL_H

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>


void set_color(int partition)
{
	int c = partition % 6;

	switch (c)
	{
	case 0:
		printf("\033[0;31m");	// read
		break;
	case 1:
		printf("\033[0;32m");	// green
		break;
	case 2:
		printf("\033[0;33m");	// yellow
		break;
	case 3:
		printf("\033[0;34m");	// blu
		break;
	case 4:
		printf("\033[0;35m");	// megenta
		break;
	case 5:
		printf("\033[0;36m");	// cyan
		break;
	default:
		break;
	}
}

void reset_color(void)
{
    printf("\033[0m");
}


#endif

