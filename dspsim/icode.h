/*
All Rights Reserved.
*/
/** 
* @file icode.h
* Header for sICode linked list and hashing table for symbol table
* - ref: http://faculty.ycp.edu/~dhovemey/spring2007/cs201/info/hashTables.html
* @date 2008-09-22
*/

#ifndef	_ICODE_H
#define	_ICODE_H

#define	MAX_OPERAND		10
#include "symtab.h"		/* for sTab */
#include "secinfo.h"	/* for sSecInfo */

/** 
* @brief Memory reference type 
*/
enum eMemRef {
	tNON, tABS, tREL,
};

/** 
* @brief After parsing, input assembly/binary program is converted to an 
* intermediate code, sICode. 
*/
typedef struct sICode {
	sSecInfo *Sec;				/**< segment */
	char 	*Line;				/**< one line of asm program */
	unsigned int	LineCntr;	/**< line counter */
	unsigned int	PMA;		/**< program memory address for .CODE segment */
	                            /*   PMA is determined at yyparse()           */
	unsigned int	DMA;		/**< data memory address for .DATA segment */
	                            /*   DMA is determined at codeScan()       */
	unsigned int	Index;		/**< opcode index: eOp value */
	unsigned int	InstType;	/**< opcode type */

	int		MemoryRefType;		/**< if memory reference exits, 
									is it absolute or relative address?
									one of tNON, tABS, tREL */
	int		MemoryRefExtern;	/**< 1 if refers to EXTERN variable */

	char	*Operand[MAX_OPERAND];			/**< operand strings */
	int		OperandCounter;			/**< number of operands */
	char	*Comment;				/**< comment */
	sTab	*Label;					/**< label */
	char	*Cond;					/**< condition code (COND) string */
	int		Conj;					/**< complex conjugate modifier (*) */

	int		MultiCounter;			/**< 1 if two instructions, 2 if three instructions. */
	struct sICode *Multi[2];		/**< pointer to next instruction in multifunction inst. */ 

	//int		Data;				
									/**< PM(Program Memory) data */

	int isDelaySlot;				/**< TRUE if delay slot */
	struct sICode *BrTarget;		/**< pointer to branch target instruction 
										- set by previous branch inst., valid only if delay slot is TRUE */
	int Latency;					/**< latency determined at compile time */
	int LatencyAdded;				/**< latency added at run-time (if necessary) */

	struct sICode *LastExecuted;	/**< pointer to last instruction (for checking adjenct ld/st stall) */
	struct sICode *Next;			/**< pointer to next instruction (in memory) */
} sICode;

typedef struct sICodeList {
	sICode *FirstNode;		/* Points to first node of list; MUST be NULL when initialized */
	sICode *LastNode;			/* Points to last node of list; MUST be NULL when initialized */
} sICodeList;

sICode *sICodeGetNode(unsigned int i, unsigned int a);
#ifdef __cplusplus
/* when called from C++ function */
extern "C" sICode *sICodeListAdd(sICodeList *list, unsigned int s, unsigned int i, int n);
extern "C" sICode *sICodeListMultiAdd(sICodeList *list, unsigned int s, unsigned int i, int n);
extern "C" int isPseudoInst(unsigned int index);
#else
/* when called from C function */
extern sICode *sICodeListAdd(sICodeList *list, unsigned int s, unsigned int i, int n);
extern sICode *sICodeListMultiAdd(sICodeList *list, unsigned int s, unsigned int i, int n);
extern int isPseudoInst(unsigned int index);
#endif
sICode *sICodeListInsertAfter(sICode *p, sICode *newNode);
sICode *sICodeListInsertMulti(sICode *p, sICode *newNode);
sICode *sICodeListInsertBeginning(sICodeList *list, sICode *newNode);
void sICodeListRemoveAfter(sICodeList *list, sICode *p);
sICode *sICodeListRemoveBeginning(sICodeList *list);
void sICodeListRemoveAll(sICodeList *list);
sICode *sICodeListSearch(sICodeList *list, unsigned int s);
void sICodeListPrint(sICodeList *list);
void sICodeListPrintType(char *s, int type);
void sICodeBinDump(sICodeList *list);
void sICodeBinDumpMultiFunc(sICode *p);
void iCodeInit(sICodeList *list);

void bitwrite(sICode *p, char *s, int n, char byteb[], char bitb[], FILE *bFP, FILE *tFP, FILE *mFP);
void bitwriteError(sICode *p, FILE *bFP, FILE *tFP, FILE *mFP);
void packbuf(char byteb[], char bitb[]);
int isCommentLabelInst(unsigned int index);
int isNotRealInst(unsigned int index);
sICode *getNextCode(sICode *p);

extern struct sICodeList iCode;       /**< Intermediate Code List */

#endif	/* _ICODE_H */
