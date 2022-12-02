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

void abvCopyFilter(Filter f1, Filter f2) {

    memcpy((char *)f1, (char *)f2, sizeof(struct FILTER));
}

void abvReadPrefix(FILE *fp, unsigned char* pref, unsigned char *len) {
    /* assumes IPv4 prefixes */
    unsigned int tpref[4], templen;

    int matches = fscanf(fp, "%d.%d.%d.%d/%d", &tpref[0], &tpref[1], &tpref[2], &tpref[3], &templen);
    pref[0] = (unsigned char)tpref[0];
    pref[1] = (unsigned char)tpref[1];
    pref[2] = (unsigned char)tpref[2];
    pref[3] = (unsigned char)tpref[3];

    *len = (unsigned char) templen;
}

void abvReadIPAddr(FILE *fp, unsigned char *pref) {
    unsigned int tpref[4];

    int matches = fscanf(fp, "%d.%d.%d.%d", &tpref[0], &tpref[1], &tpref[2], &tpref[3]);
    pref[0] = (unsigned char)tpref[0];
    pref[1] = (unsigned char)tpref[1];
    pref[2] = (unsigned char)tpref[2];
    pref[3] = (unsigned char)tpref[3];
}

void abvReadPortAddr(FILE *fp, unsigned int *port) {
    unsigned int tempPort;

    int matches = fscanf(fp, "%d", &tempPort);
    *port = tempPort;
}

/* pre: start[i:PORTLEN] == end[i:PORTLEN] && start[0:i-1]==0 && 
   end[0:j-1]==1 
*/
PortPrefix convert_full_range_to_prefix(unsigned int start, unsigned int end)  {
    PortPrefix portpref;

    SETPORT(portpref.pref, start);
    portpref.len = PORTLEN - floor(log10(end + 1 - start) / log10(2) + 0.5);

    return portpref;
}

/* arbitrary range */
int convert_any_range_to_prefix(unsigned int portstart, unsigned int portend, unsigned char *num_prefix, PortPrefix *portpref) {
    unsigned int start, end, half, k, k1, k2;

    unsigned int binary_start[PORTLEN], binary_end[PORTLEN];

    int i, j;

    start = portstart;
    end = portend;

    /* 0 : 15 from least significant to most significant */
    for (i = 0; i < PORTLEN; i++) {
        binary_start[i] = start % 2;
        start /= 2;
    }

    for (i = 0; i < PORTLEN; i++) {
        binary_end[i] = end % 2;
        end /= 2;
    }

    for (i = PORTLEN - 1; i >= 0; i--) {
        if (binary_start[i] != binary_end[i])
            break;
    }

    for (j = i; j >= 0; j--) {
        if (binary_start[j] != 0 || binary_end[j] != 1)
            break;
    }

    /* base case */
    if (j == -1) {
        portpref[(*num_prefix)++] = convert_full_range_to_prefix(portstart, portend);

        return 0;
    }

    /* split */
    k = ceil(log10(portend + 1) / log10(2));
    half = pow(2, k - 1);

    if (portstart < half) {
        convert_any_range_to_prefix(portstart, half - 1, num_prefix, portpref);
        convert_any_range_to_prefix(half, portend, num_prefix, portpref);

        return 0;
    } else {
        start = pow(2, k - 1);
        end = pow(2, k) - 1;
        
        while (start < end) {
            half = floor((start + end + 1) / 2);
            if (portstart >= half) {
                start = half;
            } else if (portend <= half-1) {
                end = half - 1;
            } else {
                convert_any_range_to_prefix(portstart, half - 1, num_prefix, portpref);
                convert_any_range_to_prefix(half, portend, num_prefix, portpref);

                return 0;
            }
        }
        convert_any_range_to_prefix(start, end, num_prefix, portpref);
    }
}     

void abvReadPort(FILE *fp, PortPrefix *portpref, unsigned char *num_prefix) {
    unsigned int portstart, portend;
    unsigned char i;

    int matches = fscanf(fp, "%d : %d", &portstart, &portend);

    /* 0 : 1 denotes wildcard */
    if (portstart == 0 && portend == 1) {
        SETPORT(portpref[0].pref, 0);
        portpref[0].len = 0;
        *num_prefix = 1;
    } else {
        *num_prefix = 0;
        convert_any_range_to_prefix(portstart, portend, num_prefix, portpref);
    }
}

int abvReadFilter(FILE *fp, FiltSet abvfiltset, unsint cost) {
    /* allocate a few more bytes just to be on the safe side to avoid overflow etc */
    char status, validfilter;
    unsigned int protocol;
    struct FILTER tempfilt1, *tempfilt;
    PortPrefix srcportpref[MAXEXPANSION], destportpref[MAXEXPANSION];
    unsigned char numSrcPortPref, numDestPortPref;
    unsigned char i, j;
    
    while (TRUE) {
        status = fscanf(fp, "%c", &validfilter);
        if (status == EOF) return ERROR;
        if (validfilter != '@') continue;	 
        
        tempfilt = &tempfilt1;

        //abvReadPrefix(fp, tempfilt->srcpref, &(tempfilt->srclen));
        abvReadPrefix(fp, tempfilt->pref[1], &(tempfilt->len[1]));
        abvReadPrefix(fp, tempfilt->pref[2], &(tempfilt->len[2]));

        abvReadPort(fp, destportpref, &numDestPortPref);
        abvReadPort(fp, srcportpref, &numSrcPortPref);
        //abvReadPort(fp, destportpref, &numDestPortPref);

        //ReadPortPrefix(fp, tempfilt->destportpref, &(tempfilt->destportlen));
        //ReadPortPrefix(fp, tempfilt->srcportpref, &(tempfilt->srcportlen));

        /* a value of 0 (wildcard) means protocol is unspecified */
        /* 1 is for TCP, 2 is for UDP */
        int matches = fscanf(fp, "%d", &protocol);
        //tempfilt->protocol[0] = (unsigned char)protocol;

        if (protocol == 0) {
            tempfilt->len[0] = 0;
            tempfilt->pref[0][0] =  0;
        }
        else {
            tempfilt->len[0] = 1;
            tempfilt->pref[0][0] = (uchar) protocol;
        }

        //abvReadPrefix(fp, tempfilt->protocolpref, &(tempfilt->protocollen));
        
        for (i = 0; i < numDestPortPref; i++)
          for (j = 0; j < numSrcPortPref; j++) {
              if (abvfiltset->numFilters == MAXFILTERS) {
                  if (DEBUG) printf("Out of memory: too many filters\n");

                  exit(3);
              }

              tempfilt->filtId = 1 + abvfiltset->numFilters;
              tempfilt->cost = cost;

              memcpy((char*)tempfilt->pref[3], (char*)destportpref[i].pref, sizeof(uchar)*DESTPORTLEN);
              tempfilt->len[3] = destportpref[i].len;

              memcpy((char*)tempfilt->pref[4], (char*)srcportpref[j].pref, sizeof(uchar)*SRCPORTLEN);
              tempfilt->len[4] = srcportpref[j].len;

              tempfilt->maxlen[0] = PROTOLEN;
              tempfilt->maxlen[1] = tempfilt->maxlen[2] = ADRLEN/8;
              tempfilt->maxlen[3] = tempfilt->maxlen[4] = PORTLEN/8;

              abvCopyFilter(&(abvfiltset->filtArr[abvfiltset->numFilters]), tempfilt);

              abvfiltset->numFilters++;
          }

        return SUCCESS;
    }

    return 0;
}

void abvLoadFilters(FILE *fp, FiltSet abvfiltset, int max) {
    int status, line = 0;
    struct FILTER tempfilt1, *tempfilt;

    abvfiltset->numFilters = 0;

    while ( (!(feof(fp))) && (abvfiltset->numFilters < max)) {
        line++;
        status = abvReadFilter(fp, abvfiltset, line);
        if (status == ERROR)
            break;
    }
}

void WritePrefix(FILE *fp, unsigned char* pref, unsigned char len) {
    /* assumes IPv4 prefixes */
    unsigned int tpref[4], templen;

    tpref[0] = (unsigned int)pref[0];
    tpref[1] = (unsigned int)pref[1];
    tpref[2] = (unsigned int)pref[2];
    tpref[3] = (unsigned int)pref[3];

    fprintf(fp, " %d.%d.%d.%d/%d ", tpref[0], tpref[1], tpref[2], tpref[3], len);
}

void WritePort(FILE *fp, uchar *portpref, uchar len) {

    fprintf(fp, " %d.%d/%d ", portpref[0], portpref[1], len);
}

int WriteFilter(FILE *fp, Filter tempfilt, char *comment) {
    /* allocate a few more bytes just to be on the safe side to avoid overflow etc */
    char status, validfilter;
    unsigned int protocol;
    struct FILTER filt;

    WritePrefix(fp, tempfilt->pref[1], tempfilt->len[1]);
    WritePrefix(fp, tempfilt->pref[2], tempfilt->len[2]);
    
    WritePort(fp, tempfilt->pref[3], tempfilt->len[3]);
    WritePort(fp, tempfilt->pref[4], tempfilt->len[4]);
    
    //WritePrefix(fp, tempfilt->protocolpref, tempfilt->protocollen);
    
    /* a value of 0 (wildcard) means protocol is unspecified */
    /* 1 is for TCP, 2 is for UDP */
    protocol = (unsigned int)tempfilt->pref[0][0];

    //fprintf(fp, "%d [%s = %d]\n", protocol, comment, tempfilt->filtId);
    fprintf(fp, "%d %d\n", protocol, tempfilt->filtId);

    return SUCCESS;
}

void DumpFilters(FILE *fp, FiltSet abvfiltset) {
    int i;

    for (i = 0; i < abvfiltset->numFilters; i++) {
        WriteFilter(fp, &(abvfiltset->filtArr[i]), "ID ");
    }
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

void printArray(FILE* fp, BitArray *bA) {
    unsint nrCuv, curCuv, cuvAggr;
    nrCuv = (unsint)(bA->noOfFilters / (8 * sizeof(unsint))) + 1;
    cuvAggr = (unsint)(bA->noOfFilters / WORDSIZE +1) / (8 * sizeof(unsint)) + 1;

    fprintf(fp, "No. of filters is:\t%u\t No. of Aggregate:\t%u\n ", bA->noOfFilters, cuvAggr);
    fprintf(fp, "MapVector:");

    for (curCuv = 0; curCuv < nrCuv; curCuv++) {
        if (curCuv%4 == 0)
            fprintf(fp, "\n");
        fprintf(fp, "%x\t", bA->map[curCuv]);
    }

    fprintf(fp, "\nAggregate:\t");
    for (curCuv = 0; curCuv < cuvAggr; curCuv++)
        fprintf(fp, "%x\t", bA->aggregate[curCuv]);

    fprintf(fp, "\n********************************\n");   
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
void setFilter(BitArray * bA, int filtNo){
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
void insertFilter(int FiltNo, uchar *pref, uchar len, Trie *trie){
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
        curNode->filters = createArray(abvfiltset.numFilters);
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

BitArray* searchPortTrie(unsint port, Trie *trie) {
    Node *curNode;
    uchar i, curByte, curBit, flag = 0;
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

    for (i = 0; i < 16; i++) {
        curBit = 15 - i;

        if (curNode->type == PREFIX) {
            temp = curNode->filters;
        }

        if (port & (1 << curBit)) {
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

unsint findMatch5(FILE *fp) {
    unsint nH = NUM_OF_FIELD, nrCuv, i, j, temp, result; 
    unsint curWord, nrBitAggr, nrCuvAggr, curPos, curCuv;

    if ((bA[0] == 0) || (bA[1] == 0) || (bA[2] == 0) || (bA[3] == 0) || (bA[4] == 0))
        return (-1);

    nrCuv = bA[0]->noOfFilters / WORDSIZE + 1;
    nrBitAggr = bA[0]->noOfFilters / WORDSIZE + 1;
    nrCuvAggr = nrBitAggr / WORDSIZE;

    curPos = 0;
    for (curCuv = 0; curCuv < nrCuvAggr; curCuv++) {
        result = 0xFFFFFFFF;

        for (i = 0; i < nH; i++)
          result &= bA[i]->aggregate[curCuv];

        if (result) { // got a match!!!
            for (i = 0; i < WORDSIZE; i++)
                if (result & (((unsint)1) << i)) {
                    curWord = (curPos << NUM_OF_FIELD + i);
                    result = 0xFFFFFFFF;

                    for (i = 0; i < nH; i++)
                        result &= bA[i]->map[curCuv];

                    for (j = 0; j < WORDSIZE; j++)
                        if (result & (((unsint)1) << j)) {
                            temp = curWord << NUM_OF_FIELD + j;
                            fprintf(fp, "%d \n", temp);

                            return temp;
                        }
                }
        }
        curPos += WORDSIZE;
    }
}

void search5Dim(struct PACKET *tempPkt) {

    if (tempPkt == (struct PACKET *)0)
        return;

    if (tempPkt->protocol > NUM_OF_PROTOCOL)
        return;

    if (protocol[tempPkt->protocol].bA == (BitArray*)0)
        bA[0] = protocol[0].bA;
    else
        bA[0] = protocol[tempPkt->protocol].bA;

    bA[1] = searchIPTrie(tempPkt->pref[0], trieArray[1]);
    bA[2] = searchIPTrie(tempPkt->pref[1], trieArray[2]);
    bA[3] = searchPortTrie(tempPkt->destPort, trieArray[3]);
    bA[4] = searchPortTrie(tempPkt->sourcePort, trieArray[4]);

    return;
}

void matchHeaders(FILE *fp, FILE *out) {
    char status, validPkt;
    struct PACKET tempPkt1, *tempPkt;

    while (TRUE) {
        status = fscanf(fp, "%c", &validPkt);
        if (status == EOF) return;
        if (validPkt != '@') continue;	 

        tempPkt = &tempPkt1;

        abvReadIPAddr(fp, tempPkt->pref[0]);
        abvReadIPAddr(fp, tempPkt->pref[1]);

        abvReadPortAddr(fp, &(tempPkt->destPort));
        abvReadPortAddr(fp, &(tempPkt->sourcePort));

        //int matches = fscanf(fp, "%d", &(tempPkt->protocol));
        int matches = fscanf(fp, "%hhd", &(tempPkt->protocol));

        search5Dim(tempPkt);

        findMatch5(out);
    }
}
