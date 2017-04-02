/*
All Rights Reserved.
*/
/** 
* @file parse.h
* Header for parser routines
* @date 2009-01-23
*/

#ifndef	_PARSE_H
#define	_PARSE_H

int readLdiFile(void);
int parseBinary(void);
void bin2txt(char bin32, char txt32[8]);
int bin2int(char str[], int width);
int findInstType(char inst[INSTLEN+1]);

int VerifyInstructionTable(void);

#endif	/* _PARSE_H */
