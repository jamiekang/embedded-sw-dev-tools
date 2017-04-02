/*
All Rights Reserved.
*/
/** 
* @file optab.cc
* Opcode table (oTab) linked list and hashing table implementation
*
* Reference 1: http://faculty.ycp.edu/~dhovemey/spring2007/cs201/info/hashTables.html
*
* Reference 2: http://en.wikipedia.org/wiki/Linked_list
* @date 2008-09-24
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "optab.h"
#include "dspdef.h"

/** 
* @brief Allocate a node dynamically.
* 
* @param s Opcode name for new node
* @param n Opcode index for new node
* 
* @return Returns pointer to newly allocated node.
*/
oTab *oTabGetNode(char *s, unsigned int n)
{
	oTab *p;
	p = (oTab *)malloc(sizeof(oTab));
	assert(p != NULL);
	strcpy(p->Name, s);
	p->Index = n;
	return p;
}

/** 
* Check for data duplication and add new node to the END(LastNode) of the list.
* * Note: Before adding first node (empty list), FirstNode MUST be NULL.
* 
* @param list Pointer to linked list
* @param s Opcode name for new node
* @param i Opcode index for new node
* 
* @return Returns pointer to newly allocated node.
*/
extern "C" oTab *oTabListAdd(oTabList *list, char *s, unsigned int i)
{
	oTab *p;

	if(s == NULL) return NULL;

	oTab *n = oTabGetNode(s, i);
	if(n == NULL) return NULL;

	if(list->FirstNode != NULL){ /* if FistNode is not NULL, 
										LastNode is also not NULL */

		p = oTabListSearch(list, s);	/* check for duplication */
		if(p == NULL)	/* if not duplicated */
			list->LastNode = oTabListInsertAfter(list->LastNode, n);
		else{	/* if duplicated */
			free(n);
			return p;
		}
	} else {	/* FirstNode == NULL (list empty) */
		list->LastNode = oTabListInsertBeginning(list, n); 
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
oTab *oTabListInsertAfter(oTab *p, oTab *newNode)	
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
oTab *oTabListInsertBeginning(oTabList *list, oTab *newNode)	
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
void oTabListRemoveAfter(oTabList *list, oTab *p)	
{
	if(p != NULL){
		if(p->Next != NULL){
			oTab *obsoleteNode = p->Next;
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
oTab *oTabListRemoveBeginning(oTabList *list)	
{
	if(list != NULL){
		if(list->FirstNode != NULL){
			oTab *obsoleteNode = list->FirstNode;
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
void oTabListRemoveAll(oTabList *list)
{
	while(list->FirstNode != NULL){
		oTabListRemoveBeginning(list);
	}
}

/** 
* @brief Search entire list matching given data value.
* 
* @param list Pointer to linked list
* @param s Opcode name to be matched
* 
* @return Returns pointer to the matched node or NULL.
*/
oTab *oTabListSearch(oTabList *list, char *s)
{
	oTab *n = list->FirstNode;

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
void oTabListPrint(oTabList *list)
{
	oTab *n = list->FirstNode;

	if(n == NULL){
		printf("list is empty\n");
		return;
	}

	if(n != NULL){
		printf("format: Node Next Opcode Index\n");
	}
	while(n != NULL){
		printf("print %p %p %11s %d\n", n, n->Next, n->Name, n->Index);
		n = n->Next;
	}
	printf("F %p L %p\n", list->FirstNode, list->LastNode);
}


/** 
* @brief Simple hash function for integer input data.
* 
* @param s Input name (string)
* 
* @return Returns index to hashing table.
*/
int oTabHashFunction(char *s)
{
	int i, key, s_i;
	int len;

	len = strlen(s);

	key = (int)s[0];
	if(key >= 'a' && key <= 'z') {	key -= ('a' - 'A'); }

	for(i = 1; i < len; i++){
		s_i = (int)s[i];
		if(s_i >= 'a' && s_i <= 'z') {	s_i -= ('a' - 'A'); }

		key = (key) + s_i;
	}

	return (key % MAX_OPHASHTABLE);
}

/** 
* @brief Check for data duplication and add new node to hash table. 
* 
* @param htable Hash table data structure
* @param s Opcode name for newly added node
* @param i Opcode index for newly added node
* 
* @return Returns pointer to the newly added node.
*/
oTab *oTabHashAdd(oTabList htable[], char *s, unsigned int i)
{
	int hindex;
	hindex = oTabHashFunction(s);
	return(oTabListAdd(&htable[hindex], s, i));
}

/** 
* @brief Print all nodes in the hash table.
* 
* @param htable Hash table data structure
*/
void oTabHashPrint(oTabList htable[])
{
	int i;

	printf("\n------------------------------\n");
	printf("** DUMP: Opcode Table ** \n");
	printf("------------------------------\n");

	for(i = 0; i < MAX_OPHASHTABLE; i++){
		printf("\nHash Index: %d\n", i);
		oTabListPrint(&htable[i]);
	}
}

/** 
* @brief Search entire hash table matching given data value.
* 
* @param htable Hash table data structure
* @param s Opcode name to be matched
* 
* @return Returns pointer to the matched node or NULL.
*/
oTab *oTabHashSearch(oTabList htable[], char *s)
{
	int hindex;
	hindex = oTabHashFunction(s);
	return(oTabListSearch(&htable[hindex], s));
}

/** 
* @brief Remove node past given node. 
* * Note: Not the given node itself, it's the one right next to that!! 
* 
* @param htable Hash table data structure
* @param p Pointer to the node just before the one to be removed
*/
void oTabHashRemoveAfter(oTabList htable[], oTab *p)
{
	oTab *n; 
	int hindex;

	hindex = oTabHashFunction(p->Name);
	n = oTabHashSearch(htable, p->Name);

	if(n != NULL){
		oTabListRemoveAfter(&htable[hindex], n);
	}
}

/** 
* @brief Remove all nodes in the hash table.
* 
* @param htable Hash table data structure
*/
void oTabHashRemoveAll(oTabList htable[])
{
	int i;

	for(i = 0; i < MAX_OPHASHTABLE; i++){
		oTabListRemoveAll(&htable[i]);
	}
}


/** 
* @brief Initialize opTable[]
* 
* @param htable Hash table data structure
*/
void opTableInit(oTabList htable[])
{
	int i;

	for(i = 0; i < MAX_OPHASHTABLE; i++){
		htable[i].FirstNode = NULL;
		htable[i].LastNode = NULL;
	}

	for(i = 1; sOp[i] != NULL; i++){
		oTabHashAdd(htable, (char *)sOp[i], i);
	}
}
