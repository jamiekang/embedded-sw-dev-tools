/*
All Rights Reserved.
*/
/** 
* @file dmem.h
* Header for dMem linked list and hashing table for symbol table
* - ref: http://faculty.ycp.edu/~dhovemey/spring2007/cs201/info/hashTables.html
* @date 2008-09-22
*/

#ifndef _DMEM_H
#define	_DMEM_H

#ifndef	MAX_HASHTABLE
#define	MAX_HASHTABLE	199		/**< 29, 31, 37, 101, ... are common prime numbers for hash functions */
#endif

#include "simsupport.h"

/** 
* @brief Symbol Table data structure
*/
typedef struct dMem {
	unsigned int	DMA;
	sint	Data;
	struct dMem	*Next;
} dMem;

typedef struct dMemList {
	dMem *FirstNode;		/* Points to first node of list; MUST be NULL when initialized */
	dMem *LastNode;			/* Points to last node of list; MUST be NULL when initialized */
} dMemList;

dMem *dMemGetNode(sint d, unsigned int a);
#ifdef __cplusplus
/* when called from C++ function */
extern "C" dMem *dMemListAdd(dMemList *list, sint d, unsigned int a);
#else
/* when called from C function */
extern dMem *dMemListAdd(dMemList *list, sint d, unsigned int a);
#endif
dMem *dMemListInsertAfter(dMem *p, dMem *newNode);
dMem *dMemListInsertBeginning(dMemList *list, dMem *newNode);
void dMemListRemoveAfter(dMemList *list, dMem *p);
dMem *dMemListRemoveBeginning(dMemList *list);
void dMemListRemoveAll(dMemList *list);
dMem *dMemListSearch(dMemList *list, unsigned int a);
void dMemListPrint(dMemList *list);

int dMemHashFunction(unsigned int a);
dMem *dMemHashAdd(dMemList htable[], sint d, unsigned int a);
void dMemHashPrint(dMemList htable[]);
dMem *dMemHashSearch(dMemList htable[], unsigned int a);
void dMemHashRemoveAll(dMemList htable[]);
void dMemHashRemoveAfter(dMemList htable[], dMem *p);
void dataMemInit(dMemList htable[]);

extern struct dMemList dataMem[MAX_HASHTABLE]; /**< Data Memory to store 12-bit data */

#endif /* _DMEM_H */
