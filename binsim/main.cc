/*
All Rights Reserved.
*/

/** 
* @file main.cc
* @brief OFDM DSP Binary Simulator
* @date 2009-01-22
*/

#include "binsim.h"
#include "parse.h"
#include "dspdef.h"
#include "bcode.h"
#include "dmem.h"
//#include "secinfo.h"
#include "binsimcore.h"
#include <stdio.h>	/* perror() */
#include <stdlib.h>	/* strtol(), exit() */
#include <string.h>	/* strcpy() */
#include <ctype.h>
#include <time.h>

#ifdef	VHPI		/* VHPI: dspsim.so library only */
#include "dsp.h"
#endif

char msgbuf[MAX_LINEBUF];

int VerboseMode = FALSE;
char SimMode = 'S';		/**< S: single step, C: continuous */
int BreakPoint = UNDEFINED;
int	QuietMode = FALSE;
int ItrCntr;	/* iteration counter */
int ItrMax = DEF_ITRMAX;		/* iteration number given by user  */
long Cycles;		/* total cycles */
int	Latency;		/* latency for a instruction */
int DisassemblerMode = FALSE;	/* for disassembler-only mode */
int VhpiMode = FALSE;	/* for VHDL interface */
int DelaySlotMode = FALSE;	/* for delay slot support */

FILE *dumpInFP;			/* file pointer to memory dump input */
int dumpInStart;		/* start address of memory dump input */
int dumpInSize = DEF_DUMPINSIZE;			/* size of memory dump input */
FILE *dumpOutFP;		/* file pointer to memory dump output */
int dumpOutStart;		/* start address of memory dump output */
int dumpOutSize = DEF_DUMPOUTSIZE;		/* size of memory dump output */

char *infile = NULL;	/* pointer to input file name */
FILE *inFP;				/* file pointer to input binary executable file */
FILE *dumpDisFP;		/* file pointer to dis-assembly file */
FILE *LdiFP;			/* file pointer to loading info file */
char filebuf[MAX_LINEBUF];

int codeSegAddr;		/* start address of code segment; specified by .ldi file */
int codeSegSize;		/* size of code segment (words); specified by .ldi file */
int dataSegAddr;		/* start address of data segment; specified by .ldi file */
int dataSegSize;		/* size of data segment (words); specified by .ldi file */

sBCodeList bCode;						/**< Binary code List */
//sSecInfoList secInfo;					/**< Section Information List */
dMemList dataMem[MAX_HASHTABLE];		/**< Data Memory Table for variables */

//sSecInfo *curSecInfo;		/* pointer to current segment info. data structure */

int OVCount;	/**< Overflow counter */


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

	/* process input arguments */
	if(!processArg(argc, argv)) return 0;

	/* self-diagnosis: verify instruction table */
	int verifyResult = VerifyInstructionTable();

	if(verifyResult){
		if(VerboseMode) printf("\nSimulator internal tables verified :)\n\n");
	}else{
		printf("\nSimulator internal tables incorrect.\n");

		exit(1);
	}

	/* initialize global variables */
	bCodeInit(&bCode);
	dataMemInit(dataMem);
	//secInfoInit(&secInfo);

	/* first pass scan */
	/* read code & data segment info */
	readLdiFile();
	/* start lexer & parser */
	if(parseBinary()){
		if(VerboseMode) printf("Parser worked :)\n\n");
	}else{
		if(VerboseMode) printf("Parser failed!!\n\n");
		exit(1);
	}

	/* second pass scan */
	/* resolve opcode index */
	codeScan(&bCode);	

	/* if disassembler-only mode */
	if(DisassemblerMode){
		sBCodeListPrint(&bCode);

		/* close file and dump memory if needed */
		closeSim();

		/* free memory */
		sBCodeListRemoveAll(&bCode);
		dMemHashRemoveAll(dataMem);
		//sSecInfoListRemoveAll(&secInfo);

		printf("\nDisassembler ended successfully.\n\n");
		exit(0);	/* no error */
	}

	/* init simulator */
	initSim();

	/* simulator main loop */
	printf("\nBegin Simulation..\n\n");
	for(ItrCntr = 0; ItrCntr < ItrMax; ItrCntr++){
		printf("Iteration %d of %d:\n", ItrCntr+1, ItrMax);

		simResult = simCore(bCode);
		if(simResult){  /* exit simulation loop */
			break;
		}
		printf("\n");
	}
	printf("End of Simulation!!\n");

	/* print statistics */
	reportResult();

	/* print output files including .dis */
	//-------------------------------------------
	// write to .DIS here
	//-------------------------------------------
	sBCodeListPrint(&bCode);
	//sSecInfoSymDump(&secInfo);
	//if(VerboseMode) sSecInfoListPrint(&secInfo);
	if(VerboseMode) dMemHashPrint(dataMem);

	/* close file and dump memory if needed */
	closeSim();

	/* free memory */
	sBCodeListRemoveAll(&bCode);
	dMemHashRemoveAll(dataMem);
	//sSecInfoListRemoveAll(&secInfo);

	exit(0);	/* no error */
}

/** 
* @brief Print results after all iterations
*/
void reportResult(void)
{
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
		printf("States Summary to be filled later here\n");
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

#ifdef VHPI			/* VHPI only: dspsim.so library */
	VhpiMode = TRUE;
	SimMode = 'C';
	QuietMode = TRUE;
	infile = DEFAULTBIN;	/* defined in dsp.h */

	inFP = fopen(infile, "rb");
	if(inFP == NULL){	/* open failed */
		printf("\nError: %s - cannot open %s\n", argv[0], infile);
		return FALSE;
	}

	if(!compareFilenameExt(filebuf, infile, "out")){
		printf("\nError: %s - input filename extension must be \".out\"\n", infile);
		return FALSE;	
	}
#else				/* simulator */
	if(argc == 1){	/* if no arguments given */
		printHelp(argv[0]);
		return FALSE;	/* early exit */
	}

	for (i = 1; i < argc; i++){
		if (!strcmp("--help", argv[i]) || !strcmp("-h", argv[i])) {	/* command help */
			printHelp(argv[0]);
			return FALSE; /* early exit */
		} else if(!strcmp("--version", argv[i])){	/* version info */
			printf("%s: OFDM DSP Binary Simulator %g\n", argv[0], VERSION);

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
			if(!DisassemblerMode){
				/* quiet mode */
				QuietMode = TRUE;
				printf("quiet mode set.\n");
			}
		} else if(!strcmp("-c", argv[i])){
			if(!DisassemblerMode){
				/* continuous mode */
				SimMode = 'C';
				printf("continuous mode set.\n");
			}
		} else if(!strcmp("-l", argv[i])){
			if(!DisassemblerMode){
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
			if(!DisassemblerMode){
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
			if(!DisassemblerMode){
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
			if(!DisassemblerMode){
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
			if(!DisassemblerMode){
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
			if(!DisassemblerMode){
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
			if(!DisassemblerMode){
				/* number of memory dump output */
				i++;
				if((argv[i] == NULL) || !isdigit(argv[i][0])){	/* error check */
					printArgError(argv[0], argv[i-1]);
					return FALSE;
				}
				dumpOutSize = atoi(argv[i]);
				printf("memory dump output size in words: %d\n", dumpOutSize);
			}
		} else if(!strcmp("-a", argv[i])){
			if(!DisassemblerMode){
				/* disassembler mode set */
				DisassemblerMode = TRUE;
				printf("disassembler mode set.\n");
			}
		} else if(!strcmp("-ac", argv[i])){
			/* specify code segment base address */
//			i++;
//			if((argv[i] == NULL) || !isxdigit(argv[i][0])){	/* error check */
//				printArgError(argv[0], argv[i-1]);
//				return FALSE;
//			}
//			codeSegAddr = (int)strtol(argv[i], NULL, 16);
//			printf("code segment base address: 0x%04X\n", codeSegAddr);
			//resetPMA(codeSegAddr); 		/* update curaddr */
		} else if(!strcmp("-ad", argv[i])){
			/* specify data segment base address */
//			i++;
//			if((argv[i] == NULL) || !isxdigit(argv[i][0])){	/* error check */
//				printArgError(argv[0], argv[i-1]);
//				return FALSE;
//			}
//			dataSegAddr = (int)strtol(argv[i], NULL, 16);
//			printf("data segment base address: 0x%04X\n", dataSegAddr);
		} else if(!strcmp("-d", argv[i])){
			/* delay slot mode */
			DelaySlotMode = TRUE;
			printf("delay slot enabled.\n");
		} else if(argv[i][0] == '-') {	/* cannot understand this command */
			printHelp(argv[0]);
			return FALSE; /* early exit */
		} else {	/* if not command, must be filename */
			infile = argv[i];
			printf("source filename (.bin): %s\n", argv[i]);

			if(!infile) {
				printf("\nError: %s - no input file\n", argv[0]);
				return FALSE;
			}

			inFP = fopen(infile, "rb");
			if(inFP == NULL){	/* open failed */
				printf("\nError: %s - cannot open %s\n", argv[0], infile);
				return FALSE;
			}

			if(!compareFilenameExt(filebuf, infile, "out")){
				printf("\nError: %s - input filename extension must be \".out\"\n", infile);
				return FALSE;	
			}

			/* .ldi file open */
			changeFilenameExt(filebuf, argv[i], "ldi");
			printf("loading info (.ldi) filename: %s\n", filebuf);
			if(!(LdiFP = fopen(filebuf, "rt"))){
				printFileOpenError(argv[0], filebuf);
				return FALSE;
			}

			/* .dis file open */
			changeFilenameExt(filebuf, argv[i], "dis");
			printf("disassembly (.dis) filename: %s\n", filebuf);
			if(!(dumpDisFP = fopen(filebuf, "wt"))){
				printFileOpenError(argv[0], filebuf);
				return FALSE;
			}
			printFileHeader(dumpDisFP, filebuf);

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
		printf("\t-q [--quiet]  \tprint only summary information (no human input)\n");
		printf("\t-c            \trun in continuous mode\n");
		printf("\t-l number     \tspecify number of loop iterations [default: %d]\n", DEF_ITRMAX);
		printf("\t-if filename  \tload data memory from file\n");
		printf("\t-ia hexnumber \tload data memory start address (absolute addr) [default: 0000]\n");
		printf("\t-is words     \tnumber of words to load into data memory [default: %d]\n", DEF_DUMPINSIZE);
		printf("\t-of filename  \tdump data memory into file\n");
		printf("\t-oa hexnumber \tdump data memory start address (absolute addr) [default: 0000]\n");
		printf("\t-os words     \tnumber of words to dump from data memory [default: %d]\n", DEF_DUMPOUTSIZE);
		printf("\t-a            \tdisassembler mode (no simulation)\n");
//		printf("\t-ac hexnumber \tspecify code segment base address [default: 0000]\n");
//		printf("\t-ad hexnumber \tspecify data segment base address [default: 0000]\n");
		printf("\t-d            \tenable delay slot\n");
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
    fprintf(fp, "; %s - generated by %s %.1f\n", filename, TARGET, VERSION);
	fprintf(fp, "; %04d/%02d/%02d %02d:%02d:%02d\n", t->tm_year+1900, 
		t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    /* copyright info below */
    fprintf(fp, "; "COPYRIGHT1);
    fprintf(fp, "; "COPYRIGHT2);
}

