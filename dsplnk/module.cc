/*
All Rights Reserved.
*/
/** 
* @file module.cc
* Module information (sModule) linked list implementation
*
* @date 2008-12-16
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "dsplnk.h"
#include "module.h"

/** 
* @brief Allocate a node dynamically.
* 
* @param name Pointer to segment name string
* @param num Module index number
* 
* @return Returns pointer to newly allocated node.
*/
sModule *sModuleGetNode(char *name, int num)
{
	sModule *p;
	p = (sModule *)calloc(1, sizeof(sModule));
	assert(p != NULL);

	p->Name = strdup(name);
	p->ModuleNo = num;

	return p;
}

/** 
* Check for data duplication and add new node to the END(LastNode) of the list.
* * Note: Before adding first node (empty list), FirstNode MUST be NULL.
* 
* @param list Pointer to linked list
* @param name Pointer to segment name string
* @param num Module index number
* 
* @return Returns pointer to newly allocated node.
*/
sModule *sModuleListAdd(sModuleList *list, char *name, int num)
{
	sModule *n = sModuleGetNode(name, num);
	if(n == NULL) return NULL;

	if(list->FirstNode != NULL){ /* if FistNode is not NULL, 
										LastNode is also not NULL */
		list->LastNode = sModuleListInsertAfter(list->LastNode, n);
	} else {	/* FirstNode == NULL (list empty) */
		list->LastNode = sModuleListInsertBeginning(list, n); 
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
sModule *sModuleListInsertAfter(sModule *p, sModule *newNode)	
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
sModule *sModuleListInsertBeginning(sModuleList *list, sModule *newNode)	
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
void sModuleListRemoveAfter(sModuleList *list, sModule *p)	
{
	if(p != NULL){
		if(p->Next != NULL){
			sModule *obsoleteNode = p->Next;
			p->Next = p->Next->Next;

			if(obsoleteNode->Next == NULL){	/* if it is LastNode */
				list->LastNode = p;		/* update LastNode */
			}
			/* free node */
			if(obsoleteNode->Name) free(obsoleteNode->Name);
			if(obsoleteNode->ObjFP) fclose(obsoleteNode->ObjFP);
			if(obsoleteNode->SymFP) fclose(obsoleteNode->SymFP);
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
sModule *sModuleListRemoveBeginning(sModuleList *list)	
{
	if(list != NULL){
		if(list->FirstNode != NULL){
			sModule *obsoleteNode = list->FirstNode;
			list->FirstNode = list->FirstNode->Next;
			if(list->FirstNode == NULL) list->LastNode = NULL; /* removed last node */

			/* free node */
			if(obsoleteNode->Name) free(obsoleteNode->Name);
			if(obsoleteNode->ObjFP) fclose(obsoleteNode->ObjFP);
			if(obsoleteNode->SymFP) fclose(obsoleteNode->SymFP);
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
void sModuleListRemoveAll(sModuleList *list)
{
	while(list->FirstNode != NULL){
		sModuleListRemoveBeginning(list);
	}
}

/** 
* @brief Search entire list matching given data value.
* 
* @param list Pointer to linked list
* @param name Pointer to segment name string
* 
* @return Returns pointer to the matched node or NULL.
*/
sModule *sModuleListSearch(sModuleList *list, char *name)
{
	sModule *n = list->FirstNode;

	while(n != NULL) {
		if(!strcasecmp(n->Name, name)){		/* search for same name */
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
void sModuleListPrint(sModuleList *list)
{
	sModule *n = list->FirstNode;
	char smod[10];

	/*
	printf("\n-------------------------------\n");
	printf("** DUMP: Module List ** \n");
	printf("-------------------------------\n");

	if(n == NULL){
		printf("list is empty\n");
	}

	if(n != NULL){
		printf("Node Ptr. Next Ptr. Type Name     Size\n");
	}

	while(n != NULL){
		sModuleListPrintType(smod, n->Type);

		if(n->Next)
			printf("%p %p %s %-8s %d", n, n->Next, smod, n->Name, n->Size);
		else
			printf("%p (NULL)    %s %-8s %d", n, smod, n->Name, n->Size);
		printf("\n");

		n = n->Next;
	}
	printf("F %p L %p\n", list->FirstNode, list->LastNode);
	*/
}


/** 
* @brief Initialize moduleList
* 
* @param list sModuleList data structure
*/
void moduleListInit(sModuleList *list)
{
	list->FirstNode = NULL;
	list->LastNode  = NULL;
}

/** 
* @brief Build module list from .sym file(s)
* 
* @param list sModuleList data structure
*/
void moduleListBuild(sModuleList *list)
{
	char linebuf[MAX_LINEBUF];
	int tabID;
	char type[10];
	char name[10];
	int sz;
	int codeOffset = 0;
	int dataOffset = 0;

	if(VerboseMode) printf("\n.sym file reading at moduleListBuild().\n");

	sModule *n = list->FirstNode;
	while(n != NULL){
		if(VerboseMode) 
			printf("-- File %0d: %s --\n", n->ModuleNo, n->Name);

		if(n == list->FirstNode){
			dataOffset = dataSegAddr;
			codeOffset = codeSegAddr;
		}

		/* process each .sym file */
		while(fgets(linebuf, MAX_LINEBUF, n->SymFP)){
			if(linebuf[0] == ';') continue;
			sscanf(linebuf, "%d ", &tabID);

			if(tabID == tSecInfo){
				sscanf(linebuf, "%d %s %s %d\n", &tabID, type, name, &sz);
				if(VerboseMode) printf("type: %s, name: %s\n", type, name);

				if(!strcasecmp(name, "DATA")){
					n->DataSize = sz;
					n->DataSegAddr = dataOffset;
				} else if(!strcasecmp(name, "CODE")){
					n->CodeSize = sz;
					n->CodeSegAddr = codeOffset;
				}
			}else{
				if(VerboseMode) printf("File %d: %s %d(@0x%04X) %d(@0x%04X)\n", 
					n->ModuleNo, n->Name, n->DataSize, n->DataSegAddr, 
					n->CodeSize, n->CodeSegAddr);	
				break;	/* move to next .sym file */
			}
		};

		/* move to next .sym file */
		dataOffset = n->DataSize + n->DataSegAddr;
		codeOffset = n->CodeSize + n->CodeSegAddr;

		n = n->Next;
	}

	/* calculate total code & data sizes */
	totalDataSize = dataOffset - dataSegAddr;
	totalCodeSize = codeOffset - codeSegAddr;
	if(VerboseMode){
		printf("Total Data Size: %d words\n", totalDataSize);
		printf("Total Code Size: %d words\n", totalCodeSize);
	}
}

/** 
* @brief Dump module data structure into .map & .ldi files
* 
* @param list Pointer to linked list
*/
void sModuleDump(sModuleList *list)
{
    /* dump module list into .map */
	sModule *n = list->FirstNode;

    fprintf(MapFP, ";-----------------------------------------------------------\n");
    fprintf(MapFP, "; Input: Object Module Information (.obj)\n");
    fprintf(MapFP, ";-----------------------------------------------------------\n");
	fprintf(MapFP, "\n");
	fprintf(MapFP, "Data Segment Base Address: 0x%04X\n", dataSegAddr);
	fprintf(MapFP, "Code Segment Base Address: 0x%04X\n", codeSegAddr);
	fprintf(MapFP, "\n");

    fprintf(MapFP, "ID Name Data(Addr) Code(Addr) \n");
	while(n != NULL){
		fprintf(MapFP, "%d %s %d(@0x%04X) %d(@0x%04X)\n", n->ModuleNo, n->Name, 
			n->DataSize, n->DataSegAddr, n->CodeSize, n->CodeSegAddr);
		n = n->Next;
	}
	fprintf(MapFP, "\n");
    fprintf(MapFP, "Total Data Segment Size: %d\n", totalDataSize);
    fprintf(MapFP, "Total Code Segment Size: %d\n", totalCodeSize);
	fprintf(MapFP, "\n");

	/* dump loading info into .ldi */
	n = list->FirstNode;

    fprintf(LdiFP, ";-----------------------------------------------------------\n");
    fprintf(LdiFP, "; Output: Loading Information for each Segment\n");
    fprintf(LdiFP, ";-----------------------------------------------------------\n");
    fprintf(LdiFP, "; DataAddr DataSize CodeAddr CodeSize\n");
	fprintf(LdiFP, "%04X %d %04X %d\n", 
		dataSegAddr, totalDataSize, codeSegAddr, totalCodeSize);
	fprintf(LdiFP, "\n");
}


