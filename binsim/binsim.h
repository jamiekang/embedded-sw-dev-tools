/*
All Rights Reserved.
*/
/** 
* @file binsim.h
* @brief Common header file for binsim project
* @date 2009-01-23
*/

#ifndef _BINSIM_H
#define _BINSIM_H

#define VERSION 1.61
#define AUTHOR  ""
#define COPYRIGHT1 "Copyright (C), 2008-2009\n"
#define COPYRIGHT2 "All Rights Reserved.\n"
#define TARGET "binsim"

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

#include <stdio.h>
//#include "bcode.h"

#ifndef UNDEFINED
#define UNDEFINED   0xFFFF
#endif

#define MAX_COMMAND_BUFFER  10  /**< max. number of characters in a command line */
#define	MAX_LINEBUF	160		/**< maximum number of characters in one input line */
#define	MAX_CONDBUF	10		/**< maximum number of characters in COND */

#define	DEF_ITRMAX	1			/* default value for ItrMax */
#define	DEF_DUMPINSIZE	256		/* default value for dumpInSize */
#define	DEF_DUMPOUTSIZE	256		/* default value for dumpOutSize */

#define	INSTLEN	32	/* instruction width: 32 bits */

/** definition for complex data structure */
typedef struct cplx {
	int r;	/**< real part */
	int i;	/**< imaginary part */
} cplx;

extern int lineno;
extern unsigned int curaddr;
extern char msgbuf[MAX_LINEBUF];

extern int	VerboseMode;
extern char SimMode;
extern int BreakPoint;
extern int QuietMode;
extern int ItrCntr;
extern int ItrMax;
extern long Cycles;
extern int	Latency;
extern int BinDumpMode;
extern int AssemblerMode;
extern int VhpiMode;
extern int DelaySlotMode;

extern	FILE *dumpInFP;
extern	int	dumpInStart;
extern	int dumpInSize;
extern	FILE *dumpOutFP;
extern	int	dumpOutStart;
extern 	int dumpOutSize;

extern char *infile;
extern FILE *inFP;
extern FILE *LdiFP;
extern FILE *dumpDisFP;

extern int codeSegAddr;
extern int codeSegSize;
extern int dataSegAddr;
extern int dataSegSize;

extern int OVCount;

void reportResult(void);

void displayInternalStates(void);
int processArg(int argc, char *argv[]);
void printHelp(char *s);
void printArgError(char *prog, char *opt);
void printFileOpenError(char *prog, char *filename);

int getFilenameExt(char *buf, char *filename, int *sp);
int changeFilenameExt(char *buf, char *filename, char *newext);
int compareFilenameExt(char *oldext, char *filename, char *newext);
void printFileHeader(FILE *fp, char *filename);

#endif  /* _BINSIM_H */
