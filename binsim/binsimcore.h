/*
All Rights Reserved.
*/
/** 
* @file binsimcore.h
* @brief Header for binary simulator's main loop description
* @date 2009-02-05
*/

#ifndef	_BINSIMCORE_H
#define	_BINSIMCORE_H

#include "bcode.h"

int simCore(sBCodeList bcode);
sBCode *binSimOneStep(sBCode *p);
sBCode *binSimOneStepMultiFunc(sBCode *p);
void initSim(void);
void closeSim(void);
int codeScan(sBCodeList *bcode);
sBCode *codeScanOneInst(sBCode *p);
//sBCode *codeScanOneInstMultiFunc(sBCode *p);

#endif	/* _BINSIMCORE_H */
