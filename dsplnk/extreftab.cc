/*
All Rights Reserved.
*/
/** 
* @file extreftab.cc
* External Symbol Reference table (sExtRefTab) linked list and hashing table implementation
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
#include "extreftab.h"
#include "symtab.h"
#include "dsplnk.h"

/** 
* @brief Allocate a node dynamically.
* 
* @param s Symbol name for new node
* @param n Symbol address for new node
* @param seg Segment attribute (enum eSeg tCODE or tDATA) for new node
* @param m Pointer to module this symbol belongs to
* 
* @return Returns pointer to newly allocated node.
*/
sExtRefTab *sExtRefTabGetNode(char *s, unsigned int n, int seg, sModule *m)
{
	sExtRefTab *p;
	p = (sExtRefTab *)malloc(sizeof(sExtRefTab));
	assert(p != NULL);

	p->Name = strdup(s);
	p->Addr = n;
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
* @param seg Segment attribute (enum eSeg tCODE or tDATA) for new node
* @param m Pointer to module this symbol belongs to
* 
* @return Returns pointer to newly allocated node.
*/
sExtRefTab *sExtRefTabListAdd(sExtRefTabList *list, char *s, unsigned int i, int seg, sModule *m)
{
	sExtRefTab *p;

	if(s == NULL) return NULL;

	sExtRefTab *n = sExtRefTabGetNode(s, i, seg, m);
	if(n == NULL) return NULL;

	if(list->FirstNode != NULL){ /* if FistNode is not NULL, 
										LastNode is also not NULL */

		p = sExtRefTabListSearch(list, i, seg);	/* check for duplication */
		if((p == NULL) || (p && p->Seg != seg))	/* if not duplicated */
			list->LastNode = sExtRefTabListInsertAfter(list->LastNode, n);
		else{	/* if duplicated: this case is not allowed. */
			free(n);	/* no longer needed */ 

			/* print error message and exit */
			char linebuf[MAX_LINEBUF];
			sprintf(linebuf, "Duplicated external symbol name not allowed in %s(.sym) and %s(.sym).", 
				p->Module->Name, m->Name);
			printErrorMsg(s, linebuf);

			return p;	/* return registered node */
		}
	} else {	/* FirstNode == NULL (list empty) */
		list->LastNode = sExtRefTabListInsertBeginning(list, n); 
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
sExtRefTab *sExtRefTabListInsertAfter(sExtRefTab *p, sExtRefTab *newNode)	
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
sExtRefTab *sExtRefTabListInsertBeginning(sExtRefTabList *list, sExtRefTab *newNode)	
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
* @param p Pointer to the node just before the one to be removed
*/
void sExtRefTabListRemoveAfter(sExtRefTabList *list, sExtRefTab *p)	
{
	if(p != NULL){
		if(p->Next != NULL){
			sExtRefTab *obsoleteNode = p->Next;
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
sExtRefTab *sExtRefTabListRemoveBeginning(sExtRefTabList *list)	
{
	if(list != NULL){
		if(list->FirstNode != NULL){
			sExtRefTab *obsoleteNode = list->FirstNode;
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
void sExtRefTabListRemoveAll(sExtRefTabList *list)
{
	while(list->FirstNode != NULL){
		sExtRefTabListRemoveBeginning(list);
	}
}

/** 
* @brief Search entire list matching given data value.
* 
* @param list Pointer to linked list
* @param i Symbol address for new node
* @param seg Segment attribute (enum eSeg tCODE or tDATA) for new node
* 
* @return Returns pointer to the matched node or NULL.
*/
sExtRefTab *sExtRefTabListSearch(sExtRefTabList *list, unsigned int i, int seg)
{
	sExtRefTab *n = list->FirstNode;

	while(n != NULL) {
		if((n->Addr == i) && (n->Seg == seg)){	/* match found */
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
void sExtRefTabListPrint(sExtRefTabList *list)
{
	sExtRefTab *n = list->FirstNode;
	char stype[10];

	if(n == NULL){
		printf("list is empty\n");
		return;
	}

	if(n != NULL){
		printf("format: Node Next Addr Name Type   Seg.\n");
	}

	while(n != NULL){
		/*
		sExtRefTabListPrintType(stype, n->Type);

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
* @brief Get string for segment ("NONE ", "CODE", or "DATA")
* 
* @param s Pointer to output string buffer
* @param seg Segment current symbol table node belongs to
*/
void sExtRefTabListPrintSeg(char *s, int seg)
{
	switch(seg){
        case    tNONE:
            strcpy(s, "NONE");
            break;
        case    tCODE:
            strcpy(s, "CODE");
            break;
        case    tDATA:
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
* @param i Symbol address for new node
* @param seg Segment attribute (enum eSeg tCODE or tDATA) for new node
* 
* @return Returns index to hashing table.
*/
int sExtRefTabHashFunction(unsigned int i, int seg)
{
	int key;

	key = (int)i + MAX_HASHTABLE;
	key += seg;

	return (key % MAX_HASHTABLE);
}

/** 
* @brief Check for data duplication and add new node to hash table. 
* 
* @param htable Hash table data structure
* @param s Symbol name for newly added node
* @param i Symbol address for newly added node
* @param seg Segment attribute (enum eSeg tCODE or tDATA) for new node
* @param m Pointer to module this symbol belongs to
* 
* @return Returns pointer to the newly added node.
*/
sExtRefTab *sExtRefTabHashAdd(sExtRefTabList htable[], char *s, unsigned int i, int seg, sModule *m)
{
	int hindex;
	hindex = sExtRefTabHashFunction(i, seg);
	return(sExtRefTabListAdd(&htable[hindex], s, i, seg, m));
}

/** 
* @brief Print all nodes in the hash table.
* 
* @param htable Hash table data structure
*/
void sExtRefTabHashPrint(sExtRefTabList htable[])
{
	int i;

	printf("\n------------------------------\n");
	printf("** DUMP: External Symbol Reference Table ** \n");
	printf("------------------------------\n");

	for(i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			printf("\nHash Index: %d\n", i);
			sExtRefTabListPrint(&htable[i]);
		}
	}
}

/** 
* @brief Search entire hash table matching given data value.
* 
* @param htable Hash table data structure
* @param i Symbol address for new node
* @param seg Segment attribute (enum eSeg tCODE or tDATA) for new node
* 
* @return Returns pointer to the matched node or NULL.
*/
sExtRefTab *sExtRefTabHashSearch(sExtRefTabList htable[], unsigned int i, int seg)
{
	int hindex;
	hindex = sExtRefTabHashFunction(i, seg);
	return(sExtRefTabListSearch(&htable[hindex], i, seg));
}

/** 
* @brief Remove node past given node. 
* * Note: Not the given node itself, it's the one right next to that!! 
* 
* @param htable Hash table data structure
* @param p Pointer to the node just before the one to be removed
*/
void sExtRefTabHashRemoveAfter(sExtRefTabList htable[], sExtRefTab *p)
{
	sExtRefTab *n; 
	int hindex;

	hindex = sExtRefTabHashFunction(p->Addr, p->Seg);
	n = sExtRefTabHashSearch(htable, p->Addr, p->Seg);

	if(n != NULL){
		sExtRefTabListRemoveAfter(&htable[hindex], n);
	}
}

/** 
* @brief Remove all nodes in the hash table.
* 
* @param htable Hash table data structure
*/
void sExtRefTabHashRemoveAll(sExtRefTabList htable[])
{
	int i;

	for(i = 0; i < MAX_HASHTABLE; i++){
		sExtRefTabListRemoveAll(&htable[i]);
	}
}


/** 
* @brief Initialize symbol table list
* 
* @param htable sExtRefTabList data structure 
*/
void sExtRefTabInit(sExtRefTabList htable[])
{
	int i;

	for(i = 0; i < MAX_HASHTABLE; i++){
		htable[i].FirstNode = NULL;
		htable[i].LastNode  = NULL;
	}
}

/** 
* @brief Build external symbol reference table from module lists
* 
* @param tablist external symbol reference table linked lists
* @param stablist global symbol table linked lists
* @param modlist input module linked lists
*/
void externRefTabBuild(sExtRefTabList tablist[], sTabList stablist[], sModuleList *modlist)
{
	char linebuf[MAX_LINEBUF];
	int tabID;
	unsigned int addr;
	char name[MAX_LINEBUF];
	char type[10];
	char segname[10];
	int seg;

	if(VerboseMode) printf("\n.sym file reading at externRefTabBuild().\n");

	sModule *n = modlist->FirstNode;
	while(n != NULL){
		if(VerboseMode) printf("-- File %0d: %s --\n", n->ModuleNo, n->Name);

		/* process each .sym file */
		while(fgets(linebuf, MAX_LINEBUF, n->SymFP)){
			if(linebuf[0] == ';') continue;
			sscanf(linebuf, "%d ", &tabID);

			if(tabID == tExtern){
				sscanf(linebuf, "%d %04X %s %s %s\n", &tabID, &addr, name, type, segname);
				sTab *sp = sTabHashSearch(stablist, name);
				if(sp == NULL){
					/* print error message and exit */
					sprintf(linebuf, "Extern symbol not found in global symbol table.");
					printErrorMsg(name, linebuf);
				}else{
					sTabListPrintSeg(segname, sp->Seg);
				}

				if(VerboseMode) 
					printf("addr: %04X, name: %s, segment: %s\n", 
						addr, name, segname);

				/* update address considering each module offset */
				if(!strcasecmp(segname, "DATA")){
					seg = tDATA;
				}else if(!strcasecmp(segname, "CODE")){
					seg = tCODE;
				}else{
					seg = tNONE;
				}
				addr += n->CodeSegAddr;

				sExtRefTab *p = sExtRefTabHashAdd(tablist, name, addr, seg, n);
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
void sExtRefTabDump(sExtRefTabList htable[])
{
	char sseg[10];
	int tabID;

	/* dump external symbol reference table */
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	fprintf(MapFP, "; Input: External Symbol Reference Table\n");
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	if(VerboseMode)
		fprintf(MapFP, "; ID ");
	else
		fprintf(MapFP, "; ");
	fprintf(MapFP, "Addr Name Module Segment\n");

	tabID = tExtern;

	/* get number of extern references */
	int externCntr = 0;

	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sExtRefTab *n = htable[i].FirstNode;

			while(n != NULL){
				externCntr++;

				n = n->Next;
			}
		}
	}

	/* initialize pointer array to be sorted */
	int j = 0;
	sExtRefTab **sortedExtern = (sExtRefTab **)malloc(externCntr*sizeof(sExtRefTab *));
	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sExtRefTab *n = htable[i].FirstNode;

			while(n != NULL){
				sortedExtern[j] = n;
				j++;

				n = n->Next;
			}
		}
	}

	bubbleSort((sTab **)sortedExtern, externCntr);

	for(int i = 0; i < externCntr; i++){
		sExtRefTab *n = sortedExtern[i];
		sExtRefTabListPrintSeg(sseg, n->Seg);

		if(VerboseMode)
			fprintf(MapFP, "%d ", tabID);
		fprintf(MapFP, "%04X %s %s %s\n", n->Addr, n->Name, n->Module->Name, sseg);
	}
	free(sortedExtern);

	/*
	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sExtRefTab *n = htable[i].FirstNode;

			while(n != NULL){
				sExtRefTabListPrintSeg(sseg, n->Seg);
				if(VerboseMode)
					fprintf(MapFP, "%d ", tabID);
				fprintf(MapFP, "%04X %s %s %s\n", n->Addr, n->Name, n->Module->Name, sseg);
				n = n->Next;
			}
		}
	}
	*/

	if(VerboseMode){
		fprintf(MapFP, "%d\n", tEnd);
		fprintf(MapFP, "; Total: %d external variable references\n", externCntr);
	} else
		fprintf(MapFP, "\n");

}

