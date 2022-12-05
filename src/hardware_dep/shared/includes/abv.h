#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>

#define ADRLEN          32
#define PORTLEN         16
#define MAXEXPANSION    PORTLEN
#define SRCADRLEN       ADRLEN/8
#define DESTADRLEN      ADRLEN/8
#define SRCPORTLEN      PORTLEN/8
#define DESTPORTLEN     PORTLEN/8
#define PROTOLEN        8/8 // ???
#define MISCLEN         16
#define TRUE            1
#define FALSE           0

#define ERROR           (-1)
#define SUCCESS         1

#define W               32

unsigned int WORDSIZE = 32; //define the size of the bit vector chunk to be aggregated

//#define INFINITY      0xffffffff
//#define MAXFILTERS    10000
#define MAXFILTERS      1048576

#define NUM_OF_PREFIX   2
#define NUM_OF_FIELD    5
#define NUM_OF_PROTOCOL 100

#define DEBUG			0
#define DEBUGC			0
#define DEBUG_SEARCH	0

typedef unsigned int unsint;
typedef unsigned char uchar;
typedef unsigned long unslong;

struct PORTPREFIX {
    uchar pref[NUM_OF_PREFIX];
    uchar len;
};

typedef struct PORTPREFIX PortPrefix;

#define SETPORT(x, y) x[0] = y / 256; x[1] = y % 256 ;

struct FILTER {
    unsint filtId;
    unsint cost;
    uchar  pref[NUM_OF_FIELD][SRCADRLEN];
    uchar  len[NUM_OF_FIELD];
    uchar  maxlen[NUM_OF_FIELD];
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
    unsint destPort;
    unsint sourcePort;
    uchar protocol;
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

struct FIND {
    //unsint gasit;
    unsint noReadAggr; // number of reads using our algorithm
    unsint noReadLuc; // number of reads using Lucent algorithm
};

typedef struct FIND Find;

unsint MaxNoReadLuc, MaxNoReadAggr, AvgNoReadLuc, AvgNoReadAggr;
unsint TotalNoCases, MaxNoReadAggrNF;

unsint MaxNoMatch = 0;

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

struct PROTOCOL {
    int valid;
    BitArray *bA;
};

typedef struct PROTOCOL Protocol;

BitArray *bA[NUM_OF_FIELD];
Trie *trieArray[NUM_OF_FIELD];
Protocol protocol[NUM_OF_PROTOCOL + 1];
