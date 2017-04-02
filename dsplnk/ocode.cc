/*
All Rights Reserved.
*/
/** 
* @file ocode.cc
* Object code (sOCode) linked list 
*
* @date 2008-12-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "dsplnk.h"
#include "ocode.h"
#include "symtab.h"	
#include "typetab.h"	/* for eType, getType() */

int bitPos;		/* for use in bitwrite() and sOCodeBinDump() */
int relAddr, relPos, relBitWidth;	/* for use with t03a, t03c, t03d, t03e 
									   in sOCodeBinDump() */

/** 
* @brief Allocate a node dynamically.
* 
* @param a Program Memory Address for new node
* @param ln Assembly source line number (optional) for new node
* @param m Pointer to module this object code belongs to
* 
* @return Returns pointer to newly allocated node.
*/
sOCode *sOCodeGetNode(unsigned int a, int ln, sModule *m)
{
	sOCode *p;
	p = (sOCode *)calloc(1, sizeof(sOCode));
	assert(p != NULL);

	p->PMA = a;
	p->LineCntr = ln;
	p->Module = m;

	return p;
}

/** 
* Check for data duplication and add new node to the END(LastNode) of the list.
* * Note: Before adding first node (empty list), FirstNode MUST be NULL.
* 
* @param list Pointer to linked list
* @param a Program Memory Address for new node
* @param ln Assembly source line number (optional) for new node
* @param m Pointer to module this object code belongs to
* 
* @return Returns pointer to newly allocated node.
*/
sOCode *sOCodeListAdd(sOCodeList *list, unsigned int a, int ln, sModule *m)
{
	sOCode *n = sOCodeGetNode(a, ln, m);
	if(n == NULL) return NULL;

	/** if only assembly input */
	n->LineCntr = ln;

	if(list->FirstNode != NULL){ /* if FistNode is not NULL, 
										LastNode is also not NULL */
		list->LastNode = sOCodeListInsertAfter(list->LastNode, n);
	} else {	/* FirstNode == NULL (list empty) */
		list->LastNode = sOCodeListInsertBeginning(list, n); 
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
sOCode *sOCodeListInsertAfter(sOCode *p, sOCode *newNode)	
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
sOCode *sOCodeListInsertBeginning(sOCodeList *list, sOCode *newNode)	
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
void sOCodeListRemoveAfter(sOCodeList *list, sOCode *p)	
{
	if(p != NULL){
		if(p->Next != NULL){
			sOCode *obsoleteNode = p->Next;
			p->Next = p->Next->Next;

			if(obsoleteNode->Next == NULL){	/* if it is LastNode */
				list->LastNode = p;		/* update LastNode */
			}
			/* free node */
			//if(obsoleteNode->Line) free(obsoleteNode->Line);
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
sOCode *sOCodeListRemoveBeginning(sOCodeList *list)	
{
	if(list != NULL){
		if(list->FirstNode != NULL){
			sOCode *obsoleteNode = list->FirstNode;
			list->FirstNode = list->FirstNode->Next;
			if(list->FirstNode == NULL) list->LastNode = NULL; /* removed last node */

			/* free node */
			//if(obsoleteNode->Line) free(obsoleteNode->Line);
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
void sOCodeListRemoveAll(sOCodeList *list)
{
	while(list->FirstNode != NULL){
		sOCodeListRemoveBeginning(list);
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
sOCode *sOCodeListSearch(sOCodeList *list, unsigned int a)
{
	sOCode *n = list->FirstNode;

	while(n != NULL) {
		if(n->PMA == a){
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
void sOCodeListPrint(sOCodeList *list)
{
	sOCode *n = list->FirstNode;
	sOCode *m;

	char sPMA[10];
	char sDMA[10];
	char sSecType[10];
	char sMemoryRefType[10];

	/*
	if(n == NULL){
		printf("list is empty\n");
	}

	if(n != NULL){
		printf("Node Ptr. Next Ptr. Line PMA  DMA  Mem Seg. Type Opcode(+Operands, IF-Cond) [Label]\n");
	}
	*/

	while(n != NULL){

		sprintf(sPMA, "%04X", n->PMA);
		//sprintf(sDMA, "%04X", n->DMA);

		sOCodeListPrintType(sMemoryRefType, n->MemoryRefType);

		/*
		if(n->Sec)
			sprintf(sSecType, "%s", n->Sec->Name);
		else
			strcpy(sSecType, "----");

		if(n->Next)
			printf("%p %p %04d %s %s %s %s %s %s", n, n->Next, n->LineCntr, sPMA, sDMA, sMemoryRefType,
				sSecType, sType[n->InstType], sOp[n->Index]);
		else
			printf("%p (NULL)    %04d %s %s %s %s %s %s", n, n->LineCntr, sPMA, sDMA, sMemoryRefType,
				sSecType, sType[n->InstType], sOp[n->Index]);
		if(n->OperandCounter){
			for(int i = 0; i < n->OperandCounter; i++)
				printf(" %s", n->Operand[i]);
		}
		if(n->Conj) printf(" *");
		if(n->Cond) printf(" IF-%s", n->Cond);
		if(n->Label){
			printf(" [%s]", n->Label->Name);
		}

		if(n->MultiCounter){
			for(int j = 0; j < n->MultiCounter; j++){
				printf(" ||\n");

				m = n->Multi[j];
				printf("%p (NULL)    %04d %04X %s %s", m, m->LineCntr, m->PMA, 
					sType[m->InstType], sOp[m->Index]);
				if(m->OperandCounter){
					for(int i = 0; i < m->OperandCounter; i++)
						printf(" %s", m->Operand[i]);
				}
			}
		}
		printf("\n");
		*/

		/* Input text line */
		/*
		if(VerboseMode){
			if(n->Line)
				printf("> %s\n", n->Line);
			printf("\n");
		}
		*/
		n = n->Next;
	}
	printf("F %p L %p\n", list->FirstNode, list->LastNode);
}

/**
* @brief Get string for type ("NON ", "ABS", or "REL")
*
* @param s Pointer to output string buffer
* @param type Type of current symbol table node
*/
void sOCodeListPrintType(char *s, int type)
{
    switch(type){
        case    tNON:
            strcpy(s, "NON");
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
* @brief Initialize oCode
* 
* @param list sOCodeList data structure
*/
void oCodeInit(sOCodeList *list)
{
	list->FirstNode = NULL;
	list->LastNode  = NULL;
}

/** 
* @brief Build global object code list from .obj file(s)
* 
* @param codelist global object code linked lists
* @param modlist input module linked lists
*/
void globalOCodeListBuild(sOCodeList *codelist, sModuleList *modlist)
{
	char linebuf[MAX_LINEBUF];
	int pma, ln, offset, pos, width;
	char sbin[INSTLEN+1];
	unsigned char shex[INSTLEN/8];
	char stype[4];
	char sinst[10];
	char smemref[4];
	
	if(VerboseMode) printf("\n.obj file reading at oCodeListBuild().\n");	

	sModule *n = modlist->FirstNode;
	while(n != NULL){
		if(VerboseMode)
			printf("-- File %0d: %s --\n", n->ModuleNo, n->Name);

		/* process each .obj file */
		while(fgets(linebuf, MAX_LINEBUF, n->ObjFP)){
			if(linebuf[0] == ';') continue;
			
			sscanf(linebuf, "%04d %04X: %s %02X %02X %02X %02X %s %s %s %04X %d %d\n",
				&ln, &pma, sbin, &shex[0], &shex[1], &shex[2], &shex[3], 
				stype, sinst, smemref, &offset, &pos, &width);

			if(VerboseMode){
				printf("PMA: %04X, Line: %d, Bin: %s, Hex: %02X%02X%02X%02X\n", 
					pma, ln, sbin, shex[0], shex[1], shex[2], shex[3]);
				printf("Type: %s, Inst: %s, Mem: %s\n", stype, sinst, smemref);
				if(!strcasecmp(smemref, "REL")){
					printf("Offset: %04X, Pos: %d, Width: %d\n", offset, pos, width);
				}
				printf("\n");
			}

			/* update start address considering each module offset */
			pma += n->CodeSegAddr;

			/* add new code to list */
			sOCode *p = sOCodeListAdd(codelist, pma, ln, n);	
			strcpy(p->BinCode, sbin);
			if(!strcasecmp(smemref, "NON"))
				p->MemoryRefType = tNON;
			else if(!strcasecmp(smemref, "ABS"))
				p->MemoryRefType = tABS;
			else if(!strcasecmp(smemref, "REL"))
				p->MemoryRefType = tREL;
			else if(!strcasecmp(smemref, "---"))
				p->MemoryRefType = tNON;

			p->Offset = offset;
			p->Pos = pos;
			p->Width = width;
			p->InstType = getType(stype);
		}

		/* move to next .obj file */
		n = n->Next;
	}
}

/** 
* @brief Write linker output (.out) binary file and .map text file
* 
* @param list Pointer to linked list
*/
void sOCodeBinDump(sOCodeList *list)
{
	char stype[10];
	char bitb[INSTLEN/4];

	/* dump global object code list */
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	fprintf(MapFP, "; Output: CODE Segment of %s\n", outFile);
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	fprintf(MapFP, "; Addr: Binary: Hex: Mem Offset Position Width: Obj Line\n");

	sOCode *n = list->FirstNode;
		
	while(n != NULL){
		sOCodeListPrintType(stype, n->MemoryRefType);
		fprintf(MapFP, "%04X: %32s: ", n->PMA, n->BinCode);

		packbuf(n->BinCode, bitb);

		/* binary write to .out file */
		fwrite(bitb, 4, 1, OutFP);

		fprintf(MapFP, "%02X %02X %02X %02X: ", (unsigned char)bitb[0], (unsigned char)bitb[1], 
			(unsigned char)bitb[2], (unsigned char) bitb[3]);
		if(n->MemoryRefType == tREL){
			fprintf(MapFP, "%s %04X %02d %02d", stype, n->Offset, n->Pos, n->Width);
		}else{
			fprintf(MapFP, "%s ---- -- --", stype);
		}
		fprintf(MapFP, ": %s %d", n->Module->Name, n->LineCntr); 
		fprintf(MapFP, "\n");

		n = n->Next;
	}
	fprintf(MapFP, "\n");
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
*/
void bitwrite(sOCode *p, char *s, int n, char byteb[], char bitb[], FILE *bFP, FILE *tFP)
{
	int i;
	char sMemoryRefType[10];

	if(!bitPos){
		if(tFP){
			fprintf(tFP,"%04d %04X: ", p->LineCntr, p->PMA);
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

		/*
		fprintf(tFP," %02X %02X %02X %02X", (unsigned char)bitb[0], 
			(unsigned char)bitb[1], (unsigned char)bitb[2], (unsigned char)bitb[3]);
		fprintf(tFP,"  %s %-10s", sType[p->InstType], sOp[p->Index]);

		sOCodeListPrintType(sMemoryRefType, p->MemoryRefType);
		fprintf(tFP," %s", sMemoryRefType);
		*/
	
		if(p->MemoryRefType == tREL){
			fprintf(tFP," %04X %d %d", 0xFFFF & relAddr, relPos, relBitWidth);
		}

		fprintf(tFP,"\n");
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
*/
void bitwriteError(sOCode *p, FILE *bFP, FILE *tFP)
{
	int i;

	if(bFP){
		char bitb[INSTLEN/8] = { 0, 0, 0, 0 };
		fwrite(bitb, 4, 1, bFP);
	}

	/*
	if(tFP){
		fprintf(tFP,"%04d %04X: ", p->LineCntr, p->PMA);
		fprintf(tFP, "--------------------------------");
		fprintf(tFP, " XX XX XX XX");
		fprintf(tFP,"  %s %-10s\n", sType[p->InstType], sOp[p->Index]);
	}

	printf("line %d: Error: Cannot assemble this instruction (Opcode: %s)\n",
		p->LineCntr, sOp[p->Index]);
	*/
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
* Patch every external symbol reference of object code 
* with relocated symbol address.
* 
* @param tablist Global symbol table data structure
* @param extlist External symbol reference table data structure
* @param codelist Object code list data structure
* @param modlist Module list data structure
*/
void patchExternRef(sTabList tablist[], sExtRefTabList extlist[], 
	sOCodeList *codelist, sModuleList *modlist)
{
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	fprintf(MapFP, "; Output: Patched External Memory References\n");
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	if(VerboseMode)
		fprintf(MapFP, "; ID ");
	else
		fprintf(MapFP, "; ");
	fprintf(MapFP, "Addr: Old-offset + Symbol-addr - Base-addr -> New-offset: ");
	fprintf(MapFP, "Name at Module, Line\n");

	for(int i = 0; i < MAX_HASHTABLE; i++){
		if(extlist[i].FirstNode){
			sExtRefTab *ep = extlist[i].FirstNode;

			while(ep != NULL){
				/* for given extRefTable entry */
				/* find matching global symbol table entry */
				sTab *sp = sTabHashSearch(tablist, ep->Name);
				if(sp){		/* found */
					/* find referring object code */
					sOCode *cp = sOCodeListSearch(codelist, ep->Addr);	
					if(cp){
						/* patch the global symbol address to the object code */
						patchOCodeExternRef(cp, sp);
					}else{		/* error */
						/* print error message and exit */
						char linebuf[MAX_LINEBUF];
						sprintf(linebuf, "This case should not happen. Please report.");
						printErrorMsg("patchExternRef()", linebuf);
					}
					break;
				}
				ep = ep->Next;
			}
		}
	}
	if(VerboseMode)
		fprintf(MapFP, "%d\n", tEnd);
	else
		fprintf(MapFP, "\n");
}


/** 
* Patch every non-external symbol reference of object code 
* with relocated symbol address.
* 
* @param tablist Global symbol table data structure
* @param extlist External symbol reference table data structure
* @param codelist Object code list data structure
* @param modlist Module list data structure
*/
void patchOtherRef(sTabList tablist[], sExtRefTabList extlist[], 
	sOCodeList *codelist, sModuleList *modlist)
{
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	fprintf(MapFP, "; Output: Patched Non-External Memory References\n");
	fprintf(MapFP, ";-----------------------------------------------------------\n");
	if(VerboseMode)
		fprintf(MapFP, "; ID ");
	else
		fprintf(MapFP, "; ");
	fprintf(MapFP, "Addr: Old-offset + Base-addr -> New-offset: at Module, Line\n");

	sOCode *n = codelist->FirstNode;
	
	while(n != NULL){
		if(n->MemoryRefType == tREL && !n->IsPatched){
			patchOCodeOtherRef(n);
		}
		n = n->Next;
	}
	if(VerboseMode)
		fprintf(MapFP, "%d\n", tEnd);
	else
		fprintf(MapFP, "\n");
}


/** 
* @brief Apply global symbol address to the external memory reference field of object code.
* 
* @param n Pointer to object code 
* @param sp Pointer to global symbol table entry
*/
void patchOCodeExternRef(sOCode *n, sTab *sp)
{
	int tabID = tPatchExternRef;

	if(VerboseMode)
		fprintf(MapFP, "%d ", tabID);
	fprintf(MapFP, "%04X: ", n->PMA);

	/* update memory reference field */
	switch(n->InstType){
		case t11a:		/* DO <IMM_INT12> UNTIL [<TERM>] */
		case t10a:		/* [IF COND] CALL/JUMP <IMM_INT13> */
		case t10b:		/* CALL/JUMP <IMM_INT16> */
						/* these instructions need to compute
						   (symbol_address - PMA)
						*/
			fprintf(MapFP, "%04X+%04X-%04X -> %04X (ABS)", n->Offset, sp->Addr, 
				n->Module->CodeSegAddr,
				0xFFFF &(n->Offset + sp->Addr - n->Module->CodeSegAddr));
			n->Offset = 0xFFFF & (n->Offset + sp->Addr - n->Module->CodeSegAddr);
			n->MemoryRefType = tABS;	/* now it's absolute offset */
			break;
		default:
			fprintf(MapFP, "%04X+%04X      -> %04X (REL)", n->Offset, sp->Addr, 
				0xFFFF &(n->Offset + sp->Addr));
			n->Offset = 0xFFFF & (n->Offset + sp->Addr);
			break;
	}
	n->IsPatched = TRUE;

	//fprintf(MapFP, "%02d %02d", n->Pos, n->Width);
	fprintf(MapFP, ": %s at %s, Line %d", sp->Name, n->Module->Name, n->LineCntr); 
	fprintf(MapFP, "\n");

	/* more routines to patch binary opcode */
	int2bin(n->BinCode + n->Pos, n->Offset, n->Width);
}

/** 
* @brief Apply segment start address to the non-external memory reference field of object code.
* 
* @param n Pointer to object code 
*/
void patchOCodeOtherRef(sOCode *n)
{
	int tabID = tPatchOtherRef;

	if(VerboseMode)
		fprintf(MapFP, "%d ", tabID);
	fprintf(MapFP, "%04X: ", n->PMA);
	fprintf(MapFP, "%04X+%04X -> %04X", n->Offset, n->Module->DataSegAddr, 
		0xFFFF &(n->Offset + n->Module->DataSegAddr));
	//fprintf(MapFP, "%02d %02d", n->Pos, n->Width);
	fprintf(MapFP, ": at %s, Line %d", n->Module->Name, n->LineCntr); 
	fprintf(MapFP, "\n");

	/* more routines to patch binary opcode */
	;
	/* update memory reference field */
	/* note: data segment symbol is assumed here.
	   Because in current implementation, 
	   non-external code segment symbol cannot come in relative addressing (tREL). */
	n->Offset = 0xFFFF & (n->Offset + n->Module->DataSegAddr);
	n->IsPatched = TRUE;

	/* more routines to patch binary opcode */
	int2bin(n->BinCode + n->Pos, n->Offset, n->Width);
}

/**
* @brief Simulate binary printf function, for example, something like '%b'
*
* @param ret String buffer to write in binary format, e.g. "0000"
* @param val Binary integer number to convert
* @param n Number of output characters
*/
void int2bin(char *ret, int val, int n)
{
    int i;
    int mask = 0x1;

    if(ret){
        /* ret[0]: MSB, ret[n-1]: LSB */
        for(i = 0; i < n; i++){
            if(val & mask){
                ret[n-1-i] = '1';
            }else
                ret[n-1-i] = '0';
            mask <<= 1;
        }
    }else{
        assert(ret != NULL);
    }
}

