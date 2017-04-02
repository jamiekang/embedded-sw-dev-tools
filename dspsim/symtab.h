/*
All Rights Reserved.
*/
/** 
* @file symtab.h
* Header for sTab linked list and hashing table for symbol table
* - ref: http://faculty.ycp.edu/~dhovemey/spring2007/cs201/info/hashTables.html
* @date 2008-09-22
*/

#ifndef	_SYMTAB_H
#define	_SYMTAB_H

#include "secinfo.h"	/* for sSecInfo */
#include "memref.h"		/* for sMemRef */
#include "dspdef.h"		/* for sResSym */

#define	MAX_SYM_LENGTH	80		/**< Maximum symbol name length */

#ifndef MAX_HASHTABLE
#define	MAX_HASHTABLE	199		/**< 29, 31, 37, 101, ... are common prime numbers for hash functions */
#endif

#ifndef	UNDEFINED
#define	UNDEFINED	0xFFFF
#endif

/** 
* @brief Symbol type (for Linker) 
*/
enum eSym {
	tLOCAL, tGLOBAL, tEXTERN,
};

/** 
* @brief Table ID (for .sym output)
*/
enum eTabID {
	tSecInfo, tGlobal, tExtern, tSymbol, tEnd = 9,
};

/** 
* @brief Symbol Table data structure
*/
typedef struct sTab {
	char	Name[MAX_SYM_LENGTH];
	unsigned int	Addr;		/* sequential address in A file */
	struct sTab	*Next;
	int	Type;					/* enum eSym: tLOCAL, tGLOBAL, or tEXTERN */
	int Size;					/* size for data variable (.VAR) */
	int Defined;				/* for LABEL: if it's defined */
	int	isConst;				/* for EQU constants: to indicate it's alway ABSOLUTE */
	sSecInfo *Sec;				/* pointer to segment info linked list */
	sMemRefList	MemRefList;			/* pointer to memory reference linked list */
} sTab;

typedef struct sTabList {
	sTab *FirstNode;		/* Points to first node of list; MUST be NULL when initialized */
	sTab *LastNode;			/* Points to last node of list; MUST be NULL when initialized */
} sTabList;

sTab *sTabGetNode(char *s, unsigned int n);
sTab *sTabListAdd(sTabList *list, char *s, unsigned int i);
sTab *sTabListInsertAfter(sTab *p, sTab *newNode);
sTab *sTabListInsertBeginning(sTabList *list, sTab *newNode);
void sTabListRemoveAfter(sTabList *list, sTab *p);
sTab *sTabListRemoveBeginning(sTabList *list);
void sTabListRemoveAll(sTabList *list);
sTab *sTabListSearch(sTabList *list, char *s);
void sTabListPrint(sTabList *list);
void sTabListPrintType(char *s, int type);

int sTabHashFunction(char *s);
#ifdef __cplusplus
/* when called from C++ function */
extern "C" sTab *sTabHashAdd(sTabList htable[], char *s, unsigned int i);
extern "C" sTab *sTabHashSearch(sTabList htable[], char *s);
#else
/* when called from C function */
extern sTab *sTabHashAdd(sTabList htable[], char *s, unsigned int i);
extern sTab *sTabHashSearch(sTabList htable[], char *s);
#endif
void sTabHashPrint(sTabList htable[]);
void sTabHashRemoveAll(sTabList htable[]);
void sTabHashRemoveAfter(sTabList htable[], sTab *p);
void symTableInit(sTabList htable[]);
void resSymTableInit(sTabList htable[]);
void sTabSymDump(sTabList htable[]);
void sTabExternSymRefDump(sTabList htable[]);
char *sTabSymNameSearchByReferredAddr(sTabList htable[], int addr);
void bubbleSort(sTab **list, int cntr);
void sTabSymEvenBytePatch(sTabList htable[], sSecInfoList *list, char *s);

extern struct sTabList symTable[MAX_HASHTABLE]; 	/**< Symbol Table to store labels */
extern struct sTabList resSymTable[MAX_HASHTABLE]; 	/**< Symbol Table to store reserved keywords */

#endif	/* _SYMTAB_H */
