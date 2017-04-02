/*
All Rights Reserved.
*/
/** 
* @file optab.h
* Header for oTab linked list and hashing table for opcode table
* - ref: http://faculty.ycp.edu/~dhovemey/spring2007/cs201/info/hashTables.html
* @date 2008-09-24
*/

#ifndef	_OPTAB_H
#define	_OPTAB_H

#define	MAX_OPCODE_LENGTH	25		/**< Maximum opcode length */

#ifndef MAX_OPHASHTABLE
#define	MAX_OPHASHTABLE	199		/**< 29, 31, 37, 101, ... are common prime numbers for hash functions */
#endif

/** 
* @brief Opcode Table data structure
*/
typedef struct oTab {
	char	Name[MAX_OPCODE_LENGTH];	/**< Opcode string */
	unsigned int	Index;				/**< Matching enum eOp value */
	struct oTab	*Next;
} oTab;

typedef struct oTabList {
	oTab *FirstNode;		/* Points to first node of list; MUST be NULL when initialized */
	oTab *LastNode;			/* Points to last node of list; MUST be NULL when initialized */
} oTabList;

oTab *oTabGetNode(char *s, unsigned int n);
#ifdef __cplusplus
/* when called from C++ function */
extern "C" oTab *oTabListAdd(oTabList *list, char *s, unsigned int i);
#else
/* when called from C function */
extern oTab *oTabListAdd(oTabList *list, char *s, unsigned int i);
#endif
oTab *oTabListInsertAfter(oTab *p, oTab *newNode);
oTab *oTabListInsertBeginning(oTabList *list, oTab *newNode);
void oTabListRemoveAfter(oTabList *list, oTab *p);
oTab *oTabListRemoveBeginning(oTabList *list);
void oTabListRemoveAll(oTabList *list);
oTab *oTabListSearch(oTabList *list, char *s);
void oTabListPrint(oTabList *list);

int oTabHashFunction(char *s);
oTab *oTabHashAdd(oTabList htable[], char *s, unsigned int i);
void oTabHashPrint(oTabList htable[]);
#ifdef __cplusplus
/* when called from C++ function */
extern "C" oTab *oTabHashSearch(oTabList htable[], char *s);
#else
/* when called from C function */
extern oTab *oTabHashSearch(oTabList htable[], char *s);
#endif
void oTabHashRemoveAll(oTabList htable[]);
void oTabHashRemoveAfter(oTabList htable[], oTab *p);

void opTableInit(oTabList htable[]);

extern struct oTabList opTable[MAX_OPHASHTABLE]; /**< Opcode Table for iCode's reference */

#endif	/* _OPTAB_H */
