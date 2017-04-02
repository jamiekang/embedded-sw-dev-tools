/*
All Rights Reserved.
*/
/** 
* @file dspsim.h
* @brief Common header file for dspsim project
* @date 2008-09-18
*/

#ifndef _DSPSIM_H
#define _DSPSIM_H

#define VERSION 2.12
#define	DSPNAME	"Nano-1 DSP (N1D)"
#define AUTHOR  ""
#define COPYRIGHT1 "Copyright (C), 2008-2010\n"
#define COPYRIGHT2 "All Rights Reserved.\n"
#ifdef DSPASM
#define TARGET "dspasm"
#else
#define TARGET "dspsim"
#endif

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

#include <stdio.h>
#include "icode.h"

#define MAX_COMMAND_BUFFER  10  /**< max. number of characters in a command line */
#define	MAX_LINEBUF	160		/**< maximum number of characters in one input line */
#define	MAX_CONDBUF	10		/**< maximum number of characters in COND */

#define	DEF_ITRMAX	1			/* default value for ItrMax */
#define	DEF_DUMPINSIZE	256		/* default value for dumpInSize */
#define	DEF_DUMPOUTSIZE	256		/* default value for dumpOutSize */

#define	INSTLEN	32	/* instruction width: 32 bits */

typedef struct ccplx {
	int r;	/**< real part */
	int i;	/**< imaginary part */
} cplx;

extern FILE* yyin;
extern int lineno;
extern unsigned int curaddr;
extern sICode *curicode;
extern char	condbuf[MAX_CONDBUF];
extern char linebuf[MAX_LINEBUF];
extern int isParsingMultiFunc;
extern char msgbuf[MAX_LINEBUF];

extern int	VerboseMode;
extern char SimMode;
extern char *infile;
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
extern int UnalignedMemoryAccessMode;
extern int InitSimMode;
extern int SIMD1Mode;
extern int SIMD2Mode;
extern int SIMD4Mode;
extern int SIMD4ForceMode;
extern int SuppressUndefinedDMMode;

extern	FILE *dumpInFP;
extern	int	dumpInStart;
extern	int dumpInSize;
extern	FILE *dumpOutFP;
extern	int	dumpOutStart;
extern 	int dumpOutSize;

extern FILE *dumpBinFP;
extern FILE *dumpMemFP;
extern FILE *dumpTxtFP;
extern FILE *dumpSymFP;
extern FILE *dumpErrFP;
extern FILE *dumpLstFP;
extern char	bytebuf[INSTLEN];
extern char bitbuf[INSTLEN/8];
extern int bitPosition;
extern int bitWidth;

extern int codeSegAddr;
extern int dataSegAddr;

/** 
* @brief lex/yacc function declaration 
*/
#ifdef	__cplusplus
extern "C"
{
#endif
	int yyparse(void);
	int yylex(void);
#ifdef	__cplusplus
	int yyerror(char *msg);
};
#endif

#ifndef	__cplusplus
int yyerror(char *msg);
#endif

extern int AssemblerError;

extern struct sTab* symlook(char *s, unsigned int n);
void addlabel(char *name, unsigned int address);
void yyerror2(char *s);
unsigned int isOpcode(char *s);
extern struct sTab* isResSym(char *s);

int addOperand(char *s);
void addConjugate(void);
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
void printMemFileHeader(FILE *fp);

#endif  /* _DSPSIM_H */
