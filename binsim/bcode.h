/*
All Rights Reserved.
*/
/** 
* @file bcode.h
* Header for sBCode linked list
* @date 2009-02-04
*/

#ifndef	_BCODE_H
#define	_BCODE_H

#include "binsim.h"	/* for INSTLEN */
#include "dspdef.h"	/* for MAX_FIELD */

/** 
* @brief After parsing, input binary program is converted to an 
* binary code, sBCode. 
*/
typedef struct sBCode {
	char 	Binary[INSTLEN+1];				/**< one line of asm program */
	unsigned int	PMA;		/**< program memory address for .CODE segment */
	                            /*   PMA is determined at yyparse()           */
	unsigned int	InstType;	/**< opcode type */
	unsigned int	Index;		/**< opcode index: eOp value */

	int		Field[MAX_FIELD];			/**< field strings */
	int		FieldCounter;			/**< number of fields */

	//sSecInfo *Sec;				/**< segment */
	int		Cond;				/**< condition code (COND) */
	int		Conj;				/**< complex conjugate modifier (*) */
	int		MultiCounter;		/**< 1 if two instructions, 2 if three instructions */
	unsigned int IndexMulti[2];		/** array of other instruction indices 
									in multifunction inst. */
	int isDelaySlot;				/**< TRUE if delay slot */
	struct sBCode *BrTarget;		/**< pointer to branch target instruction 
										- set by previous branch inst., valid only if delay slot is TRUE */
	int Latency;					/**< latency */

	struct sBCode *Next;	/**< pointer to next instruction */
} sBCode;

typedef struct sBCodeList {
	sBCode *FirstNode;		/* Points to first node of list; MUST be NULL when initialized */
	sBCode *LastNode;			/* Points to last node of list; MUST be NULL when initialized */
} sBCodeList;

sBCode *sBCodeGetNode(char tdata[], unsigned int a);
sBCode *sBCodeListAdd(sBCodeList *list, char tdata[], unsigned int a);
sBCode *sBCodeListInsertAfter(sBCode *p, sBCode *newNode);
sBCode *sBCodeListInsertBeginning(sBCodeList *list, sBCode *newNode);
void sBCodeListRemoveAfter(sBCodeList *list, sBCode *p);
sBCode *sBCodeListRemoveBeginning(sBCodeList *list);
void sBCodeListRemoveAll(sBCodeList *list);
sBCode *sBCodeListSearch(sBCodeList *list, unsigned int s);
void sBCodeListPrint(sBCodeList *list);
void bCodeInit(sBCodeList *list);

sBCode *getNextCode(sBCode *p);

extern struct sBCodeList bCode;       /**< Binary Code List */

#endif	/* _BCODE_H */
