/*
All Rights Reserved.
*/
/** 
* @file module.h
* Header for sModule linked list for module list
*
* @date 2008-12-16
*/

#ifndef	_MODULE_H
#define	_MODULE_H

/** 
* @brief Memory space used by DSP is divided into multiple segments.
*/
typedef struct sModule {
	int		ModuleNo;			/* module index */
	char	*Name;				/* module name */

	FILE	*ObjFP;				/* pointer to .obj file */
	FILE	*SymFP;				/* pointer to .sym file */

	int		DataSize;			/* data segment size in words */
	int		DataSegAddr;		/* data segment start address */

	int		CodeSize;			/* code segment size in words */
	int		CodeSegAddr;		/* code segment start address */

	struct sModule	*Next;		/** pointer to next module info */
} sModule;

typedef struct sModuleList {
	sModule *FirstNode;		/* Points to first node of list; MUST be NULL when initialized */
	sModule *LastNode;			/* Points to last node of list; MUST be NULL when initialized */
} sModuleList;

sModule *sModuleGetNode(char *name, int t);
sModule *sModuleListAdd(sModuleList *list, char *name, int t);
sModule *sModuleListInsertAfter(sModule *p, sModule *newNode);
sModule *sModuleListInsertMulti(sModule *p, sModule *newNode);
sModule *sModuleListInsertBeginning(sModuleList *list, sModule *newNode);
void sModuleListRemoveAfter(sModuleList *list, sModule *p);
sModule *sModuleListRemoveBeginning(sModuleList *list);
void sModuleListRemoveAll(sModuleList *list);
sModule *sModuleListSearch(sModuleList *list, char *name);
void sModuleListPrint(sModuleList *list);
void moduleListInit(sModuleList *list);
void moduleListBuild(sModuleList *list);
void sModuleDump(sModuleList *list);

extern struct sModuleList moduleList;       /** Module List */

#endif	/* _MODULE_H */
