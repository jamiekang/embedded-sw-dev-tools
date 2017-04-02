/*
All Rights Reserved.
*/
/** 
* @file extreftab.h
* Header for sExtRefTab linked list and hashing table for symbol table
* Note: this version is for linker(dsplnk) only.
*
* @date 2008-12-17
*/

#ifndef	_EXTREF_H
#define	_EXTREF_H

#ifndef MAX_HASHTABLE
#define	MAX_HASHTABLE	199		/**< 29, 31, 37, 101, ... are common prime numbers for hash functions */
#endif

#ifndef	UNDEFINED
#define	UNDEFINED	0xFFFF
#endif

#include "module.h"
#include "symtab.h"

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
typedef struct sExtRefTab {
	char *Name;					/* symbol name */
	unsigned int	Addr;		/* address */

	int Seg;                    /* enum eSeg: tNONE, tCODE, or tDATA */
	sModule *Module;			/* pointer to module (.obj) linked list */

	struct sExtRefTab	*Next;
} sExtRefTab;

typedef struct sExtRefTabList {
	sExtRefTab *FirstNode;		/* Points to first node of list; MUST be NULL when initialized */
	sExtRefTab *LastNode;			/* Points to last node of list; MUST be NULL when initialized */
} sExtRefTabList;

sExtRefTab *sExtRefTabGetNode(char *s, unsigned int n, int seg, sModule *m);
sExtRefTab *sExtRefTabListAdd(sExtRefTabList *list, char *s, unsigned int i, int seg, sModule *m);
sExtRefTab *sExtRefTabListInsertAfter(sExtRefTab *p, sExtRefTab *newNode);
sExtRefTab *sExtRefTabListInsertBeginning(sExtRefTabList *list, sExtRefTab *newNode);
void sExtRefTabListRemoveAfter(sExtRefTabList *list, sExtRefTab *p);
sExtRefTab *sExtRefTabListRemoveBeginning(sExtRefTabList *list);
void sExtRefTabListRemoveAll(sExtRefTabList *list);
sExtRefTab *sExtRefTabListSearch(sExtRefTabList *list, unsigned int i, int seg);
void sExtRefTabListPrint(sExtRefTabList *list);
void sExtRefTabListPrintSeg(char *s, int seg);

int sExtRefTabHashFunction(unsigned int i, int seg);
sExtRefTab *sExtRefTabHashSearch(sExtRefTabList htable[], unsigned int i, int seg);
sExtRefTab *sExtRefTabHashAdd(sExtRefTabList htable[], char *s, unsigned int i, int seg, sModule *m);
void sExtRefTabHashPrint(sExtRefTabList htable[]);
void sExtRefTabHashRemoveAll(sExtRefTabList htable[]);
void sExtRefTabHashRemoveAfter(sExtRefTabList htable[], sExtRefTab *p);
void sExtRefTabInit(sExtRefTabList htable[]);
void externRefTabBuild(sExtRefTabList tabList[], sTabList stabList[], sModuleList *modlist);
void sExtRefTabDump(sExtRefTabList htable[]);

extern struct sExtRefTabList sExtRefTable[MAX_HASHTABLE]; /**< External Symbol Reference Table */

#endif	/* _EXTREF_H */
