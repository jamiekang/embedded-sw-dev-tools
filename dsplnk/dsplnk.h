/*
All Rights Reserved.
*/
/** 
* @file dsplnk.h
* @brief Common header file for dsplnk project
* @date 2008-12-09
*/

#ifndef _DSPLNK_H
#define _DSPLNK_H

#define VERSION 1.6
#define AUTHOR  ""
#define	COPYRIGHT1 "Copyright (C), 2008-2009\n"
#define COPYRIGHT2 "All Rights Reserved.\n"
#define	TARGET	"dsplnk"

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

#include <stdio.h>

#define	MAX_OBJ		20		/* maximum number of object files */
#define	MAX_LINEBUF	100		/**< maximum number of characters in one input line */
#define	INSTLEN	32	/* instruction width: 32 bits */


/** 
* @brief Table ID (for reading .sym)
*/
enum eTabID {
	tSecInfo, tGlobal, tExtern, tAll, tPatchExternRef, tPatchOtherRef, tSymbol, tEnd = 9,
};

extern char linebuf[MAX_LINEBUF];
extern int lineno;
extern char filebuf[MAX_LINEBUF];
extern char msgbuf[MAX_LINEBUF];

extern int	VerboseMode;
extern char *infile;
extern char outFile[MAX_LINEBUF];

extern FILE *LdfFP;				/* .ldf file pointer */
extern FILE *OutFP;				/* .out file pointer */
extern FILE *MapFP;				/* .map file pointer */
extern FILE *LdiFP;				/* .ldi file pointer */
extern int ModuleCntr;

extern char	bytebuf[INSTLEN];
extern char bitbuf[INSTLEN/8];

extern int codeSegAddr;
extern int dataSegAddr;

extern int totalCodeSize;
extern int totalDataSize;

void reportResult(void);
int processArg(int argc, char *argv[]);
void printHelp(char *s);
void printArgError(char *prog, char *opt);
void printFileOpenError(char *prog, char *filename);
void printErrorMsg(char *s, char *msg);

int getFilenameExt(char *buf, char *filename, int *sp);
int changeFilenameExt(char *buf, char *filename, char *newext);
int compareFilenameExt(char *oldext, char *filename, char *newext);
void closeLnk(void);
void printFileHeader(FILE *fp, char *filename);

#endif  /* _DSPLNK_H */
