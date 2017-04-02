/*
All Rights Reserved.
*/
/** 
* @file memref.h
* Header for sMemRef linked list to track memory reference of a symbol
*
* @date 2008-12-08
*/

#ifndef	_MEMREF_H
#define	_MEMREF_H

/** 
* @brief Record every memory reference of a given symbol
*/
typedef struct sMemRef {
	int		Addr;				/** program memory address where this memory location is read/write */
	struct sMemRef	*Next;		/** pointer to next node */
} sMemRef;

typedef struct sMemRefList {
	sMemRef *FirstNode;			/* Points to first node of list; MUST be NULL when initialized */
	sMemRef *LastNode;			/* Points to last node of list; MUST be NULL when initialized */
} sMemRefList;

sMemRef *sMemRefGetNode(int addr);
#ifdef __cplusplus
/* when called from C++ function */
extern "C" sMemRef *sMemRefListAdd(sMemRefList *list, int addr);
#else
/* when called from C function */
extern sMemRef *sMemRefListAdd(sMemRefList *list, int addr);
#endif
sMemRef *sMemRefListInsertAfter(sMemRef *p, sMemRef *newNode);
sMemRef *sMemRefListInsertMulti(sMemRef *p, sMemRef *newNode);
sMemRef *sMemRefListInsertBeginning(sMemRefList *list, sMemRef *newNode);
void sMemRefListRemoveAfter(sMemRefList *list, sMemRef *p);
sMemRef *sMemRefListRemoveBeginning(sMemRefList *list);
void sMemRefListRemoveAll(sMemRefList *list);
sMemRef *sMemRefListSearch(sMemRefList *list, int addr);
void memRefInit(sMemRefList *list);

#endif	/* _MEMREF_H */
