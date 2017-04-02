/*
All Rights Reserved.
*/
/** 
* @file symtab.cc
* Symbol table (sTab) linked list and hashing table implementation
*
* Reference 1: http://faculty.ycp.edu/~dhovemey/spring2007/cs201/info/hashTables.html
*
* Reference 2: http://en.wikipedia.org/wiki/Linked_list
* @date 2008-09-22
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "symtab.h"
#include "memref.h"
#include "dspsim.h"
#include "dmem.h"

/** 
* @brief Allocate a node dynamically.
* 
* @param s Symbol name for new node
* @param n Symbol address for new node
* 
* @return Returns pointer to newly allocated node.
*/
sTab *sTabGetNode(char *s, unsigned int n)
{
	sTab *p;
	p = (sTab *)malloc(sizeof(sTab));
	assert(p != NULL);
	strcpy(p->Name, s);
	p->Addr = n;

	memRefInit(&(p->MemRefList));	/* init memory reference list */
	return p;
}

/** 
* Check for data duplication and add new node to the END(LastNode) of the list.
* * Note: Before adding first node (empty list), FirstNode MUST be NULL.
* 
* @param list Pointer to linked list
* @param s Symbol name for new node
* @param i Symbol address for new node
* 
* @return Returns pointer to newly allocated node.
*/
sTab *sTabListAdd(sTabList *list, char *s, unsigned int i)
{
	sTab *p;

	if(s == NULL) return NULL;

	sTab *n = sTabGetNode(s, i);
	if(n == NULL) return NULL;

	if(list->FirstNode != NULL){ /* if FistNode is not NULL, 
										LastNode is also not NULL */

		p = sTabListSearch(list, s);	/* check for duplication */
		if(p == NULL)	/* if not duplicated */
			list->LastNode = sTabListInsertAfter(list->LastNode, n);
		else{	/* if duplicated */
			if(p->Addr = UNDEFINED)
				p->Addr = n->Addr;	/* update addr */
			free(n);	/* no longer needed */ 
			return p;	/* return registered node */
		}
	} else {	/* FirstNode == NULL (list empty) */
		list->LastNode = sTabListInsertBeginning(list, n); 
	}

	return n;	/* return new node (= LastNode) */
}

/** 
* Insert new node after 'previous' node - No data duplication check.
* * Note: LastNode should be updated outside. 
* 
* @param p Pointer to 'previous' node
* @param newNode Pointer to new node to be added
* 
* @return Returns pointer to new node.
*/
sTab *sTabListInsertAfter(sTab *p, sTab *newNode)	
{
	newNode->Next = p->Next;
	p->Next = newNode;

	return newNode;	/* return new node */
}

/** 
* Insert new node before the FIRST node - No data duplication check.
* * Note: LastNode should be updated outside if FirstNode was NULL.
* 
* @param list Pointer to linked list
* @param newNode Pointer to new node to be inserted
* 
* @return Returns pointer to new node.
*/
sTab *sTabListInsertBeginning(sTabList *list, sTab *newNode)	
{
	newNode->Next = list->FirstNode;
	list->FirstNode = newNode;

	return newNode;	/* return new node (= first node) */
}

/** 
* @brief Remove node past given node. 
* * Note: Not the given node itself, it's the one right next to that!! 
* 
* @param list Pointer to linked list
 @param p Pointer to the node just before the one to be removed
*/
void sTabListRemoveAfter(sTabList *list, sTab *p)	
{
	if(p != NULL){
		if(p->Next != NULL){
			sTab *obsoleteNode = p->Next;
			p->Next = p->Next->Next;

			if(obsoleteNode->Next == NULL){	/* if it is LastNode */
				list->LastNode = p;		/* update LastNode */
			}

			sMemRefListRemoveAll(&(obsoleteNode->MemRefList));
			free(obsoleteNode);
		}
	}
}

/** 
* @brief Remove FIRST node of the list.
* 
* @param list Pointer to linked list
* 
* @return Returns updated FirstNode 
*/
sTab *sTabListRemoveBeginning(sTabList *list)	
{
	if(list != NULL){
		if(list->FirstNode != NULL){
			sTab *obsoleteNode = list->FirstNode;
			list->FirstNode = list->FirstNode->Next;
			if(list->FirstNode == NULL) list->LastNode = NULL; /* removed last node */

			sMemRefListRemoveAll(&(obsoleteNode->MemRefList));
			free(obsoleteNode);
			return list->FirstNode;	/* return new FirstNode */
		} else {
			return NULL;
		}
	}
}

/** 
* @brief Remove all nodes in the list.
* 
* @param list Pointer to linked list
*/
void sTabListRemoveAll(sTabList *list)
{
	while(list->FirstNode != NULL){
		sTabListRemoveBeginning(list);
	}
}

/** 
* @brief Search entire list matching given data value.
* 
* @param list Pointer to linked list
* @param s symbol name to be matched
* 
* @return Returns pointer to the matched node or NULL.
*/
sTab *sTabListSearch(sTabList *list, char *s)
{
	sTab *n = list->FirstNode;

	while(n != NULL) {
		if(!strcasecmp(n->Name, s)){	/* match found */
			return n;
		}
		n = n->Next;
	}

	/* match not found */
	return NULL;
}

/** 
* @brief Print all nodes in the linked list.
* 
* @param list Pointer to linked list
*/
void sTabListPrint(sTabList *list)
{
	sTab *n = list->FirstNode;
	char stype[10];

	if(n == NULL){
		printf("list is empty\n");
		return;
	}

	if(n != NULL){
		printf("format: Node Next Addr Name Type   Seg.\n");
	}

	while(n != NULL){
		sTabListPrintType(stype, n->Type);

		if(n->Sec){
			printf("print %p %p %04X %s %s %s\n", n, n->Next, n->Addr, 
				n->Name, stype, n->Sec->Name);
		}else{
			printf("print %p %p %04X %s %s ----\n", n, n->Next, n->Addr, 
				n->Name, stype);
		}
		n = n->Next;
	}
	printf("F %p L %p\n", list->FirstNode, list->LastNode);
}

/** 
* @brief Get string for type ("LOCAL ", "GLOBAL", or "EXTERN")
* 
* @param s Pointer to output string buffer
* @param type Type of current symbol table node
*/
void sTabListPrintType(char *s, int type)
{
	switch(type){
		case	tLOCAL:
			strcpy(s, "LOCAL");
			break;
		case	tGLOBAL:
			strcpy(s, "GLOBAL");
			break;
		case	tEXTERN:
			strcpy(s, "EXTERN");
			break;
		default:
			strcpy(s, "------");
			break;
	}
}

/** 
* @brief Simple hash function for integer input data.
* 
* @param s Input string 
* 
* @return Returns index to hashing table.
*/
int sTabHashFunction(char *s)
{
	int i, key;
	int len;

	len = strlen(s);

	key = (int)s[0];
	for(i = 1; i < len; i++){
		key = key ^ (int)s[i];
	}

	return (key % MAX_HASHTABLE);
}

/** 
* @brief Check for data duplication and add new node to hash table. 
* 
* @param htable Hash table data structure
* @param s Symbol name for newly added node
* @param i Symbol address for newly added node
* 
* @return Returns pointer to the newly added node.
*/
extern "C" sTab *sTabHashAdd(sTabList htable[], char *s, unsigned int i)
{
	int hindex;
	hindex = sTabHashFunction(s);
	if(VerboseMode) printf("> Hash Index: %d\n", hindex); 
	return(sTabListAdd(&htable[hindex], s, i));
}

/** 
* @brief Print all nodes in the hash table.
* 
* @param htable Hash table data structure
*/
void sTabHashPrint(sTabList htable[])
{
	int i;

	printf("\n------------------------------\n");
	printf("** DUMP: Symbol Table ** \n");
	printf("------------------------------\n");

	for(i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			printf("\nHash Index: %d\n", i);
			sTabListPrint(&htable[i]);
		}
	}
}

/** 
* @brief Search entire hash table matching given data value.
* 
* @param htable Hash table data structure
* @param s symbol name to be matched
* 
* @return Returns pointer to the matched node or NULL.
*/
extern "C" sTab *sTabHashSearch(sTabList htable[], char *s)
{
	int hindex;
	hindex = sTabHashFunction(s);
	return(sTabListSearch(&htable[hindex], s));
}

/** 
* @brief Remove node past given node. 
* * Note: Not the given node itself, it's the one right next to that!! 
* 
* @param htable Hash table data structure
* @param p Pointer to the node just before the one to be removed
*/
void sTabHashRemoveAfter(sTabList htable[], sTab *p)
{
	sTab *n; 
	int hindex;

	hindex = sTabHashFunction(p->Name);
	n = sTabHashSearch(htable, p->Name);

	if(n != NULL){
		sTabListRemoveAfter(&htable[hindex], n);
	}
}

/** 
* @brief Remove all nodes in the hash table.
* 
* @param htable Hash table data structure
*/
void sTabHashRemoveAll(sTabList htable[])
{
	int i;

	for(i = 0; i < MAX_HASHTABLE; i++){
		sTabListRemoveAll(&htable[i]);
	}
}


/** 
* @brief Initialize symTable[]
* 
* @param htable sTabList data structure 
*/
void symTableInit(sTabList htable[])
{
	int i;

	for(i = 0; i < MAX_HASHTABLE; i++){
		htable[i].FirstNode = NULL;
		htable[i].LastNode  = NULL;
	}
}


/** 
* @brief Initialize resSymTable[] - to contain reserved keywords
* 
* @param htable sTabList data structure 
*/
void resSymTableInit(sTabList htable[])
{
	int i;

	for(i = 0; i < MAX_HASHTABLE; i++){
		htable[i].FirstNode = NULL;
		htable[i].LastNode  = NULL;
	}

	/* list-up reserved keywords */
	/* purpose: e.g., "CNTR" must not be used instead of "_CNTR". */
	i = 0;
	while(sResSym[i] != NULL){
		sTabHashAdd(htable, (char *)sResSym[i++], UNDEFINED);
	}
}


/** 
* @brief Generate symbol table (.sym) and source listing (.lst) file for linker
* 
* @param htable Symbol table data structure
*/
void sTabSymDump(sTabList htable[])
{
	char stype[10];
	int tabID;

	/* dump global symbol definition table to .sym file */
	fprintf(dumpSymFP, ";-----------------------------------------------------------\n");
	fprintf(dumpSymFP, "; Global Symbol Definition Table: CODE & DATA Segments\n");
	fprintf(dumpSymFP, ";-----------------------------------------------------------\n");
	fprintf(dumpSymFP, "; ID Addr Name Size Type Segment\n");

	/* dump global symbol definition table to .lst file */
	fprintf(dumpLstFP, ";-----------------------------------------------------------\n");
	fprintf(dumpLstFP, "; Global Symbol Definition Table: CODE & DATA Segments\n");
	fprintf(dumpLstFP, ";-----------------------------------------------------------\n");
	fprintf(dumpLstFP, "; Addr Name Size Type Segment\n");

	tabID = tGlobal;

    /* get number of global symbols */
    int globalCntr = 0;

    for(int i = 0; i < MAX_HASHTABLE; i++){
        if(htable[i].FirstNode){
            sTab *n = htable[i].FirstNode;

            while(n != NULL){
				if(n->Type == tGLOBAL){
               		globalCntr++;
				}
                n = n->Next;
            }
        }
    }

    /* initialize pointer array to be sorted */
    int j = 0;
    sTab **sortedGlobal = (sTab **)malloc(globalCntr*sizeof(sTab *));
    for(int i = 0; i < MAX_HASHTABLE; i++){
        if(htable[i].FirstNode){
            sTab *n = htable[i].FirstNode;

            while(n != NULL){
				if(n->Type == tGLOBAL){
                	sortedGlobal[j] = n;
                	j++;
				}
                n = n->Next;
            }
        }
    }

    bubbleSort(sortedGlobal, globalCntr);

	for(int i = 0; i < globalCntr; i++){
		sTab *n = sortedGlobal[i];

		sTabListPrintType(stype, n->Type);
		if(n->Sec){
			fprintf(dumpSymFP, "%d %04X %s %d %s %s\n", tabID, n->Addr, 
				n->Name, n->Size, stype, n->Sec->Name);
			fprintf(dumpLstFP, "%04X %s %d %s %s\n", n->Addr, 
				n->Name, n->Size, stype, n->Sec->Name);
		}else{
			fprintf(dumpSymFP, "%d %04X %s %d %s ----\n", tabID, n->Addr, 
				n->Name, n->Size, stype);
			fprintf(dumpLstFP, "%04X %s %d %s ----\n", n->Addr, 
				n->Name, n->Size, stype);
		}
	}
	free(sortedGlobal);

	fprintf(dumpSymFP, "%d\n", tEnd);
	fprintf(dumpLstFP, "\n");

	/* dump external symbol reference table to .sym & .lst file */
	sTabExternSymRefDump(htable);

	/* dump symbol table to .sym file */
	fprintf(dumpSymFP, ";-----------------------------------------------------------\n");
	fprintf(dumpSymFP, "; Symbol Table: CODE & DATA Segments\n");
	fprintf(dumpSymFP, ";-----------------------------------------------------------\n");
	fprintf(dumpSymFP, "; ID Addr Name Size Type Segment EQU\n");

	tabID = tSymbol;
	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sTab *n = htable[i].FirstNode;

			while(n != NULL){
				sTabListPrintType(stype, n->Type);
				if(n->Sec){
					fprintf(dumpSymFP, "%d %04X %s %d %s %s %d\n", tabID, n->Addr, 
						n->Name, n->Size, stype, n->Sec->Name, n->isConst);
				}else{
					fprintf(dumpSymFP, "%d %04X %s %d %s ---- %d\n", tabID, n->Addr, 
						n->Name, n->Size, stype, n->isConst);
				}
				n = n->Next;
			}
		}
	}
	fprintf(dumpSymFP, "%d\n", tEnd);

	/* dump code segment labels to .lst file */
	fprintf(dumpLstFP, ";-----------------------------------------------------------\n");
	fprintf(dumpLstFP, "; Symbol Table (Labels): CODE Segment\n");
	fprintf(dumpLstFP, ";-----------------------------------------------------------\n");
	fprintf(dumpLstFP, "; Addr Name Type Segment EQU\n");

    /* get number of labels */
    int labelCntr = 0;

    for(int i = 0; i < MAX_HASHTABLE; i++){
        if(htable[i].FirstNode){
            sTab *n = htable[i].FirstNode;

            while(n != NULL){
				if(n->Sec && n->Sec->Type == tCODE){
               		labelCntr++;
				}
                n = n->Next;
            }
        }
    }

    /* initialize pointer array to be sorted */
    j = 0;
    sTab **sortedLabel = (sTab **)malloc(labelCntr*sizeof(sTab *));
    for(int i = 0; i < MAX_HASHTABLE; i++){
        if(htable[i].FirstNode){
            sTab *n = htable[i].FirstNode;

            while(n != NULL){
				if(n->Sec && n->Sec->Type == tCODE){
                	sortedLabel[j] = n;
                	j++;
				}
                n = n->Next;
            }
        }
    }

    bubbleSort(sortedLabel, labelCntr);

	for(int i = 0; i < labelCntr; i++){
		sTab *n = sortedLabel[i];

		sTabListPrintType(stype, n->Type);
		if(n->Sec){
			fprintf(dumpLstFP, "%04X %s %s %s %d\n", n->Addr, 
				n->Name, stype, n->Sec->Name, n->isConst);
		}else{
			fprintf(dumpLstFP, "%04X %s %s ---- %d\n", n->Addr, 
				n->Name, stype, n->isConst);
		}
	}
	free(sortedLabel);

	fprintf(dumpLstFP, "\n");

	/* dump data segment variables to .lst file */
	fprintf(dumpLstFP, ";-----------------------------------------------------------\n");
	fprintf(dumpLstFP, "; Symbol Table (Variables): DATA Segment\n");
	fprintf(dumpLstFP, ";-----------------------------------------------------------\n");
	fprintf(dumpLstFP, "; Addr Name Size Type Segment\n");

    /* get number of variables */
    int varCntr = 0;

    for(int i = 0; i < MAX_HASHTABLE; i++){
        if(htable[i].FirstNode){
            sTab *n = htable[i].FirstNode;

            while(n != NULL){
				if(n->Sec && n->Sec->Type == tDATA){
               		varCntr++;
				}
                n = n->Next;
            }
        }
    }

    /* initialize pointer array to be sorted */
    j = 0;
    sTab **sortedVar = (sTab **)malloc(varCntr*sizeof(sTab *));
    for(int i = 0; i < MAX_HASHTABLE; i++){
        if(htable[i].FirstNode){
            sTab *n = htable[i].FirstNode;

            while(n != NULL){
				if(n->Sec && n->Sec->Type == tDATA){
                	sortedVar[j] = n;
                	j++;
				}
                n = n->Next;
            }
        }
    }

    bubbleSort(sortedVar, varCntr);

	for(int i = 0; i < varCntr; i++){
		sTab *n = sortedVar[i];

		sTabListPrintType(stype, n->Type);
		if(n->Sec){
			fprintf(dumpLstFP, "%04X %s %d %s %s\n", n->Addr, 
				n->Name, n->Size, stype, n->Sec->Name);
		}else{
			fprintf(dumpLstFP, "%04X %s %d %s ----\n", n->Addr, 
				n->Name, n->Size, stype);
		}
	}
	free(sortedVar);

	fprintf(dumpLstFP, "\n");

	/* dump segment-less symbols to .lst file */
	fprintf(dumpLstFP, ";-----------------------------------------------------------\n");
	fprintf(dumpLstFP, "; Symbol Table: Extern or Undefined Segment\n");
	fprintf(dumpLstFP, ";-----------------------------------------------------------\n");
	fprintf(dumpLstFP, "; Addr Name Size Type Segment\n");

    /* get number of extern & undefined symbols */
    int undefCntr = 0;

    for(int i = 0; i < MAX_HASHTABLE; i++){
        if(htable[i].FirstNode){
            sTab *n = htable[i].FirstNode;

            while(n != NULL){
				if(!(n->Sec) || n->Sec->Type == tNONE){
               		undefCntr++;
				}
                n = n->Next;
            }
        }
    }

   	/* initialize pointer array to be sorted */
   	j = 0;
	sTab **sortedUndef = (sTab **)malloc(undefCntr*sizeof(sTab *));
   	for(int i = 0; i < MAX_HASHTABLE; i++){
       	if(htable[i].FirstNode){
           	sTab *n = htable[i].FirstNode;

           	while(n != NULL){
				if(!(n->Sec) || n->Sec->Type == tNONE){
               		sortedUndef[j] = n;
               		j++;
				}
               	n = n->Next;
           	}
       	}
   	}

   	bubbleSort(sortedUndef, undefCntr);

	for(int i = 0; i < undefCntr; i++){
		sTab *n = sortedUndef[i];

		sTabListPrintType(stype, n->Type);
		if(n->Sec){
			fprintf(dumpLstFP, "%04X %s %d %s %s\n", n->Addr, 
				n->Name, n->Size, stype, n->Sec->Name);
		}else{
			fprintf(dumpLstFP, "%04X %s %d %s ----\n", n->Addr, 
				n->Name, n->Size, stype);
		}
	}
	free(sortedUndef);

	fprintf(dumpLstFP, "\n");
}

/** 
* @brief Dump external symbol reference table to symbol table (.sym) and source listing (.lst) file 
* 
* @param htable Symbol table data structure
*/
void sTabExternSymRefDump(sTabList htable[])
{
	char stype[10];
	int tabID;

	/* dump external symbol reference table to .sym file */
	fprintf(dumpSymFP, ";-----------------------------------------------------------\n");
	fprintf(dumpSymFP, "; External Symbol Reference Table: CODE Segment\n");
	fprintf(dumpSymFP, ";-----------------------------------------------------------\n");
	fprintf(dumpSymFP, "; ID Addr Name Type Segment\n");

	/* dump external symbol reference table to .lst file */
	fprintf(dumpLstFP, ";-----------------------------------------------------------\n");
	fprintf(dumpLstFP, "; External Symbol Reference Table: CODE Segment\n");
	fprintf(dumpLstFP, ";-----------------------------------------------------------\n");
	fprintf(dumpLstFP, "; Name Addr Type\n");

	tabID = tExtern;

    /* get number of extern symbols */
    int externCntr = 0;

    for(int i = 0; i < MAX_HASHTABLE; i++){
        if(htable[i].FirstNode){
            sTab *n = htable[i].FirstNode;

            while(n != NULL){
				if(n->Type == tEXTERN){
               		externCntr++;
				}
                n = n->Next;
            }
        }
    }

    /* initialize pointer array to be sorted */
    int j = 0;
    sTab **sortedExtern = (sTab **)malloc(externCntr*sizeof(sTab *));
    for(int i = 0; i < MAX_HASHTABLE; i++){
        if(htable[i].FirstNode){
            sTab *n = htable[i].FirstNode;

            while(n != NULL){
				if(n->Type == tEXTERN){
                	sortedExtern[j] = n;
                	j++;
				}
                n = n->Next;
            }
        }
    }

    bubbleSort(sortedExtern, externCntr);

	for(int i = 0; i < externCntr; i++){
		sTab *n = sortedExtern[i];

		sTabListPrintType(stype, n->Type);

		sMemRef *mp = n->MemRefList.FirstNode;
		if(mp){	/* if reference list not empty */
			while(mp != NULL){
				fprintf(dumpSymFP, "%d %04X %s %s ----\n", tabID, mp->Addr, 
					n->Name, stype);
				fprintf(dumpLstFP, "%s %04X %s\n", n->Name, mp->Addr, stype);
				mp = mp->Next;
			}
		}
	}
	free(sortedExtern);

	fprintf(dumpSymFP, "%d\n", tEnd);
	fprintf(dumpLstFP, "\n");
}

/** 
* @brief Return symbol name according to the given referring instruction address
* 
* @param htable Symbol table data structure
* @param addr Program memory address 
* 
* @return Pointer to symbol name
*/
char *sTabSymNameSearchByReferredAddr(sTabList htable[], int addr)
{
	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(htable[i].FirstNode){
			sTab *n = htable[i].FirstNode;

			while(n != NULL){
				if(n->Type == tEXTERN){
					sMemRef *mp = n->MemRefList.FirstNode;
					if(mp){	/* if reference list not empty */
						while(mp != NULL){
							if(mp->Addr == addr){ /* match found */
								return (n->Name);
							}
							mp = mp->Next;
						}
					}
				}
				n = n->Next;
			}
		}
	}
	return NULL;
}


/**
* @brief Bubble sort implementation of (sTab *) type array
*
* @param list Symbol table array to be sorted
* @param cntr Number of elements in the list
*/
void bubbleSort(sTab **list, int cntr)
{
    int swapped;
    int n = cntr;

    do{
        swapped = FALSE;
        n = n-1;
        for(int i = 0; i < n; i++){
            if((list[i]->Addr) > (list[i+1]->Addr)){
                /* swap */
                sTab *temp = list[i];
                list[i] = list[i+1];
                list[i+1] = temp;

                swapped = TRUE;
            }
        }
    } while (swapped);
}

/** 
* @brief If total size of data segment is odd number, insert dummy variable to make it even.
* 
* @param htable Symbol table data structure
* @param list Pointer to secinfo linked list
* @param s Filename without .asm extension
*/
void sTabSymEvenBytePatch(sTabList htable[], sSecInfoList *list, char *s)
{
	int isOdd = FALSE;
	char ts[MAX_SYM_LENGTH];	/* temp. string buffer */

	/* Check if the total size of data segment is odd. */
	sSecInfo *n = list->FirstNode;

	while(n != NULL){
		if(n->Type == tDATA){
			if(n->Size & 0x1){		/* if odd  */
				isOdd = TRUE;
				n->isOdd = TRUE;
			}else{					/* if even */
				;					/* isOdd = FALSE */
			}
			break;
		}
		n = n->Next;
	}

	/* If odd, insert __DUMMY_FILENAME__ local variable to make it even. */
	if(isOdd){
		/* convert file name to UPPER cases */
		int len = strlen(s);
		for (int i = 0; i < len; i++){
			s[i] = (char)toupper(s[i]);
		}
		if (len > 0) s[len-1] = '\0'; 	/* remove trailing . */

		sprintf(ts, "__DUMMY_%s__", s);

		sTab *dummy = sTabHashAdd(htable, ts, n->Size);
		dummy->Type = tLOCAL;
		dummy->Sec = n;
		dummy->Size = 1;

		sint sUNDEFINED;                                                                             
		for(int j = 0; j < NUMDP; j++) {                                                             
			sUNDEFINED.dp[j] = 0x0FFF & UNDEFINED;                                                   
		}                      
		dMemHashAdd(dataMem, sUNDEFINED, curaddr);
	}

	/* Update data segment size. */
	if(isOdd){
		n->Size += 1;
	}
}
