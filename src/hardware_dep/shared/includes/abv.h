#ifndef _ABV_H
#define _ABV_H

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

//unsigned int WORDSIZE = 32; //define the size of the bit vector chunk to be aggregated
#define WORDSIZE        32 //define the size of the bit vector chunk to be aggregated

//#define MAXFILTERS    10000
#define MAXFILTERS      1048576

#define LASTFILTERSYMBOLOFMASK       255

#define NUM_OF_PREFIX   2

#define DEBUG			0
#define DEBUGC			0
#define DEBUG_SEARCH	0

typedef unsigned int unsint;
typedef unsigned char uchar;
typedef unsigned long unslong;

struct PACKET {
    uchar pref[NUM_OF_PREFIX][SRCADRLEN];
};

/* Data structure to keep a bit vector for the filters associated
 * with any valid prefix
 */
struct BITARRAY {
    unsint *aggregate; // aggregate map of the bit vector
    unsint *map; // bit vector keeping the filters associated with a given prefix
    unsint noOfFilters; // total number of filters
    unsint pos;
    //unsint curFilter; // the filter which is checked(initial 0) < noOfFilters
    //unsint curAggr; // the aggregate which is currently checked(initial 0) <noAggr
    //unsigned noAggr;
};

typedef struct BITARRAY BitArray;

#define NOPREFIX 0
#define PREFIX 1

struct NODE {
    unsint type; // valid prefix
    struct NODE *zero;
    struct NODE *one;
    BitArray *filters;
    unsint pos;
};

typedef struct NODE Node;

struct TRIE {
    unsint noNodes;
    Node * root;
    unsint noPrefixNodes;
};

typedef struct TRIE Trie;

struct FILTER {
    unsint filtId;
    unsint cost;
    uchar  pref[NUM_OF_PREFIX][SRCADRLEN];
    uchar  len[NUM_OF_PREFIX];
    uchar  maxlen[NUM_OF_PREFIX];

    uint8_t* value;
};

typedef struct FILTER* FilterPtr;

struct FILTSET {
    unsint numFilters;
    BitArray *bA[NUM_OF_PREFIX];
    Trie *trieArray[NUM_OF_PREFIX];
    struct FILTER filtArr[MAXFILTERS];
};

typedef struct FILTSET* FiltSetPtr;

struct FILTSET * abv_init(struct FILTSET *);
void abv_release(struct FILTSET *filtset);
void abv_add(struct FILTSET *, uint8_t*, uint8_t*, uint8_t*);
uint8_t * abv_lookup(struct FILTSET *, uint8_t*);

#endif
