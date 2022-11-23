/*****************************************************************************
 *
 *	EGT-PC (Extended Grid of Tries with Path Compression)
 *
 *	Author:	Sumeet Singh
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
 *	Input Format For Filters:
 *
 * 	@134.32.31.22/30 232.123.222.198/26 0 : 1 1024 : 65535 17/8 1
 *
 * 	each filter should start with the @ symbol followed by
 *
 *	SourceIP/Length DestIP/Length SPortMin : SPortMax DPortMin : DPortMax Protocol/8 Action
 *	
 *	where,
 *	0 : 1 for Port signifies 0 : 65535 (i.e. all prots)
 *	0.0.0.0/0  for IP represents *.*.*.*
 *	also please remember to add the /8 to the Protocol Number
 *
 * **************************************************************************/

#include "egtpc.h"

void CopyFilter(PFilter f1,PFilter f2) {

	memcpy((char *)f1, (char *)f2, sizeof(struct FILTER));
}

void ReadPrefix(FILE *fp, unsigned char* pref, unsigned char *len) {
	/* assumes IPv4 prefixes */
	unsigned int tpref[4], templen;

	int matches = fscanf(fp, "%d.%d.%d.%d/%d", &tpref[0], &tpref[1], &tpref[2], &tpref[3], &templen);
	pref[0] = (unsigned char)tpref[0];
	pref[1] = (unsigned char)tpref[1];
	pref[2] = (unsigned char)tpref[2];
	pref[3] = (unsigned char)tpref[3];

	*len = (unsigned char) templen;
}

void ReadPort(FILE *fp, unsigned int *from, unsigned int *to) {
	unsigned int tfrom;
	unsigned int tto;

	int matches = fscanf(fp, "%d : %d", &tfrom, &tto);

	*from = tfrom;
	*to = tto;
}

void ReadProtocol(FILE *fp, unsigned char *protocol, unsigned char * protocolLen) {
	unsigned int tprotocol;
	unsigned int tprotocolLen;

    int matches = fscanf(fp, "%d/%d", &tprotocol, &tprotocolLen);
	*protocol = (unsigned char)tprotocol;
	*protocolLen = (unsigned char)tprotocolLen;
}

int ReadFilter(FILE *fp, FiltSet filtset, unsint cost) {
	/* allocate a few more bytes just to be on the safe side to avoid overflow etc */
	char status, validfilter;
	unsigned int protocol;
	struct FILTER tempfilt1, *tempfilt;
	unsigned char numSrcPortPref, numDestPortPref;
	unsigned char i, j;
	unsigned int tact;

	while (TRUE) {
		status = fscanf(fp, "%c", &validfilter);

		if (status == EOF) return ERROR;

		if (validfilter != '@') continue;	 

		tempfilt = &tempfilt1;

		ReadPrefix(fp, tempfilt->pref[0], &(tempfilt->len[0]));
		ReadPrefix(fp, tempfilt->pref[1], &(tempfilt->len[1]));

		tempfilt->ipHEX[0] =  (tempfilt->pref[0][0] << 24) ^ (tempfilt->pref[0][1] << 16) ^ (tempfilt->pref[0][2] << 8) ^ (tempfilt->pref[0][3]); 
		tempfilt->ipHEX[1] =  (tempfilt->pref[1][0] << 24) ^ (tempfilt->pref[1][1] << 16) ^ (tempfilt->pref[1][2] << 8) ^ (tempfilt->pref[1][3]); 

		ReadPort(fp, &(tempfilt->fromPort[0]), &(tempfilt->toPort[0]));

		if ((tempfilt->fromPort[0] == 0) && (tempfilt->toPort[0] == 1)) {
			tempfilt->toPort[0] = (unsigned int)(65535);
		}

		ReadPort(fp, &(tempfilt->fromPort[1]), &(tempfilt->toPort[1]));
		if ((tempfilt->fromPort[1] == 0) && (tempfilt->toPort[1] == 1)) {
			tempfilt->toPort[1] = (unsigned int)(65535);
		}

		ReadProtocol(fp, &(tempfilt->protPref), &(tempfilt->protLen));
		int matches = fscanf(fp, "%d", &tact); // ReadAction
		tempfilt->act = (unsigned char) tact;
		tempfilt->cost = cost;

		CopyFilter(&(filtset->filtArr[filtset->numFilters]), tempfilt);
		filtset->numFilters++;

		return SUCCESS;
	}

	return 0;
}

void LoadFilters(FILE *fp, FiltSet filtset, int max) {
	int status, line = 0;
	struct FILTER tempfilt1, *tempfilt;

	filtset->numFilters = 0;

	while ((!(feof(fp))) && (filtset->numFilters < max)) {
		line++;
		status = ReadFilter(fp, filtset, line);
    	if (status == ERROR)
      		break;
	}
}

PTrieSubNode NewTrieSubNode() {
	PTrieSubNode ptsubnode = (TrieSubNode*) calloc(1, sizeof(struct TRIESUBNODE));

	if (!ptsubnode) {
		if (DEBUG) printf("out of memory\n");

		exit(ERROR);
	}
	ptsubnode->next = NULL;

	return ptsubnode;
}

void CopyTrieSubNode(PTrieSubNode sn1, PTrieSubNode sn2) {

	memcpy((char *)sn1, (char *)sn2, sizeof(struct TRIESUBNODE));
}

void WriteTrieSubNode(PTrieSubNode ptsubnode) {
	
	/*
	ptsubnode->protPref = pfilter->protPref;
	ptsubnode->protLen = pfilter->protLen;
	ptsubnode->fromPort[0] = pfilter->fromPort[0];
	ptsubnode->fromPort[1] = pfilter->fromPort[1];
	ptsubnode->toPort[0] = pfilter->toPort[0];
	ptsubnode->toPort[1] = pfilter->toPort[1];
	ptsubnode->act = pfilter->act;
	ptsubnode->pfilter = pfilter;
	*/

	if (DEBUG_SEARCH) printf("<<%d>>", ptsubnode->cost);
}

void AddFilterToNode(PTrieNode ptnode, PFilter pfilter) {
	PTrieSubNode ptsubnode = NewTrieSubNode();

	ptsubnode->protPref = pfilter->protPref;
	ptsubnode->protLen = pfilter->protLen;
	ptsubnode->fromPort[0] = pfilter->fromPort[0];
	ptsubnode->fromPort[1] = pfilter->fromPort[1];
	ptsubnode->toPort[0] = pfilter->toPort[0];
	ptsubnode->toPort[1] = pfilter->toPort[1];
	ptsubnode->act = pfilter->act;
	ptsubnode->pfilter = pfilter;
	ptsubnode->cost = pfilter->cost;
	ptsubnode->next = NULL;

	if (ptnode->pdimList != NULL) { // add the new rule node at the head of the rules array..
	 	if (DEBUG) printf(":");

		ptsubnode->next = ptnode->pdimList;
		ptnode->pdimList = ptsubnode;
	}
	else {
		ptnode->pdimList = ptsubnode;
	}
}

PTrieNode NewTrieNode() {
	PTrieNode ptnode = (TrieNode*) calloc(1, sizeof(struct TRIENODE));

	if (!ptnode) {
		if (DEBUG) printf("out of meory\n");

		exit(ERROR);
	}

	ptnode->zero = NULL;
	ptnode->one = NULL;
	ptnode->dest = NULL;
	ptnode->tempFiltCount = 0;
	ptnode->pdimList=NULL;
	ptnode->level=0;
	ptnode->longestPath=0;

	return ptnode;
}

PTrieNodeC NewTrieNodeC() {
	PTrieNodeC ptnode = (TrieNodeC*) calloc(1,sizeof(struct TRIENODEC));

	if (!ptnode) {
		if (DEBUG) printf("out of meory\n");

		exit(ERROR);
	}

	ptnode->zero = NULL;
	ptnode->one = NULL;
	ptnode->dest = NULL;
	ptnode->tempFiltCount = 0;
	ptnode->pdimList=NULL;
	ptnode->level=0;

	return ptnode;
}

void AddRuleToGrid(TrieNode* ptnode, PFilter pfilter, int rDim) { // ptnode is rootnode of the trie; pfilter is the pointer to the filter to add..
	int i, j;
	TrieNode* currentNode = ptnode;
	int prefixesTillNow = 0;

    //fprintf(stdout, "%i.%i.%i.%i/%i (0x%x) %i.%i.%i.%i/%i (0x%x) %d : %d %d : %d %i/%i\n", pfilter->pref[0][0], pfilter->pref[0][1], pfilter->pref[0][2], pfilter->pref[0][3], pfilter->len[0], pfilter->ipHEX[0], pfilter->pref[1][0], pfilter->pref[1][1], pfilter->pref[1][2], pfilter->pref[1][3], pfilter->len[1], pfilter->ipHEX[1], pfilter->fromPort[0], pfilter->toPort[0], pfilter->fromPort[1], pfilter->toPort[1], pfilter->protPref, pfilter->protLen);

	unsint destPref = pfilter->ipHEX[1];
	unsint destLen =  pfilter->len[1];
	unsint sourcePref = pfilter->ipHEX[0];
	unsint sourceLen = pfilter->len[0];
	int filtID = pfilter->cost;
	unsint prefix = 0x0;

	if (rDim) {
		destPref = pfilter->ipHEX[0];
		destLen = pfilter->len[0];
		sourcePref =  pfilter->ipHEX[1];
		sourceLen =  pfilter->len[1];
	}

	if (DEBUG) printf(" (0x%x/%d) ", destPref, destLen);

	currentNode->prefix = 0x0;

	for (i = 0; i < destLen; i++) {
		if (currentNode->dest) prefixesTillNow++;
		if (MSBBitX(destPref, i)) {  // eg. when i == 5, this macro return 1 when [destPref & 0000,0100,0000,0000] == true, 0 when false.. 
			// 1 right sub tree
			if (currentNode->one != NULL) { // if there have existed a node, go on to this subnode..
				currentNode = currentNode->one;
			}
			else {
				currentNode->one = NewTrieNode();
				currentNode->one->parent = currentNode;
				currentNode->one->level = currentNode->level + 1;
				currentNode = currentNode->one;
			}
			prefix = prefix << 1 | 0x1;
			currentNode->prefix = prefix;
		}
		else {
			// 0 left sub tree
			if (currentNode->zero != NULL) {
				currentNode = currentNode->zero;
			}
			else {
				currentNode->zero = NewTrieNode();
				currentNode->zero->parent = currentNode;
				currentNode->zero->level = currentNode->level + 1;
				currentNode = currentNode->zero;
			}
			prefix = prefix << 1 | 0x0;
			currentNode->prefix = prefix;
		}
	}
	currentNode->validPrefixes++;
	currentNode->prefixesTillNow = prefixesTillNow+1;

	prefixesTillNow = 0;
	if (currentNode->dest) { // go to the second dimention trie..
		currentNode = currentNode->dest;
	}
	else {
		currentNode->dest = NewTrieNode();
		currentNode->dest->parent = currentNode;
		currentNode->dest->level = 0;
		currentNode = currentNode->dest;
	}

	if (DEBUG) printf(" (0x%x/%d) ", sourcePref, sourceLen);

	for (j = 0; j < sourceLen; j++) {
		if (currentNode->pdimList) prefixesTillNow++;

		if (MSBBitX(sourcePref, j)) {
			// 1 right sub tree
			if (currentNode->one != NULL) {
				currentNode = currentNode->one;
			}
			else {
				currentNode->one = NewTrieNode();
				currentNode->one->parent = currentNode;
				currentNode->one->level = currentNode->level + 1;
				currentNode = currentNode->one;
			}
		}
		else {
			// 0 left sub tree
			if (currentNode->zero != NULL) {
				currentNode = currentNode->zero;
			}
			else {
				currentNode->zero = NewTrieNode();
				currentNode->zero->parent = currentNode;
				currentNode->zero->level = currentNode->level + 1;
				currentNode = currentNode->zero;
			}
		}
	}
	
	// we need to put the filter off the location we are at..
	AddFilterToNode(currentNode, pfilter);

	// update stat counters
	currentNode->prefixesTillNow = prefixesTillNow+1;
	currentNode->tempFiltCount++;

	currentNode->validPrefixes++;

	if (DEBUG) printf(". = %d\n", currentNode->tempFiltCount);
}

void AddPointerToLastNode(PTrieNode ptnode, PTrieSubNode ptsubnode) {
	PTrieSubNode currentSubNode;

	if (!(ptnode->pdimList)) {
		ptnode->pdimList = ptsubnode;
	}
	else {
		currentSubNode = ptnode->pdimList;

		while (currentSubNode->next) {
			currentSubNode = currentSubNode->next;
		}

		currentSubNode->next = ptsubnode;
	}
}

int AddLinkToSubNode(PTrieNode start, PTrieSubNode ptsubnode) {
	PTrieNode currentNode = start;

	if (!start) return 0;

	if (currentNode->pdimList) {
		AddPointerToLastNode(currentNode, ptsubnode);

		return (AddLinkToSubNode(currentNode->zero, currentNode->pdimList) + AddLinkToSubNode(currentNode->one, currentNode->pdimList));
	}
	else {
		return (AddLinkToSubNode(currentNode->zero, ptsubnode) + AddLinkToSubNode(currentNode->one, ptsubnode));
	}

	return 0;
}

int AddSubNodeToTrie(PTrieNode start, PTrieSubNode ptsubnode) {
	PTrieNode currentNode = start;
	PTrieSubNode newSubNode;

	if (!start) return 0;

	if (currentNode->pdimList) {
		while (ptsubnode) {
			newSubNode = NewTrieSubNode();
			CopyTrieSubNode(newSubNode, ptsubnode);
			newSubNode->next = NULL;
			AddPointerToLastNode(currentNode, newSubNode);
			ptsubnode = ptsubnode->next;
		}

		return (AddSubNodeToTrie(currentNode->zero, currentNode->pdimList) + AddSubNodeToTrie(currentNode->one, currentNode->pdimList));
	}
	else {
	 	return (AddSubNodeToTrie(currentNode->zero, ptsubnode) + AddSubNodeToTrie(currentNode->one, ptsubnode));
	}

	return 0;
}

void AddSubNodeToTrieBase(PTrieNode start) {
	PTrieNode currentNode = start;
	int num = 0;

	if (!(start)) return;

	if (currentNode->pdimList) {
	 	num = (AddLinkToSubNode(currentNode->zero, currentNode->pdimList) + AddLinkToSubNode(currentNode->one, currentNode->pdimList));
	}
	else {
		AddSubNodeToTrieBase(currentNode->zero);
		AddSubNodeToTrieBase(currentNode->one);
		AddSubNodeToTrieBase(currentNode->dest);
	}

	return;
}

/* COUNTING FUNCTIONS */

int Count2DNodes(PTrieNode cN) {

	if (!cN) return 0;

	return(1 + Count2DNodes(cN->zero) + Count2DNodes(cN->one));
}

int Count1DNodes(PTrieNode cN, int flag2d) {
	int i = 0;
	int j = 0;
	int k = 0;

	if (!cN) return 0;

	i = Count1DNodes(cN->zero, flag2d);
	j = Count1DNodes(cN->one, flag2d);

	if (cN->dest && flag2d) {
		k = Count2DNodes(cN->dest);

		return (1 + i + j + k);
	}

	return(1 + i + j);
}

PTrieNode FindNextLongestNodeN(PTrieNode start, unsint prefix, int prefixLen) {
	PTrieNode cN = start;
	PTrieNode lastFound = NULL;
	PTrieNode lastValid = NULL;
	PTrieNode currentFound = NULL;
	int i = 0;

	if (!start) return NULL;

	if (!start->dest) return FindNextLongestNodeN(start->parent, prefix, prefixLen);

	cN = start->dest;
	if (cN->pdimList) lastFound = cN;

	for (i = 0;((cN) && (i<prefixLen)); i++) {
		if (cN->pdimList)
			lastFound = cN;

		lastValid = cN;

		if (MSBBitX(prefix, i)) {
			if (DEBUG) printf("1");

			cN = cN->one;
		}
		else {
			if (DEBUG) printf("0");

			cN = cN->zero;
		}
	}

	if (DEBUG) printf("\n");

	if (cN && cN->pdimList) return cN;
	else {
		if (lastFound)
			return lastFound;
		else {
			if (lastValid)
				return lastValid;
			else
				return FindNextLongestNodeN(start->parent, prefix, prefixLen);
		}
	}
}

PTrieSubNode GetSubNode(PTrieNode start, unsint prefix, unsint prefixLen) {
	PTrieNode currentNode = NULL;
	PTrieNode lastNode = NULL;
	PTrieSubNode ptsubnode = NULL;
	int i;

	// TODO: special case.. start = root node

	if (!start) return NULL;

	if (!(start->dest)) return (GetSubNode(start->parent, prefix, prefixLen));

	currentNode = start->dest;

	i = 0;
	while (currentNode && (i<prefixLen)) {
		lastNode = currentNode;
		if (MSBBitX(prefix, i)) {
			currentNode = currentNode->one;
		}
		else {
			currentNode = currentNode->zero;
		}

		i++;
	}

	if (currentNode)
		ptsubnode = currentNode->pdimList;
	else
		ptsubnode = lastNode->pdimList;

	return ptsubnode;
}

void AddLinksToSubNodes(PTrieNode start, PTrieNode lastGoodSource, unsint prefix, int prefixLen) {
	PTrieNode currentNode;
	int num = 0;
	PTrieSubNode tsubnode;

	if (DEBUG) printf("L");

	if (!(start)) return;

	currentNode = start;

	if (lastGoodSource) {
		if (DEBUG) printf("GOT GOOD LAST SOURCE\n");

		if (!lastGoodSource->parent && DEBUG) printf("BUT NO PARENT\n");

		tsubnode = GetSubNode(lastGoodSource->parent, prefix, prefixLen);
		AddPointerToLastNode(currentNode, tsubnode);
	}
	//else {
		if (currentNode->dest != NULL) {
			lastGoodSource = currentNode;

			if (DEBUG) printf("SET LAST GOOD SOURCE\n");

			AddLinksToSubNodes(currentNode->dest, lastGoodSource, 0x0, 0);
		}
		if (currentNode->zero) AddLinksToSubNodes(currentNode->zero, lastGoodSource, ((prefix >> 1)), prefixLen + 1);
		if (currentNode->one) AddLinksToSubNodes(currentNode->one, lastGoodSource, ((prefix >> 1) | 0x80000000 ), prefixLen + 1);
	//}

	if (DEBUG) printf("\n");

	return;
}

void AddLinksToLongestPathsDim2(PTrieNode start, PTrieNode LGS, int prefixLen, unsint prefix) {
	PTrieNode cN;
	unsint tp;

	cN = start;

	if (!(start && LGS))
		return;

	if (cN->zero) {
			tp = prefix >> 1;

			if (DEBUG) printf("tp:0x%x", tp);

			AddLinksToLongestPathsDim2(cN->zero, LGS, tp, prefixLen + 1);
	}
	else {
			tp = prefix >> 1;

			if (DEBUG) printf("tp:0x%x", tp);

			cN->zero = FindNextLongestNodeN(LGS, tp, prefixLen + 1);

			if (DEBUG) {
				if (!cN->zero)
					printf("could not find next\n");
				else
					printf("got find next\n");
			}

			if (cN->zero == cN) cN->zero = NULL;
	}

	if (cN->one) {
			tp = (prefix >> 1) | 0x80000000;

			if (DEBUG) printf("tp:0x%x", tp);

			AddLinksToLongestPathsDim2(cN->one, LGS, tp, prefixLen + 1);
	}
	else {
			tp = (prefix>>1) | 0x80000000;

			if (DEBUG) printf("tp:0x%x", tp);

			cN->one = FindNextLongestNodeN(LGS, tp, prefixLen + 1);

			if (DEBUG) {
				if (!cN->one)
					printf("cound not findNext\n");
				else
					printf("got findNext\n");
			}

			if (cN->one == cN) cN->one = NULL;
	}
}

void AddLinksToLongestPathsDim1(PTrieNode start) {

	if (!start) return;

	if (start->zero)
		AddLinksToLongestPathsDim1(start->zero);

	if (start->one)
		AddLinksToLongestPathsDim1(start->one);

	if (start->dest)
		AddLinksToLongestPathsDim2(start->dest, start, 0x0,0);

	return;
}

void AddLinksToLongestPaths(PTrieNode start, PTrieNode lastGoodSource, unsint prefix, int prefixLen, int dim) {
	PTrieNode currentNode;
	PTrieNode tnode;
	int num = 0;

	if (!(start)) return;

	currentNode = start;

	if (DEBUG) printf("(H:%d:%d:%d)", prefixLen, dim, (currentNode->zero && currentNode->one));

	if (currentNode->zero) {
		if (DEBUG) printf("0");

		AddLinksToLongestPaths(currentNode->zero, lastGoodSource, ((prefix>>1)), prefixLen + 1, dim);
	}
	else {
		if (dim == 1) {
			tnode = FindNextLongestNodeN(lastGoodSource->parent, prefix, prefixLen);
			currentNode->zero = tnode;
			if (currentNode->zero == currentNode) currentNode->zero = NULL;

			if (DEBUG) {
				printf("\n[0x%x/%d]", prefix, prefixLen);
				printf("-> ");
				printf("Z > ");
		 		printf("(LEVEL %d:", currentNode->level);
				if (tnode) printf("%d", tnode->level);
				printf(")\n");
				if (tnode && tnode->pdimList) printf("<<VP>>");
			}
		}
	}

	if (currentNode->one) {
		if (DEBUG) printf("1");

		AddLinksToLongestPaths(currentNode->one, lastGoodSource, ((prefix>>1) | 0x80000000), prefixLen + 1, dim);
	}
	else {
	 	if (dim == 1) {
			tnode = FindNextLongestNodeN(lastGoodSource->parent, prefix, prefixLen);
			currentNode->one = tnode;
			if (currentNode->one == currentNode) currentNode->one = NULL;

			if (DEBUG) {	
				printf("\n[0x%x/%d]", prefix, prefixLen);
				printf("-> ");
				printf("0 > ");
		 		printf("(LEVEL %d:", currentNode->level);
				if (tnode) printf("%d", tnode->level);
				printf(")\n");
				if (tnode && tnode->pdimList) printf("<<VP>>");
			}
		}
	}

	if (currentNode->dest != NULL) {
		// lastGoodSource = currentNode;

		if (DEBUG) printf("changed LGS: 0x%x P: 0x%x\nD", currentNode, currentNode->parent);

		// HEREY
		AddLinksToLongestPaths(currentNode->dest, currentNode, 0x0, 0, 1);
	}

	return;
}

TrieNode* CreateGridOfTrie(FiltSet filtset, int rDim) {
	int i;
	int j = 0;

	TrieNode *newNode = (TrieNode*) malloc(sizeof(struct TRIENODE)); // create root node..
	newNode->zero = NULL; // pointer to left(zero) subnode..
	newNode->one = NULL; // pointer to right(one) subnode..
	newNode->dest = NULL; // ??

  	for (i = 0; i < filtset->numFilters; i++) {
		if (DEBUG) printf("%d--", i);

		if (!((filtset->filtArr[i].len[0] == 0) && (filtset->filtArr[i].len[1] == 0))) // if the i'th rule set is not empty.. then..
			AddRuleToGrid(newNode, &(filtset->filtArr[i]), rDim); // add the i'th rule to the current trie..
		else { // record the invalid rules..
			if (DEBUG) printf("SKIP: %d %d\n", filtset->filtArr[i].len[0], filtset->filtArr[i].len[1]);

			j++;
		}
  	}

	if (DEBUG) printf("*** RULES FOR STAR STAR: %d\n", j);

	return newNode;
}

/* Search Functions for EGT */

int SearchSourceTrie(PTrieNode ptnode, unsint dest, unsint source, int memAcc) {
	int i;
	PTrieSubNode tpsubnode;

	if (!ptnode) return ERROR;

	for (i = ptnode->level; (ptnode); i = ptnode->level) {
		if(ptnode && ptnode->pdimList) {
			tpsubnode = ptnode->pdimList;
			while(tpsubnode)  {
				if (DEBUG_SEARCH) printf("F");
				if (DEBUG_SEARCH) WriteTrieSubNode(tpsubnode);

				tpsubnode=tpsubnode->next;
			}
		}

		if (MSBBitX(source, i)) {
				if (DEBUG_SEARCH) printf("1");

				if (ptnode->one) ptnode = ptnode->one;
				else {
					if (DEBUG) printf("SORRY");

					break;
				}
		}
		else {
				if (DEBUG_SEARCH) printf("0");

				if (ptnode->zero) ptnode = ptnode->zero;
				else {
					if (DEBUG) printf("SORRY");

					break;
				}
		}

		if (!ptnode) {
				if (DEBUG_SEARCH) printf(" >>%d<<END\n", i);

				break;
		}
	}

	return memAcc;
}

int SearchTrie(PTrieNode ptnode, unsint dest, unsint source, int memAcc) {
	int i;
	int notdone = 1;
	PTrieSubNode tpsubnode;
	PTrieNode lastGoodNode = ptnode->dest;

	if (DEBUG_SEARCH) printf("\nD: 0x%x  S: 0x%x L: 0x%x\n", dest, source, lastGoodNode);

	for (i = 0;(notdone && (i < 32)); i++) {
		memAcc++;
		if (!(ptnode))  {
			if (DEBUG) printf("SORRY!");

			memAcc += SearchSourceTrie(lastGoodNode, dest, source, memAcc);
			notdone = 0;

			break;
		}

		if (ptnode->dest) lastGoodNode = ptnode->dest;

		if (MSBBitX(dest, i)) {
			if (DEBUG_SEARCH) printf("1");

			if (ptnode->one) ptnode = ptnode->one;
			else {
				if (DEBUG) printf("SORRY!");

				memAcc += SearchSourceTrie(lastGoodNode, dest, source, memAcc);
				notdone = 0;

				break;
			}
		}
		else {
			if (DEBUG_SEARCH) printf("0");

			if (ptnode->zero) ptnode = ptnode->zero;
			else {
				if (DEBUG) printf("SORRY!");

				memAcc += SearchSourceTrie(lastGoodNode, dest, source, memAcc);
				notdone = 0;

				break;
			}
		}
	}

	if (ptnode->dest)  lastGoodNode = ptnode->dest;

	if (DEBUG_SEARCH) printf("D");

	if (notdone)
		memAcc += SearchSourceTrie(lastGoodNode, dest, source, memAcc);
	else
		if (DEBUG_SEARCH) printf("already done\n");

	return memAcc;
}

void ValidateTrie(PTrieNode ptnode) {
	PTrieSubNode tpsubnode;

	if (!ptnode) return;

	if (ptnode->pdimList) {
		tpsubnode = ptnode->pdimList;
		while(tpsubnode) {
			validate[tpsubnode->cost]++;
			tpsubnode = tpsubnode->next;
		}
	}

	ValidateTrie(ptnode->zero);
	ValidateTrie(ptnode->one);
	ValidateTrie(ptnode->dest);
}

void WriteTrie(PTrieNode ptnode) {
	PFilter tpfilter;
	PTrieSubNode tpsubnode;

	if (DEBUG_SEARCH) printf("(%d)",ptnode->level);

	if (ptnode->pdimList) {
		tpsubnode = ptnode->pdimList;
		while(tpsubnode) {
			if (DEBUG_SEARCH) printf("F");
			if (DEBUG_SEARCH) WriteTrieSubNode(tpsubnode);

			validate[tpsubnode->cost]++;
			tpsubnode = tpsubnode->next;
		}
	}

	if (ptnode->zero) {
		WriteTrie(ptnode->zero);

		if (DEBUG_SEARCH) printf("0");
	}

	if (ptnode->one) {
		WriteTrie(ptnode->one);

		if (DEBUG_SEARCH) printf("1");
	}

	if (ptnode->dest) {
		WriteTrie(ptnode->dest);

		if (DEBUG_SEARCH) printf("D");
	}

	if (!(ptnode->zero && ptnode->dest && ptnode->one))
		if (DEBUG_SEARCH) printf("E");
}

/**********
 *
 * PATH COMPRESSION FUNCTIONS
 *
 **********/

void CompressDestTrie(PTrieNodeC parent, PTrieNode ptnode, unsint cmask, uchar cmaskLen, int level, uchar branch) {

	if ((ptnode->zero && ptnode->one) || (ptnode->dest) || (cmaskLen==MAX_STRIDE) || (ptnode->pdimList)) {
		// create new trie node
		PTrieNodeC newNode;
		newNode = NewTrieNodeC();

		newNode->level = level;
		newNode->pdimList = ptnode->pdimList;
		newNode->validPrefixes = ptnode->validPrefixes;
		newNode->parent = parent;

		if (branch == 1) {
			// set parent one mask / pointer
			parent->one = newNode;
			parent->omask = cmask;
			parent->omaskLen = cmaskLen;
		}
		else {
			parent->zero = newNode;
			parent->zmask = cmask;
			parent->zmaskLen = cmaskLen;
		}

		if (ptnode->dest) {
			// create a new node for ptnode->dest
			PTrieNodeC newDestNode;

			newDestNode = NewTrieNodeC();
			newNode->dest = newDestNode;
			newDestNode->parent = newNode;

			newDestNode->pdimList = ptnode->dest->pdimList;

			newDestNode->level = 1;
			newDestNode->validPrefixes = ptnode->dest->validPrefixes;

			// call CompressSourceTrie
			// with the newDestNode as parent
			if (ptnode->dest->one) {
				CompressDestTrie(newDestNode, ptnode->dest->one, 0x80000000, 1, 1, 1);
			}
			if (ptnode->dest->zero) {
				CompressDestTrie(newDestNode, ptnode->dest->zero, 0x00000000, 1, 1, 0);
			}
		}

		// call CompressDestTrie on zero / one with new parent / mask
		if (ptnode->zero) {
			CompressDestTrie(newNode, ptnode->zero, 0x0, 1, level + 1, 0);
		}
		if(ptnode->one) {
			CompressDestTrie(newNode, ptnode->one, 0x80000000, 1, level + 1, 1);
		}
	}
	else {
		if (ptnode->zero) {
			unsint nmask = 0x0;
			nmask = cmask >> 1;
			nmask = nmask ^ 0x0;

			CompressDestTrie(parent, ptnode->zero, nmask, cmaskLen + 1, level + 1, branch);
		}
		if (ptnode->one)
		{
			unsint nmask = 0x0;
			nmask = cmask >> 1;
			nmask = nmask ^ 0x80000000;

			CompressDestTrie(parent, ptnode->one, nmask, cmaskLen + 1, level + 1, branch);
		}
	}
}

void CreateCompressedTrie(PTrieNodeC nodeC, PTrieNode trieroot)  {

	nodeC->level = 0;

	if (trieroot->zero) {
		CompressDestTrie(nodeC, trieroot->zero, 0x0, 1, 1, 0);
    }
	if (trieroot->one) {
		CompressDestTrie(nodeC, trieroot->one, 0x80000000, 1, 1, 1);
    }
	if (trieroot->dest) {
		// create a new node for ptnode->dest
		PTrieNodeC newDestNode;

		newDestNode = NewTrieNodeC();
		nodeC->dest = newDestNode;

		newDestNode->pdimList = trieroot->dest->pdimList;
		newDestNode->pdimListI[0] = trieroot->dest->pdimListI[0];
		newDestNode->pdimListI[1] = trieroot->dest->pdimListI[1];
		newDestNode->pdimListI[2] = trieroot->dest->pdimListI[2];
		newDestNode->level = 1;
		newDestNode->validPrefixes = trieroot->dest->validPrefixes;

		// call CompressSourceTrie
		// with the newDestNode as parent
		if (trieroot->dest->one) {
			CompressDestTrie(newDestNode, trieroot->dest->one, 0x80000000,1, 1, 1 );
		}
		if (trieroot->dest->zero) {
			CompressDestTrie(newDestNode, trieroot->dest->zero, 0x00000000,1, 1, 0 );
		}
	}
}

PTrieNodeC FindNextLongestNodeC(PTrieNodeC start, unsint prefix, int prefixLen) {
	PTrieNodeC cN = start;
	PTrieNodeC lastFound = NULL;
	PTrieNodeC currentFound = NULL;
	PTrieNodeC lastValid = NULL;

	int i = 0;
	int j = 0;
	int k = 0;
	int found = 0;
	int done = 1;

	if (!start) return NULL;

	if (!start->dest)
		return FindNextLongestNodeC(start->parent, prefix, prefixLen);

	cN = start->dest;

	if (cN->pdimList) lastFound = cN;
	lastValid = cN;

	j = 0;
	done = 1;

	for (j = 0; ((j < prefixLen) && done); j++) {
		if (cN) {
			lastValid = cN;
			if (cN->pdimList) lastFound = cN;
		}
		else {
			done = 0;

			continue;
		}

		if (MSBBitX(prefix, j)) {
			// make sure there is 1 pointer, else return lastFound
			if (cN->one) {
				if (cN->omaskLen > prefixLen) {
					done = 0;
					// return current node
				}
				else {
					found = 1;
					for (k = 0; ((k < cN->omaskLen) && found); k++) {
						if (MSBBitX(prefix, (j + k)) == MSBBitX(cN->omask, k))
							continue;

						// compare the bits k (for CN) and (j+k) for mask
						// if any bits dont match found = 0;
						found = 0;
					}
					if (found) {
						//lastFound = cN->one;
						cN = cN->one;
					}
					else {
						done = 0;
					}
					j = j + k - 1;
				}
			}
			else {
				//return lastFound || lastValid
				done = 0;
			}
		}
		else {
			// make sure there is 0 pointer, else return lastFound
			if (cN->zero) {
				if (cN->zmaskLen > prefixLen) {
					done = 0;
					// return current node
				}
				else {
					found = 1;
					for (k = 0; ((k < cN->zmaskLen) && found); k++) {
						if (MSBBitX(prefix, (j + k)) == MSBBitX(cN->zmask, k))
							continue;

						// compare the bits k (for CN) and (j+k) for mask
						// if any bits dont match found = 0;
						found = 0;
					}

					if (found) {
						//lastFound = cN->zero;
						cN = cN->zero;
					}
					else {
						done = 0;
					}
					j = j + k - 1;
				}
			}
			else {
				//return lastFound || lastValid
				done = 0;
			}
		}
	}

	if (cN && cN->pdimList)
		lastFound = cN;

	if (lastFound)
		return (lastFound);
	else {	
		if (lastValid)
			return (lastValid);
		else
			return FindNextLongestNodeC(start->parent, prefix, prefixLen);
	}
}

void AddLinksToLongestPathsC(PTrieNodeC start, PTrieNodeC lastGoodSource, unsint prefix, int prefixLen, int dim) {
	PTrieNodeC currentNode;
	PTrieNodeC tnode;
	int num = 0;

	if (!(start)) return;

	currentNode = start;

	if (currentNode->zero) {
		unsint nzp = (prefix >> currentNode->zmaskLen) ^ currentNode->zmask;
		int nzpl = prefixLen+currentNode->zmaskLen;
		AddLinksToLongestPathsC(currentNode->zero, lastGoodSource, nzp, nzpl, dim);
	}

	if(currentNode->one) {
		unsint nop = (prefix >> currentNode->omaskLen) ^ currentNode->omask;
		int nopl = prefixLen+currentNode->omaskLen;

		AddLinksToLongestPathsC(currentNode->one, lastGoodSource, nop, nopl, dim);
	}

	if (dim == 1) {
		tnode = FindNextLongestNodeC(lastGoodSource->parent, prefix, prefixLen);
		currentNode->fail = tnode;

		if (currentNode->fail == currentNode)
			currentNode->fail = NULL;
	}

	if ((currentNode->dest != NULL) && (dim == 0)) {
		AddLinksToLongestPathsC(currentNode->dest, currentNode, 0x0, 0, 1);
	}

	return;
}

void ValidateTrieC(PTrieNodeC ptnode) {
	PTrieSubNode	tpsubnode;

	if (!ptnode) return;

	if (ptnode->pdimList) {
		tpsubnode = ptnode->pdimList;
		while(tpsubnode) {
			validate[tpsubnode->cost]++;
			tpsubnode = tpsubnode->next;
		}
	}

	ValidateTrieC(ptnode->zero);
	ValidateTrieC(ptnode->one);
	ValidateTrieC(ptnode->dest);
}

int Count2DNodesC(PTrieNodeC cN) {

	if (!cN) return 0;

	return(1 + Count2DNodesC(cN->zero) + Count2DNodesC(cN->one));
}

int Count1DNodesC(PTrieNodeC cN, int flag2d) {
	int i = 0;
	int j = 0;
	int k = 0;

	if(!cN) return 0;

	i = Count1DNodesC(cN->zero, flag2d);
	j = Count1DNodesC(cN->one, flag2d);

	if (cN->dest && flag2d) {
		k = Count2DNodesC(cN->dest);

		return (1 + i + j + k);
	}

	return(1 + i + j);
}

int CountPDListsC(PTrieNodeC cN, int cC) {

	if(!(cN)) return cC;
	
	if (cN->pdimList)
		return (1 + CountPDListsC(cN->zero, cC) + CountPDListsC(cN->one, cC) + CountPDListsC(cN->dest, cC));
	else
		return (0 + CountPDListsC(cN->zero, cC) + CountPDListsC(cN->one, cC) + CountPDListsC(cN->dest, cC));
}

int CountPDListsSizeC(PTrieNodeC cN, int cC) {

	if(!(cN)) return cC;

	if(cN->pdimList)
		return (cN->validPrefixes + CountPDListsSizeC(cN->zero, cC) + CountPDListsSizeC(cN->one, cC) + CountPDListsSizeC(cN->dest, cC));
	else
		return (0 + CountPDListsSizeC(cN->zero, cC) + CountPDListsSizeC(cN->one, cC) + CountPDListsSizeC(cN->dest, cC));
}

/* Search Functions for EGT-WPC */

int SearchSourceTrieC(PTrieNodeC ptnode, unsint dest, unsint source, int memAcc) {
	int i, j, k;
	int done = 1;
	int found = 1;
	int prefixLen = 32;
	unsint prefix = source;

	PTrieSubNode tpsubnode;
	PTrieNodeC cN = ptnode;

	if (!ptnode) return ERROR;

	for (j = 0; (cN); j++) {
		if (cN->pdimList) {
			if (DEBUG_SEARCH) printf("LIST");

			// traverse through the list
			tpsubnode = cN->pdimList;
			while (tpsubnode) {
				if (DEBUG_SEARCH) printf("F");
				if (DEBUG_SEARCH) WriteTrieSubNode(tpsubnode);

				SEARCH_RESULTS[nSearchResults++] = tpsubnode;
				tpsubnode = tpsubnode->next;
				memAcc++;
			}
		}

		if (MSBBitX(prefix, j)) {
			// make sure there is 1 pointer, else return lastFound
			if (cN->one) {
				if (cN->omaskLen > prefixLen) {
					// FOLLOW THE FAIL POINTER
					cN = cN->fail;
					//done = 0;

					continue;
				}
				else {
					found = 1;
					for(k = 0; ((k < cN->omaskLen) && found); k++) {
						if (MSBBitX(prefix, (j + k)) == MSBBitX(cN->omask, k))
							continue;

						// compare the bits k (for CN) and (j+k) for mask
						// if any bits dont match found = 0;
						found = 0;
					}

					if (found) {
						//lastFound = cN->one;
						cN = cN->one;
					}
					else {
						cN = cN->fail;
						done = 0;
					}
					if (cN) j = cN->level - 1;
				}
			}
			else {
				// FOLLOW FAIL POINTER
				if (cN) cN = cN->fail;
				//done = 0;
			}
		}
		else {
			if (cN->zero) {
				if (cN->zmaskLen > prefixLen) {
					// FOLLOW THE FAIL POINTER
					cN = cN->fail;
					//done = 0;

					continue;
				}
				else {
					found = 1;
					for (k = 0; ((k < cN->zmaskLen) && found); k++) {
						if (MSBBitX(prefix, (j + k)) == MSBBitX(cN->zmask, k))
							continue;

						// compare the bits k (for CN) and (j+k) for mask
						// if any bits dont match found = 0;
						found = 0;
					}

					if (found) {
						//lastFound = cN->zero;
						cN = cN->zero;
					}
					else {
						cN = cN->fail;
						done = 0;
					}
					if (cN) j = cN->level - 1;
				}
			}
			else {
				// FOLLOW FAIL POINTER
				cN = cN->fail;
				//done = 0;
			}
		}
	}

	return SUCCESS;
}

int SearchTrieC(PTrieNodeC ptnode, unsint dest, unsint source, int memAcc) {
	int i, j, k;
	int done = 1;
	int found = 1;
	int prefixLen = 32;
	unsint prefix = dest;

	PTrieSubNode tpsubnode;
	PTrieNodeC lastFound = ptnode->dest;
	PTrieNodeC cN = ptnode;
	
	if (DEBUG_SEARCH) printf("SF: %x %x\n", dest, source);
	//if (DEBUG_SEARCH) printf("%d 0x%x  0x%x 0x%x\n", MSBBitX(prefix, j), cN->zero, cN->one, cN->dest);

	for (j = 0;((j < prefixLen) && done && cN); j++) {
		if (DEBUG_SEARCH) printf(".");

		if (cN->dest) 
			lastFound = cN->dest;

		if (MSBBitX(prefix, j)) {
			if (DEBUG_SEARCH) printf("1");

			// make sure there is 1 pointer, else return lastFound
			if (cN->one) {
				if (cN->omaskLen > prefixLen) {
					// return current node
					done = 0;

					continue;
				}
				else {
					found = 1;
					for (k = 0; ((k < cN->omaskLen) && found); k++) {
						if (DEBUG_SEARCH) printf("'");

						if (MSBBitX(prefix, (j + k)) == MSBBitX(cN->omask, k))
							continue;

						// compare the bits k (for CN) and (j+k) for mask
						// if any bits dont match found = 0;
						found = 0;
					}

					if (found) {
						//lastFound = cN->one;
						cN = cN->one;
						if (cN->dest) lastFound = cN->dest;
					}
					else {
						done = 0;
					}
					j = j + k - 1;
				}
			}
			else {
				done = 0;
			}
		}
		else {
			if (DEBUG_SEARCH) printf("0");

			// make sure there is 0 pointer, else return lastFound
			if (cN->zero) {
				if (cN->zmaskLen > prefixLen) {
					// return current node
					done = 0;

					continue;
				}
				else {
					found = 1;
					for (k = 0; ((k < cN->zmaskLen) && found); k++) {
						if (DEBUG_SEARCH) printf("'");

						if (MSBBitX(prefix, (j + k)) == MSBBitX(cN->zmask, k))
							continue;

						// compare the bits k (for CN) and (j+k) for mask
						// if any bits dont match found = 0;
						found = 0;
					}

					if (found) {
						//lastFound = cN->zero;
						cN = cN->zero;
						if (cN->dest) lastFound = cN->dest;
					}
					else {
						done = 0;
					}
					j = j + k - 1;
				}
			}
			else {
				//return lastFound || lastValid
				done = 0;
			}
		}
	}

	if (cN && cN->pdimList)
		lastFound = cN->dest;

	return (SearchSourceTrieC(lastFound, dest, source, 0));
}

unsigned int SearchOtherDims(unsigned int sp, unsigned int dp, uchar pr) {
	int i = 0;
	unsigned int result = 0xffffffff;
	PFilter tFilter;

	for (i = 0; i < nSearchResults; i++) {
		tFilter = SEARCH_RESULTS[i]->pfilter; //&(filtset.filtArr[SEARCH_RESULTS[i]->cost]);
		if(tFilter->cost > result) continue;

		if (((sp >= tFilter->fromPort[0]) && (sp <= tFilter->toPort[0])) && ((dp >= tFilter->fromPort[1]) && (dp <= tFilter->toPort[1]))) {
			if ((tFilter->protLen > 0)) {
				if  (pr == tFilter->protPref) {
					result = tFilter->cost;
				}
			}
			else
				result = tFilter->cost;
		}
	}

	return result;
}
