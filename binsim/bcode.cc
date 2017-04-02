/*
All Rights Reserved.
*/
/** 
* @file bcode.cc
* Binary code (sBCode) linked list 
*
* @date 2009-02-04
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "binsim.h"
#include "bcode.h"
#include "parse.h"
#include "dspdef.h"
#include "binsimsupport.h"

/** 
* @brief Allocate a node dynamically.
* 
* @param tdata Binary string buffer for this instruction 
* @param a Program Memory Address for new node
* 
* @return Returns pointer to newly allocated node.
*/
sBCode *sBCodeGetNode(char tdata[], unsigned int a)
{
	int i, j;
	sBCode *p;
	char tbuf[INSTLEN+1];
	int oprCntr = 0;	

	p = (sBCode *)calloc(1, sizeof(sBCode));
	assert(p != NULL);

	strcpy(p->Binary, tdata);
	p->PMA = a + codeSegAddr;
	p->InstType = findInstType(tdata);
	p->Index = iUNKNOWN;

	i = 0;
	for(j = 0; j < MAX_FIELD; j++){
		int w = sInstFormatTable[p->InstType].widths[j];
		if(w != 0){
			strncpy(tbuf, tdata + i, w);
			tbuf[w] = '\0';
			p->Field[oprCntr] = bin2int(tbuf, w);	
			/* Note: Field[0] reserved for OpcodeType */

			oprCntr++;
			i += w;
		} else {
			break;
		}
	}
	p->FieldCounter = oprCntr;	/* FieldCounter including OpcodeType */

	/* Note: MultiCounter & IndexMulti[] set by codeScanOneInst() */

	p->Cond = getIntField(p, fCOND);	
	if(p->Cond == UNDEF_FIELD) p->Cond = 0x0F; 	/* set default: TRUE */

	p->Conj = getIntField(p, fCONJ);	
	if(p->Conj == UNDEF_FIELD) p->Conj = 0; 	/* set default: FALSE */

	return p;
}

/** 
* Check for data duplication and add new node to the END(LastNode) of the list.
* * Note: Before adding first node (empty list), FirstNode MUST be NULL.
* 
* @param list Pointer to linked list
* @param tdata Binary string buffer for this instruction 
* @param a Program Memory Address for new node
* 
* @return Returns pointer to newly allocated node.
*/
sBCode *sBCodeListAdd(sBCodeList *list, char tdata[INSTLEN+1], unsigned int a)
{
	sBCode *n = sBCodeGetNode(tdata, a);
	if(n == NULL) return NULL;

	if(list->FirstNode != NULL){ /* if FistNode is not NULL, 
										LastNode is also not NULL */
		list->LastNode = sBCodeListInsertAfter(list->LastNode, n);
	} else {	/* FirstNode == NULL (list empty) */
		list->LastNode = sBCodeListInsertBeginning(list, n); 
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
sBCode *sBCodeListInsertAfter(sBCode *p, sBCode *newNode)	
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
sBCode *sBCodeListInsertBeginning(sBCodeList *list, sBCode *newNode)	
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
* @param p Pointer to the node just before the one to be removed
*/
void sBCodeListRemoveAfter(sBCodeList *list, sBCode *p)	
{
	if(p != NULL){
		if(p->Next != NULL){
			sBCode *obsoleteNode = p->Next;
			p->Next = p->Next->Next;

			if(obsoleteNode->Next == NULL){	/* if it is LastNode */
				list->LastNode = p;		/* update LastNode */
			}
			/* free node */
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
sBCode *sBCodeListRemoveBeginning(sBCodeList *list)	
{
	if(list != NULL){
		if(list->FirstNode != NULL){
			sBCode *obsoleteNode = list->FirstNode;
			list->FirstNode = list->FirstNode->Next;
			if(list->FirstNode == NULL) list->LastNode = NULL; /* removed last node */

			/* free node */
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
void sBCodeListRemoveAll(sBCodeList *list)
{
	while(list->FirstNode != NULL){
		sBCodeListRemoveBeginning(list);
	}
}

/** 
* @brief Search entire list matching given data value.
* 
* @param list Pointer to linked list
* @param a Program Memory Address to be matched
* 
* @return Returns pointer to the matched node or NULL.
*/
sBCode *sBCodeListSearch(sBCodeList *list, unsigned int a)
{
	sBCode *n = list->FirstNode;

	while(n != NULL) {
		if(n->PMA == a && n->Index != iCOMMENT){	/** ignore comment-only line */
			/* match found */
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
void sBCodeListPrint(sBCodeList *list)
{
	char data[INSTLEN/8];
	char tbuf[INSTLEN+1];
	char tformat1[14] = "%s(%2d):%0nX ";
	char tformat2[13] = "%s(%d):%0nX ";
	int w;

	sBCode *n = list->FirstNode;

	fprintf(dumpDisFP, ";-----------------------------------------------------------\n");
	fprintf(dumpDisFP, "; Addr: Bin: Number_of_Fields Type(bits):Hex Field1(bits):Hex, "
		"Field2(bits):Hex, ..., (Opcode)\n");
	fprintf(dumpDisFP, ";-----------------------------------------------------------\n");

	if(n == NULL){
		fprintf(dumpDisFP, "list is empty.\n");
	}

	while(n != NULL){
		fprintf(dumpDisFP, "%04X: ", n->PMA);	
		fprintf(dumpDisFP, "%s: ", n->Binary); 

		/* print - Number of fields (including OpcodeType) */
		fprintf(dumpDisFP, "%02d: ", n->FieldCounter);	

		/* print - Type(bits):Hex */
		int i = sInstFormatTable[n->InstType].widths[fOpcodeType];
		tformat1[10] = '0' + ((i-1)/4 +1);
		fprintf(dumpDisFP, tformat1,
				sType[n->InstType], i, n->Field[fOpcodeType]);

		/* print - Field_n(bits):Hex */
		for(int j = 1; j < n->FieldCounter; j++){
			w = sInstFormatTable[n->InstType].widths[j];
			tformat2[9] = '0' + ((w-1)/4 +1);
			fprintf(dumpDisFP, tformat2,
				sField[sInstFormatTable[n->InstType].fields[j]], w, n->Field[j]);
			i += w;
		}

		/* print - Opcode Index */
		if(n->Index == iUNKNOWN){		/* Opcode Index unknown */
			fprintf(dumpDisFP, "(----)");
		}else{
			if(!n->MultiCounter){	/* if not multifunction */
				fprintf(dumpDisFP, "(%s)", sOp[n->Index]);
			}else{					/* if multifunction */
				fprintf(dumpDisFP, "(%s", sOp[n->Index]);
				for(int k = 0; k < n->MultiCounter; k++){
					fprintf(dumpDisFP, " || %s", sOp[n->IndexMulti[k]]);
				}
				fprintf(dumpDisFP, ")");
			}
		}

		fprintf(dumpDisFP, "\n");

		n = n->Next;
	}
	fprintf(dumpDisFP, "\n");
}

/** 
* @brief Initialize bCode
* 
* @param list sBCodeList data structure
*/
void bCodeInit(sBCodeList *list)
{
	list->FirstNode = NULL;
	list->LastNode  = NULL;
}

/** 
* @brief Skip pseudo inst./comment/label and return next instruction
* 
* @param p Pointer to current instruction
* 
* @return Pointer to next 'real' instruction
*/
sBCode *getNextCode(sBCode *p)
{
	p = p->Next;
	return p;
}
