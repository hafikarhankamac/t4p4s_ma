#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include <time.h>

#define ADRLEN          32
#define SRCADRLEN       ADRLEN/8

#define TRUE            1
#define FALSE           0

#define ERROR           (-1)
#define SUCCESS         1

#define W               32
#define BINTH			1

#define MAXFILTERS		800000

#define MAX_STRIDE		8

#define DEBUG			0
#define DEBUGC			0
#define DEBUG_SEARCH	0

typedef unsigned int unsint;
typedef unsigned char uchar;
typedef unsigned long unslong;

#define max(X,Y)	( ((X) > (Y)) ? (X) : (Y) )
#define min(X,Y)	( ((X) > (Y)) ? (Y) : (X) )

// masks
#define HEXMinMask(A, B)	((A) & (((0xffffffff >> (32 - B))) << (32 - B)))
#define HEXMaxMask(A, B)	((B == 32) ? (A) :((A) | (0xffffffff >> (B))))

unsint MASKS[] = { 0x00000000, 0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080, 0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000, 0x00010000, 0x00020000, 0x00040000, 0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000, 0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000, 0x20000000, 0x40000000, 0x80000000 };

#define BitX(A, X)		( ((A) & MASKS[(X % 32)]) ? 1 : 0)
#define MSBBitX(A, X)	( ((A) & MASKS[(32 - X)]) ? 1 : 0)

struct FILTER {					// == 24 bytes..
	unsint filtId;			
	unsint cost;				// 4 bytes
	uchar pref[2][SRCADRLEN];	// 4 + 4 bytes
	unsint ipHEX[2];
	uchar len[2];				// 1 + 1 bytes
	uchar protPref;				// 1 byte
	uchar protLen;			
	unsint fromPort[2];			// 2 + 2 bytes
	unsint toPort[2];			// 2 + 2 bytes
	uchar act;					// 1 byte
};

typedef struct FILTER* PFilter;
typedef struct FILTER* Filter;

struct FILTSET {
	unsint numFilters;
	struct FILTER filtArr[MAXFILTERS];
};

typedef struct FILTSET* FiltSet;
struct FILTSET filtset;

struct TRIESUBNODE {
	uchar protPref;
	uchar protLen;
	unsint fromPort[2];
	unsint toPort[2];
	uchar  act;
	unsigned int cost;
	struct FILTER *pfilter;
	struct TRIESUBNODE* next;
	struct TRIESUBNODE* nextI[3];
};

typedef struct TRIESUBNODE TrieSubNode;
typedef struct TRIESUBNODE* PTrieSubNode;

struct TRIENODE {
	struct TRIENODE *zero;
	struct TRIENODE *one;
	struct TRIENODE *dest;
	struct TRIENODE *parent;
	struct TRIENODE *jump;
	struct TRIESUBNODE* pdimList; // pointer to rules array belongs to this node..
	struct TRIESUBNODE* pdimListI[3];
	uchar level;
	int tempFiltCount;
	int longestPath;
	unsint prefix;
	unsint validPrefixes;
	//int filtID[MAXFILTERS];
	unsint prefixesTillNow;
};

typedef struct TRIENODE TrieNode;
typedef struct TRIENODE* PTrieNode;

struct TRIENODEC {
	uchar  level;
	unsint zmask;
	unsint omask;
	uchar  zmaskLen;
	uchar  omaskLen;
	struct TRIENODEC *zero;
	struct TRIENODEC *one;
	struct TRIENODEC *dest;
	struct TRIENODEC *parent;
	struct TRIENODEC *jump; // not used
	struct TRIENODEC *fail;
	struct TRIESUBNODE* pdimList;
	struct TRIESUBNODE* pdimListI[3];

	// for accounting
	int longestPath;
	int tempFiltCount;
	unsint prefix;
	unsint validPrefixes;
	unsint prefixesTillNow;
};

typedef struct TRIENODEC TrieNodeC;
typedef struct TRIENODEC* PTrieNodeC;

/* Global Variables */

// pointer to root node
TrieNode *trieroot;
//TrieNode *trierootR;
TrieNodeC *rootC;

unsigned int validate[MAXFILTERS];

PTrieSubNode SEARCH_RESULTS[100];
int nSearchResults = 0;
