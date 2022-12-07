#ifndef _RANGE_H
#define _RANGE_H

#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <sys/types.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#define ADRLEN          32
#define SRCADRLEN       ADRLEN/8
#define DESTADRLEN      ADRLEN/8
#define TRUE            1
#define FALSE           0

#define ERROR           (-1)
#define SUCCESS         1

unsigned int WORDSIZE = 32; //define the size of the bit vector chunk to be aggregated

//#define MAXFILTERS    10000
#define MAXFILTERS      1048576

#define NUM_OF_PREFIX   2
//#define NUM_OF_FIELD    2

#define DEBUG			0
#define DEBUGC			0
#define DEBUG_SEARCH	0

typedef unsigned int unsint;
typedef unsigned char uchar;
typedef unsigned long unslong;

void range_init();
void range_release();
void range_add(uint8_t* key, uint8_t* mask, uint8_t* value);
uint8_t * range_lookup(uint8_t* key);

#endif
