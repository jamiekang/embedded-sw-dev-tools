/*
All Rights Reserved.
*/
/** 
* @file secinfo.cc
* Section information (sSecInfo) linked list implementation
*
* @date 2008-12-01
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "dspsim.h"
#include "symtab.h"	//just for eTabID
#include "secinfo.h"

/** 
* @brief Allocate a node dynamically.
* 
* @param name Pointer to segment name string
* @param t enum eSec value (tCODE or tDATA)
* 
* @return Returns pointer to newly allocated node.
*/
sSecInfo *sSecInfoGetNode(char *name, int t)
{
	sSecInfo *p;
	p = (sSecInfo *)calloc(1, sizeof(sSecInfo));
	assert(p != NULL);

	p->Name = strdup(name);
	p->Type = t;

	return p;
}

/** 
* Check for data duplication and add new node to the END(LastNode) of the list.
* * Note: Before adding first node (empty list), FirstNode MUST be NULL.
* 
* @param list Pointer to linked list
* @param name Pointer to segment name string
* @param t enum eSec value (tCODE or tDATA)
* 
* @return Returns pointer to newly allocated node.
*/
extern "C" sSecInfo *sSecInfoListAdd(sSecInfoList *list, char *name, int t)
{
	sSecInfo *n = sSecInfoGetNode(name, t);
	if(n == NULL) return NULL;

	if(list->FirstNode != NULL){ /* if FistNode is not NULL, 
										LastNode is also not NULL */
		list->LastNode = sSecInfoListInsertAfter(list->LastNode, n);
	} else {	/* FirstNode == NULL (list empty) */
		list->LastNode = sSecInfoListInsertBeginning(list, n); 
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
sSecInfo *sSecInfoListInsertAfter(sSecInfo *p, sSecInfo *newNode)	
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
sSecInfo *sSecInfoListInsertBeginning(sSecInfoList *list, sSecInfo *newNode)	
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
void sSecInfoListRemoveAfter(sSecInfoList *list, sSecInfo *p)	
{
	if(p != NULL){
		if(p->Next != NULL){
			sSecInfo *obsoleteNode = p->Next;
			p->Next = p->Next->Next;

			if(obsoleteNode->Next == NULL){	/* if it is LastNode */
				list->LastNode = p;		/* update LastNode */
			}
			/* free node */
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
sSecInfo *sSecInfoListRemoveBeginning(sSecInfoList *list)	
{
	if(list != NULL){
		if(list->FirstNode != NULL){
			sSecInfo *obsoleteNode = list->FirstNode;
			list->FirstNode = list->FirstNode->Next;
			if(list->FirstNode == NULL) list->LastNode = NULL; /* removed last node */

			/* free node */
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
void sSecInfoListRemoveAll(sSecInfoList *list)
{
	while(list->FirstNode != NULL){
		sSecInfoListRemoveBeginning(list);
	}
}

/** 
* @brief Search entire list matching given data value.
* 
* @param list Pointer to linked list
* @param name Pointer to segment name string
* 
* @return Returns pointer to the matched node or NULL.
*/
sSecInfo *sSecInfoListSearch(sSecInfoList *list, char *name)
{
	sSecInfo *n = list->FirstNode;

	while(n != NULL) {
		if(!strcasecmp(n->Name, name)){		/* search for same name */
			/* match found */
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
void sSecInfoListPrint(sSecInfoList *list)
{
	sSecInfo *n = list->FirstNode;
	char ssec[10];

	printf("\n-------------------------------\n");
	printf("** DUMP: Section Information ** \n");
	printf("-------------------------------\n");

	if(n == NULL){
		printf("list is empty\n");
	}

	if(n != NULL){
		printf("Node Ptr. Next Ptr. Type Name     Size\n");
	}

	while(n != NULL){
		sSecInfoListPrintType(ssec, n->Type);

		if(n->Next)
			printf("%p %p %s %-8s %d", n, n->Next, ssec, n->Name, n->Size);
		else
			printf("%p (NULL)    %s %-8s %d", n, ssec, n->Name, n->Size);
		printf("\n");

		n = n->Next;
	}
	printf("F %p L %p\n", list->FirstNode, list->LastNode);
}


/**
* @brief Get string for type ("NONE ", "CODE", or "DATA")
*
* @param s Pointer to output string buffer
* @param type Type of current symbol table node
*/
void sSecInfoListPrintType(char *s, int type)
{
    switch(type){
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
* @brief Initialize secInfo
* 
* @param list sSecInfoList data structure
*/
void secInfoInit(sSecInfoList *list)
{
	list->FirstNode = NULL;
	list->LastNode  = NULL;
}

/** 
* @brief Dump secinfo data structure into symbol table(.sym) file for linker
* 
* @param list Pointer to linked list
*/
void sSecInfoSymDump(sSecInfoList *list)
{
	sSecInfo *n = list->FirstNode;
	char ssec[10];
	int tabID;

    /* dump secinfo list to .sym file */
    fprintf(dumpSymFP, ";-----------------------------------------------------------\n");
    fprintf(dumpSymFP, "; Segment Information\n");
    fprintf(dumpSymFP, ";-----------------------------------------------------------\n");
    fprintf(dumpSymFP, "; ID Type Name Size\n");

    /* dump secinfo list to .lst file */
    fprintf(dumpLstFP, ";-----------------------------------------------------------\n");
    fprintf(dumpLstFP, "; Segment Information\n");
    fprintf(dumpLstFP, ";-----------------------------------------------------------\n");
    fprintf(dumpLstFP, "; Type Name Size\n");

	tabID = tSecInfo;

	if(!n){
		fprintf(dumpLstFP, "(No segments defined!!)\n");
		fprintf(dumpLstFP, "Code Size: %d (0x%X)\n\n", (curaddr - codeSegAddr), (curaddr - codeSegAddr));
	}else{
		while(n != NULL){
			sSecInfoListPrintType(ssec, n->Type);

			if(n->isOdd){
				fprintf(dumpSymFP, "%d %s %s %d\n", tabID, ssec, n->Name, n->Size);
				fprintf(dumpLstFP, "%s %s %d (including 1-word __DUMMY_FILENAME__ variable inserted for even byte alignment.)\n", ssec, n->Name, n->Size);
			}else{
				fprintf(dumpSymFP, "%d %s %s %d\n", tabID, ssec, n->Name, n->Size);
				fprintf(dumpLstFP, "%s %s %d\n", ssec, n->Name, n->Size);
			}

			n = n->Next;
		}
		fprintf(dumpSymFP, "%d\n", tEnd);
		fprintf(dumpLstFP, "\n");
	}
}


