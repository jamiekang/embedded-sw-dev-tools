/*
All Rights Reserved.
*/
/** 
* @file secinfo.h
* Header for sSecInfo linked list for segment(section) information list
*
* @date 2008-12-01
*/

#ifndef	_SECINFO_H
#define	_SECINFO_H

/** 
* @brief Segment type should be either tCODE or tDATA if not external variable
*/
enum eSec {
	tNONE, tCODE, tDATA,
};

/** 
* @brief Memory space used by DSP is divided into multiple segments.
*/
typedef struct sSecInfo {
	char	*Name;				/** section name */
	int		Size;				/** section size in words */
	int		Type;				/** enum eSec value */
	//unsigned int	SecNo;		/** section index */
	//unsigned int	Addr;		/** address */
	int		isOdd;				/** for data segment, true if size is odd number */
	struct sSecInfo	*Next;		/** pointer to next section info */
} sSecInfo;

typedef struct sSecInfoList {
	sSecInfo *FirstNode;		/* Points to first node of list; MUST be NULL when initialized */
	sSecInfo *LastNode;			/* Points to last node of list; MUST be NULL when initialized */
} sSecInfoList;

sSecInfo *sSecInfoGetNode(char *name, int t);
#ifdef __cplusplus
/* when called from C++ function */
extern "C" sSecInfo *sSecInfoListAdd(sSecInfoList *list, char *name, int t);
#else
/* when called from C function */
extern sSecInfo *sSecInfoListAdd(sSecInfoList *list, char *name, int t);
#endif
sSecInfo *sSecInfoListInsertAfter(sSecInfo *p, sSecInfo *newNode);
sSecInfo *sSecInfoListInsertMulti(sSecInfo *p, sSecInfo *newNode);
sSecInfo *sSecInfoListInsertBeginning(sSecInfoList *list, sSecInfo *newNode);
void sSecInfoListRemoveAfter(sSecInfoList *list, sSecInfo *p);
sSecInfo *sSecInfoListRemoveBeginning(sSecInfoList *list);
void sSecInfoListRemoveAll(sSecInfoList *list);
sSecInfo *sSecInfoListSearch(sSecInfoList *list, char *name);
void sSecInfoListPrint(sSecInfoList *list);
void sSecInfoListPrintType(char *s, int type);
void secInfoInit(sSecInfoList *list);
void sSecInfoSymDump(sSecInfoList *list);

extern struct sSecInfoList secInfo;       /** Section Information List */
extern struct sSecInfo *curSecInfo;		  /** pointer to current segment info. */

#endif	/* _SECINFO_H */
