/*
All Rights Reserved.
*/
/** 
* @file typetab.cc
* Instruction type definition
* @date 2008-12-23
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "typetab.h"
#include "dspdef.h"

/** 
* @brief Instruction type mnemonics; use eType for array indexing
*/
/*
const char *sType[] =
{
        "---",
        "01a", "01b", "01c",
        "03a", "03c", "03d", "03e", "03f", "03g", "03h", "03i",
        "04a", "04b", "04c", "04d", "04e", "04f", "04g", "04h",
        "06a", "06b", "06c", "06d",
        "08a", "08b", "08c", "08d",
        "09c", "09d", "09e", "09f", "09g", "09h", "09i",
        "10a", "10b",
        "11a",
        "12a", "12b", "12c", "12d",
        "14a", "14b",
        "15a", "15b", "15c", "15d",
        "16a", "16b", "16c", "16d",
        "17a", "17b", "17c", "17d", "17e", "17f",
        "18a",
        "19a",
        "20a",
        "22a",
        "23a",
        "25a", "25b",
        "26a",
        "29a", "29b", "29c", "29d",
        "30a",
        "31a",
        "32a", "32b", "32c", "32d",
        "37a",
        "40e", "40f", "40g",
        "41a", "41b", "41c", "41d",
        "42a", "42b",
        "43a",
        "44a",
        "45a",
        NULL
};
*/


/** 
* @brief Return enum eType for given instruction type string
* 
* @param s Instruction type string (which must match one of sType[] elements)
* 
* @return Index of sType (enum eType)
*/
int getType(char *s)
{
	int i;

	for(i = 0; sType[i] != NULL; i++){
		if(!strcasecmp(sType[i], s)){	/* if a match found */
			return i;
		}
	}
	return 0;	/* if not found */
}
