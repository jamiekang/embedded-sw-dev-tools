/*
All Rights Reserved.
*/
/** 
* @file memref.cc
* Memory reference information (sMemRef) linked list implementation
*
* @date 2008-12-08
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "dspsim.h"
#include "memref.h"
//#include "simsupport.h"

/** 
* @brief Allocate a node dynamically.
* 
* @param addr Program memory address where the symbol is referenced.
* 
* @return Returns pointer to newly allocated node.
*/
sMemRef *sMemRefGetNode(int addr)
{
	sMemRef *p;
	p = (sMemRef *)calloc(1, sizeof(sMemRef));
	assert(p != NULL);

	p->Addr = addr;

	return p;
}

/** 
* Check for data duplication and add new node to the END(LastNode) of the list.
* * Note: Before adding first node (empty list), FirstNode MUST be NULL.
* 
* @param list Pointer to linked list
* @param addr Program memory address where the symbol is referenced.
* 
* @return Returns pointer to newly allocated node.
*/
extern "C" sMemRef *sMemRefListAdd(sMemRefList *list, int addr)
{
	sMemRef *n = sMemRefGetNode(addr);
	if(n == NULL) return NULL;

	if(list->FirstNode != NULL){ /* if FistNode is not NULL, 
										LastNode is also not NULL */
		list->LastNode = sMemRefListInsertAfter(list->LastNode, n);
	} else {	/* FirstNode == NULL (list empty) */
		list->LastNode = sMemRefListInsertBeginning(list, n); 
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
sMemRef *sMemRefListInsertAfter(sMemRef *p, sMemRef *newNode)	
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
sMemRef *sMemRefListInsertBeginning(sMemRefList *list, sMemRef *newNode)	
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
void sMemRefListRemoveAfter(sMemRefList *list, sMemRef *p)	
{
	if(p != NULL){
		if(p->Next != NULL){
			sMemRef *obsoleteNode = p->Next;
			p->Next = p->Next->Next;

			if(obsoleteNode->Next == NULL){	/* if it is LastNode */
				list->LastNode = p;		/* update LastNode */
			}
			/* free node */
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
sMemRef *sMemRefListRemoveBeginning(sMemRefList *list)	
{
	if(list != NULL){
		if(list->FirstNode != NULL){
			sMemRef *obsoleteNode = list->FirstNode;
			list->FirstNode = list->FirstNode->Next;
			if(list->FirstNode == NULL) list->LastNode = NULL; /* removed last node */

			/* free node */
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
void sMemRefListRemoveAll(sMemRefList *list)
{
	while(list->FirstNode != NULL){
		sMemRefListRemoveBeginning(list);
	}
}

/** 
* @brief Search entire list matching given data value.
* 
* @param list Pointer to linked list
* @param addr Program memory address where the symbol is referenced.
* 
* @return Returns pointer to the matched node or NULL.
*/
sMemRef *sMemRefListSearch(sMemRefList *list, int addr)
{
	sMemRef *n = list->FirstNode;

	while(n != NULL) {
		if(n->Addr == addr){		/* search for same addr */
			/* match found */
			return n;
		}
		n = n->Next;
	}

	/* match not found */
	return NULL;
}


/** 
* @brief Initialize memRef
* 
* @param list sMemRefList data structure
*/
void memRefInit(sMemRefList *list)
{
	list->FirstNode = NULL;
	list->LastNode  = NULL;
}

