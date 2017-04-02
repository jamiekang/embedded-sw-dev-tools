/*
All Rights Reserved.
*/

/** 
* @file main.cc
* @brief Linker for OFDM DSP
* @date 2008-12-09
*/

#include <stdio.h>	/* perror() */
#include <stdlib.h>	/* strtol(), exit() */
#include <string.h>	/* strcpy() */
#include <ctype.h>
#include <time.h>
#include "dsplnk.h"
#include "module.h"
#include "symtab.h"
#include "ocode.h"
#include "extreftab.h"

char linebuf[MAX_LINEBUF];
int lineno = 1;
char msgbuf[MAX_LINEBUF];
char *inFile = NULL;
char outFile[MAX_LINEBUF];

int VerboseMode = FALSE;

FILE *LdfFP = NULL;		/* .ldf */
FILE *OutFP = NULL;		/* .out */
FILE *MapFP = NULL;		/* .map */
FILE *LdiFP = NULL;		/* .ldi */
int ModuleCntr = 0;		/* Number of input modules (= .obj) */

char bytebuf[INSTLEN];		
char bitbuf[INSTLEN/8]; 

int codeSegAddr;		/* start address of code segment; specified by user */
int dataSegAddr;		/* start address of data segment; specified by user */

int totalCodeSize;		/* sum of all code segments */
int totalDataSize;		/* sum of all data segments */

sModuleList moduleList;		/* input module (.obj) linked list */
sTabList globalSymTable[MAX_HASHTABLE];		/* global symbol table */
sTabList allSymTable[MAX_HASHTABLE];		/* all symbol table */
sOCodeList globalObjCode;			/* object code list */
sExtRefTabList extRefTable[MAX_HASHTABLE];	/* external symbol reference table */

/** 
* @brief Program entry point - includes input argument processing & post-processing
* 
* @param argc 
* @param argv 
* 
* @return 
*/
int main(int argc, char *argv[])
{
	int i;

	/* initialize global variables */
	moduleListInit(&moduleList);

	if(!processArg(argc, argv)) return 0;

	/* initialize global variables */
	sTabInit(globalSymTable);
	sExtRefTabInit(extRefTable);
	oCodeInit(&globalObjCode);
	sTabInit(allSymTable);

	/* first pass scan */
	/* make a list of input obj modules */
	moduleListBuild(&moduleList);
	/* merge global symbol table */
	globalSymTabBuild(globalSymTable, &moduleList);
	/* merge external symbol reference table */
	externRefTabBuild(extRefTable, globalSymTable, &moduleList);
	/* merge symbol table */
	allSymTabBuild(allSymTable, &moduleList);
	/* merge object files */
	globalOCodeListBuild(&globalObjCode, &moduleList);

	/* dump information into .map & .ldi */
	sModuleDump(&moduleList);
	sTabSymDump(globalSymTable);
	sExtRefTabDump(extRefTable);

	/* code segment dump before second pass */
	//sOCodeBinDump(&globalObjCode);

	/* second pass scan */
	/* update each extern reference with relocated symbol address */
	patchExternRef(globalSymTable, extRefTable, &globalObjCode, &moduleList);
	/* update each non-extern reference with relocated symbol address */
	patchOtherRef(globalSymTable, extRefTable, &globalObjCode, &moduleList);

	/* print statistics */
	reportResult();

	/* data segment (& symbols) dump */
	/* write .map file */
	sTabAllSymDump(allSymTable);

	/* code segment dump after second pass */
	/* write .out & .map file */
	sOCodeBinDump(&globalObjCode);

	/* close file and dump memory if needed */
	closeLnk();

	/* free memory */
	sModuleListRemoveAll(&moduleList);
	sTabHashRemoveAll(globalSymTable);
	sExtRefTabHashRemoveAll(extRefTable);
	sOCodeListRemoveAll(&globalObjCode);
	sTabHashRemoveAll(allSymTable);

	printf("\nLinker ended successfully.\n\n");
	exit(0);	/* no error */
}

/** 
* @brief Print results after all iterations
*/
void reportResult(void)
{
	printf("\n");
	printf("-------------------------------\n");
	printf("** Linker Statistics Summary **\n");
	printf("-------------------------------\n");
	printf("Linker output file: %s\n", outFile);
	printf("Number of input object modules: %d\n", ModuleCntr);
	printf("Total data segment size: %d words\n", totalDataSize);
	printf("Total code segment size: %d words\n", totalCodeSize);
	printf("\n");
}


/** 
* @brief Process input command arguments
* 
* @param argc Number of input arguments
* @param argv Pointer to input argument strings
* 
* @return 
*/
int	processArg(int argc, char *argv[])
{
	int i;
	char filebuf[MAX_LINEBUF];

	if(argc == 1){	/* if no arguments given */
		printHelp(argv[0]);
		return FALSE;	/* early exit */
	}

	for (i = 1; i < argc; i++){
		if (!strcmp("--help", argv[i]) || !strcmp("-h", argv[i])) {	/* command help */
			printHelp(argv[0]);
			return FALSE; /* early exit */
		} else if(!strcmp("--version", argv[i])){	/* version info */
			printf("%s: Linker for OFDM DSP %g\n", argv[0], VERSION);
			/* copyright info below */
			printf(COPYRIGHT1);
			printf(COPYRIGHT2);
			/* build info below */
			printf("Last Built: %s %s\n", __DATE__, __TIME__);
			return FALSE; /* early exit */
		} else if(!strcmp("-v", argv[i]) || !strcmp("--verbose", argv[i])){
			/* verbose mode */
			VerboseMode = TRUE;
			printf("verbose mode set.\n");
		} else if(!strcmp("-l", argv[i])){
			/* linker description file (.ldf) filename */
			i++;
			if((argv[i] == NULL) || !isalnum(argv[i][0])){	/* error check */
				printArgError(argv[0], argv[i-1]);
				return FALSE;
			}
			printf("link description (.ldf) filename: %s\n", argv[i]);

			/* file open */
			if(!(LdfFP = fopen(argv[i], "r"))){
				printFileOpenError(argv[0], argv[i]);
				return FALSE;
			}
		} else if(!strcmp("-o", argv[i])){
			/* output file (.out) filename */
			i++;
			if((argv[i] == NULL) || !isalnum(argv[i][0])){	/* error check */
				printArgError(argv[0], argv[i-1]);
				return FALSE;
			}

			strcpy(outFile, argv[i]);	/* save output filename */
		} else if(!strcmp("-ac", argv[i])){
			/* specify code segment base address */
			i++;
			if((argv[i] == NULL) || !isxdigit(argv[i][0])){	/* error check */
				printArgError(argv[0], argv[i-1]);
				return FALSE;
			}
			codeSegAddr = (int)strtol(argv[i], NULL, 16);
			printf("code segment base address: 0x%04X\n", codeSegAddr);
			//resetPMA(codeSegAddr); 		/* update curaddr */
		} else if(!strcmp("-ad", argv[i])){
			/* specify data segment base address */
			i++;
			if((argv[i] == NULL) || !isxdigit(argv[i][0])){	/* error check */
				printArgError(argv[0], argv[i-1]);
				return FALSE;
			}
			dataSegAddr = (int)strtol(argv[i], NULL, 16);
			printf("data segment base address: 0x%04X\n", dataSegAddr);
		} else if(argv[i][0] == '-') {	/* cannot understand this command */
			printHelp(argv[0]);
			return FALSE; /* early exit */
		} else {	/* if not command, must be filename */
			inFile = argv[i];
			printf("object filename (.obj): %s\n", argv[i]);

			if(!inFile) {
				printf("\nError: %s - no input file\n", argv[0]);
				return FALSE;
			}

			if(!compareFilenameExt(filebuf, inFile, "obj")){
				printf("\nError: %s - input filename extension must be \".obj\"\n", inFile);
				return FALSE;	
			}

			sModule *p = sModuleListAdd(&moduleList, inFile, ModuleCntr++);

			/* .obj file open */
			p->ObjFP = fopen(inFile, "r");
			if(p->ObjFP == NULL){	/* open failed */
				printFileOpenError(argv[0], inFile);
				return FALSE;
			}

			/* .sym file open */
			changeFilenameExt(filebuf, argv[i], "sym");
			printf("symbol table (.sym) filename: %s\n", filebuf);
			if(!(p->SymFP = fopen(filebuf, "r"))){
				printFileOpenError(argv[0], filebuf);
				return FALSE;
			}
		}
	}

	/* feedback parameters */
	if(!ModuleCntr){
		printf("\nError: at least one object (.obj) filename must be specified.\n");
		return FALSE;
	}
	if(LdfFP && (codeSegAddr || dataSegAddr)) 
		printf("Note: ldf parameters will override -ac and/or -ad options.\n", codeSegAddr);
	if(!LdfFP && !codeSegAddr) printf("code segment base address: 0x%04X (default)\n", codeSegAddr);
	if(!LdfFP && !dataSegAddr) printf("data segment base address: 0x%04X (default)\n", dataSegAddr);
	if(!LdfFP) printf("link description (.ldf) filename: (not given)\n");
	if(!strlen(outFile)){
		printf("\nError: linker output (.out) filename must be specified.\n");
		return FALSE;
	}

	/* .out file open */
	printf("linker output (.out) filename: %s\n", outFile);
	if(!(OutFP = fopen(outFile, "wb"))){
		printFileOpenError(argv[0], outFile);
		return FALSE;
	}

	/* .map file open */
	changeFilenameExt(filebuf, outFile, "map");
	printf("linker map (.map) filename: %s\n", filebuf);
	if(!(MapFP = fopen(filebuf, "wt"))){
		printFileOpenError(argv[0], filebuf);
		return FALSE;
	}
	printFileHeader(MapFP, filebuf);

	/* .ldi file open */
	changeFilenameExt(filebuf, outFile, "ldi");
	printf("loading info (.ldi) filename: %s\n", filebuf);
	if(!(LdiFP = fopen(filebuf, "wt"))){
		printFileOpenError(argv[0], filebuf);
		return FALSE;
	}
	printFileHeader(LdiFP, filebuf);

	return TRUE;
}


/** 
* @brief Give command line help
* 
* @param s Name of program
*/
void printHelp(char *s)
{
		printf("Usage: %s [option(s)] -o outfile objfile1 objfile2 ...\n", s);
		printf("Options:\n");
		printf("\t-h [--help]   \tdisplay this help and exit\n");
		printf("\t--version     \toutput version information and exit\n");
		printf("\t-v [--verbose]\tprint extra information\n");
		/* .ldf not supported yet: Ver 0.9
		printf("\t-l ldffile    \tspecify linker description file (.ldf)\n");
		*/
		printf("\t-ac hexnumber \tspecify code segment base address [default: 0000]\n");
		printf("\t-ad hexnumber \tspecify data segment base address [default: 0000]\n");
		/* .ldf not supported yet: Ver 0.9
		printf("\t* Note: ldf file overrides -ac & -ad options.\n");
		*/
		printf("\n");
		printf("Report bugs to %s.\n", AUTHOR);
}

/** 
* @brief Print error message during argument processing
* 
* @param prog Name of program
* @param opt Option string
*/
void printArgError(char *prog, char *opt)
{
	printf("\nError: %s - argument to '%s' is missing\n", prog, opt);
}


/** 
* @brief Print error message during file open process
* 
* @param prog Name of program
* @param filename Name of the file
*/
void printFileOpenError(char *prog, char *filename)
{
	printf("\nError: %s - cannot open file %s\n", prog, filename);
}

/**
* @brief Print run-time error message to message buffer
*
* @param s Pointer to error string
* @param msg Error message
*/
void printErrorMsg(char *s, char *msg)
{
	printf("\nError: %s - %s\n", s, msg);
	if(MapFP){
		fprintf(MapFP, "\nError: %s - %s\n", s, msg);
	}
	closeLnk();
    exit(1);
}


/** 
* @brief Get filename extension from given string.
* 
* @param buf Pointer to output string to store extension.
* @param filename Given string containing input filename.
* @param sp Pointer to integer variable to store starting position of extension.
* 
* @return 1 if success, 0 otherwise
*/
int getFilenameExt(char *buf, char *filename, int *sp)
{
	int success = FALSE;
	int len = strlen(filename);

	/* if . is at the end of the filename, no ext */
	if(filename[len-1] == '.') 
		return success;	

	for(int i = len-1; i >= 0; i--){
		if(filename[i] == '.'){
			strcpy(buf, filename+i+1);
			*sp = i+1;	/* i+1: start position of ext */
			success = TRUE;
			break;
		}
	}

	return success;	
}

/** 
* @brief Change given filename's extension to new one.
* 
* @param buf Pointer to output string to store new filename with changed extension.
* @param filename Pointer to string containing input filename.
* @param newext Pointer to string containing new extension.
* 
* @return 1 if success, 0 otherwise
*/
int changeFilenameExt(char *buf, char *filename, char *newext)
{
	int success = FALSE;
	char oldext[MAX_LINEBUF];
	int sp;

	if(getFilenameExt(oldext, filename, &sp)){
		strcpy(buf, filename);
		strcpy(buf+sp, newext);
		success = TRUE;
	}else{	/* if there was no extension */
		int len = strlen(filename);

		if(!len){
			return success;
		}

		/* if . is at the end of the filename, just append */
		if(filename[len-1] == '.') {
			sprintf(buf, "%s%s", filename, newext);
			success = TRUE;
		}else{	/* else, append with "." */
			sprintf(buf, "%s.%s", filename, newext);
			success = TRUE;
		}

	}

	return success;
}

/** 
* @brief Compare given filename's extension to given string.
* 
* @param oldext Pointer to output string to store old extension.
* @param filename Pointer to string containing input filename.
* @param newext Pointer to string containing new extension.
* 
* @return 1 if old extension is same as new extension, 0 otherwise
*/
int compareFilenameExt(char *oldext, char *filename, char *newext)
{
	int success = FALSE;
	int sp;

	if(getFilenameExt(oldext, filename, &sp)){
		if(!strcasecmp(oldext, newext)) success = TRUE;
	}

	return success;
}

/** 
* @brief Close files and write data if needed.
*/
void closeLnk(void)
{
	if(LdfFP) fclose(LdfFP);
	if(OutFP) fclose(OutFP);
	if(MapFP) fclose(MapFP);
	if(LdiFP) fclose(LdiFP);
}

/** 
* @brief Print file header information 
* 
* @param fp Pointer to file
* @param filename Pointer to filename string
*/
void printFileHeader(FILE *fp, char *filename)
{
	time_t timer;
	struct tm *t;
	
	/* gets time of day */
	timer = time(NULL);
	/* converts date/time to a structure */
	t = localtime(&timer);

	fprintf(fp, ";-----------------------------------------------------------\n");
	fprintf(fp, "; %s - generated by %s %.1f\n", filename, TARGET, VERSION);
	fprintf(fp, "; %04d/%02d/%02d %02d:%02d:%02d\n", t->tm_year+1900,
		t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	/* copyright info below */
	fprintf(fp, "; "COPYRIGHT1);
	fprintf(fp, "; "COPYRIGHT2);
}
