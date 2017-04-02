/*
All Rights Reserved.
*/

/** 
* @file main.cc
* @brief OFDM DSP Instruction Simulator/Assembler
* @date 2008-09-17
*/

#include "dspsim.h"
#include "symtab.h"
#include "icode.h"
#include "dmem.h"
#include "optab.h"
#include "secinfo.h"
#include "simcore.h"
#include "simsupport.h"
#include <stdio.h>	/* perror() */
#include <stdlib.h>	/* strtol(), exit() */
#include <string.h>	/* strcpy() */
#include <ctype.h>
#include <time.h>

#ifdef	VHPI		/* VHPI: dspsim.so library only */
#include "dsp.h"
#endif

char linebuf[MAX_LINEBUF];
char condbuf[MAX_CONDBUF];
int lineno = 1;
unsigned int curaddr = 0;
sICode *curicode = NULL;
int isParsingMultiFunc = FALSE;
char msgbuf[MAX_LINEBUF];

int VerboseMode = FALSE;
char SimMode = 'S';		/**< S: single step, C: continuous */
char *infile = NULL;
int BreakPoint = UNDEFINED;
int	QuietMode = FALSE;
int ItrCntr;	/* iteration counter */
int ItrMax = DEF_ITRMAX;		/* iteration number given by user  */
long Cycles;		/* total cycles */
int	Latency;		/* latency for a instruction */
//int BinDumpMode = FALSE;
int	AssemblerMode = FALSE;
int VhpiMode = FALSE;	/* for VHDL interface */
int DelaySlotMode = FALSE;	/* for delay slot support */
int UnalignedMemoryAccessMode = FALSE;	/* unaligned memory access support */
int InitSimMode = FALSE;	/* for WrDataMem() & initDumpIn() */
int	SIMD1Mode = TRUE;		/* for -1 option: default */
int SIMD2Mode = FALSE;		/* for -2 option */
int	SIMD4Mode = FALSE;		/* for -4 option */
int	SIMD4ForceMode = FALSE;		/* for -4f option */
int SuppressUndefinedDMMode = FALSE;		/* for -x option */

FILE *dumpInFP;			/* file pointer to memory dump input */
int dumpInStart;		/* start address of memory dump input */
int dumpInSize = DEF_DUMPINSIZE;			/* size of memory dump input */
FILE *dumpOutFP;		/* file pointer to memory dump output */
int dumpOutStart;		/* start address of memory dump output */
int dumpOutSize = DEF_DUMPOUTSIZE;		/* size of memory dump output */

FILE *dumpBinFP;			/* file pointer to binary code dump */
FILE *dumpMemFP;			/* file pointer to binmem code dump */
FILE *dumpTxtFP;			/* file pointer to text code dump (= object) */
FILE *dumpSymFP;			/* file pointer to symbol table dump */
FILE *dumpErrFP;			/* file pointer to error msg dump */
FILE *dumpLstFP;			/* file pointer to list file */
char filebuf[MAX_LINEBUF];
char bytebuf[INSTLEN];		/* for use in sICodeBinDump() */
char bitbuf[INSTLEN/8]; 	/* for use in sICodeBinDump() */

int codeSegAddr;		/* start address of code segment; specified by user */
int dataSegAddr;		/* start address of data segment; specified by user */

sTabList symTable[MAX_HASHTABLE]; 		/**< Symbol Table for labels */
sICodeList iCode;						/**< Intermediate Code List */
sSecInfoList secInfo;					/**< Section Information List */
dMemList dataMem[MAX_HASHTABLE];		/**< Data Memory Table for variables */
oTabList opTable[MAX_OPHASHTABLE];		/**< Opcode Table for iCode's reference */

sTabList resSymTable[MAX_HASHTABLE]; 		/**< Reserved Symbol Table */

sSecInfo *curSecInfo;		/* pointer to current segment info. data structure */

sint OVCount;	/**< Overflow counter */
int AssemblerError = 0;	/* number of assembler errors */


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
	int simResult;

	if(!processArg(argc, argv)) return 0;

	/* initialize global variables */
	iCodeInit(&iCode);
	symTableInit(symTable);
	resSymTableInit(resSymTable);
	dataMemInit(dataMem);
	opTableInit(opTable);
	secInfoInit(&secInfo);

	/* first pass scan */
	/* start lexer & parser */
	if(!yyparse() && !AssemblerError){
		if(VerboseMode) printf("Parser worked :)\n\n");
	}else{
		if(VerboseMode) printf("Parser failed :(\n\n");
		if(!AssemblerError){		/* parser syntax error */
			printf("\nSimulator/Assembler failed - Syntax error(s) found.\n\n");
		}else{						/* duplicated label detected */
			printf("\nSimulator/Assembler failed - Total %d error(s) found.\n\n", AssemblerError);
		}

		/* free memory */
		sICodeListRemoveAll(&iCode);
		sTabHashRemoveAll(symTable);
		sTabHashRemoveAll(resSymTable);
		dMemHashRemoveAll(dataMem);
		oTabHashRemoveAll(opTable);
		sSecInfoListRemoveAll(&secInfo);
		exit(1);
	}

	/* second pass scan */
	/* type resolution & resolve pseudo instructions */
	codeScan(&iCode);	

	/* for data segment even-byte patching */
	changeFilenameExt(filebuf, filebuf, "");
	sTabSymEvenBytePatch(symTable, &secInfo, filebuf);

	/* for debug */
	if(VerboseMode) sTabHashPrint(symTable);

	/* if assembler-only mode */
	if(AssemblerMode){
		sICodeListPrint(&iCode);
		sSecInfoSymDump(&secInfo);
		sTabSymDump(symTable);
		sICodeBinDump(&iCode);
		if(VerboseMode) sSecInfoListPrint(&secInfo);
		if(VerboseMode) sTabHashPrint(symTable);
		if(VerboseMode) sTabHashPrint(resSymTable);
		if(VerboseMode) dMemHashPrint(dataMem);
		closeSim();

		/* free memory */
		sICodeListRemoveAll(&iCode);
		sTabHashRemoveAll(symTable);
		sTabHashRemoveAll(resSymTable);
		dMemHashRemoveAll(dataMem);
		oTabHashRemoveAll(opTable);
		sSecInfoListRemoveAll(&secInfo);

		if(!AssemblerError)
			printf("\nAssembler ended successfully.\n\n");
		else
			printf("\nAssembler failed - Total %d error(s) found.\n\n", AssemblerError);
		exit(0);	/* no error */
	}

#ifndef DSPASM
	/* init simulator */
	InitSimMode = TRUE;
	initSim();
	initDumpIn();
	InitSimMode = FALSE;

	/* simulator main loop */
	printf("\nBegin Simulation..\n\n");
	for(ItrCntr = 0; ItrCntr < ItrMax; ItrCntr++){
		printf("Iteration %d of %d:\n", ItrCntr+1, ItrMax);

		simResult = simCore(iCode);
		if(simResult){  /* exit simulation loop */
			break;
		}
		printf("\n");
	}
	printf("End of Simulation!!\n");

	/* print statistics */
	reportResult();

	/* print output files including .lst */
	/*
	sICodeListPrint(&iCode);
	sSecInfoSymDump(&secInfo);
	sTabSymDump(symTable);
	if(BinDumpMode) sICodeBinDump(&iCode);
	if(VerboseMode) sSecInfoListPrint(&secInfo);
	if(VerboseMode) sTabHashPrint(symTable);
	if(VerboseMode) sTabHashPrint(resSymTable);
	if(VerboseMode) dMemHashPrint(dataMem);
	*/

	/* close file and dump memory if needed */
	closeSim();

	/* free memory */
	sICodeListRemoveAll(&iCode);
	sTabHashRemoveAll(symTable);
	sTabHashRemoveAll(resSymTable);
	dMemHashRemoveAll(dataMem);
	oTabHashRemoveAll(opTable);
	sSecInfoListRemoveAll(&secInfo);

	exit(0);	/* no error */
#endif
}

/** 
* @brief Print results after all iterations
*/
void reportResult(void)
{
	printf("\n");
	printf("----------------------------------\n");
	dumpRegister(NULL);

	printf("\n");
	printf("----------------------------------\n");
	printf("** Simulator Statistics Summary **\n");
	printf("----------------------------------\n");
	printf("Time: %ld cycles for %d iteration\n", Cycles, ItrMax);
	printf("Overflow: %d times\n", OVCount);
	printf("\n");
}

/** 
* @brief Report information on internal states between each instruction.
*/
void displayInternalStates(void)
{
	if(!QuietMode){
/*
		printf("========\n");
		printf("States Summary to be filled later here\n");
		printf("========\n");
*/
	}
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

#ifdef DSPASM
	AssemblerMode = TRUE;
#endif

#ifdef VHPI			/* VHPI only: dspsim.so library */
	VhpiMode = TRUE;
	SimMode = 'C';
	QuietMode = TRUE;
	infile = DEFAULTASM;	/* defined in dsp.h */

	yyin = fopen(infile, "r");
	if(yyin == NULL){	/* open failed */
		printf("\nError: %s - cannot open %s\n", argv[0], infile);
		return FALSE;
	}

	if(!compareFilenameExt(filebuf, infile, "asm")){
		printf("\nError: %s - input filename extension must be \".asm\"\n", infile);
		return FALSE;	
	}
#else				/* simulator & assembler */
	if(argc == 1){	/* if no arguments given */
		printHelp(argv[0]);
		return FALSE;	/* early exit */
	}

	for (i = 1; i < argc; i++){
		if (!strcmp("--help", argv[i]) || !strcmp("-h", argv[i])) {	/* command help */
			printHelp(argv[0]);
			return FALSE; /* early exit */
		} else if(!strcmp("--version", argv[i])){	/* version info */
			if(AssemblerMode){
				printf("%s: %s Assembler %g\n", argv[0], DSPNAME, VERSION);
			}else{
				printf("%s: %s Simulator %g\n", argv[0], DSPNAME, VERSION);
			}
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
		} else if(!strcmp("-q", argv[i]) || !strcmp("--quiet", argv[i])){
			if(!AssemblerMode){
				/* quiet mode */
				QuietMode = TRUE;
				printf("quiet mode set.\n");
			}
		} else if(!strcmp("-c", argv[i])){
			if(!AssemblerMode){
				/* continuous mode */
				SimMode = 'C';
				printf("continuous mode set.\n");
			}
		} else if(!strcmp("-l", argv[i])){
			if(!AssemblerMode){
				/* number of loop iterations */
				i++;
				if((argv[i] == NULL) || !isdigit(argv[i][0])){	/* error check */
					printArgError(argv[0], argv[i-1]);
					return FALSE;
				}
				ItrMax = atoi(argv[i]);
				printf("number of iteration: %d\n", ItrMax);
			}
		} else if(!strcmp("-if", argv[i])){
			if(!AssemblerMode){
				/* memory dump input filename */
				i++;
				if((argv[i] == NULL) || !isalnum(argv[i][0])){	/* error check */
					printArgError(argv[0], argv[i-1]);
					return FALSE;
				}
				printf("memory dump input filename: %s\n", argv[i]);

				/* file open */
				if(!(dumpInFP = fopen(argv[i], "r"))){
					printFileOpenError(argv[0], argv[i]);
					return FALSE;
				}
			}
		} else if(!strcmp("-ia", argv[i])){
			if(!AssemblerMode){
				/* memory dump input start address (absolute) */
				i++;
				if((argv[i] == NULL) || !isxdigit(argv[i][0])){	/* error check */
					printArgError(argv[0], argv[i-1]);
					return FALSE;
				}
				dumpInStart = (int)strtol(argv[i], NULL, 16);
				printf("memory dump input start address (absolute): 0x%04X\n", dumpInStart);
			}
		} else if(!strcmp("-is", argv[i])){
			if(!AssemblerMode){
				/* number of memory dump input */
				i++;
				if((argv[i] == NULL) || !isdigit(argv[i][0])){	/* error check */
					printArgError(argv[0], argv[i-1]);
					return FALSE;
				}
				dumpInSize = atoi(argv[i]);
				printf("memory dump input size in words: %d\n", dumpInSize);
			}
		} else if(!strcmp("-of", argv[i])){
			if(!AssemblerMode){
				/* memory dump output filename */
				i++;
				if((argv[i] == NULL) || !isalnum(argv[i][0])){	/* error check */
					printArgError(argv[0], argv[i-1]);
					return FALSE;
				}
				printf("memory dump output filename: %s\n", argv[i]);

				/* file open */
				if(!(dumpOutFP = fopen(argv[i], "w"))){
					printFileOpenError(argv[0], argv[i]);
					return FALSE;
				}
			}
		} else if(!strcmp("-oa", argv[i])){
			if(!AssemblerMode){
				/* memory dump output start address (absolute) */
				i++;
				if((argv[i] == NULL) || !isxdigit(argv[i][0])){	/* error check */
					printArgError(argv[0], argv[i-1]);
					return FALSE;
				}
				dumpOutStart = (int)strtol(argv[i], NULL, 16);
				printf("memory dump output start address (absolute): 0x%04X\n", dumpOutStart);
			}
		} else if(!strcmp("-os", argv[i])){
			if(!AssemblerMode){
				/* number of memory dump output */
				i++;
				if((argv[i] == NULL) || !isdigit(argv[i][0])){	/* error check */
					printArgError(argv[0], argv[i-1]);
					return FALSE;
				}
				dumpOutSize = atoi(argv[i]);
				printf("memory dump output size in words: %d\n", dumpOutSize);
			}
////////////////////////////////////////////////////////////////////////////////////////////////////////
//	/* bin dump mode option disabled in v2.12 (2010/07/14) */
//		} else if(!strcmp("-b", argv[i])){
//			if(!AssemblerMode){
//				/* binary code dump mode set */
//				BinDumpMode = TRUE;
//				printf("binary code dump mode set.\n");
//			}
////////////////////////////////////////////////////////////////////////////////////////////////////////
		} else if(!strcmp("-a", argv[i])){
			if(!AssemblerMode){
				/* assembler mode set */
				AssemblerMode = TRUE;
				printf("assembler mode set.\n");
			}
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
////////////////////////////////////////////////////////////////////////////////////////////////////////
//	/* delay slot mode option disabled in v2.07 (2010/06/01) */
//		} else if(!strcmp("-d", argv[i])){
//			/* delay slot mode */
//			DelaySlotMode = TRUE;
//			printf("delay slot enabled.\n");
////////////////////////////////////////////////////////////////////////////////////////////////////////
		} else if(!strcmp("-u", argv[i])){
			/* unaligned memory access mode */
			UnalignedMemoryAccessMode = TRUE;
			printf("unaligned memory access allowed.\n");
		} else if(!strcmp("-1", argv[i])){
			if(!AssemblerMode){
				/* 1 SIMD data path simulation mode set */
				SIMD1Mode = TRUE;
				printf("single SIMD data path mode set.\n");
			}
		} else if(!strcmp("-2", argv[i])){
			if(!AssemblerMode){
				/* 2 SIMD data path simulation mode set */
				SIMD1Mode = FALSE;
				SIMD2Mode = TRUE;
				printf("dual SIMD data path mode set.\n");
			}
		} else if(!strcmp("-4", argv[i])){
			if(!AssemblerMode){
				/* 4 SIMD data path simulation mode set */
				SIMD1Mode = FALSE;
				SIMD4Mode = TRUE;
				printf("quad SIMD data path mode set.\n");
			}
		} else if(!strcmp("-4f", argv[i])){
			if(!AssemblerMode){
				/* force display 4 SIMD data path mode set */
				SIMD4ForceMode = TRUE;
				printf("forced quad SIMD data path display mode set.\n");
			}
		} else if(!strcmp("-x", argv[i])){
			if(!AssemblerMode){
				/* suppress undefined data memory message mode set */
				SuppressUndefinedDMMode = TRUE;
				printf("suppressing undefined data memory message mode set.\n");
			}
		} else if(argv[i][0] == '-') {	/* cannot understand this command */
			printHelp(argv[0]);
			return FALSE; /* early exit */
		} else {	/* if not command, must be filename */
			infile = argv[i];
			printf("source filename (.asm): %s\n", argv[i]);

			if(!infile) {
				printf("\nError: %s - no input file\n", argv[0]);
				return FALSE;
			}

			yyin = fopen(infile, "r");
			if(yyin == NULL){	/* open failed */
				printf("\nError: %s - cannot open %s\n", argv[0], infile);
				return FALSE;
			}

			if(!compareFilenameExt(filebuf, infile, "asm")){
				printf("\nError: %s - input filename extension must be \".asm\"\n", infile);
				return FALSE;	
			}

			/* .err file open */
			changeFilenameExt(filebuf, argv[i], "err");
			printf("error message filename: %s\n", filebuf);
			if(!(dumpErrFP = fopen(filebuf, "wt"))){
				printFileOpenError(argv[0], filebuf);
				return FALSE;
			}
			printFileHeader(dumpErrFP, filebuf);

			/* if binary code dump mode set */
//			if(BinDumpMode || AssemblerMode){
			if(AssemblerMode){

				/* .lst file open */
				changeFilenameExt(filebuf, argv[i], "lst");
				printf("list filename: %s\n", filebuf);
				if(!(dumpLstFP = fopen(filebuf, "wt"))){
					printFileOpenError(argv[0], filebuf);
					return FALSE;
				}
				printFileHeader(dumpLstFP, filebuf);

				/* .sym file open */
				changeFilenameExt(filebuf, argv[i], "sym");
				printf("symbol table filename: %s\n", filebuf);
				if(!(dumpSymFP = fopen(filebuf, "wt"))){
					printFileOpenError(argv[0], filebuf);
					return FALSE;
				}
				printFileHeader(dumpSymFP, filebuf);

				/* file open */
				changeFilenameExt(filebuf, argv[i], "bin");
				printf("binary code dump filename: %s\n", filebuf);
				if(!(dumpBinFP = fopen(filebuf, "wb"))){
					printFileOpenError(argv[0], filebuf);
					return FALSE;
				}

				changeFilenameExt(filebuf, argv[i], "mem");
				printf("binary memory dump filename: %s\n", filebuf);
				if(!(dumpMemFP = fopen(filebuf, "wb"))){
					printFileOpenError(argv[0], filebuf);
					return FALSE;
				}
				printMemFileHeader(dumpMemFP);

				changeFilenameExt(filebuf, argv[i], "obj");
				printf("object filename: %s\n", filebuf);
				if(!(dumpTxtFP = fopen(filebuf, "wt"))){
					printFileOpenError(argv[0], filebuf);
					return FALSE;
				}
				printFileHeader(dumpTxtFP, filebuf);
    			fprintf(dumpTxtFP, 
					";-----------------------------------------------------------\n");
			}
		}
	}
#endif
	return TRUE;
}


/** 
* @brief Give command line help
* 
* @param s Name of program
*/
void printHelp(char *s)
{
		printf("Usage: %s [option(s)] filename\n", s);
		printf("Options:\n");
		printf("\t-h [--help]   \tdisplay this help and exit\n");
		printf("\t--version     \toutput version information and exit\n");
		printf("\t-v [--verbose]\tprint extra information\n");
#ifndef DSPASM
		printf("\t-q [--quiet]  \tprint only summary information (no human input)\n");
		printf("\t-c            \trun in continuous mode\n");
		printf("\t-l number     \tspecify number of loop iterations [default: %d]\n", DEF_ITRMAX);
		printf("\t-if filename  \tload data memory from file (format: a decimal number per line)\n");
		printf("\t-ia hexnumber \tload data memory start address (absolute addr) [default: 0000]\n");
		printf("\t-is words     \tnumber of words to load into data memory [default: %d]\n", DEF_DUMPINSIZE);
		printf("\t-of filename  \tdump data memory into file\n");
		printf("\t-oa hexnumber \tdump data memory start address (absolute addr) [default: 0000]\n");
		printf("\t-os words     \tnumber of words to dump from data memory [default: %d]\n", DEF_DUMPOUTSIZE);
//		printf("\t-b            \tbinary code dump after execution (.bin & .mem)\n");
		printf("\t-a            \tassembler mode (no simulation)\n");
		printf("\t-u            \tallow unaligned memory access\n");
		printf("\t-1            \tSingle data path mode (no SIMD)\n");
		printf("\t-2            \tDual data path mode (2 SIMD)\n");
		printf("\t-4            \tQuad data path mode (4 SIMD)\n");
		printf("\t-4f           \tforce quad data path display mode (4 SIMD)\n");
		printf("\t-x            \tsuppress undefined data memory message mode\n");
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////
//	/* delay slot mode option disabled in v2.07 (2010/06/01) */
//		printf("\t-d            \tenable delay slot\n");
////////////////////////////////////////////////////////////////////////////////////////////////////////
		//printf("\t-ac hexnumber \tspecify code segment base address [default: 0000]\n");
		//printf("\t-ad hexnumber \tspecify data segment base address [default: 0000]\n");
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
    fprintf(fp, "; %s - generated by %s %.2f\n", filename, TARGET, VERSION);
	fprintf(fp, "; %04d/%02d/%02d %02d:%02d:%02d\n", t->tm_year+1900, 
		t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    /* copyright info below */
    fprintf(fp, "; "COPYRIGHT1);
    fprintf(fp, "; "COPYRIGHT2);
}

/**
* @brief Print file header for .mem file
*
* @param fp Pointer to file
*/
void printMemFileHeader(FILE *fp)
{
	fprintf(fp, "$ADDRESSFMT H\n");
	fprintf(fp, "$DATAFMT H\n");
}
