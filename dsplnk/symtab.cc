/*
All Rights Reserved.
*/
/** 
* @file symtab.cc
* Symbol table (sTab) linked list and hashing table implementation
* Note: this version is for linker(dsplnk) only.
*
* Reference 1: http://faculty.ycp.edu/~dhovemey/spring2007/cs201/info/hashTables.html
*
* Reference 2: http://en.wikipedia.org/wiki/Linked_list
* @date 2008-12-17
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "symtab.h"
#include "dsplnk.h"

/** 
* @brief Allocate a node dynamically.
* 
* @param s Symbol name for new node
* @param n Symbol address for new node
* @param size Size if data variable (words)
* @param seg Segment attribute (enum eSeg tCODE or tDATA) for new node
* @param m Pointer to module this symbol belongs to
* 
* @return Returns pointer to newly allocated node.
*/
sTab *sTabGetNode(char *s, unsigned int n, int size, int seg, sModule *m)
{
	sTab *p;
	p = (sTab *)malloc(sizeof(sTab));
	assert(p != NULL);

	p->Name = strdup(s);
	p->Addr = n;
	p->Size = size;
	p->Seg = seg;
	p->Module = m;

	return p;
}

/** 
* Check for data duplication and add new node to the END(LastNode) of the list.
* * Note: Before adding first node (empty list), FirstNode MUST be NULL.
* 
* @param list Pointer to linked list
* @param s Symbol name for new node
* @param i Symbol address for new node
* @param size Size if data variable (words)
* @param seg Segment attribute (enum eSeg tCODE or tDATA) for new node
* @param m Pointer to module this symbol belongs to
* 
* @return Returns pointer to newly allocated node.
*/
sTab *sTabListAdd(sTabList *list, char *s, unsigned int i, int size, int seg, sModule *m)
{
	sTab *p;

	if(s == NULL) return NULL;

	sTab *n = sTabGetNode(s, i, size, seg, m);
	if(n == NULL) return NULL;

	if(list->FirstNode != NULL){ /* if FistNode is not NULL, 
										LastNode is also not NULL */

		p = sTabListSearch(list, s);	/* check for duplication */
		if(p == NULL)	/* if not duplicated */
			list->LastNode = sTabListInsertAfter(list->LastNode, n);
		else{	/* if duplicated: this case is not allowed. */
			free(n);	/* no longer needed */ 

			/* print error message and exit */
			char linebuf[MAX_LINEBUF];
			sprintf(linebuf, "Duplicated symbol name not allowed in %s(.sym) and %s(.sym).", 
				p->Module->Name, m->Name);
			printErrorMsg(s, linebuf);

			return p;	/* return registered node */
		}
	} else {	/* FirstNode == NULL (list empty) */
		list->LastNode = sTabListInsertBeginning(list, n); 
	}

	return n;	/* return new node (= LastNode) */
}

/** 
* Insert new node after 'previous' node - No data duplication check.
* * Note: LastNode should be updated outside. 
* 
* @param p Pointer to 'previous' node
* @param newNode Pointer to new node to be added
* 
* @return Returns pointer to new node.
*/
sTab *sTabListInsertAfter(sTab *p, sTab *newNode)	
{
	newNode->Next = p->Next;
	p->Next = newNode;

	return newNode;	/* return new node */
}

/** 
* Insert new node before the FIRST node - No data duplication check.
* * Note: LastNode should be updated outside if FirstNode was NULL.
* 
* @param list Pointer to linked list
* @param newNode Pointer to new node to be inserted
* 
* @return Returns pointer to new node.
*/
sTab *sTabListInsertBeginning(sTabList *list, sTab *newNode)	
{
	newNode->Next = list->FirstNode;
	list->FirstNode = newNode;

	return newNode;	/* return new node (= first node) */
}

/** 
* @brief Remove node past given node. 
* * Note: Not the given node itself, it's the one right next to that!! 
* 
* @param list Pointer to linked list
 @param p Pointer to the node just before the one to be removed
*/
void sTabListRemoveAfter(sTabList *list, sTab *p)	
{
	if(p != NULL){
		if(p->Next != NULL){
			sTab *obsoleteNode = p->Next;
			p->Next = p->Next->Next;

			if(obsoleteNode->Next == NULL){	/* if it is LastNode */
				list->LastNode = p;		/* update LastNode */
			}

			if(obsoleteNode->Name) free(obsoleteNode->Name);
			free(obsoleteNode);
		}
	}
}

/** 
* @brief Remove FIRST node of the list.
* 
* @param list Pointer to linked list
* 
* @return Returns updated FirstNode 
*/
sTab *sTabListRemoveBeginning(sTabList *list)	
{
	if(list != NULL){
		if(list->FirstNode != NULL){
			sTab *obsoleteNode = list->FirstNode;
			list->FirstNode = list->FirstNode->Next;
			if(list->FirstNode == NULL) list->LastNode = NULL; /* removed last node */

			if(obsoleteNode->Name) free(obsoleteNode->Name);
			free(obsoleteNode);
			return list->FirstNode;	/* return new FirstNode */
		} else {
			return NULL;
		}
	}
}

/** 
* @brief Remove all nodes in the list.
* 
* @param list Pointer to linked list
*/
void sTabListRemoveAll(sTabList *list)
{
	while(list->FirstNode != NULL){
		sTabListRemoveBeginning(list);
	}
}

/** 
* @brief Search entire list matching given data value.
* 
* @param list Pointer to linked list
* @param s symbol name to be matched
* 
* @return Returns pointer to the matched node or NULL.
*/
sTab *sTabListSearch(sTabList *list, char *s)
{
	sTab *n = list->FirstNode;

	while(n != NULL) {
		if(!strcasecmp(n->Name, s)){	/* match found */
			return n;
		}
		n = n->Next;
	}

	/* match not found */
	return NULL;
}

/** 
* @brief Print all nodes in the linked list.
* 
* @param list Pointer to linked list
*/
void sTabListPrint(sTabList *list)
{
	sTab *n = list->FirstNode;
	char stype[10];

	if(n == NULL){
		printf("list is empty\n");
		return;
	}

	if(n != NULL){
		printf("format: Node Next Addr Name Type   Seg.\n");
	}

	while(n != NULL){
		sTabListPrintType(stype, n->Type);

		/*
		if(n->Sec){
			printf("print %p %p %04X %s %s %s\n", n, n->Next, n->Addr, 
				n->Name, stype, n->Sec->Name);
		}else{
			printf("print %p %p %04X %s %s ----\n", n, n->Next, n->Addr, 
				n->Name, stype);
		}
		*/
		n = n->Next;
	}
	printf("F %p L %p\n", list->FirstNode, list->LastNode);
}

/** 
* @brief Get string for type ("LOCAL ", "GLOBAL", or "EXTERN")
* 
* @param s Pointer to output string buffer
* @param type Type of current symbol table node
*/
void sTabListPrintType(char *s, int type)
{
	switch(type){
		case	tLOCAL:
			strcpy(s, "LOCAL");
			break;
		case	tGLOBAL:
			strcpy(s, "GLOBAL");
			break;
		case	tEXTERN:
			strcpy(s, "EXTERN");
			break;
		default:
			strcpy(s, "------");
			break;
	}
}

/** 
* @brief Get string for segment ("NONE ", "CODE", or "DATA")
* 
* @param s Pointer to output string buffer
* @param seg Segment current symbol table node belongs to
*/
void sTabListPrintSeg(char *s, int seg)
{
	switch(seg){
		case	tNONE:
			strcpy(s, "NONE");
			break;
		case	tCODE:
			strcpy(s, "CODE");
			break;
		case	tDATA:
			strcpy(s, "DATA");
			break;
		default:
			strcpy(s, "----");
			break;
	}
}

/** 
* @brief Simple hash function for integer input data.
* 
* @param s Input string 
* 
* @return Returns index to hashing table.
*/
int sTabHashFunction(char *s)
{
	int i, key;
	int len;

	len = strlen(s);

	key = (int)s[0];
	for(i = 1; i < len; i++){
		key = key ^ (int)s[i];
	}

	return (key % MAX_HASHTABLE);
}

/** 
* @brief Check for data duplication and add new node to hash table. 
* 
* @param htable Hash table data structure
* @param s Symbol name for newly added node
* @param i Symbol address for newly added node
* @param size Size if data variable (words)
* @param seg Segment attribute (enum eSeg tCODE or tDATA) for new node
* @param m Pointer to module this symbol belongs to
* 
* @return Returns pointer to the newly added node.
*/
sTab *sTabHashAdd(sTabList htable[], char *s, unsigned int i, int size, int seg, sModule *m)
{
	int hindex;
	hindex = sTabHashFunction(s);
	return(sTabListAdd(&htable[hindex], s, i, size, seg, m));
}

/** 
* @brief Print all nodes in the hash table.
* 
* @param htable Hash table data structure
*/
void sTabHashPrint(sTabList htable[])
{
	int i;

	printf("\n------------------------------\n");
	printf("** DUMP: Symbol Table ** \n");
	printf("------------------------------\n");

	for(i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			printf("\nHash Index: %d\n", i);
			sTabListPrint(&htable[i]);
		}
	}
}

/** 
* @brief Search entire hash table matching given data value.
* 
* @param htable Hash table data structure
* @param s symbol name to be matched
* 
* @return Returns pointer to the matched node or NULL.
*/
sTab *sTabHashSearch(sTabList htable[], char *s)
{
	int hindex;
	hindex = sTabHashFunction(s);
	return(sTabListSearch(&htable[hindex], s));
}

/** 
* @brief Remove node past given node. 
* * Note: Not the given node itself, it's the one right next to that!! 
* 
* @param htable Hash table data structure
* @param p Pointer to the node just before the one to be removed
*/
void sTabHashRemoveAfter(sTabList htable[], sTab *p)
{
	sTab *n; 
	int hindex;

	hindex = sTabHashFunction(p->Name);
	n = sTabHashSearch(htable, p->Name);

	if(n != NULL){
		sTabListRemoveAfter(&htable[hindex], n);
	}
}

/** 
* @brief Remove all nodes in the hash table.
* 
* @param htable Hash table data structure
*/
void sTabHashRemoveAll(sTabList htable[])
{
	int i;

	for(i = 0; i < MAX_HASHTABLE; i++){
		sTabListRemoveAll(&htable[i]);
	}
}


/** 
* @brief Initialize symbol table list
* 
* @param htable sTabList data structure 
*/
void sTabInit(sTabList htable[])
{
	int i;

	for(i = 0; i < MAX_HASHTABLE; i++){
		htable[i].FirstNode = NULL;
		htable[i].LastNode  = NULL;
	}
}

/** 
* @brief Build global symbol table from module lists
* 
* @param tablist global symbol table linked lists
* @param modlist input module linked lists
*/
void globalSymTabBuild(sTabList tablist[], sModuleList *modlist)
{
	char linebuf[MAX_LINEBUF];
	int tabID;
	unsigned int addr;
	char name[MAX_LINEBUF];
	int size;
	char type[10];
	char segname[10];
	int seg;

	if(VerboseMode) printf("\n.sym file reading at globalSymTabBuild().\n");

	sModule *n = modlist->FirstNode;
	while(n != NULL){
		if(VerboseMode) printf("-- File %0d: %s --\n", n->ModuleNo, n->Name);

		/* process each .sym file */
		while(fgets(linebuf, MAX_LINEBUF, n->SymFP)){
			if(linebuf[0] == ';') continue;
			sscanf(linebuf, "%d ", &tabID);

			if(tabID == tGlobal){
				sscanf(linebuf, "%d %04X %s %d %s %s\n", &tabID, &addr, name, &size, type, segname);
				if(VerboseMode) 
					printf("addr: %04X, name: %s, size: %d, segment: %s\n", 
						addr, name, size, segname);

				/* update address considering each module offset */
				if(!strcasecmp(segname, "DATA")){
					addr += n->DataSegAddr;
					seg = tDATA;
				}else if(!strcasecmp(segname, "CODE")){
					addr += n->CodeSegAddr;
					seg = tCODE;
				}else{
					seg = tNONE;
				}

				sTab *p = sTabHashAdd(tablist, name, addr, size, seg, n);
			}else{
				break;	/* move to next .sym file */
			}
		};

		/* move to next .sym file */
		n = n->Next;
	};
}

/** 
* @brief Build all code & data symbol table from module lists
* 
* @param tablist all symbol table linked lists
* @param modlist input module linked lists
*/
void allSymTabBuild(sTabList tablist[], sModuleList *modlist)
{
	char linebuf[MAX_LINEBUF];
	int tabID;
	unsigned int addr;
	char name[MAX_LINEBUF];
	int size;
	char type[10];
	char segname[10];
	int seg;

	if(VerboseMode) printf("\n.sym file reading at allSymTabBuild().\n");

	sModule *n = modlist->FirstNode;
	while(n != NULL){
		if(VerboseMode) printf("-- File %0d: %s --\n", n->ModuleNo, n->Name);

		/* process each .sym file */
		while(fgets(linebuf, MAX_LINEBUF, n->SymFP)){
			if(linebuf[0] == ';') continue;
			sscanf(linebuf, "%d ", &tabID);

			if(tabID == tAll){
				sscanf(linebuf, "%d %04X %s %d %s %s\n", &tabID, &addr, name, &size, type, segname);
				if(VerboseMode) 
					printf("addr: %04X, name: %s, size: %d, type: %s, segment: %s\n", 
						addr, name, size, type, segname);

				/* update address considering each module offset */
				if(!strcasecmp(segname, "DATA")){
					addr += n->DataSegAddr;
					seg = tDATA;
				}else if(!strcasecmp(segname, "CODE")){
					addr += n->CodeSegAddr;
					seg = tCODE;
				}else{
					seg = tNONE;
				}

				if(strcasecmp(type, "EXTERN")){		/* add only if not EXTERN */
					sTab *p = sTabHashAdd(tablist, name, addr, size, seg, n);

					if(!strcasecmp(type, "GLOBAL")) p->Type = tGLOBAL;					
					else if(!strcasecmp(type, "LOCAL")) p->Type = tLOCAL;					
				}
			}else{
				break;	/* move to next .sym file */
			}
		};

		/* move to next .sym file */
		n = n->Next;
	};
}

/** 
* @brief Generate symbol table (.sym) file for linker
* 
* @param htable Symbol table data structure
*/
void sTabSymDump(sTabList htable[])
{
	char stype[10];
	char sseg[10];
	int tabID;

	/* dump global symbol definition table */
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	fprintf(MapFP, "; Input: Global Symbol Definition Table\n");
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	if(VerboseMode)
		fprintf(MapFP, "; ID ");
	else
		fprintf(MapFP, "; ");
	fprintf(MapFP, "Addr Name Module Segment\n");

	tabID = tGlobal;

	/* get number of global symbols */
	int globalCntr = 0;

	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sTab *n = htable[i].FirstNode;

			while(n != NULL){
				globalCntr++;

				n = n->Next;
			}
		}
	}

	/* initialize pointer array to be sorted */
	int j = 0;
	sTab **sortedGlobal = (sTab **)malloc(globalCntr*sizeof(sTab *));
	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sTab *n = htable[i].FirstNode;

			while(n != NULL){
				sortedGlobal[j] = n;
				j++;

				n = n->Next;
			}
		}
	}

	bubbleSort(sortedGlobal, globalCntr);

	for(int i = 0; i < globalCntr; i++){
		sTab *n = sortedGlobal[i];
		sTabListPrintSeg(sseg, n->Seg);
		if(VerboseMode)
			fprintf(MapFP, "%d ", tabID);
		fprintf(MapFP, "%04X %s %d %s %s\n", n->Addr, n->Name, n->Size, n->Module->Name, sseg);
	}
	free(sortedGlobal);

	/*
	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sTab *n = htable[i].FirstNode;

			while(n != NULL){
				sTabListPrintSeg(sseg, n->Seg);
				if(VerboseMode)
					fprintf(MapFP, "%d ", tabID);
				fprintf(MapFP, "%04X %s %d %s %s\n", n->Addr, n->Name, n->Size, n->Module->Name, sseg);

				n = n->Next;
			}
		}
	}
	*/


	if(VerboseMode){
		fprintf(MapFP, "%d\n", tEnd);
		fprintf(MapFP, "; Total: %d global symbols\n", globalCntr);
	} else
		fprintf(MapFP, "\n");

}

/** 
* @brief Write data segment map and code segment labels to .map file
* 
* @param htable Symbol table data structure
*/
void sTabAllSymDump(sTabList htable[])
{
	char stype[10];
	char sseg[10];

	/* dump code segment labels */
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	fprintf(MapFP, "; Output: CODE Segment Symbol Table (Labels)\n");
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	fprintf(MapFP, "Addr Name Type Module Segment\n");

	/* get number of labels */
	int labelCntr = 0;

	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sTab *n = htable[i].FirstNode;

			while(n != NULL){
				if(n->Seg == tCODE){
					labelCntr++;
				}
				n = n->Next;
			}
		}
	}

	/* initialize pointer array to be sorted */
	int j = 0;
	sTab **sortedLabel = (sTab **)malloc(labelCntr*sizeof(sTab *));
	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sTab *n = htable[i].FirstNode;

			while(n != NULL){
				if(n->Seg == tCODE){
					sortedLabel[j] = n;
					j++;
				}
				n = n->Next;
			}
		}
	}

	bubbleSort(sortedLabel, labelCntr);

	for(int i = 0; i < labelCntr; i++){
		sTab *n = sortedLabel[i];
		sTabListPrintSeg(sseg, n->Seg);
		sTabListPrintType(stype, n->Type);
		fprintf(MapFP, "%04X %s %s %s %s\n", 
			n->Addr, n->Name, stype, n->Module->Name, sseg);
	}
	free(sortedLabel);

	/*
	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sTab *n = htable[i].FirstNode;

			while(n != NULL){
				if(n->Seg == tCODE){
					sTabListPrintSeg(sseg, n->Seg);
					sTabListPrintType(stype, n->Type);
					fprintf(MapFP, "%04X %s %s %s %s\n", 
						n->Addr, n->Name, stype, n->Module->Name, sseg);
				}

				n = n->Next;
			}
		}
	}
	*/
	if(VerboseMode) fprintf(MapFP, "; Total: %d labels\n", labelCntr);
	fprintf(MapFP, "\n");

	/* dump data segment */
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	fprintf(MapFP, "; Output: DATA Segment Symbol Table (Variables)\n");
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	fprintf(MapFP, "Addr Name Size Type Module Segment\n");

	/* get number of variables */
	int varCntr = 0;
	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sTab *n = htable[i].FirstNode;

			while(n != NULL){
				if(n->Seg == tDATA){
					varCntr++;
				}

				n = n->Next;
			}
		}
	}

	/* initialize pointer array to be sorted */
	j = 0;
	sTab **sortedVar = (sTab **)malloc(varCntr*sizeof(sTab *));

	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sTab *n = htable[i].FirstNode;

			while(n != NULL){
				if(n->Seg == tDATA){
					sortedVar[j] = n;
					j++;
				}
				n = n->Next;
			}
		}
	}

	bubbleSort(sortedVar, varCntr);

	for(int i = 0; i < varCntr; i++){
		sTab *n = sortedVar[i];
		sTabListPrintSeg(sseg, n->Seg);
		sTabListPrintType(stype, n->Type);
		fprintf(MapFP, "%04X %s %d %s %s %s\n", 
			n->Addr, n->Name, n->Size, stype, n->Module->Name, sseg);
	}
	free(sortedVar);

	/*
	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sTab *n = htable[i].FirstNode;

			while(n != NULL){
				if(n->Seg == tDATA){
					sTabListPrintSeg(sseg, n->Seg);
					sTabListPrintType(stype, n->Type);
					fprintf(MapFP, "%04X %s %d %s %s %s\n", 
						n->Addr, n->Name, n->Size, stype, n->Module->Name, sseg);
				}

				n = n->Next;
			}
		}
	}
	*/
	if(VerboseMode) fprintf(MapFP, "; Total: %d variables\n", varCntr);
	fprintf(MapFP, "\n");
}

/** 
* @brief Bubble sort implementation of (sTab *) type array
* 
* @param list Symbol table array to be sorted
* @param cntr Number of elements in the list
*/
void bubbleSort(sTab **list, int cntr)
{
	int swapped;
	int n = cntr;

	do{
		swapped = FALSE;
		n = n-1;
		for(int i = 0; i < n; i++){
			if((list[i]->Addr) > (list[i+1]->Addr)){
				/* swap */
				sTab *temp = list[i];
				list[i] = list[i+1];
				list[i+1] = temp;

				swapped = TRUE;
			}
		}
	} while (swapped);
}
