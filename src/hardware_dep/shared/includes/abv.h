#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>

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

struct FILTER {
    unsint filtId;
    unsint cost;
    uchar  pref[NUM_OF_PREFIX][SRCADRLEN];
    uchar  len[NUM_OF_PREFIX];
    uchar  maxlen[NUM_OF_PREFIX];
};

typedef struct FILTER* Filter;

struct FILTSET {
    unsint numFilters;
    struct FILTER filtArr[MAXFILTERS];
};

typedef struct FILTSET* FiltSet;
struct FILTSET filtset;

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

BitArray *bA[NUM_OF_PREFIX];
Trie *trieArray[NUM_OF_PREFIX];
