 /*******************************************************************************************
  *	ABV (Aggregated Bit Vector)
  *   
  *	Author: Florin Baboescu
  *
  *	Last Update: Dec 08, 2002
  *
  *
  *	This source code is part of the Packet Classification Repository (PCR)
  *	at http://www.ial.ucsd.edu/
  *
  *	If you use this code we will apprecaite it if you cite the
  *	Packet Classification Repository in your publication.
  *
  *	If you would like to contribute paper publications, or
  *	source code to the PCR please email
  *	Sumeet Singh at susingh@cs.ucsd.edu
  *
  *
  *
  *
  *	Build:
  *	simply type make
  *	the binary will be called abv
  *
  *
  *	Usage:
  *	abv ruleSet traceFile outFile
  *
  *
  ********************************************************************************************/

#include "abv.h"
#include <stdio.h>

void CopyFilter(Filter f1, Filter f2) {

    memcpy((char *)f1, (char *)f2, sizeof(struct FILTER));
}

/* Allocates memory for a bit vector structure with a number of nF bits
 * It returns a pointer to the new allocated data structure if it succeeds or
 * it returns a null pointer and display an error message at stderr
 */
BitArray* createArray(int nF) {
    BitArray *temp;
    unsint t1, t2;

    t1 = (unsint)(nF / WORDSIZE) + 1;
    t2 = (unsint)(((unsint)(nF / WORDSIZE) + 1) / WORDSIZE) + 1;

    temp = (BitArray*)calloc(1, sizeof(BitArray));
    if (temp == (BitArray*)0) {
        if (DEBUG) printf("Error allocating memory in createArray\n");

        return temp;
    }

    temp->map = (unsint*)calloc(t1, sizeof(unsint));
    if (temp->map ==(unsint*)0) {
        free(temp);
        if (DEBUG) printf("Error allocating memory in createArray\n");

        return (BitArray*)0;
    }

    temp->aggregate = (unsint*)calloc(t2, sizeof(unsint));
    if (temp->aggregate == (unsint*)0) {
        free(temp);
        if (DEBUG) printf("Error allocating memory in createArray\n");

        return (BitArray*)0;
    }

    temp->noOfFilters = nF;

    return temp;
}

/* Releases the memory space allocated for a bit vector using createArray */
void removeArray(BitArray* bA) {

    if (bA == (BitArray*)0)
        return;

    if (bA->aggregate)
        free(bA->aggregate);

    if (bA->map)
        free(bA->map);

    free(bA);
}

/* Set the bit in the bit vector. It also sets the aggregate bit accordingly
 * filtNo should have a value between in [0, bA->noOfFilters)
 */
void setFilter(BitArray * bA, int filtNo) {
    unsint cuv, bitAggr, posInCuv, sunst, mask;

    if (bA == (BitArray*)0) {
        if (DEBUG) printf("ERROR: setFilter function: Null pointer argument\n");

        return;
    }

    if (filtNo >= bA->noOfFilters) {
        if (DEBUG) printf("ERROR: setFilter fnct:Not enough bits in the vector struct\n");

        return;
    }

    bitAggr = (unsint)(filtNo / WORDSIZE);
    sunst = 8 * sizeof(unsint); // No of bits in an unsigned int
    cuv = (unsint)(filtNo / sunst);
    posInCuv = filtNo % sunst;
    mask = 1 << posInCuv;
    if (bA->map == (unsint*)0) {
        if (DEBUG) printf("ERROR: setFilter function: Null pointer access\n");

        return;
    }
    bA->map[cuv] |= mask;

    cuv = (unsint)(bitAggr / sunst);
    posInCuv = bitAggr % sunst;
    mask = 1 << posInCuv;
    if (bA->aggregate == (unsint*)0) {
        if (DEBUG) printf("ERROR: setFilter function: Null pointer access\n");

        return;
    }
    bA->aggregate[cuv] |= mask;

    return;
}

/* Creates a Trie structure and returns a pointer to it. Takes the total 
 * number of filters as argument.
 * In case of an error prints a message and returns a null pointer
 */
Trie* InitTrie(int nF) {
    Trie *temp;

    temp = (Trie*)calloc(1, sizeof(Trie));
    if (temp == (Trie*)0) {
      if (DEBUG) printf("ERROR: InitTrie: Mem. allocation error\n");

      return temp;
    }

    temp->root = (Node*)calloc(1, sizeof(Node));
    if (temp->root == (Node*)0) {
      if (DEBUG) printf("ERROR: InitTrie: Mem. allocation error\n");

      free(temp);

      return (Trie*)0;
    }

    /*
    temp->root->filters = createArray(nF);
    if (temp->root->filters == (BitArray*)0) {
        if (DEBUG) printf("ERROR: InitTrie: Mem. allocation error\n");
        free(temp->root);
        free(temp);

        return (Trie*)0;
    }
    temp->root->type = PREFIX;
    temp->noPrefixNodes = 1;
    */

    temp->noNodes = 1;

    return temp;
}

void setFilterChild(Node*nod, int FiltNo) {

    if (nod == (Node *)0) return;

    if (nod->type == PREFIX)
      setFilter(nod->filters, FiltNo);

    setFilterChild(nod->zero, FiltNo);
    setFilterChild(nod->one, FiltNo);
}

void copyFilters(BitArray * source, BitArray *dest) {
    unsint nF, t1, t2, sunst, i;

    nF = source->noOfFilters;
    sunst = 8 * sizeof(unsint);
    t1 = (unsint)(nF / sunst) + 1;
    t2 = (unsint)(((unsint)(nF / WORDSIZE) + 1) / sunst) + 1;

    for (i = 0; i < t1; i++)
        dest->map[i] = source->map[i];

    //for (i = 0; i < t2; i++)
        //dest->aggregate[i] = source->aggregate[i];
}

/* Set the bit associated for the filter number given in the argument. It 
 * traverses the trie and it finds the node corresonding to the longest 
 * prefix. If it has childrens than it continues and it sets the same bit
 * in all the childrens.
 * If the node is not a PREFIX than it sets it to prefix and it does the same.
 */
void insertFilter(int FiltNo, int nF, uchar *pref, uchar len, Trie *trie) {
    Node *curNode;
    uchar i, curByte, curBit, flag = 0;
    BitArray *temp = (BitArray*)0;

    if (trie == (Trie*)0) {
        if (DEBUG) printf("ERROR: insertFilter: wrong trie initialization!\n");

        return;
    }

    curNode = trie->root;
    if (curNode ==(Node*)0) {
        if (DEBUG) printf("ERROR: insertFilter: wrong trie initialization!\n");

        return;
    }

    for (i = 0; i < len; i++) {
        if (curNode->type == PREFIX) {
            //setFilter(curNode->filters, FiltNo);
            temp = curNode->filters;
        }

        curByte = (uchar)(i / 8);
        curBit = i % 8;
        if (pref[curByte] & (1 << (7 - curBit))) {
            /* select bit i<len from the prefix */
            if (curNode->one == (Node*)0) {
                flag = 1;
                curNode->one = (Node*)calloc(1, sizeof(Node));
                curNode->one->pos = i + 1;;
                trie->noNodes++;
                if (curNode->one == (Node*)0) {
                    if (DEBUG) printf("ERROR: insertFilter: not enough memory!\n");

                    return;
                }
            }
            curNode = curNode->one;
        } else {
            if (curNode->zero == (Node*)0) {
                flag = 1;
                curNode->zero = (Node*)calloc(1,sizeof(Node));
                curNode->zero->pos = i + 1;
                trie->noNodes++;
                if (curNode->zero == (Node*)0) {
                    if (DEBUG) printf("ERROR: insertFilter: not enough memory!\n");

                    return;
                }
            }
            curNode = curNode->zero;
        }
    }

    if ((flag == 1) || (curNode->type == NOPREFIX)) {
        /* if a new created node (flag =1) or a node changing from 
         * NONPREFIX to PREFIX
         */
        //curNode->filters = createArray(filtset.numFilters); //by HAK
        curNode->filters = createArray(nF);
        //trie->noPrefixNodes++;
        curNode->type = PREFIX;
        if (curNode->filters == (BitArray*)0) {
            if (DEBUG) printf("ERROR: insertFilter: not enough memory!\n");

            return;
        }
        if (temp != (BitArray *)0)
            copyFilters(temp, curNode->filters);
    }

    setFilter(curNode->filters, FiltNo);
    curNode->filters->pos = len;

    setFilterChild(curNode->one, FiltNo);
    setFilterChild(curNode->zero, FiltNo);
}

BitArray* searchIPTrie(uchar *pref, Trie *trie) {
    Node *curNode;
    uchar i, curByte, curBit, flag=0;
    BitArray *temp = (BitArray*)0;

    if (trie == (Trie*)0) {
        if (DEBUG) printf("ERROR: searchTrie: wrong trie initialization!\n");

        return NULL;
    }

    curNode = trie->root;
    if (curNode == (Node*)0) {
        if (DEBUG) printf("ERROR: searchTrie: wrong trie initialization!\n");

        return NULL;
    }

    for (i = 0; i < WORDSIZE; i++) {
        if (curNode->type == PREFIX) {
            temp = curNode->filters;
        }
        curByte = (uchar)(i / 8);
        curBit = i%8;
        if (pref[curByte] & (1 << (7 - curBit))) {
            /* select bit i < len from the prefix */
            if (curNode->one == (Node*)0) {
                return temp;
            }
            curNode = curNode->one;
        } else {
            if (curNode->zero == (Node*)0) {
                return temp;
            }
            curNode=curNode->zero;
        }
    }
}

//unsint findMatch(struct FILTSET *filtset) {
uint8_t* findMatch(struct FILTSET *filtset) {
    unsint nrCuv, i, j, temp, result; 
    unsint curWord, nrBitAggr, nrCuvAggr, curPos, curCuv;

    for (int i = 0; i < NUM_OF_PREFIX; i++) {
        if (filtset->bA[i] == 0)
            return NULL;
    }

    nrCuv = filtset->bA[0]->noOfFilters / WORDSIZE + 1;
    nrBitAggr = filtset->bA[0]->noOfFilters / WORDSIZE + 1;
    nrCuvAggr = nrBitAggr / WORDSIZE;

    curPos = 0;
    for (curCuv = 0; curCuv < nrCuvAggr; curCuv++) {
        result = 0xFFFFFFFF;

        for (i = 0; i < NUM_OF_PREFIX; i++)
          result &= filtset->bA[i]->aggregate[curCuv];

        if (result) { // got a match!!!
            for (i = 0; i < WORDSIZE; i++)
                if (result & (((unsint)1) << i)) {
                    curWord = (curPos << NUM_OF_PREFIX + i);
                    result = 0xFFFFFFFF;

                    for (i = 0; i < NUM_OF_PREFIX; i++)
                        result &= filtset->bA[i]->map[curCuv];

                    for (j = 0; j < WORDSIZE; j++)
                        if (result & (((unsint)1) << j)) {
                            temp = curWord << NUM_OF_PREFIX + j;

                            return (uint8_t*)temp;
                        }
                }
        }
        curPos += WORDSIZE;
    }

    return NULL;
}

struct FILTSET * abv_init(struct FILTSET *filtset) {

    /* Allocate for the data structure when the argument is not NULL, and then
       clear all the variables */
    //filtset = malloc(sizeof(struct FILTSET));
    filtset = (FiltSet)calloc(1, sizeof(FiltSet));
    if (NULL == filtset) {
        /* Memory allocation error */
        return NULL;
    }

    debug("abv_init calloc\n");

    filtset->numFilters = 0;

    for (int i = 0; i < NUM_OF_PREFIX; i++)
        filtset->trieArray[i] = InitTrie(0);

    return filtset;
}

void abv_release(struct FILTSET *filtset) {

    return;
}

void abv_add(struct FILTSET *filtset, uint8_t* key, uint8_t* mask, uint8_t* value) {
    struct FILTER tempfilt1, *tempfilt;

    if (filtset->numFilters == MAXFILTERS) {
        if (DEBUG) printf("Out of memory: too many filters\n");

        return;
    }

    if (*mask == LASTFILTERSYMBOLOFMASK) { // symbol of the end of filter (/255) "x.x.x.x/255"
        for (int i = 0; i < filtset->numFilters; i++) {
            for (int j = 0; j < NUM_OF_PREFIX; j++)
                insertFilter(i, filtset->numFilters, filtset->filtArr[i].pref[j], filtset->filtArr[i].len[j], filtset->trieArray[j]);
        }

        //for (int i = 0; i < NUM_OF_PREFIX; i++)
        //    insertFilter(filtset->numFilters, filtset->numFilters + 1, filtset->filtArr[filtset->numFilters].pref[i], filtset->filtArr[filtset->numFilters].len[i], filtset->trieArray[i]);
    }
    else {
        tempfilt = &tempfilt1;

        // ACL like "0.0.0.0/0 10.0.1.0/24"
        tempfilt->pref[0][0] = 0;
        tempfilt->pref[0][1] = 0;
        tempfilt->pref[0][2] = 0;
        tempfilt->pref[0][3] = 0;
        tempfilt->len[0] = 0;

        tempfilt->pref[1][0] = *(key);
        tempfilt->pref[1][1] = *(key+1);
        tempfilt->pref[1][2] = *(key+2);
        tempfilt->pref[1][3] = *(key+3);
        tempfilt->len[1] = *mask;

        tempfilt->filtId = 1 + filtset->numFilters;
        tempfilt->cost = 1 + filtset->numFilters;

        tempfilt->maxlen[0] = tempfilt->maxlen[1] = ADRLEN/8;

        tempfilt->value = value;

        CopyFilter(&(filtset->filtArr[filtset->numFilters]), tempfilt);

        filtset->numFilters++;
    }
}

uint8_t * abv_lookup(struct FILTSET *filtset, uint8_t* key) {
    struct PACKET tempPkt1, *tempPkt;

    tempPkt = &tempPkt1;

    // Lookup like "0.0.0.0 10.0.1.0"
    tempPkt->pref[0][0] = 0;
    tempPkt->pref[0][1] = 0;
    tempPkt->pref[0][2] = 0;
    tempPkt->pref[0][3] = 0;

    tempPkt->pref[1][0] = *(key);
    tempPkt->pref[1][1] = *(key+1);
    tempPkt->pref[1][2] = *(key+2);
    tempPkt->pref[1][3] = *(key+3);
 

    for (int i = 0; i < NUM_OF_PREFIX; i++)
        filtset->bA[i] = searchIPTrie(tempPkt->pref[i], filtset->trieArray[i]);

    //return (uint64_t)findMatch(filtset);
    return (uint8_t*)findMatch(filtset);
}
