/*
All Rights Reserved.
*/
/** 
* @file ocode.h
* Header for sOCode linked list 
* @date 2008-12-18
*/

#ifndef	_OCODE_H
#define	_OCODE_H

#include "extreftab.h"	/* for sExtRefTab */
#include "symtab.h"		/* for sTab */
#include "module.h"		/* for sModule */

/** 
* @brief Memory reference type 
*/
enum eMemRef {
	tNON, tABS, tREL,
};

/** 
* @brief After parsing, input assembly/binary program is converted to an 
* intermediate code, sOCode. 
*/
typedef struct sOCode {
	unsigned int	PMA;		/**< program memory address for .CODE segment */
	//unsigned int	DMA;		/**< data memory address for .DATA segment */
	unsigned int	LineCntr;	/**< line counter */
	unsigned int	InstType;	/**< opcode type */
	sModule	*Module;			/**< pointer to the module this code belongs to */

	//char 	*Line;				/**< one line of asm program */

	char BinCode[INSTLEN+1];			/**< opcode in binary string format */

	int	MemoryRefType;			/**< if memory reference exits, 
									is it absolute or relative address?
									one of tNON, tABS, tREL */
	int	Offset;					/**< Offset to be used for relocating */
	int Pos;					/**< Bit position of offset */
	int Width;					/**< Bit width of offset */
	int IsPatched;				/**< Binary flag to indicate whether relocated or not */

	struct sOCode *Next;	/**< pointer to next instruction */

	//int		MemoryRefExtern;	/**< 1 if refers to EXTERN variable */
	//unsigned int	Index;		/**< opcode index: eOp value */
	//char	*Operand[MAX_OPERAND];			/**< operand strings */
	//int		OperandCounter;			/**< number of operands */

} sOCode;

typedef struct sOCodeList {
	sOCode *FirstNode;		/* Points to first node of list; MUST be NULL when initialized */
	sOCode *LastNode;			/* Points to last node of list; MUST be NULL when initialized */
} sOCodeList;

sOCode *sOCodeGetNode(unsigned int a, int ln, sModule *n);
sOCode *sOCodeListAdd(sOCodeList *list, unsigned int s, int ln, sModule *n);
sOCode *sOCodeListInsertAfter(sOCode *p, sOCode *newNode);
sOCode *sOCodeListInsertBeginning(sOCodeList *list, sOCode *newNode);
void sOCodeListRemoveAfter(sOCodeList *list, sOCode *p);
sOCode *sOCodeListRemoveBeginning(sOCodeList *list);
void sOCodeListRemoveAll(sOCodeList *list);
sOCode *sOCodeListSearch(sOCodeList *list, unsigned int s);
void sOCodeListPrint(sOCodeList *list);
void sOCodeListPrintType(char *s, int type);
void sOCodeBinDump(sOCodeList *list);
void oCodeInit(sOCodeList *list);
void globalOCodeListBuild(sOCodeList *codelist, sModuleList *modlist);

void bitwrite(sOCode *p, char *s, int n, char byteb[], char bitb[], FILE *bFP, FILE *tFP);
void bitwriteError(sOCode *p, FILE *bFP, FILE *tFP);
void packbuf(char byteb[], char bitb[]);
void patchExternRef(sTabList tablist[], sExtRefTabList extlist[], sOCodeList *codelist, sModuleList *modlist);
void patchOtherRef(sTabList tablist[], sExtRefTabList extlist[], sOCodeList *codelist, sModuleList *modlist);
void patchOCodeExternRef(sOCode *n, sTab *sp);
void patchOCodeOtherRef(sOCode *n);
void int2bin(char *ret, int val, int n);

extern struct sOCodeList globalObjCode;       /**< Object Code List */

#endif	/* _OCODE_H */
