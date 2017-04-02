/*
All Rights Reserved.
*/
/** 
* @file dmem.cc
* Data memory (dMem) linked list and hashing table implementation
*
* Reference 1: http://faculty.ycp.edu/~dhovemey/spring2007/cs201/info/hashTables.html
*
* Reference 2: http://en.wikipedia.org/wiki/Linked_list
* @date 2008-09-22
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "binsim.h"
#include "dmem.h"

/** 
* @brief Allocate a node dynamically.
* 
* @param d 12-bit Data for new node
* @param a Data Memory Address for new node
* 
* @return Returns pointer to newly allocated node.
*/
dMem *dMemGetNode(int d, unsigned int a)
{
	dMem *p;
	p = (dMem *)malloc(sizeof(dMem));
	assert(p != NULL);
	p->Data = d;
	p->DMA = a + dataSegAddr;
	return p;
}

/** 
* Check for data duplication and add new node to the END(LastNode) of the list.
* * Note: Before adding first node (empty list), FirstNode MUST be NULL.
* 
* @param list Pointer to linked list
* @param d 12-bit Data for new node
* @param a Data Memory Address for new node
* 
* @return Returns pointer to newly allocated node.
*/
extern "C" dMem *dMemListAdd(dMemList *list, int d, unsigned int a)
{
	dMem *p;

	dMem *n = dMemGetNode(d, a);
	if(n == NULL) return NULL;

	if(list->FirstNode != NULL){ /* if FistNode is not NULL, 
										LastNode is also not NULL */

		p = dMemListSearch(list, a);	/* check for duplication */
		if(p == NULL)	/* if not exist, insert new node */
			list->LastNode = dMemListInsertAfter(list->LastNode, n);
		else{	/* if already exists, return found node  */
			free(n);
			return p;
		}
	} else {	/* FirstNode == NULL (list empty) */
		list->LastNode = dMemListInsertBeginning(list, n); 
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
dMem *dMemListInsertAfter(dMem *p, dMem *newNode)	
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
dMem *dMemListInsertBeginning(dMemList *list, dMem *newNode)	
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
void dMemListRemoveAfter(dMemList *list, dMem *p)	
{
	if(p != NULL){
		if(p->Next != NULL){
			dMem *obsoleteNode = p->Next;
			p->Next = p->Next->Next;

			if(obsoleteNode->Next == NULL){	/* if it is LastNode */
				list->LastNode = p;		/* update LastNode */
			}
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
dMem *dMemListRemoveBeginning(dMemList *list)	
{
	if(list != NULL){
		if(list->FirstNode != NULL){
			dMem *obsoleteNode = list->FirstNode;
			list->FirstNode = list->FirstNode->Next;
			if(list->FirstNode == NULL) list->LastNode = NULL; /* removed last node */
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
void dMemListRemoveAll(dMemList *list)
{
	while(list->FirstNode != NULL){
		dMemListRemoveBeginning(list);
	}
}

/** 
* @brief Search entire list matching given data value.
* 
* @param list Pointer to linked list
* @param a Data Memory Address to be matched
* 
* @return Returns pointer to the matched node or NULL.
*/
dMem *dMemListSearch(dMemList *list, unsigned int a)
{
	dMem *n = list->FirstNode;

	while(n != NULL) {
		if(n->DMA == a){	/* match found */
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
void dMemListPrint(dMemList *list)
{
	dMem *n = list->FirstNode;

	if(n == NULL){
		printf("list is empty\n");
		return;
	}

	if(n != NULL){
		printf("format: Node Next DMA Data\n");
	}
	while(n != NULL){
		printf("print %p %p %04X %03X\n", n, n->Next, n->DMA, 0xFFF & (n->Data));
		n = n->Next;
	}
	printf("F %p L %p\n", list->FirstNode, list->LastNode);
}


/** 
* @brief Simple hash function for integer input data.
* 
* @param a Input data (Data Memory Address) 
* 
* @return Returns index to hashing table.
*/
int dMemHashFunction(unsigned int a)
{
	int key;

	key = (int)a + MAX_HASHTABLE;
	return (key % MAX_HASHTABLE);
}

/** 
* @brief Check for data duplication and add new node to hash table. 
* 
* @param htable Hash table data structure
* @param d 12-bit Data for new node
* @param a Data Memory Address for new node
* 
* @return Returns pointer to the newly added node.
*/
dMem *dMemHashAdd(dMemList htable[], int d, unsigned int a)
{
	int hindex;
	hindex = dMemHashFunction(a);	/**< get hashing-by-address index */
	return(dMemListAdd(&htable[hindex], d, a));
}

/** 
* @brief Print all nodes in the hash table.
* 
* @param htable Hash table data structure
*/
void dMemHashPrint(dMemList htable[])
{
	int i;

	printf("\n------------------------------\n");
	printf("** DUMP: Data Memory ** \n");
	printf("------------------------------\n");

	for(i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			printf("\nHash Index: %d\n", i);
			dMemListPrint(&htable[i]);
		}
	}
}

/** 
* @brief Search entire hash table matching given data value.
* 
* @param htable Hash table data structure
* @param a Data Memory Address to be matched
* 
* @return Returns pointer to the matched node or NULL.
*/
dMem *dMemHashSearch(dMemList htable[], unsigned int a)
{
	int hindex;
	hindex = dMemHashFunction(a);
	return(dMemListSearch(&htable[hindex], a));
}

/** 
* @brief Remove node past given node. 
* * Note: Not the given node itself, it's the one right next to that!! 
* 
* @param htable Hash table data structure
* @param p Pointer to the node just before the one to be removed
*/
void dMemHashRemoveAfter(dMemList htable[], dMem *p)
{
	dMem *n; 
	int hindex;

	hindex = dMemHashFunction(p->DMA); /**< get hashing-by-address index */
	n = dMemHashSearch(htable, p->DMA);

	if(n != NULL){
		dMemListRemoveAfter(&htable[hindex], n);
	}
}

/** 
* @brief Remove all nodes in the hash table.
* 
* @param htable Hash table data structure
*/
void dMemHashRemoveAll(dMemList htable[])
{
	int i;

	for(i = 0; i < MAX_HASHTABLE; i++){
		dMemListRemoveAll(&htable[i]);
	}
}


/** 
* @brief Initialize dataMem[]
* 
* @param htable dMemList data structure
*/
void dataMemInit(dMemList htable[])
{
	int i;

	for(i = 0; i < MAX_HASHTABLE; i++){
		htable[i].FirstNode = NULL;
		htable[i].LastNode  = NULL;
	}
}

