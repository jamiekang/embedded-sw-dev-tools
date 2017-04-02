/*
All Rights Reserved.
*/
/** 
* @file icode.cc
* Intermediate code (sICode) linked list and hashing table implementation
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
#include "dspsim.h"
#include "icode.h"
#include "optab.h"	/* because of sOp */
#include "symtab.h"	
#include "simsupport.h"
#include "dspdef.h"

int bitPos;		/* for use in bitwrite() and sICodeBinDump() */
int relAddr, relPos, relBitWidth;	/* for use with t03a, t03c, t03d, t03e 
									   in sICodeBinDump() */

/** 
* @brief Allocate a node dynamically.
* 
* @param i Opcode index for new node
* @param a Program Memory Address for new node
* 
* @return Returns pointer to newly allocated node.
*/
sICode *sICodeGetNode(unsigned int i, unsigned int a)
{
	sICode *p;
	p = (sICode *)calloc(1, sizeof(sICode));
	assert(p != NULL);
	p->Index = i;
	p->PMA = a;

	return p;
}

/** 
* Check for data duplication and add new node to the END(LastNode) of the list.
* * Note: Before adding first node (empty list), FirstNode MUST be NULL.
* 
* @param list Pointer to linked list
* @param i Opcode index for new node
* @param a Program Memory Address for new node
* @param ln Assembly source line number (optional) for new node
* 
* @return Returns pointer to newly allocated node.
*/
extern "C" sICode *sICodeListAdd(sICodeList *list, unsigned int i, 
	unsigned int a, int ln)
{
	sICode *n = sICodeGetNode(i, a);
	if(n == NULL) return NULL;

	/** if only assembly input */
	n->LineCntr = ln;

	if(list->FirstNode != NULL){ /* if FistNode is not NULL, 
										LastNode is also not NULL */
		list->LastNode = sICodeListInsertAfter(list->LastNode, n);
	} else {	/* FirstNode == NULL (list empty) */
		list->LastNode = sICodeListInsertBeginning(list, n); 
	}

	return n;	/* return new node (= LastNode) */
}

/** 
* In case of Multifunction: Add second/third opcode to current instruction
* 
* @param list Pointer to linked list
* @param i Opcode index for new node
* @param a Program Memory Address for new node
* @param ln Assembly source line number (optional) for new node
* 
* @return Returns pointer to newly allocated node.
*/
extern "C" sICode *sICodeListMultiAdd(sICodeList *list, unsigned int i, unsigned int a, int ln)
{
	sICode *n = sICodeGetNode(i, a);
	if(n == NULL) return NULL;

	/** if only assembly input */
	n->LineCntr = ln;

	sICodeListInsertMulti(list->LastNode, n);

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
sICode *sICodeListInsertAfter(sICode *p, sICode *newNode)	
{
	newNode->Next = p->Next;
	p->Next = newNode;

	return newNode;	/* return new node */
}

/** 
* Insert new node at Multi[] of current node - in case of multifunction
* 
* @param p Pointer to current instruction node
* @param newNode Pointer to new node to be added
* 
* @return Returns pointer to new node.
*/
sICode *sICodeListInsertMulti(sICode *p, sICode *newNode)	
{
	newNode->Next = NULL;

	p->Multi[p->MultiCounter++] = newNode;

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
sICode *sICodeListInsertBeginning(sICodeList *list, sICode *newNode)	
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
void sICodeListRemoveAfter(sICodeList *list, sICode *p)	
{
	if(p != NULL){
		if(p->Next != NULL){
			sICode *obsoleteNode = p->Next;
			p->Next = p->Next->Next;

			if(obsoleteNode->Next == NULL){	/* if it is LastNode */
				list->LastNode = p;		/* update LastNode */
			}
			/* free node */
			if(obsoleteNode->Line) free(obsoleteNode->Line);
			if(obsoleteNode->Comment) free(obsoleteNode->Comment);
			if(obsoleteNode->Cond) free(obsoleteNode->Cond);
			if(obsoleteNode->OperandCounter){
				for(int i = 0; i < obsoleteNode->OperandCounter; i++){
					free(obsoleteNode->Operand[i]);
				}
			}

			if(obsoleteNode->Multi[0]) {
				sICode *nm1 = obsoleteNode->Multi[0];
				if(nm1->Line) free(nm1->Line);
				if(nm1->Comment) free(nm1->Comment);
				if(nm1->Cond) free(nm1->Cond);
				if(nm1->OperandCounter){
					for(int i = 0; i < nm1->OperandCounter; i++){
						free(nm1->Operand[i]);
					}
				}
				free(obsoleteNode->Multi[0]);
			}

			if(obsoleteNode->Multi[1]) {
				sICode *nm1 = obsoleteNode->Multi[1];
				if(nm1->Line) free(nm1->Line);
				if(nm1->Comment) free(nm1->Comment);
				if(nm1->Cond) free(nm1->Cond);
				if(nm1->OperandCounter){
					for(int i = 0; i < nm1->OperandCounter; i++){
						free(nm1->Operand[i]);
					}
				}
				free(obsoleteNode->Multi[1]);
			}

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
sICode *sICodeListRemoveBeginning(sICodeList *list)	
{
	if(list != NULL){
		if(list->FirstNode != NULL){
			sICode *obsoleteNode = list->FirstNode;
			list->FirstNode = list->FirstNode->Next;
			if(list->FirstNode == NULL) list->LastNode = NULL; /* removed last node */

			/* free node */
			if(obsoleteNode->Line) free(obsoleteNode->Line);
			if(obsoleteNode->Comment) free(obsoleteNode->Comment);
			if(obsoleteNode->Cond) free(obsoleteNode->Cond);
			if(obsoleteNode->OperandCounter){
				for(int i = 0; i < obsoleteNode->OperandCounter; i++){
					free(obsoleteNode->Operand[i]);
				}
			}

			if(obsoleteNode->Multi[0]) {
				sICode *nm1 = obsoleteNode->Multi[0];
				if(nm1->Line) free(nm1->Line);
				if(nm1->Comment) free(nm1->Comment);
				if(nm1->Cond) free(nm1->Cond);
				if(nm1->OperandCounter){
					for(int i = 0; i < nm1->OperandCounter; i++){
						free(nm1->Operand[i]);
					}
				}
				free(obsoleteNode->Multi[0]);
			}

			if(obsoleteNode->Multi[1]) {
				sICode *nm1 = obsoleteNode->Multi[1];
				if(nm1->Line) free(nm1->Line);
				if(nm1->Comment) free(nm1->Comment);
				if(nm1->Cond) free(nm1->Cond);
				if(nm1->OperandCounter){
					for(int i = 0; i < nm1->OperandCounter; i++){
						free(nm1->Operand[i]);
					}
				}
				free(obsoleteNode->Multi[1]);
			}
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
void sICodeListRemoveAll(sICodeList *list)
{
	while(list->FirstNode != NULL){
		sICodeListRemoveBeginning(list);
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
sICode *sICodeListSearch(sICodeList *list, unsigned int a)
{
	sICode *n = list->FirstNode;

	while(n != NULL) {
		if(n->PMA == a && n->Index != iCOMMENT){	/** ignore comment-only line */
			if(n->Sec != NULL && n->Sec->Type == tCODE){
				/* modified for .CODE segment only checking */
				/* match found */
				return n;
			}
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
void sICodeListPrint(sICodeList *list)
{
	sICode *n = list->FirstNode;
	sICode *m;

	char sAddr[10];
	char sSecType[10];
	char sMemoryRefType[10];
	char sTiming[10];
	char sDelaySlot[10];

	fprintf(dumpLstFP, ";-----------------------------------------------------------\n");
	fprintf(dumpLstFP, "; Output: Source Listing\n");
	fprintf(dumpLstFP, ";-----------------------------------------------------------\n");

	if(n == NULL){
		fprintf(dumpLstFP, "list is empty.\n");
	}

	if(n != NULL){
		//if(VerboseMode) fprintf(dumpLstFP, "Node Ptr. Next Ptr. ");
		fprintf(dumpLstFP, "Line Addr Tp. Seg. Mem C D ");
		if(VerboseMode) fprintf(dumpLstFP, "Opcode(+Operands, IF-Cond) [Label]");
		fprintf(dumpLstFP, "\tSource\n");
	}
	while(n != NULL){

		/* do not print PMA if comment */
		if(!isNotRealInst(n->Index) || (n->Index == i_VAR))	{
			if(n->Sec){
				if(n->Sec->Type == tCODE)
					sprintf(sAddr, "%04X", n->PMA);
				else	
					sprintf(sAddr, "%04X", n->DMA);
			}else{
				sprintf(sAddr, "%04X", n->PMA);
			}
		}else{
			strcpy(sAddr, "----");
		}

		/* do not print MemoryRefType if comment */
		if(!isNotRealInst(n->Index))	{
			if(!(n->MultiCounter)){
				sICodeListPrintType(sMemoryRefType, n->MemoryRefType);
			}else{		/* multi-instruction cases */
				strcpy(sMemoryRefType, "---");

				if(n->MemoryRefType != tNON)
					sICodeListPrintType(sMemoryRefType, n->MemoryRefType);

				for(int j = 0; j < n->MultiCounter; j++){
					m = n->Multi[j];

					if(m->MemoryRefType != tNON)
						sICodeListPrintType(sMemoryRefType, m->MemoryRefType);
				}
			}
		}else{
			strcpy(sMemoryRefType, "---");
		}

		/* do not print Cycles if comment */
		if(!isNotRealInst(n->Index))	{		/* if real instruction */
			sprintf(sTiming, "%d", n->Latency);
		}else{									/* if pseudo inst. or comment or label */
			strcpy(sTiming, "-");
			if(n->Latency){
				sprintf(sTiming, "%d", n->Latency);
				printRunTimeError(n->LineCntr, sTiming,  
					"Latency for pseudo inst., comment, label must be zero.\n");           
			}
		}

		/* do not print isDelaySlot if comment */
		if(!isNotRealInst(n->Index))	{		/* if real instruction */
			if(n->isDelaySlot){		/* if TRUE */
				strcpy(sDelaySlot, "D");
			}else{					/* if FALSE (default) */
				strcpy(sDelaySlot, "-");
			}			
		}else{									/* if pseudo inst. or comment or label */
			strcpy(sDelaySlot, "-");
		}

		/* do not print Section if pseudo(except .VAR) or comment or label instruction */
		if(!isNotRealInst(n->Index) || (n->Index == i_VAR))	{
			if(n->Sec)
				sprintf(sSecType, "%s", n->Sec->Name);
			else
				strcpy(sSecType, "----");
		} else 
			strcpy(sSecType, "----");

		if(n->Next){
			//if(VerboseMode) fprintf(dumpLstFP, "%p %p ", n, n->Next);
			fprintf(dumpLstFP, "%04d %s %s %s %s %s %s ", n->LineCntr, sAddr, 
				sType[n->InstType], sSecType, sMemoryRefType, sTiming, sDelaySlot);
			if(VerboseMode) fprintf(dumpLstFP, "%s", sOp[n->Index]);
		}else{
			//if(VerboseMode) fprintf(dumpLstFP, "%p (NULL)    ", n);
			fprintf(dumpLstFP, "%04d %s %s %s %s %s %s ", n->LineCntr, sAddr, 
				sType[n->InstType], sSecType, sMemoryRefType, sTiming, sDelaySlot);
			if(VerboseMode) fprintf(dumpLstFP, "%s", sOp[n->Index]);
		}
		if(VerboseMode){
			if(n->OperandCounter){
				for(int i = 0; i < n->OperandCounter; i++)
					fprintf(dumpLstFP, " %s", n->Operand[i]);
			}
			if(n->Conj) fprintf(dumpLstFP, " *");
			if(n->Cond) fprintf(dumpLstFP, " IF-%s", n->Cond);
			if(n->Label){
				fprintf(dumpLstFP, " [%s]", n->Label->Name);
			}

			if(n->MultiCounter){
				for(int j = 0; j < n->MultiCounter; j++){
					fprintf(dumpLstFP, " ||\n");
	
					m = n->Multi[j];
					//if(VerboseMode) fprintf(dumpLstFP, "%p (NULL)    ", m);
					fprintf(dumpLstFP, "%04d %04X %s %s", m->LineCntr, m->PMA, 
						sType[m->InstType], sOp[m->Index]);
					if(m->OperandCounter){
						for(int i = 0; i < m->OperandCounter; i++)
							fprintf(dumpLstFP, " %s", m->Operand[i]);
					}
				}
			}
			fprintf(dumpLstFP, "\n");
		}

		/* Input text line */
		if(n->Index != iCOMMENT){
			if(n->Line){
				if(!VerboseMode) fprintf(dumpLstFP, "\t");
				fprintf(dumpLstFP, "%s\n", n->Line);
			}
		}else{		/* comment */
			if(!VerboseMode) fprintf(dumpLstFP, "\t");
			fprintf(dumpLstFP, "%s\n", n->Comment);
		}
		if(VerboseMode) fprintf(dumpLstFP, "\n");

		n = n->Next;
	}
	//if(VerboseMode) fprintf(dumpLstFP, "; F %p L %p\n", list->FirstNode, list->LastNode);
	fprintf(dumpLstFP, "\n");
}

/**
* @brief Get string for type ("NON", "ABS", or "REL")
*
* @param s Pointer to output string buffer
* @param type Type of current symbol table node
*/
void sICodeListPrintType(char *s, int type)
{
    switch(type){
        case    tNON:
            strcpy(s, "---");
            break;
        case    tABS:
            strcpy(s, "ABS");
            break;
        case    tREL:
            strcpy(s, "REL");
            break;
        default:
            strcpy(s, "---");
            break;
    }
}

	
/** 
* @brief Initialize iCode
* 
* @param list sICodeList data structure
*/
void iCodeInit(sICodeList *list)
{
	list->FirstNode = NULL;
	list->LastNode  = NULL;
}

/** 
* @brief Dump all nodes in the linked list (in binary & text formats).
* 
* @param list Pointer to linked list
*/
void sICodeBinDump(sICodeList *list)
{
	char	str[INSTLEN+1];
	int	yz, offset;

	sICode *p = list->FirstNode;

	if(p == NULL){
		printf("list is empty\n");
		return;
	}

	while(p != NULL){
		if(p->MultiCounter) {
			sICodeBinDumpMultiFunc(p);
			p = p->Next;
			continue;
		}

		switch(p->Index){
			case	iADD:
			case	iADDC:
			case	iSUB:
			case	iSUBC:
			case	iSUBB:
			case	iSUBBC:
			case	iAND:
			case	iOR:
			case	iXOR:
			case	iTSTBIT:
			case	iSETBIT:
			case	iCLRBIT:
			case	iTGLBIT:
			case	iNOT:
			case	iABS:
			case	iINC:
			case	iDEC:
			case	iSCR:
				switch(p->InstType){
					case t09c:
						/* write opcode: 7b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write AF: 4b */
						getCodeAF(str, p);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG12): 6b */
						getCodeDReg12(str, p->Operand[0]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP12): 5b */
						getCodeXOP12(str, p->Operand[1]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (YOP12 or 0): 5b */
						if(!p->Operand[2]){		/* iNOT, iABS, iINC, iDEC */
							strcpy(str, "00000");
							bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						} else {
							getCodeXOP12(str, p->Operand[2]);
							bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						}
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t09e:
						/* write opcode: 8b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write AF: 4b */
						getCodeAF(str, p);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG12): 6b */
						getCodeDReg12(str, p->Operand[0]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP12): 5b */
						getCodeXOP12(str, p->Operand[1]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (IMM_INT4/IMM_UINT4): 4b */
						int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 4);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t09i:
						/* write opcode: 7b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write AF: 4b */
						getCodeAF(str, p);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write YZ: 1b */
						if(p->Index == iCLRBIT){	
							/* because CLRBIT is same as bit ANDing with 0 */
							strcpy(str, "1"); /* yz = 1; */
						} else {
							strcpy(str, "0"); /* yz = 0; */
						}
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG12): 6b */
						getCodeDReg12(str, p->Operand[0]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP12): 5b */
						getCodeXOP12(str, p->Operand[1]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (IMM_INT4/IMM_UINT4): 4b */
						int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 4);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t09g:
						/* write opcode: 13b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write AMF: 5b */
						getCodeAMF(str, p);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC32): 3b */
						getCodeACC32(str, p->Operand[0]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (ACC32): 3b */
						getCodeACC32(str, p->Operand[1]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (ACC32): 3b */
						getCodeACC32(str, p->Operand[2]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t46a:
						/* write opcode: 11b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG12): 6b */
						getCodeDReg12(str, p->Operand[0]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP12): 5b */
						getCodeXOP12(str, p->Operand[1]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (YOP12): 5b */
						getCodeXOP12(str, p->Operand[2]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case	iADD_C:
			case	iADDC_C:
			case	iSUB_C:
			case	iSUBC_C:
			case	iSUBB_C:
			case	iSUBBC_C:
			case	iNOT_C:
			case	iABS_C:
			case	iSCR_C:
			case	iRCCW_C:
			case	iRCW_C:
				switch(p->InstType){
					case t09d:
						/* write opcode: 9b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write AF: 4b */
						getCodeAF(str, p);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG24): 5b */
						getCodeDReg24(str, p->Operand[0]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[1]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (XOP24 or (0,0)): 4b */
						if(!p->Operand[2]){		/* iNOT_C, iABS_C */
							strcpy(str, "0000");
							bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						} else {
							getCodeXOP24(str, p->Operand[2]);
							bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						}
						/* write CONJ: 1b */
						getCodeCONJ(str, p->Conj);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t09f:
						/* write opcode: 5b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write AF: 4b */
						getCodeAF(str, p);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG24): 5b */
						getCodeDReg24(str, p->Operand[0]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[1]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (IMM_COMPLEX8): 8b */
						int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 4);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						int2bin(str, getIntSymAddr(p, symTable, p->Operand[3]), 4);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write CONJ: 1b */
						getCodeCONJ(str, p->Conj);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t09h:
						/* write opcode: 15b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write AMF: 5b */
						getCodeAMF(str, p);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC64): 2b */
						getCodeACC64(str, p->Operand[0]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (ACC64): 2b */
						getCodeACC64(str, p->Operand[1]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (ACC64): 2b */
						getCodeACC64(str, p->Operand[2]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write CONJ: 1b */
						getCodeCONJ(str, p->Conj);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t46b:
						/* write opcode: 13b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG24): 5b */
						getCodeDReg24(str, p->Operand[0]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[1]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[2]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write CONJ: 1b */
						getCodeCONJ(str, p->Conj);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t50a:
						/* write opcode: 12b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG24): 5b */
						getCodeDReg24(str, p->Operand[0]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[1]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (YOP12): 5b */
						getCodeXOP12(str, p->Operand[2]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write CW: 1b */
						if(p->Index == iRCW_C){	
							strcpy(str, "1"); /* RCW.C : CW = 1; */
						} else {
							strcpy(str, "0"); /* RCCW.C: CW = 0; */
						}
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iMPY:
			case iMAC:
			case iMAS:
				switch(p->InstType){
					case t40e:
						/* write opcode: 8b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write AMF: 5b */
						getCodeAMF(str, p);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC32): 3b */
						getCodeACC32(str, p->Operand[0]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP12): 5b */
						getCodeXOP12(str, p->Operand[1]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (YOP12 or 0): 5b */
						getCodeXOP12(str, p->Operand[2]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write MN: 1b */
						getCodeMN(str, p->Operand[0]);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iMPY_C:
			case iMPY_RC:
			case iMAC_C:
			case iMAC_RC:
			case iMAS_C:
			case iMAS_RC:
				switch(p->InstType){
					case t40f:
						/* write opcode: 10b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write AMF: 5b */
						getCodeAMF(str, p);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC64): 2b */
						getCodeACC64(str, p->Operand[0]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[1]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (YOP24): 4b */
						getCodeXOP24(str, p->Operand[2]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write MN: 1b */
						getCodeMN(str, p->Operand[0]);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write CONJ: 1b */
						getCodeCONJ(str, p->Conj);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t40g:
						/* write opcode: 9b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write AMF: 5b */
						getCodeAMF(str, p);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC64): 2b */
						getCodeACC64(str, p->Operand[0]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP12): 5b */
						getCodeXOP12(str, p->Operand[1]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (YOP24): 4b */
						getCodeXOP24(str, p->Operand[2]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write MN: 1b */
						getCodeMN(str, p->Operand[0]);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write CONJ: 1b */
						getCodeCONJ(str, p->Conj);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iRNDACC:
			case iRNDACC_C:
			case iCLRACC:
			case iCLRACC_C:
			case iSATACC:
			case iSATACC_C:
				switch(p->InstType){
					case t41a:
					case t41c:
						/* write opcode: 24b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC32): 3b */
						getCodeACC32(str, p->Operand[0]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t41b:
						/* write opcode: 24b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC64): 2b */
						getCodeACC64(str, p->Operand[0]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write CONJ: 1b */
						getCodeCONJ(str, p->Conj);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t41d:
						/* write opcode: 25b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC64): 2b */
						getCodeACC64(str, p->Operand[0]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t25a:
						/* write opcode: 24b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC32): 3b */
						getCodeACC32(str, p->Operand[0]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t25b:
						/* write opcode: 24b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC64): 2b */
						getCodeACC64(str, p->Operand[0]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write CONJ: 1b */
						getCodeCONJ(str, p->Conj);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iASHIFT:
			case iASHIFTOR:
			case iLSHIFT:
			case iLSHIFTOR:
			case iEXP:
			case iEXPADJ:
				switch(p->InstType){
					case t16a:
						/* write opcode: 9b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SF: 5b */
						getCodeSF(str, p->Operand[3], p->Index);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC32): 3b */
						getCodeACC32(str, p->Operand[0]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP12): 5b */
						getCodeXOP12(str, p->Operand[1]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (YOP12): 5b */
						getCodeXOP12(str, p->Operand[2]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t16c:
						/* write opcode: 11b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SF: 5b */
						getCodeSF(str, p->Operand[2], p->Index);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG12): 6b */
						getCodeDReg12(str, p->Operand[0]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP12): 5b */
						getCodeXOP12(str, p->Operand[1]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t16e:
						/* write opcode: 6b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SF: 5b */
						getCodeSF(str, p->Operand[3], p->Index);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG12): 6b */
						getCodeDReg12(str, p->Operand[0]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP12): 5b */
						getCodeXOP12(str, p->Operand[1]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (YOP12): 5b */
						getCodeXOP12(str, p->Operand[2]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t15a:
						/* write opcode: 9b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SF: 5b */
						getCodeSF(str, p->Operand[3], p->Index);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC32): 3b */
						getCodeACC32(str, p->Operand[0]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP12): 5b */
						getCodeXOP12(str, p->Operand[1]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (IMM_INT5): 5b */
						int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 5);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t15c:
						/* write opcode: 10b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SF: 5b */
						getCodeSF(str, p->Operand[3], p->Index);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC32): 3b */
						getCodeACC32(str, p->Operand[0]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (ACC32): 3b */
						getCodeACC32(str, p->Operand[1]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (IMM_INT6): 6b */
						int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 6);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t15e:
						/* write opcode: 6b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SF: 5b */
						getCodeSF(str, p->Operand[3], p->Index);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG12): 6b */
						getCodeDReg12(str, p->Operand[0]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP12): 5b */
						getCodeXOP12(str, p->Operand[1]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (IMM_INT5): 5b */
						int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 5);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iASHIFT_C:
			case iASHIFTOR_C:
			case iLSHIFT_C:
			case iLSHIFTOR_C:
			case iEXP_C:
			case iEXPADJ_C:
				switch(p->InstType){
					case t16b:
						/* write opcode: 11b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SF: 5b */
						getCodeSF(str, p->Operand[3], p->Index);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC64): 2b */
						getCodeACC64(str, p->Operand[0]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[1]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (YOP12 or 0): 5b */
						getCodeXOP12(str, p->Operand[2]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t16d:
						/* write opcode: 13b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SF: 5b */
						getCodeSF(str, p->Operand[2], p->Index);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG24): 5b */
						getCodeDReg24(str, p->Operand[0]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[1]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t16f:
						/* write opcode: 8b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SF: 5b */
						getCodeSF(str, p->Operand[3], p->Index);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG24): 5b */
						getCodeDReg24(str, p->Operand[0]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[1]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (YOP12 or 0): 5b */
						getCodeXOP12(str, p->Operand[2]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t15b:
						/* write opcode: 11b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SF: 5b */
						getCodeSF(str, p->Operand[3], p->Index);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC64): 2b */
						getCodeACC64(str, p->Operand[0]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[1]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (IMM_INT5): 5b */
						int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 5);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t15d:
						/* write opcode: 12b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SF: 5b */
						getCodeSF(str, p->Operand[3], p->Index);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC64): 2b */
						getCodeACC64(str, p->Operand[0]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (ACC64): 2b */
						getCodeACC64(str, p->Operand[1]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (IMM_INT6): 6b */
						int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 6);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t15f:
						/* write opcode: 8b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SF: 5b */
						getCodeSF(str, p->Operand[3], p->Index);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG24): 5b */
						getCodeDReg24(str, p->Operand[0]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[1]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC2 (IMM_INT5): 5b */
						int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 5);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iLD:
			case iST:
				switch(p->InstType){
					case t03a:
					case t03f:
						/* write opcode: 5b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG12): 6b */
						if(p->Index == iST){	/* iST */
							getCodeDReg12(str, p->Operand[2]);
						} else {				/* iLD */
							getCodeDReg12(str, p->Operand[0]);
						}
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (IMM_UINT16): 16b */
						if(p->Index == iST){	/* iST */
							relAddr = getIntSymAddr(p, symTable, p->Operand[0]);
							relPos = bitPos;
							relBitWidth = 16;
							int2bin(str, relAddr, 16);
						} else {				/* iLD */
							relAddr = getIntSymAddr(p, symTable, p->Operand[1]);
							relPos = bitPos;
							relBitWidth = 16;
							int2bin(str, relAddr, 16);
						}
						bitwrite(p, str, 16, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t03d:
					case t03h:
						/* write opcode: 7b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC32): 3b */
						if(p->Index == iST){	/* iST */
							getCodeACC32(str, p->Operand[2]);
						} else {				/* iLD */
							getCodeACC32(str, p->Operand[0]);
						}
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (IMM_UINT16): 16b */
						if(p->Index == iST){	/* iST */
							relAddr = getIntSymAddr(p, symTable, p->Operand[0]);
							relPos = bitPos;
							relBitWidth = 16;
							int2bin(str, relAddr, 16);
						} else {				/* iLD */
							relAddr = getIntSymAddr(p, symTable, p->Operand[1]);
							relPos = bitPos;
							relBitWidth = 16;
							int2bin(str, relAddr, 16);
						}
						bitwrite(p, str, 16, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write LSF: 1b */
						getCodeLSF(str, p->Operand[3], p->Index);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t06a:	/* iLD only */
						/* write opcode: 5b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (RREG16): 6b */
						getCodeRReg16(str, p->Operand[0]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (IMM_INT16): 16b */
						relAddr = getIntSymAddr(p, symTable, p->Operand[1]);
						relPos = bitPos;
						relBitWidth = 16;
						int2bin(str, relAddr, 16);
						bitwrite(p, str, 16, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t06b:	/* iLD only */
						/* write opcode: 9b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG12): 6b */
						getCodeDReg12(str, p->Operand[0]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (IMM_INT12): 12b */
						relAddr = getIntSymAddr(p, symTable, p->Operand[1]);
						relPos = bitPos;
						relBitWidth = 12;
						int2bin(str, relAddr, 12);
						bitwrite(p, str, 12, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t06d:	/* iLD only */
						/* write opcode: 5b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC32): 3b */
						getCodeACC32(str, p->Operand[0]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (IMM_INT24): 24b */
						relAddr = getIntSymAddr(p, symTable, p->Operand[1]);
						relPos = bitPos;
						relBitWidth = 24;
						int2bin(str, relAddr, 24);
						bitwrite(p, str, 24, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t32a:
					case t32c:
						/* write opcode: 14b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG12): 6b */
						if(p->Index == iST){	/* iST */
							getCodeDReg12(str, p->Operand[4]);
						} else {				/* iLD */
							getCodeDReg12(str, p->Operand[0]);
						}
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
						if(p->Index == iST){	/* iST */
							getCodeU(str, p->Operand[0]);
						} else {				/* iLD */
							getCodeU(str, p->Operand[1]);
						}
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DMM: 3b */
						if(p->Index == iST){	/* iST */
							getCodeMReg3b(str, p->Operand[2]);
						} else {				/* iLD */
							getCodeMReg3b(str, p->Operand[3]);
						}
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DMI: 3b */
						if(p->Index == iST){	/* iST */
							getCodeIReg3b(str, p->Operand[1]);
						} else {				/* iLD */
							getCodeIReg3b(str, p->Operand[2]);
						}
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t29a:
					case t29c:
						/* write opcode: 9b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG12): 6b */
						if(p->Index == iST){	/* iST */
							if(p->Operand[3]){	/* ST DM(IREG +/+= <IMM_INT8>), DREG12 */
								getCodeDReg12(str, p->Operand[4]);
							}else{				/* ST DM(IREG), DREG12 */
								getCodeDReg12(str, p->Operand[2]);
							}
						} else {				/* iLD */
							if(p->Operand[3]){	/* LD DREG12, DM(IREG +/+= <IMM_INT8>) */
								getCodeDReg12(str, p->Operand[0]);
							}else{				/* LD DREG12, DM(IREG) */
								getCodeDReg12(str, p->Operand[0]);
							}
						}
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (IMM_INT8): 8b */
						if(p->Index == iST){	/* iST */
							if(p->Operand[3]){	/* ST DM(IREG +/+= <IMM_INT8>), DREG12 */
								int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 8);
							}else{				/* ST DM(IREG), DREG12 */
								int2bin(str, 0, 8);
							}
						} else {				/* iLD */
							if(p->Operand[3]){	/* LD DREG12, DM(IREG +/+= <IMM_INT8>) */
								int2bin(str, getIntSymAddr(p, symTable, p->Operand[3]), 8);
							}else{				/* LD DREG12, DM(IREG) */
								int2bin(str, 0, 8);
							}
						}
						bitwrite(p, str, 8, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
						if(p->Index == iST){	/* iST */
							if(p->Operand[3]){	/* ST DM(IREG +/+= <IMM_INT8>), DREG12 */
								getCodeU(str, p->Operand[0]);
							}else{				/* ST DM(IREG), DREG12 */
								strcpy(str, "0");
							}
						} else {				/* iLD */
							if(p->Operand[3]){	/* LD DREG12, DM(IREG +/+= <IMM_INT8>) */
								getCodeU(str, p->Operand[1]);
							}else{				/* LD DREG12, DM(IREG) */
								strcpy(str, "0");
							}
						}
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DMI: 3b */
						if(p->Index == iST){	/* iST */
							if(p->Operand[3]){	/* ST DM(IREG +/+= <IMM_INT8>), DREG12 */
								getCodeIReg3b(str, p->Operand[1]);
							}else{				/* ST DM(IREG), DREG12 */
								getCodeIReg3b(str, p->Operand[0]);
							}
						} else {				/* iLD */
							if(p->Operand[3]){	/* LD DREG12, DM(IREG +/+= <IMM_INT8>) */
								getCodeIReg3b(str, p->Operand[2]);
							}else{				/* LD DREG12, DM(IREG) */
								getCodeIReg3b(str, p->Operand[1]);
							}
						}
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t22a:
						/* write opcode: 8b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (IMM_INT12): 12b */
						int2bin(str, getIntSymAddr(p, symTable, p->Operand[4]), 12);
						bitwrite(p, str, 12, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
						getCodeU(str, p->Operand[0]);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DMM: 3b */
						getCodeMReg3b(str, p->Operand[2]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DMI: 3b */
						getCodeIReg3b(str, p->Operand[1]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iLD_C:
			case iST_C:
				switch(p->InstType){
					case t03c:
					case t03g:
						/* write opcode: 6b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG24): 5b */
						if(p->Index == iST_C){	/* iST */
							getCodeDReg24(str, p->Operand[2]);
						} else {				/* iLD_C */
							getCodeDReg24(str, p->Operand[0]);
						}
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (IMM_UINT16): 16b */
						if(p->Index == iST_C){	/* iST */
							relAddr = getIntSymAddr(p, symTable, p->Operand[0]);
							relPos = bitPos;
							relBitWidth = 16;
							int2bin(str, relAddr, 16);
						} else {				/* iLD_C */
							relAddr = getIntSymAddr(p, symTable, p->Operand[1]);
							relPos = bitPos;
							relBitWidth = 16;
							int2bin(str, relAddr, 16);
						}
						bitwrite(p, str, 16, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t03e:
					case t03i:
						/* write opcode: 8b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (ACC64): 2b */
						if(p->Index == iST_C){	/* iST_C */
							getCodeACC64(str, p->Operand[2]);
						} else {				/* iLD_C */
							getCodeACC64(str, p->Operand[0]);
						}
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (IMM_UINT16): 16b */
						if(p->Index == iST_C){	/* iST_C */
							relAddr = getIntSymAddr(p, symTable, p->Operand[0]);
							relPos = bitPos;
							relBitWidth = 16;
							int2bin(str, relAddr, 16);
						} else {				/* iLD_C */
							relAddr = getIntSymAddr(p, symTable, p->Operand[1]);
							relPos = bitPos;
							relBitWidth = 16;
							int2bin(str, relAddr, 16);
						}
						bitwrite(p, str, 16, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write LSF: 1b */
						getCodeLSF(str, p->Operand[3], p->Index);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t06c:	/* iLD_C only */
						/* write opcode: 3b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DRE24): 5b */
						getCodeDReg24(str, p->Operand[0]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (IMM_COMPLEX24): 24b */
						int2bin(str, getIntSymAddr(p, symTable, p->Operand[1]), 12);
						bitwrite(p, str, 12, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 12);
						bitwrite(p, str, 12, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t32b:
					case t32d:
						/* write opcode: 15b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (CREG): 5b */
						if(p->Index == iST_C){	/* iST_C */
							getCodeCReg(str, p->Operand[4]);
						} else {				/* iLD_C */
							getCodeCReg(str, p->Operand[0]);
						}
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
						if(p->Index == iST_C){	/* iST_C */
							getCodeU(str, p->Operand[0]);
						} else {				/* iLD_C */
							getCodeU(str, p->Operand[1]);
						}
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DMM: 3b */
						if(p->Index == iST_C){	/* iST_C */
							getCodeMReg3b(str, p->Operand[2]);
						} else {				/* iLD_C */
							getCodeMReg3b(str, p->Operand[3]);
						}
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DMI: 3b */
						if(p->Index == iST_C){	/* iST_C */
							getCodeIReg3b(str, p->Operand[1]);
						} else {				/* iLD_C */
							getCodeIReg3b(str, p->Operand[2]);
						}
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t29b:
					case t29d:
						/* write opcode: 10b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG24): 5b */
						if(p->Index == iST_C){	/* iST_C */
							if(p->Operand[3]){	/* ST.C DM(IREG +/+= <IMM_INT8>), DREG24 */
								getCodeDReg24(str, p->Operand[4]);
							}else{				/* ST.C DM(IREG), DREG24 */
								getCodeDReg24(str, p->Operand[2]);
							}
						} else {				/* iLD_C */
							if(p->Operand[3]){	/* LD.C DREG24, DM(IREG +/+= <IMM_INT8>) */
								getCodeDReg24(str, p->Operand[0]);
							}else{				/* LD.C DREG24, DM(IREG) */
								getCodeDReg24(str, p->Operand[0]);
							}
						}
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (IMM_INT8): 8b */
						if(p->Index == iST_C){	/* iST_C */
							if(p->Operand[3]){	/* ST.C DM(IREG +/+= <IMM_INT8>), DREG24 */
								int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 8);
							}else{				/* ST.C DM(IREG), DREG24 */
								int2bin(str, 0, 8);
							}
						} else {				/* iLD_C */
							if(p->Operand[3]){	/* LD.C DREG24, DM(IREG +/+= <IMM_INT8>) */
								int2bin(str, getIntSymAddr(p, symTable, p->Operand[3]), 8);
							}else{				/* LD.C DREG24, DM(IREG) */
								int2bin(str, 0, 8);
							}
						}
						bitwrite(p, str, 8, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
						if(p->Index == iST_C){	/* iST_C */
							if(p->Operand[3]){	/* ST.C DM(IREG +/+= <IMM_INT8>), DREG24 */
								getCodeU(str, p->Operand[0]);
							}else{				/* ST.C DM(IREG), DREG24 */
								strcpy(str, "0");
							}
						} else {				/* iLD_C */
							if(p->Operand[3]){	/* LD.C DREG24, DM(IREG +/+= <IMM_INT8>) */
								getCodeU(str, p->Operand[1]);
							}else{				/* LD.C DREG24, DM(IREG) */
								strcpy(str, "0");
							}
						}
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DMI: 3b */
						if(p->Index == iST_C){	/* iST_C */
							if(p->Operand[3]){	/* ST.C DM(IREG +/+= <IMM_INT8>), DREG24 */
								getCodeIReg3b(str, p->Operand[1]);
							}else{				/* ST.C DM(IREG), DREG24 */
								getCodeIReg3b(str, p->Operand[0]);
							}
						} else {				/* iLD_C */
							if(p->Operand[3]){	/* LD.C DREG24, DM(IREG +/+= <IMM_INT8>) */
								getCodeIReg3b(str, p->Operand[2]);
							}else{				/* LD.C DREG24, DM(IREG) */
								getCodeIReg3b(str, p->Operand[1]);
							}
						}
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iCP:
			case iCP_C:
				switch(p->InstType){
					case t17a:
						/* write opcode: 15b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DSTLS (DREG12): 6b */
						getCodeDReg12(str, p->Operand[0]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRCLS (DREG12): 6b */
						getCodeDReg12(str, p->Operand[1]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t17b:
						/* write opcode: 14b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						if(isDReg12(p, p->Operand[0]) && isRReg16(p, p->Operand[1])){
							/* write DSTLS (DREG12): 6b */
							getCodeDReg12(str, p->Operand[0]);
							bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
							/* write SRCLS (RREG16): 6b */
							getCodeRReg16(str, p->Operand[1]);
							bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						} else if(isRReg16(p, p->Operand[0]) && isDReg12(p, p->Operand[1])){
							/* write SRCLS (DREG12): 6b */
							getCodeDReg12(str, p->Operand[1]);
							bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
							/* write DSTLS (RREG16): 6b */
							getCodeRReg16(str, p->Operand[0]);
							bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						}
						/* write DC: 1b */
						getCodeDC(p, str, p->Operand[0], p->Operand[1]);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t17c:
						/* write opcode: 21b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DSTLS (ACC32): 3b */
						getCodeACC32(str, p->Operand[0]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRCLS (ACC32): 3b */
						getCodeACC32(str, p->Operand[1]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t17d:
						/* write opcode: 17b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DSTLS (DREG24): 5b */
						getCodeDReg24(str, p->Operand[0]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRCLS (DREG24): 5b */
						getCodeDReg24(str, p->Operand[1]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t17e:
						/* write opcode: 15b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						if(isDReg24(p, p->Operand[0]) && isRReg16(p, p->Operand[1])){
							/* write DSTLS (DREG24): 5b */
							getCodeDReg24(str, p->Operand[0]);
							bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
							/* write SRCLS (RREG16): 6b */
							getCodeRReg16(str, p->Operand[1]);
							bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						} else if(isRReg16(p, p->Operand[0]) && isDReg24(p, p->Operand[1])){
							/* write SRCLS (DREG24): 5b */
							getCodeDReg24(str, p->Operand[1]);
							bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
							/* write DSTLS (RREG16): 6b */
							getCodeRReg16(str, p->Operand[0]);
							bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						}
						/* write DC: 1b */
						getCodeDC(p, str, p->Operand[0], p->Operand[1]);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t17f:
						/* write opcode: 23b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DSTLS (ACC64): 2b */
						getCodeACC64(str, p->Operand[0]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRCLS (ACC64): 2b */
						getCodeACC64(str, p->Operand[1]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t17g:
						/* write opcode: 15b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DSTLS (RREG16): 6b */
						getCodeRReg16(str, p->Operand[0]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRCLS (RREG16): 6b */
						getCodeRReg16(str, p->Operand[1]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t17h:
						/* write opcode: 18b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DSTLS (ACC32): 3b */
						getCodeACC32(str, p->Operand[0]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRCLS (DREG12): 6b */
						getCodeDReg12(str, p->Operand[1]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iCPXI:
			case iCPXI_C:
			case iCPXO:
			case iCPXO_C:
				switch(p->InstType){
					case t49a:
						/* write opcode: 16b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DSTLS (DREG12): 6b */
						getCodeDReg12(str, p->Operand[0]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRCLS (XREG12): 3b */
						getCodeXReg12(str, p->Operand[1]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write IDR (IMM_UINT2): 2b */		
                        int2bin(str, 0x03 & getIntSymAddr(p, symTable, p->Operand[2]), 2);
                        bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write IDN (IMM_UINT4): 4b */		
                        int2bin(str, 0x0F & getIntSymAddr(p, symTable, p->Operand[3]), 4);
                        bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t49b:
						/* write opcode: 18b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DSTLS (DREG24): 5b */
						getCodeDReg24(str, p->Operand[0]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRCLS (XREG24): 2b */
						getCodeXReg24(str, p->Operand[1]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write IDR (IMM_UINT2): 2b */		
                        int2bin(str, 0x03 & getIntSymAddr(p, symTable, p->Operand[2]), 2);
                        bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write IDN (IMM_UINT4): 4b */		
                        int2bin(str, 0x0F & getIntSymAddr(p, symTable, p->Operand[3]), 4);
                        bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t49c:
						/* write opcode: 16b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DSTLS (XREG12): 3b */
						getCodeXReg12(str, p->Operand[0]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRCLS (DREG12): 6b */
						getCodeDReg12(str, p->Operand[1]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write IDR (IMM_UINT2): 2b */		
                        int2bin(str, 0x03 & getIntSymAddr(p, symTable, p->Operand[2]), 2);
                        bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write IDN (IMM_UINT4): 4b */		
                        int2bin(str, 0x0F & getIntSymAddr(p, symTable, p->Operand[3]), 4);
                        bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t49d:
						/* write opcode: 18b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DSTLS (XREG24): 2b */
						getCodeXReg24(str, p->Operand[0]);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRCLS (DREG24): 5b */
						getCodeDReg24(str, p->Operand[1]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write IDR (IMM_UINT2): 2b */		
                        int2bin(str, 0x03 & getIntSymAddr(p, symTable, p->Operand[2]), 2);
                        bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write IDN (IMM_UINT4): 4b */		
                        int2bin(str, 0x0F & getIntSymAddr(p, symTable, p->Operand[3]), 4);
                        bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iDO:
				switch(p->InstType){
					case t11a:
						/* write opcode: 16b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write BRDST (IMM_UINT12): 12b */
						relAddr = getIntSymAddr(p, symTable, p->Operand[1]) - p->PMA;
						relPos = bitPos;
						relBitWidth = 12;
						int2bin(str, relAddr, 12);
						bitwrite(p, str, 12, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write TERM: 4b */
						getCodeTERM(str, p->Operand[0]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iCALL:
			case iJUMP:
				switch(p->InstType){
					case t10a:
						/* write opcode: 13b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write BRDST (IMM_INT13): 13b */
						relAddr = getIntSymAddr(p, symTable, p->Operand[0]) - p->PMA;
						relPos = bitPos;
						relBitWidth = 13;
						int2bin(str, relAddr, 13);
						bitwrite(p, str, 13, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write S: 1b */
						if(p->Index == iCALL){	/* iCALL */
							strcpy(str, "1");
						} else {				/* iJUMP */
							strcpy(str, "0");
						}
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t10b:
						/* write opcode: 15b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write BRDST (IMM_INT16): 16b */
						relAddr = getIntSymAddr(p, symTable, p->Operand[0]) - p->PMA;
						relPos = bitPos;
						relBitWidth = 16;
						int2bin(str, relAddr, 16);
						bitwrite(p, str, 16, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write S: 1b */
						if(p->Index == iCALL){	/* iCALL */
							strcpy(str, "1");
						} else {				/* iJUMP */
							strcpy(str, "0");
						}
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					case t19a:
						/* write opcode: 23b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write S: 1b */
						if(p->Index == iCALL){	/* iCALL */
							strcpy(str, "1");
						} else {				/* iJUMP */
							strcpy(str, "0");
						}
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DMI: 3b */
						getCodeIReg3b(str, p->Operand[0]);
						bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iRTI:
			case iRTS:
				switch(p->InstType){
					case t20a:
						/* write opcode: 26b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write T: 1b */
						if(p->Index == iRTI){	/* iRTI */
							strcpy(str, "1");
						} else {				/* iRTS */
							strcpy(str, "0");
						}
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iPUSH:
			case iPOP:
				switch(p->InstType){
					case t26a:
						/* write opcode: 21b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write LPP: 2b */
						getCodeLPP(str, p->Operand[0], p->Index);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write PPP: 2b */
						getCodePPP(str, p->Operand[0], p->Index);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SPP: 2b */
						getCodeSPP(str, p->Operand[0], p->Operand[1], p->Index);
						bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iNOP:
				switch(p->InstType){
					case t30a:
						/* write opcode: 27b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iENA:
			case iDIS:
				switch(p->InstType){
					case t18a:
						/* write opcode: 9b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write ENA: 18b */
						getCodeENA(str, p);
						bitwrite(p, str, 18, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iENADP:
				switch(p->InstType){
					case t18b:
						/* write opcode: 21b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write IDE: 4b */
						getCodeIDE(str, p);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write IDM (IMM_UINT2): 2b */		
                        int2bin(str, 0x03 & getIntSymAddr(p, symTable, p->Operand[0]), 2);
                        bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
			case iDPID:
				switch(p->InstType){
					case t18c:
						/* write opcode: 21b */
						bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
							bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DSTID (DREG12): 6b */
						getCodeDReg12(str, p->Operand[0]);
						bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
					default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
						break;
				}
				break;
            case iIDLE:
                switch(p->InstType){
                    case t31a:
                        /* write opcode: 27b */
                        bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width,
                            bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        break;
                    default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
                        break;
                }
                break;
            case iSETINT:
            case iCLRINT:
                switch(p->InstType){
                    case t37a:
                        /* write opcode: 22b */
                        bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width,
                            bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        /* write INTN (IMM_UINT4): 4b */
                        int2bin(str, 0x0F & getIntSymAddr(p, symTable, p->Operand[0]), 4);
                        bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        /* write C: 1b */
                        if(p->Index == iSETINT){    /* SETINT */
                            strcpy(str, "1");
                        } else {                    /* CLRINT */
                            strcpy(str, "0");
                        }
                        bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        break;
                    default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
                        break;
                }
                break;
            case iDIVS:
            case iDIVQ:
                switch(p->InstType){
                    case t23a:
                        /* write opcode: 11b */
                        bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width,
                            bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        /* write DST (REG12): 5b */
                        getCodeReg12(str, p->Operand[0]);
                        bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        /* write SRC1 (XOP12): 5b */
                        getCodeXOP12(str, p->Operand[1]);
                        bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        /* write SRC2 (YOP12): 5b */
                        getCodeXOP12(str, p->Operand[2]);
                        bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        /* write DF: 1b */
                        if(p->Index == iDIVS){    /* DIVS */
                            strcpy(str, "1");
                        } else {                  /* DIVQ */
                            strcpy(str, "0");
                        }
                        bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        break;
                    default:
                        bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
                        break;
                }
                break;
            case iPOLAR_C:
            case iRECT_C:
			case iCONJ_C:
			case iMAG_C:
                switch(p->InstType){
                    case t42a:
                        /* write opcode: 16b */
                        bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width,
                            bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG24): 5b */
						getCodeDReg24(str, p->Operand[0]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[1]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write CONJ: 1b */
						getCodeCONJ(str, p->Conj);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        /* write POLAR: 1b */
                        if(p->Index == iRECT_C){    /* RECT.C */
                            strcpy(str, "1");
                        } else {                  /* POLAR.C */
                            strcpy(str, "0");
                        }
                        bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        break;
                    case t42b:
                        /* write opcode: 18b */
                        bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width,
                            bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG24): 5b */
						getCodeDReg24(str, p->Operand[0]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[1]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        break;
                    case t47a:
                        /* write opcode: 17b */
                        bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width,
                            bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write DST (DREG24): 5b */
						getCodeDReg24(str, p->Operand[0]);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write SRC1 (XOP24): 4b */
						getCodeXOP24(str, p->Operand[1]);
						bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write CONJ: 1b */
						getCodeCONJ(str, p->Conj);
						bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        break;
                    default:
                        bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
                        break;
                }
                break;
            case iRESET:
                switch(p->InstType){
                    case t48a:
                        /* write opcode: 27b */
                        bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width,
                            bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
						/* write COND: 5b */
						getCodeCOND(str, p->Cond);
						bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
                        break;
                    default:
                		bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
                        break;
                }
                break;
			case i_GLOBAL:
			case i_EXTERN:
			case i_VAR:
			case i_CODE:
			case i_DATA:
			case i_EQU:
				/* do nothing here */
				break;
			default:
				if(!isCommentLabelInst(p->Index))
                	bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
				break;
		}
		p = p->Next;
	}
}

/** 
* @brief Dump a multifunction instruction (in binary & text formats).
* 
* @param p Pointer to current instruction
*/
void sICodeBinDumpMultiFunc(sICode *p)
{
	sICode *m1, *m2;
	char	str[INSTLEN+1];
	int	yz;

	switch(p->MultiCounter){
		case 1:			/* two-opcode multifunction */
			m1 = p->Multi[0];
			m2 = NULL;

			switch(p->InstType){
				case t01a:		/* LD || LD */
					/* write opcode: 4b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write MF: 4b */
					strcpy(str, "0000");	/* NOP */
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC32S): 2b */
					strcpy(str, "00");
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					strcpy(str, "000");
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP12S): 3b */
					strcpy(str, "000");
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
					getCodeU(str, p->Operand[1]);
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U2: 1b */		/* 0 for premodify, 1 for postmodify */
					getCodeU(str, m1->Operand[1]);
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST2 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[0]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST3 (YOP12S): 3b */
					getCodeReg12S(str, m1->Operand[0]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write PMM: 2b */
					getCodeMReg2b(str, m1->Operand[3]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write PMI: 2b */
					getCodeIReg2b(str, m1->Operand[2]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMM: 2b */
					getCodeMReg2b(str, p->Operand[3]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMI: 2b */
					getCodeIReg2b(str, p->Operand[2]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t01b:		/* LD.C || LD.C */
					/* write opcode: 8b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write MF: 4b */
					strcpy(str, "0000");	/* NOP */
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC64S): 2b */
					strcpy(str, "00");
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP24S): 2b */
					strcpy(str, "00");
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP24S): 2b */
					strcpy(str, "00");
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
					getCodeU(str, p->Operand[1]);
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U2: 1b */		/* 0 for premodify, 1 for postmodify */
					getCodeU(str, m1->Operand[1]);
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST2 (XOP24S): 2b */
					getCodeReg24S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST3 (YOP24S): 2b */
					getCodeReg24S(str, m1->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write PMM: 2b */
					getCodeMReg2b(str, m1->Operand[3]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write PMI: 2b */
					getCodeIReg2b(str, m1->Operand[2]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMM: 2b */
					getCodeMReg2b(str, p->Operand[3]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMI: 2b */
					getCodeIReg2b(str, p->Operand[2]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t04a:
				case t04e:
					/* write opcode: 7b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write MF: 4b */
					getCodeMF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC32S): 2b */
					getCodeACC32S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP12S): 3b */
					getCodeReg12S(str, p->Operand[2]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DDREG/SDREG (DREG12): 6b */
					if(m1->Index == iST){	/* iST */
						getCodeDReg12(str, m1->Operand[4]);
					} else {				/* iLD */
						getCodeDReg12(str, m1->Operand[0]);
					}
					bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
					if(m1->Index == iST){	/* iST */
						getCodeU(str, m1->Operand[0]);
					} else {				/* iLD */
						getCodeU(str, m1->Operand[1]);
					}
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMM: 3b */
					if(m1->Index == iST){	/* iST */
						getCodeMReg3b(str, m1->Operand[2]);
					} else {				/* iLD */
						getCodeMReg3b(str, m1->Operand[3]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMI: 3b */
					if(m1->Index == iST){	/* iST */
						getCodeIReg3b(str, m1->Operand[1]);
					} else {				/* iLD */
						getCodeIReg3b(str, m1->Operand[2]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t04b:
				case t04f:
					/* write opcode: 10b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write MF: 4b */
					getCodeMF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC64S): 2b */
					getCodeACC64S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP24S): 2b */
					getCodeReg24S(str, p->Operand[1]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP24S): 2b */
					getCodeReg24S(str, p->Operand[2]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DDREG/SDREG (DREG24): 5b */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeDReg24(str, m1->Operand[4]);
					} else {				/* iLD_C */
						getCodeDReg24(str, m1->Operand[0]);
					}
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeU(str, m1->Operand[0]);
					} else {				/* iLD_C */
						getCodeU(str, m1->Operand[1]);
					}
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMM: 3b */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeMReg3b(str, m1->Operand[2]);
					} else {				/* iLD_C */
						getCodeMReg3b(str, m1->Operand[3]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMI: 3b */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeIReg3b(str, m1->Operand[1]);
					} else {				/* iLD_C */
						getCodeIReg3b(str, m1->Operand[2]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t04c:
				case t04g:
					/* write opcode: 5b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write AF: 4b */
					getCodeAF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (DREG12S): 4b */
					getCodeDReg12S(str, p->Operand[0]);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP12S): 3b */
					if(p->Operand[2]){
						getCodeReg12S(str, p->Operand[2]);
					}else{
						strcpy(str, "000");
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DDREG/SDREG (DREG12): 6b */
					if(m1->Index == iST){	/* iST */
						getCodeDReg12(str, m1->Operand[4]);
					} else {				/* iLD */
						getCodeDReg12(str, m1->Operand[0]);
					}
					bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
					if(m1->Index == iST){	/* iST */
						getCodeU(str, m1->Operand[0]);
					} else {				/* iLD */
						getCodeU(str, m1->Operand[1]);
					}
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMM: 3b */
					if(m1->Index == iST){	/* iST */
						getCodeMReg3b(str, m1->Operand[2]);
					} else {				/* iLD */
						getCodeMReg3b(str, m1->Operand[3]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMI: 3b */
					if(m1->Index == iST){	/* iST */
						getCodeIReg3b(str, m1->Operand[1]);
					} else {				/* iLD */
						getCodeIReg3b(str, m1->Operand[2]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t04d:
				case t04h:
					/* write opcode: 9b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write AF: 4b */
					getCodeAF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (DREG24S): 3b */
					getCodeDReg24S(str, p->Operand[0]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP24S): 2b */
					getCodeReg24S(str, p->Operand[1]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP24S): 2b */
					if(p->Operand[2]){
						getCodeReg24S(str, p->Operand[2]);
					}else{
						strcpy(str, "00");
					}
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DDREG/SDREG (DREG24): 5b */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeDReg24(str, m1->Operand[4]);
					} else {				/* iLD_C */
						getCodeDReg24(str, m1->Operand[0]);
					}
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeU(str, m1->Operand[0]);
					} else {				/* iLD_C */
						getCodeU(str, m1->Operand[1]);
					}
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMM: 3b */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeMReg3b(str, m1->Operand[2]);
					} else {				/* iLD_C */
						getCodeMReg3b(str, m1->Operand[3]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMI: 3b */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeIReg3b(str, m1->Operand[1]);
					} else {				/* iLD_C */
						getCodeIReg3b(str, m1->Operand[2]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t08a:
					/* write opcode: 8b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write MF: 4b */
					getCodeMF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC32S): 2b */
					getCodeACC32S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP12S): 3b */
					getCodeReg12S(str, p->Operand[2]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DDREG (DREG12): 6b */
					getCodeDReg12(str, m1->Operand[0]);
					bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SDREG (DREG12): 6b */
					getCodeDReg12(str, m1->Operand[1]);
					bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t08b:
					/* write opcode: 12b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write MF: 4b */
					getCodeMF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC64S): 1b */
					getCodeACC64S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP24S): 2b */
					getCodeReg24S(str, p->Operand[1]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP24S): 2b */
					getCodeReg24S(str, p->Operand[2]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DDREG (DREG24): 5b */
					getCodeDReg24(str, m1->Operand[0]);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SDREG (DREG24): 5b */
					getCodeDReg24(str, m1->Operand[1]);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t08c:
					/* write opcode: 6b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write AF: 4b */
					getCodeAF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (DREG12S): 4b */
					getCodeDReg12S(str, p->Operand[0]);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP12S): 3b */
					if(p->Operand[2]){
						getCodeReg12S(str, p->Operand[2]);
					}else{
						strcpy(str, "000");
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DDREG (DREG12): 6b */
					getCodeDReg12(str, m1->Operand[0]);
					bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SDREG (DREG12): 6b */
					getCodeDReg12(str, m1->Operand[1]);
					bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t08d:
					/* write opcode: 11b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write AF: 4b */
					getCodeAF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (DREG24S): 3b */
					getCodeDReg24S(str, p->Operand[0]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP24S): 2b */
					getCodeReg24S(str, p->Operand[1]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP24S): 2b */
					if(p->Operand[2]){
						getCodeReg24S(str, p->Operand[2]);
					}else{
						strcpy(str, "00");
					}
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DDREG (DREG24): 5b */
					getCodeDReg24(str, m1->Operand[0]);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SDREG (DREG24): 5b */
					getCodeDReg24(str, m1->Operand[1]);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t12e:
				case t12g:
					/* write opcode: 5b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SF2: 4b */
					getCodeSF2(str, p->Operand[3], p->Index);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC32S): 2b */
					getCodeACC32S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (IMM_INT5): 5b */
					int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 5);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DSTLS/SRCLS (DREG12): 6b */
					if(m1->Index == iST){	/* iST */
						getCodeDReg12(str, m1->Operand[4]);
					} else {				/* iLD */
						getCodeDReg12(str, m1->Operand[0]);
					}
					bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
					if(m1->Index == iST){	/* iST */
						getCodeU(str, m1->Operand[0]);
					} else {				/* iLD */
						getCodeU(str, m1->Operand[1]);
					}
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMM: 3b */
					if(m1->Index == iST){	/* iST */
						getCodeMReg3b(str, m1->Operand[2]);
					} else {				/* iLD */
						getCodeMReg3b(str, m1->Operand[3]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMI: 3b */
					if(m1->Index == iST){	/* iST */
						getCodeIReg3b(str, m1->Operand[1]);
					} else {				/* iLD */
						getCodeIReg3b(str, m1->Operand[2]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t12f:
				case t12h:
					/* write opcode: 7b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SF2: 4b */
					getCodeSF2(str, p->Operand[3], p->Index);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC64S): 2b */
					getCodeACC64S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP24S): 2b */
					getCodeReg24S(str, p->Operand[1]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (IMM_INT5): 5b */
					int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 5);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DSTLS/SRCLS (DREG24): 5b */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeDReg24(str, m1->Operand[4]);
					} else {				/* iLD_C */
						getCodeDReg24(str, m1->Operand[0]);
					}
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeU(str, m1->Operand[0]);
					} else {				/* iLD_C */
						getCodeU(str, m1->Operand[1]);
					}
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMM: 3b */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeMReg3b(str, m1->Operand[2]);
					} else {				/* iLD_C */
						getCodeMReg3b(str, m1->Operand[3]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMI: 3b */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeIReg3b(str, m1->Operand[1]);
					} else {				/* iLD_C */
						getCodeIReg3b(str, m1->Operand[2]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t12m:
				case t12o:
					/* write opcode: 6b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SF2: 4b */
					getCodeSF2(str, p->Operand[3], p->Index);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC32S): 2b */
					getCodeACC32S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (ACC32S): 2b */
					getCodeACC32S(str, p->Operand[1]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (IMM_INT5): 5b */
					int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 5);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DSTLS/SRCLS (DREG12): 6b */
					if(m1->Index == iST){	/* iST */
						getCodeDReg12(str, m1->Operand[4]);
					} else {				/* iLD */
						getCodeDReg12(str, m1->Operand[0]);
					}
					bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
					if(m1->Index == iST){	/* iST */
						getCodeU(str, m1->Operand[0]);
					} else {				/* iLD */
						getCodeU(str, m1->Operand[1]);
					}
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMM: 3b */
					if(m1->Index == iST){	/* iST */
						getCodeMReg3b(str, m1->Operand[2]);
					} else {				/* iLD */
						getCodeMReg3b(str, m1->Operand[3]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMI: 3b */
					if(m1->Index == iST){	/* iST */
						getCodeIReg3b(str, m1->Operand[1]);
					} else {				/* iLD */
						getCodeIReg3b(str, m1->Operand[2]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t12n:
				case t12p:
					/* write opcode: 7b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SF2: 4b */
					getCodeSF2(str, p->Operand[3], p->Index);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC64S): 2b */
					getCodeACC64S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (ACC64S): 2b */
					getCodeACC64S(str, p->Operand[1]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (IMM_INT5): 5b */
					int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 5);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DSTLS/SRCLS (DREG24): 5b */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeDReg24(str, m1->Operand[4]);
					} else {				/* iLD_C */
						getCodeDReg24(str, m1->Operand[0]);
					}
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeU(str, m1->Operand[0]);
					} else {				/* iLD_C */
						getCodeU(str, m1->Operand[1]);
					}
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMM: 3b */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeMReg3b(str, m1->Operand[2]);
					} else {				/* iLD_C */
						getCodeMReg3b(str, m1->Operand[3]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMI: 3b */
					if(m1->Index == iST_C){	/* iST_C */
						getCodeIReg3b(str, m1->Operand[1]);
					} else {				/* iLD_C */
						getCodeIReg3b(str, m1->Operand[2]);
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t14c:
					/* write opcode: 6b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SF2: 4b */
					getCodeSF2(str, p->Operand[3], p->Index);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC32S): 2b */
					getCodeACC32S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (IMM_INT5): 5b */
					int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 5);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DDREG (DREG12): 6b */
					getCodeDReg12(str, m1->Operand[0]);
					bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SDREG (DREG12): 6b */
					getCodeDReg12(str, m1->Operand[1]);
					bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t14d:
					/* write opcode: 9b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SF2: 4b */
					getCodeSF2(str, p->Operand[3], p->Index);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC64S): 2b */
					getCodeACC64S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP24S): 2b */
					getCodeReg24S(str, p->Operand[1]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (IMM_INT5): 5b */
					int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 5);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DDREG (DREG24): 5b */
					getCodeDReg24(str, m1->Operand[0]);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SDREG (DREG24): 5b */
					getCodeDReg24(str, m1->Operand[1]);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t14k:
					/* write opcode: 7b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SF2: 4b */
					getCodeSF2(str, p->Operand[3], p->Index);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC32S): 2b */
					getCodeACC32S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (ACC32S): 2b */
					getCodeACC32S(str, p->Operand[1]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (IMM_INT5): 5b */
					int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 5);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DDREG (DREG12): 6b */
					getCodeDReg12(str, m1->Operand[0]);
					bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SDREG (DREG12): 6b */
					getCodeDReg12(str, m1->Operand[1]);
					bitwrite(p, str, 6, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t14l:
					/* write opcode: 9b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SF2: 4b */
					getCodeSF2(str, p->Operand[3], p->Index);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC64S): 2b */
					getCodeACC64S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (ACC64S): 2b */
					getCodeACC64S(str, p->Operand[1]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (IMM_INT5): 5b */
					int2bin(str, getIntSymAddr(p, symTable, p->Operand[2]), 5);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DDREG (DREG24): 5b */
					getCodeDReg24(str, m1->Operand[0]);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SDREG (DREG24): 5b */
					getCodeDReg24(str, m1->Operand[1]);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t43a:
					/* write opcode: 6b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write AF: 4b */
					getCodeAF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write MF: 4b */
					getCodeMF(str, m1);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (DREG12S): 4b */
					getCodeDReg12S(str, p->Operand[0]);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP12S): 3b */
					if(p->Operand[2]){
						getCodeReg12S(str, p->Operand[2]);
					}else{
						strcpy(str, "000");
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST2 (ACC32S): 2b */
					getCodeACC32S(str, m1->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC3 (XOP12S): 3b */
					getCodeReg12S(str, m1->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC4 (YOP12S): 3b */
					getCodeReg12S(str, m1->Operand[2]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t44b:
					/* write opcode: 4b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SF2: 4b */
					getCodeSF2(str, m1->Operand[3], m1->Index);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write AF: 4b */
					getCodeAF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (DREG12S): 4b */
					getCodeDReg12S(str, p->Operand[0]);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP12S): 3b */
					if(p->Operand[2]){
						getCodeReg12S(str, p->Operand[2]);
					}else{
						strcpy(str, "000");
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST2 (ACC32S): 2b */
					getCodeACC32S(str, m1->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC3 (XOP12S): 3b */
					getCodeReg12S(str, m1->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC4 (IMM_INT5): 5b */
					int2bin(str, getIntSymAddr(p, symTable, m1->Operand[2]), 5);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t44d:
					/* write opcode: 5b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SF2: 4b */
					getCodeSF2(str, m1->Operand[3], m1->Index);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write AF: 4b */
					getCodeAF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (DREG12S): 4b */
					getCodeDReg12S(str, p->Operand[0]);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP12S): 3b */
					if(p->Operand[2]){
						getCodeReg12S(str, p->Operand[2]);
					}else{
						strcpy(str, "000");
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST2 (ACC32S): 2b */
					getCodeACC32S(str, m1->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC3 (ACC32S): 2b */
					getCodeACC32S(str, m1->Operand[1]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC4 (IMM_INT5): 5b */
					int2bin(str, getIntSymAddr(p, symTable, m1->Operand[2]), 5);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t45b:
					/* write opcode: 6b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SF2: 4b */
					getCodeSF2(str, m1->Operand[3], m1->Index);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write MF: 4b */
					getCodeMF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC32S): 2b */
					getCodeACC32S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP12S): 3b */
					getCodeReg12S(str, p->Operand[2]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST2 (ACC32S): 2b */
					getCodeACC32S(str, m1->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC3 (XOP12S): 3b */
					getCodeReg12S(str, m1->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC4 (IMM_INT5): 5b */
					int2bin(str, getIntSymAddr(p, symTable, m1->Operand[2]), 5);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t45d:
					/* write opcode: 7b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SF2: 4b */
					getCodeSF2(str, m1->Operand[3], m1->Index);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write MF: 4b */
					getCodeMF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC32S): 2b */
					getCodeACC32S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP12S): 3b */
					getCodeReg12S(str, p->Operand[2]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST2 (ACC32S): 2b */
					getCodeACC32S(str, m1->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC3 (ACC32S): 2b */
					getCodeACC32S(str, m1->Operand[1]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC4 (IMM_INT5): 5b */
					int2bin(str, getIntSymAddr(p, symTable, m1->Operand[2]), 5);
					bitwrite(p, str, 5, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				default:
                	bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
			}
			break;
		case 2:			/* three-opcode multifunction */
			m1 = p->Multi[0];
			m2 = p->Multi[1];

			switch(p->InstType){
				case t01a:		/* MAC || LD || LD */
					/* write opcode: 4b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write MF: 4b */
					getCodeMF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC32S): 2b */
					getCodeACC32S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP12S): 3b */
					getCodeReg12S(str, p->Operand[2]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
					getCodeU(str, m1->Operand[1]);
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U2: 1b */		/* 0 for premodify, 1 for postmodify */
					getCodeU(str, m2->Operand[1]);
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DD (XOP12S): 3b */
					getCodeReg12S(str, m1->Operand[0]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write PD (YOP12S): 3b */
					getCodeReg12S(str, m2->Operand[0]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write PMM: 2b */
					getCodeMReg2b(str, m2->Operand[3]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write PMI: 2b */
					getCodeIReg2b(str, m2->Operand[2]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMM: 2b */
					getCodeMReg2b(str, m1->Operand[3]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMI: 2b */
					getCodeIReg2b(str, m1->Operand[2]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t01b:		/* MAC.C || LD.C || LD.C */
					/* write opcode: 8b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write MF: 4b */
					getCodeMF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DST1 (ACC64S): 2b */
					getCodeACC64S(str, p->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP24S): 2b */
					getCodeReg24S(str, p->Operand[1]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP24S): 2b */
					getCodeReg24S(str, p->Operand[2]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U: 1b */		/* 0 for premodify, 1 for postmodify */
					getCodeU(str, m1->Operand[1]);
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write U2: 1b */		/* 0 for premodify, 1 for postmodify */
					getCodeU(str, m2->Operand[1]);
					bitwrite(p, str, 1, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DD (XOP24S): 2b */
					getCodeReg24S(str, m1->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write PD (YOP24S): 2b */
					getCodeReg24S(str, m2->Operand[0]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write PMM: 2b */
					getCodeMReg2b(str, m2->Operand[3]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write PMI: 2b */
					getCodeIReg2b(str, m2->Operand[2]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMM: 2b */
					getCodeMReg2b(str, m1->Operand[3]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMI: 2b */
					getCodeIReg2b(str, m1->Operand[2]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				case t01c:		/* ALU || LD || LD */
					/* write opcode: 4b */
					bitwrite(p, sBinOp[p->InstType].bits, sBinOp[p->InstType].width, 
						bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write AF: 4b */
					getCodeAF(str, p);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DSTA (DREG12S): 4b */
					getCodeDReg12S(str, p->Operand[0]);
					bitwrite(p, str, 4, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC1 (XOP12S): 3b */
					getCodeReg12S(str, p->Operand[1]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write SRC2 (YOP12S): 3b */
					if(p->Operand[2]){
						getCodeReg12S(str, p->Operand[2]);
					}else{
						strcpy(str, "000");
					}
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DD (XOP12S): 3b */
					getCodeReg12S(str, m1->Operand[0]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write PD (YOP12S): 3b */
					getCodeReg12S(str, m2->Operand[0]);
					bitwrite(p, str, 3, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write PMM: 2b */
					getCodeMReg2b(str, m2->Operand[3]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write PMI: 2b */
					getCodeIReg2b(str, m2->Operand[2]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMM: 2b */
					getCodeMReg2b(str, m1->Operand[3]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					/* write DMI: 2b */
					getCodeIReg2b(str, m1->Operand[2]);
					bitwrite(p, str, 2, bytebuf, bitbuf, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
				default:
                	bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
					break;
			}
			break;
		default:
			if(!isCommentLabelInst(p->Index))
                bitwriteError(p, dumpBinFP, dumpTxtFP, dumpMemFP);
			break;
	}
}

/** 
* @brief Write n bits of binary data to binary and text files.
* 
* @param p Pointer to current instruction
* @param s Bitstream in string format
* @param n Number of bits to write
* @param byteb Character array to append input string s, where 1 char is binary 1 bit 
* @param bitb Character array, where 1 bit is binary 1 bit
* @param bFP File pointer to binary code dump file 
* @param tFP File pointer to text code dump file (= object file)
* @param mFP File pointer to mem code dump file (= .mem file)
*/
void bitwrite(sICode *p, char *s, int n, char byteb[], char bitb[], FILE *bFP, FILE *tFP, FILE *mFP)
{
	int i;
	char sMemoryRefType[10];

	if(!bitPos){
		if(tFP){
			fprintf(tFP,"%04d %04X: ", p->LineCntr, p->PMA);
		}

		if(mFP){
			fprintf(mFP,"%x/ ", p->PMA);
		}
	}

	if(bFP){
		for(i = 0; i < n; i++){
			byteb[bitPos++] = s[i];
		}
	}

	if(tFP){
		for(i = 0; i < n; i++){
			fprintf(tFP, "%d", s[i] - '0');
		}
	}

	if(bitPos == INSTLEN){
		packbuf(byteb, bitb);

		//fwrite(bitb, sizeof(bitb), 1, bFP);
		fwrite(bitb, 4, 1, bFP);

		bitPos = 0;	/* reset counter */

		//print .obj
		fprintf(tFP," %02X %02X %02X %02X", (unsigned char)bitb[0], 
			(unsigned char)bitb[1], (unsigned char)bitb[2], (unsigned char)bitb[3]);
		fprintf(tFP,"  %s %-10s", sType[p->InstType], sOp[p->Index]);

		sICodeListPrintType(sMemoryRefType, p->MemoryRefType);
		fprintf(tFP," %s", sMemoryRefType);
	
		if(p->MemoryRefType == tREL){
			fprintf(tFP," %04X %d %d", 0xFFFF & relAddr, relPos, relBitWidth);
		}

		fprintf(tFP,"\n");

		//print .mem
		fprintf(mFP,"%02x%02x%02x%02x\n", (unsigned char)bitb[0], (unsigned char)bitb[1], (unsigned char)bitb[2], (unsigned char)bitb[3]);

	}else if(bitPos > INSTLEN){
		assert(bitPos <= INSTLEN);
	}
}

/** 
* @brief Report bitwrite error to the text & binary dump file.
* 
* @param p Pointer to current instruction
* @param bFP File pointer to binary code dump file 
* @param tFP File pointer to text code dump file (= object file)
* @param mFP File pointer to mem code dump file (= .mem file)
*/
void bitwriteError(sICode *p, FILE *bFP, FILE *tFP, FILE *mFP)
{
	int i;

	if(bFP){
		char bitb[INSTLEN/8] = { 0, 0, 0, 0 };
		fwrite(bitb, 4, 1, bFP);
	}

	if(tFP){
		fprintf(tFP,"%04d %04X: ", p->LineCntr, p->PMA);
		fprintf(tFP, "--------------------------------");
		fprintf(tFP, " XX XX XX XX");
		fprintf(tFP,"  %s %-10s\n", sType[p->InstType], sOp[p->Index]);
	}

	if(mFP){
		fprintf(mFP,"%x/ ", p->PMA);
		fprintf(mFP, "XXXXXXXX\n");
	}

	printf("line %d: Error: Cannot assemble this instruction (Opcode: %s)\n",
		p->LineCntr, sOp[p->Index]);
	AssemblerError++;
}

/** 
* @brief Pack INSTLEN bytes into INSTLEN bits
* 
* @param byteb Input character array, where 1 char is binary 1 bit 
* @param bitb Output character array, where 1 bit is binary 1 bit
*/
void packbuf(char byteb[], char bitb[])
{
	int i, j;
	char c;

	for(i = 0; i < (INSTLEN/8); i++){
		c = 0;
		for(j = 0; j < 8; j++){
			if(byteb[i*8 + j] == '1') c |= 0x1;
			if(j != 7) c <<= 1;
		}
		bitb[i] = c;
	}
}

/** 
* @brief Check if this instruction is one of pseudo instructions
* 
* @param index Instruction index
* 
* @return 1 if pseudo instruction, 0 if not
*/
int isPseudoInst(unsigned int index)
{
	int result = FALSE;

	switch(index){
		case i_CODE:
		case i_DATA:
		case i_GLOBAL:
		case i_EXTERN:
		case i_VAR:
		case i_EQU:
			result = TRUE;
			break;
		default:
			break;
	}
	return result;
}

/** 
* @brief Check if this instruction is comment or label
* 
* @param index Instruction index
* 
* @return 1 if pseudo instruction, 0 if not
*/
int isCommentLabelInst(unsigned int index)
{
	int result = FALSE;

	switch(index){
		case iCOMMENT:
		case iLABEL:
			result = TRUE;
			break;
		default:
			break;
	}
	return result;
}

/** 
* @brief Check if this instruction is one of pseudo inst. or comment or label
* 
* @param index Instruction index
* 
* @return 1 if pseudo instruction, 0 if not
*/
int isNotRealInst(unsigned int index)
{
	int result = FALSE;

	if(isPseudoInst(index) || isCommentLabelInst(index))
		result = TRUE;

	return result;
}


/** 
* @brief Skip pseudo inst./comment/label and return next instruction
* 
* @param p Pointer to current instruction
* 
* @return Pointer to next 'real' instruction
*/
sICode *getNextCode(sICode *p)
{
	p = p->Next;

	if(p != NULL){
		while(p && isNotRealInst(p->Index)){
			p = p->Next;
		}
	}
	return p;
}
