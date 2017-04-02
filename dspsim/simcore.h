/*
All Rights Reserved.
*/
/** 
* @file simcore.h
* @brief Header for simulator's main loop description
* @date 2008-09-24
*/

#ifndef	_SIMCORE_H
#define	_SIMCORE_H

int simCore(sICodeList icode);
sICode *asmSimOneStep(sICode *p, sICodeList icode);    
sICode *asmSimOneStepMultiFunc(sICode *p);
void initSim(void);
void resetSim(void);
void initDumpIn(void);        
void closeSim(void);
void closeDumpOut(void);        
int codeScan(sICodeList *icode);
sICode *codeScanOneInst(sICode *p);
sICode *codeScanOneInstMultiFunc(sICode *p);

#endif	/* _SIMCORE_H */
