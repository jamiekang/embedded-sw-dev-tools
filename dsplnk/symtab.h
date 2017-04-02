/*
All Rights Reserved.
*/
/** 
* @file symtab.h
* Header for sTab linked list and hashing table for symbol table
* Note: this version is for linker(dsplnk) only.
*
* @date 2008-12-17
*/

#ifndef	_SYMTAB_H
#define	_SYMTAB_H

#define	MAX_SYM_LENGTH	80		/**< Maximum symbol name length */

#ifndef MAX_HASHTABLE
#define	MAX_HASHTABLE	199		/**< 29, 31, 37, 101, ... are common prime numbers for hash functions */
#endif

#ifndef	UNDEFINED
#define	UNDEFINED	0xFFFF
#endif

#include "module.h"

#ifndef ESYM
#define ESYM
/** 
* @brief Symbol type (for Linker) 
*/
enum eSym {
	tLOCAL, tGLOBAL, tEXTERN,
};
#endif

#ifndef ESEG
#define ESEG
/**
* @brief Segment type should be either tCODE or tDATA 
*/
enum eSeg {
	    tNONE, tCODE, tDATA,
};
#endif

/** 
* @brief Symbol Table data structure
*/
typedef struct sTab {
	char *Name;					/* symbol name */
	unsigned int	Addr;		/* sequential address in A file */

	int	Type;					/* enum eSym: tLOCAL, tGLOBAL, or tEXTERN */
	int Size;					/* size for data variable (.VAR) */
	int	Seg;					/* enum eSeg: tNONE, tCODE, or tDATA */
	sModule *Module;			/* pointer to module (.obj) linked list */

	struct sTab	*Next;
} sTab;

typedef struct sTabList {
	sTab *FirstNode;		/* Points to first node of list; MUST be NULL when initialized */
	sTab *LastNode;			/* Points to last node of list; MUST be NULL when initialized */
} sTabList;

sTab *sTabGetNode(char *s, unsigned int n, int size, int seg, sModule *m);
sTab *sTabListAdd(sTabList *list, char *s, unsigned int i, int size, int seg, sModule *m);
sTab *sTabListInsertAfter(sTab *p, sTab *newNode);
sTab *sTabListInsertBeginning(sTabList *list, sTab *newNode);
void sTabListRemoveAfter(sTabList *list, sTab *p);
sTab *sTabListRemoveBeginning(sTabList *list);
void sTabListRemoveAll(sTabList *list);
sTab *sTabListSearch(sTabList *list, char *s);
void sTabListPrint(sTabList *list);
void sTabListPrintType(char *s, int type);
void sTabListPrintSeg(char *s, int seg);

int sTabHashFunction(char *s);
sTab *sTabHashSearch(sTabList htable[], char *s);
sTab *sTabHashAdd(sTabList htable[], char *s, unsigned int i, int size, int seg, sModule *m);
void sTabHashPrint(sTabList htable[]);
void sTabHashRemoveAll(sTabList htable[]);
void sTabHashRemoveAfter(sTabList htable[], sTab *p);
void sTabInit(sTabList htable[]);
void globalSymTabBuild(sTabList tablist[], sModuleList *modlist);
void allSymTabBuild(sTabList tablist[], sModuleList *modlist);
void sTabSymDump(sTabList htable[]);
void sTabAllSymDump(sTabList htable[]);
void bubbleSort(sTab **list, int cntr);

extern struct sTabList globalSymTable[MAX_HASHTABLE]; 	/**< Global Symbol Table */
extern struct sTabList allSymTable[MAX_HASHTABLE]; 		/**< All code & data Symbol Table */

#endif	/* _SYMTAB_H */
