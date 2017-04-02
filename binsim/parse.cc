/*
All Rights Reserved.
*/
/** 
* @file parse.cc
* Parser routines
* @date 2009-01-23
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "binsim.h"
#include "parse.h"
#include "dspdef.h"
#include "bcode.h"

/** 
* @brief Read loading info file (*.ldi)
* 
* @return TRUE if success, FALSE if not
*/
int readLdiFile(void)
{
	char linebuf[MAX_LINEBUF];

	if(VerboseMode) printf(".ldi file reading at readLdiFile().\n");

	while(fgets(linebuf, MAX_LINEBUF, LdiFP)){
		if(linebuf[0] == ';') continue;
		sscanf(linebuf, "%04X %d %04X %d\n", 
			&dataSegAddr, &dataSegSize, &codeSegAddr, &codeSegSize ); 
	};

	if(VerboseMode){
		printf(".ldi info: DATA (%d words @ %04X), "
			"CODE (%d words @ %04X)\n\n", 
			dataSegSize, dataSegAddr, codeSegSize, codeSegAddr);
	}
	return TRUE;
}

/** 
* @brief Read input binary executable file (*.out)
* 
* @return TRUE if success, FALSE if not
*/
int parseBinary(void)
{
	char data[INSTLEN/8];
	char tdata[INSTLEN+1];
	int numInt = 0;
	int instType;

	while(fread(&data, sizeof(char), (INSTLEN/8), inFP)){
		for(int i = 0; i < (INSTLEN/8); i++){
			bin2txt(data[i], &tdata[8*i]);
		}
		tdata[INSTLEN] = '\0';

		sBCodeListAdd(&bCode, tdata, numInt);
		/*
		fprintf(dumpDisFP, "%04d: %02X %02X %02X %02X: ", numInt, 
			0xFF & data[0], 0xFF & data[1], 
			0xFF & data[2], 0xFF & data[3]); 

		instType = findInstType(tdata);
		if(instType){
			fprintf(dumpDisFP, "%s %2d", sType[instType], sBinOp[instType].width);
		}else{	
			fprintf(dumpDisFP, "%s %2d", sType[instType], sBinOp[instType].width);
		}
		fprintf(dumpDisFP, "\n");
		*/

		numInt++;
	}

	if(VerboseMode)
		printf("\nParser: %d Bytes Read\n", numInt*4);

	return TRUE;
}

/** 
* @brief Convert a 8-bit binary into a matching 8-byte string (MSB: array[0])
* 
* @param bin32 Input character (8-bit binary)
* @param txt32 Output string buffer (8-byte characters)
*/
void bin2txt(char bin32, char txt32[8])
{
	int mask = 0x80;
	for(int i = 0; i < 8; i++){
		if(bin32 & mask) 
			txt32[i] = '1';
		else 
			txt32[i] = '0';
		mask >>= 1;
	}
}

/** 
* @brief Convert a n-byte binary string to integer value
* 
* @param str Output string buffer (8-byte characters)
* @param width Number of valid characters
* 
* @return Integer value of input string
*/
int bin2int(char str[], int width)
{
	int sum = 0;

	for(int i = 0; i < width; i++){
		sum <<= 1;	
		sum += (str[i] - '0');
	}
	//printf("bin2int: str %s, width %d, sum %d\n", str, width, sum);
	return sum;
}


/** 
* @brief Read opcode and determine instruction type
* 
* @param inst Character string which contain instruction (32 bytes)
* 
* @return eType type index
*/
int findInstType(char inst[INSTLEN+1])
{
	int i = 1;

	while(sType[i] != NULL){
		if(!strncmp(sBinOp[i].bits, inst, sBinOp[i].width)){	/* match found */
			if(VerboseMode){
				printf("%s: %s: %d\n", sBinOp[i].bits, inst, sBinOp[i].width);
			}

			return i;
		}else{	/* not found - try next one */
			i++;
			continue;
		}
	}
	return 0;
}


/** 
* @brief Before start-up, self-test if instruction table is ok.
* 
* @return TRUE or FALSE
*/
int VerifyInstructionTable(void)
{
    int sum;
	int i;
    int j = 1;
    int result = TRUE;

	if(VerboseMode){
		printf("Verifying sInstFormatTable[]...\n");
	}

    while(j != eTypeEnd){
        const sInstFormat *sp = &(sInstFormatTable[j]);

		if(VerboseMode){
			printf("Type %s: ", sType[j]);
        	for(i = 0; i < MAX_FIELD; i++){
				printf("%d ", sp->widths[i]);
        	}
			printf("\n");
		}

        sum = 0;
        for(i = 0; i < MAX_FIELD; i++){
            if(!sp->widths[i]) break;
            sum += sp->widths[i];
        }

        if(sum != INSTLEN){
            printf("Verifying instruction table failed: Type %s(%d), width %d, "
				"sum %d, i %d, sp->width %d, opcode %s\n", 
				sType[j], j, sBinOp[j].width, sum, i, sp->widths[i], sBinOp[j].bits);
            result = FALSE;
        }

        j++;
    }

    return result;
}

