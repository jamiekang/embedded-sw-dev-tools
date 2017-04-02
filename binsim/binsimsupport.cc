/*
All Rights Reserved.
*/

/** 
* @file binsimsupport.cc
* @brief Misc supporting routines for binary simulator main
* @date 2009-02-05
*/

#include <stdio.h>	/* perror() */
#include <stdlib.h>	/* strtol(), exit() */
#include <string.h>	/* strcpy() */
#include <ctype.h>	/* isdigit() */
#include <assert.h> /* assert() */
#include "binsim.h"
//#include "symtab.h"
#include "parse.h"
#include "bcode.h"
#include "dmem.h"
#include "dspdef.h"
#include "binsimcore.h"
#include "binsimsupport.h"
#include "stack.h"
#include "cordic.h"

/** Register set definitions */
int rR[32];		/**< Rx data registers: 12b */
sAcc rAcc[8];	/**< Accumulator registers: 32/36b */
int rI[8];		/**< Ix registers: 16b */
int rM[8];		/**< Mx registers: 16b */
int rL[8];		/**< Lx registers: 16b */
int rB[8];		/**< Bx registers: 16b */

/* backup copies of Ix/Mx/Lx/Bx (not real registers): 2009.07.08 */
int rbI[8];      /**< Ix registers: 16b */
int rbM[8];      /**< Mx registers: 16b */
int rbL[8];      /**< Lx registers: 16b */
int rbB[8];      /**< Bx registers: 16b */
 
/**< to record when Ix/Mx/Lx/Bx registers were written: 2009.07.08 */
int LastAccessIx[8];    
int LastAccessMx[8];    
int LastAccessLx[8];    
int LastAccessBx[8];    

int rLPSTACK;   /**< LPSTACK */
int rPCSTACK;   /**< PCSTACK */
int	rLPEVER;	/**< LPEVER: set if DO UNTIL FOREVER */
sIcntl rICNTL;     /**< ICNTL */
sIrptl rIMASK;     /**< IMASK */
sIrptl rIRPTL;     /**< IRPTL */
int rCNTR;		/**< CNTR */
//int rCACTL;     /**< CACTL */
//int rPX;        /**< PX */
int rIVEC[4];   /**< IVECx registers: 16b */

sAstat rAstatR;	/**< ASTAT.R: Note - Real instruction affects only ASTAT.R */
sAstat rAstatI;	/**< ASTAT.I */
sAstat rAstatC;	/**< ASTAT.C: Note - Complex instruction affects ASTAT.R, ASTAT.I, and ASTAT.C */
sMstat rMstat;	/**< MSTAT */
sSstat rSstat;	/**< SSTAT */

/* backup copy of MSTAT (not a real register): 2009.07.08 */
sMstat rbMstat;	

/**< to record when MSTAT register was written: 2009.07.08 */
int LastAccessMstat;

/** Hardware stacks */
sStack PCStack;				/**< PC Stack */
sStack LoopBeginStack;		/**< Loop Begin Stack */
sStack LoopEndStack;		/**< Loop End Stack */
sStack LoopCounterStack;	/**< Loop Counter Stack */
sStack StatusStack[4];		/**< Status Stack */
sStack LPEVERStack;	        /**< LPEVER Stack */

/** Simulator internal variables */
int oldPC = UNDEFINED, PC;


/** 
* @brief Get register ID of given fval according to regtype
* 
* @param p Pointer to current instruction
* @param fval Integer value of register field
* @param regtype eRegType value (i.e. rDREG12, ...)
* 
* @return Register ID (Similar to 7-bit RREG: defined as enum eRegIndex in dspdef.h)
*/
int getRegIndex(sBCode *p, int fval, int regtype)
{
	int ridx = UNDEF_REG;
	switch(regtype){
		case eREG12:
		case eXOP12:
		case eYOP12:
		case eREG12S:
		case eXOP12S:
		case eYOP12S:
			ridx = fval;
			break;
		case eDREG12S:
			if(fval <= 0x7)		/* Rx registers */
				ridx = fval;
			else{				/* ACCx.L registers */
				ridx = (fval << 2);
			}
			break;
		case eDREG12:
			if(fval == 0x3F)	/* ALU NONE */
				ridx = eNONE;
			else
				ridx = fval;
			break;
		case eREG24:
		case eXOP24:
		case eYOP24:
		case eREG24S:
		case eXOP24S:
		case eYOP24S:
			ridx = (fval << 1);
			break;
		case eDREG24:
			if(fval == 0x1F)	/* ALU NONE */
				ridx = eNONE;
			else{
				if(fval > 0x10){	/* if ACC?.[L/M/H] */
					ridx = ((0x1C & fval)<<1) + (0x3 & fval);
				}else				/* if Rx */
					ridx = (fval << 1);
			}
			break;
		case eDREG24S:
			if(fval <= 0x3)		/* Rx registers */
				ridx = (fval << 1);
			else{				/* ACCx.L registers */
				ridx = (fval << 3);
			}
			break;
		case eRREG16:
			ridx = fval + 0x40;
			break;
		case eACC32:
			ridx = eACC0 + (fval << 2);
			break;
		case eACC32S:
			ridx = eACC0 + (fval << 2);
			break;
		case eACC64:
			ridx = eACC0 + (fval << 3);
			break;
		case eACC64S:
			ridx = eACC0 + (fval << 3);
			break;
		case eIREG:
			ridx = eI0 + fval;
			break;
		case eIX:
			ridx = eI0 + fval;
			break;
		case eIY:
			ridx = eI4 + fval;
			break;
		case eMREG:
			ridx = eM0 + fval;
			break;
		case eMX:
			ridx = eM0 + fval;
			break;
		case eMY:
			ridx = eM4 + fval;
			break;
		default:
			/* print error message */
			printRunTimeError(p->PMA, (char *)sOp[p->Index], 
				"Parse Error: getRegIndex().\n");
			break;
	}

	return ridx;
}

/** 
* @brief Read register data (12-bit)
* 
* @param p Pointer to current instruction
* @param ridx Register ID
* 
* @return Register data (12-bit)
*/
int RdReg(sBCode *p, int ridx)
{
	int val;

	switch(ridx){
		case eR0:		/* Rx register */
		case eR1:
		case eR2:
		case eR3:
		case eR4:
		case eR5:
		case eR6:
		case eR7:
		case eR8:
		case eR9:
		case eR10:
		case eR11:
		case eR12:
		case eR13:
		case eR14:
		case eR15:
		case eR16:
		case eR17:
		case eR18:
		case eR19:
		case eR20:
		case eR21:
		case eR22:
		case eR23:
		case eR24:
		case eR25:
		case eR26:
		case eR27:
		case eR28:
		case eR29:
		case eR30:
		case eR31:
			val = rR[ridx - eR0];
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC0_L:
			val = rAcc[0].L;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC0_M:
			val = rAcc[0].M;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC0_H:
			val = rAcc[0].H;
			if(isNeg(val, 8))	val |= 0xFFFFFF00;
			break;
		case eACC0:		/* ACC0 */
			val = rAcc[0].value;
			break;
		case eACC1_L:
			val = rAcc[1].L;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC1_M:
			val = rAcc[1].M;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC1_H:
			val = rAcc[1].H;
			if(isNeg(val, 8))	val |= 0xFFFFFF00;
			break;
		case eACC1:		/* ACC1 */
			val = rAcc[1].value;
			break;
		case eACC2_L:
			val = rAcc[2].L;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC2_M:
			val = rAcc[2].M;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC2_H:
			val = rAcc[2].H;
			if(isNeg(val, 8))	val |= 0xFFFFFF00;
			break;
		case eACC2:		/* ACC2 */
			val = rAcc[2].value;
			break;
		case eACC3_L:
			val = rAcc[3].L;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC3_M:
			val = rAcc[3].M;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC3_H:
			val = rAcc[3].H;
			if(isNeg(val, 8))	val |= 0xFFFFFF00;
			break;
		case eACC3:		/* ACC3 */
			val = rAcc[3].value;
			break;
		case eACC4_L:
			val = rAcc[4].L;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC4_M:
			val = rAcc[4].M;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC4_H:
			val = rAcc[4].H;
			if(isNeg(val, 8))	val |= 0xFFFFFF00;
			break;
		case eACC4:		/* ACC4 */
			val = rAcc[4].value;
			break;
		case eACC5_L:
			val = rAcc[5].L;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC5_M:
			val = rAcc[5].M;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC5_H:
			val = rAcc[5].H;
			if(isNeg(val, 8))	val |= 0xFFFFFF00;
			break;
		case eACC5:		/* ACC5 */
			val = rAcc[5].value;
			break;
		case eACC6_L:
			val = rAcc[6].L;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC6_M:
			val = rAcc[6].M;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC6_H:
			val = rAcc[6].H;
			if(isNeg(val, 8))	val |= 0xFFFFFF00;
			break;
		case eACC6:		/* ACC6 */
			val = rAcc[6].value;
			break;
		case eACC7_L:
			val = rAcc[7].L;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC7_M:
			val = rAcc[7].M;
			if(isNeg(val, 12))	val |= 0xFFFFF000;
			break;
		case eACC7_H:
			val = rAcc[7].H;
			if(isNeg(val, 8))	val |= 0xFFFFFF00;
			break;
		case eACC7:		/* ACC7 */
			val = rAcc[7].value;
			break;
		case eI0:		/* Ix register */
		case eI1:
		case eI2:
		case eI3:
		case eI4:
		case eI5:
		case eI6:
		case eI7:
			val = rI[ridx - eI0];
			break;
		case eM0:		/* Mx register */
		case eM1:
		case eM2:
		case eM3:
		case eM4:
		case eM5:
		case eM6:
		case eM7:
			val = rM[ridx - eM0];
			break;
		case eL0:		/* Lx register */
		case eL1:
		case eL2:
		case eL3:
		case eL4:
		case eL5:
		case eL6:
		case eL7:
			val = rL[ridx - eL0];
			break;
		case eB0:		/* Bx register */
		case eB1:
		case eB2:
		case eB3:
		case eB4:
		case eB5:
		case eB6:
		case eB7:
			val = rB[ridx - eB0];
			break;
		case eCNTR:		/* CNTR */
			val = rCNTR;
			break;
		case eMSTAT:	/* MSTAT */
			val = 0;
			val |= rMstat.MB; val <<= 1;	/* bit 7: different from ADSP-219x */
			val |= rMstat.SD; val <<= 1;	/* bit 6 */
			val |= rMstat.TI; val <<= 1;	/* bit 5 */
			val |= rMstat.MM; val <<= 1;	/* bit 4 */
			val |= rMstat.AS; val <<= 1;	/* bit 3 */
			val |= rMstat.OL; val <<= 1;	/* bit 2 */
			val |= rMstat.BR; val <<= 1;	/* bit 1 */
			val |= rMstat.SR; 				/* bit 0 */
			break;
		case eSSTAT:	/* SSTAT */
			val = 0;
			val |= rSstat.SOV; val <<= 1;	/* bit 7 */
			val |= rSstat.SSE; val <<= 1;	/* bit 6 */
			val |= rSstat.LSF; val <<= 1;	/* bit 5 */
			val |= rSstat.LSE; val <<= 1;	/* bit 4 */
			val <<= 1;	/* bit 3 */
			val |= rSstat.PCL; val <<= 1;	/* bit 2 */
			val |= rSstat.PCF; val <<= 1;	/* bit 1 */
			val |= rSstat.PCE; 				/* bit 0 */
			break;
		case eLPSTACK:	/* LPSTACK */
			val = rLPSTACK;
			break;
		case ePCSTACK:	/* PCSTACK */
			val = rPCSTACK;
			break;
		case eASTAT_R:	/* ASTAT.R */
			val = 0;
			val |= rAstatR.SV; val <<= 1;	/* bit 8 */
			val |= rAstatR.SS; val <<= 1;	/* bit 7 */
			val |= rAstatR.MV; val <<= 1;	/* bit 6 */
			val |= rAstatR.AQ; val <<= 1;	/* bit 5 */
			val |= rAstatR.AS; val <<= 1;	/* bit 4 */
			val |= rAstatR.AC; val <<= 1;	/* bit 3 */
			val |= rAstatR.AV; val <<= 1;	/* bit 2 */
			val |= rAstatR.AN; val <<= 1;	/* bit 1 */
			val |= rAstatR.AZ; 				/* bit 0 */
			break;
		case eASTAT_I:	/* ASTAT.I */
			val = 0;
			val |= rAstatI.SV; val <<= 1;	/* bit 8 */
			val |= rAstatI.SS; val <<= 1;	/* bit 7 */
			val |= rAstatI.MV; val <<= 1;	/* bit 6 */
			val |= rAstatI.AQ; val <<= 1;	/* bit 5 */
			val |= rAstatI.AS; val <<= 1;	/* bit 4 */
			val |= rAstatI.AC; val <<= 1;	/* bit 3 */
			val |= rAstatI.AV; val <<= 1;	/* bit 2 */
			val |= rAstatI.AN; val <<= 1;	/* bit 1 */
			val |= rAstatI.AZ; 				/* bit 0 */
			break;
		case eASTAT_C:	/* ASTAT.C */
			val = 0;
			val |= rAstatC.SV; val <<= 1;	/* bit 8 */
			val |= rAstatC.SS; val <<= 1;	/* bit 7 */
			val |= rAstatC.MV; val <<= 1;	/* bit 6 */
			val |= rAstatC.AQ; val <<= 1;	/* bit 5 */
			val |= rAstatC.AS; val <<= 1;	/* bit 4 */
			val |= rAstatC.AC; val <<= 1;	/* bit 3 */
			val |= rAstatC.AV; val <<= 1;	/* bit 2 */
			val |= rAstatC.AN; val <<= 1;	/* bit 1 */
			val |= rAstatC.AZ; 				/* bit 0 */
			break;
		case eICNTL:	/* ICNTL */
			val = 0;
			val <<= 1;	/* bit 7 */
			val <<= 1;	/* bit 6 */
			val |= rICNTL.GIE; val <<= 1;	/* bit 5 */
			val |= rICNTL.INE; val <<= 1;	/* bit 4 */
			break;
		case eIMASK:	/* IMASK */
        	val = 0;
			for(int i = 0; i < 12; i++){
           		val |= rIMASK.UserDef[11-i]; val <<= 1; /* bit 15-i */
        	}
			val |= rIMASK.STACK; val <<= 1; /* bit 3 */
			val |= rIMASK.SSTEP; val <<= 1; /* bit 2 */
			val |= rIMASK.PWDN;  val <<= 1; /* bit 1 */
			val |= rIMASK.EMU;              /* bit 0 */
			break;
		case eIRPTL:	/* IRPTL */
        	val = 0;
			for(int i = 0; i < 12; i++){
           		val |= rIRPTL.UserDef[11-i]; val <<= 1; /* bit 15-i */
        	}
			val |= rIRPTL.STACK; val <<= 1; /* bit 3 */
			val |= rIRPTL.SSTEP; val <<= 1; /* bit 2 */
			val |= rIRPTL.PWDN;  val <<= 1; /* bit 1 */
			val |= rIRPTL.EMU;              /* bit 0 */
			break;
		case eCACTL:	/* CACTL */
			//val = rCACTL;
			break;
		case ePX:		/* PX */
			//val = rPX;
			break;
		case eLPEVER:	/* LPEVER */
			val = rLPEVER;
			break;
		case eIVEC0:	/* IVECx register */
		case eIVEC1:
		case eIVEC2:
		case eIVEC3:
			val = rIVEC[ridx - eIVEC0];
			break;
		case eNONE:		/* ALU NONE */
			val = 0;
			break;
		default: 		/* other special registers should be added here */
			val = UNDEFINED;
			break;
	}										
	return val;
}

/** 
* @brief Read Ix/Mx/Lx/Mx/MSTAT register data (16-bit) considering latency restriction
* 
* @param p Pointer to current instruction
* @param ridx Register ID
* 
* @return Register data (16-bit)
*/
int RdReg2(sBCode *p, int ridx)
{
	int val;

	switch(ridx){
		case eMSTAT:	/* MSTAT */
			if((::Cycles - LastAccessMstat) <= 1){		/* restriction: if too close, use old value */
				val = 0;
				val |= rbMstat.MB; val <<= 1;	/* bit 7: different from ADSP-219x */
				val |= rbMstat.SD; val <<= 1;	/* bit 6 */
				val |= rbMstat.TI; val <<= 1;	/* bit 5 */
				val |= rbMstat.MM; val <<= 1;	/* bit 4 */
				val |= rbMstat.AS; val <<= 1;	/* bit 3 */
				val |= rbMstat.OL; val <<= 1;	/* bit 2 */
				val |= rbMstat.BR; val <<= 1;	/* bit 1 */
				val |= rbMstat.SR; 				/* bit 0 */

				if(VerboseMode) printf("RdReg2(): MSTAT: backup value used due to latency restriction.\n");
			}else{
				val = 0;
				val |= rMstat.MB; val <<= 1;	/* bit 7: different from ADSP-219x */
				val |= rMstat.SD; val <<= 1;	/* bit 6 */
				val |= rMstat.TI; val <<= 1;	/* bit 5 */
				val |= rMstat.MM; val <<= 1;	/* bit 4 */
				val |= rMstat.AS; val <<= 1;	/* bit 3 */
				val |= rMstat.OL; val <<= 1;	/* bit 2 */
				val |= rMstat.BR; val <<= 1;	/* bit 1 */
				val |= rMstat.SR; 				/* bit 0 */
			}
			break;
		case eI0:		/* Ix register */
		case eI1:
		case eI2:
		case eI3:
		case eI4:
		case eI5:
		case eI6:
		case eI7:
			{
				int rn = ridx - eI0;

				if((::Cycles - LastAccessIx[rn]) <= 1){		/* restriction: if too close, use old value */
					val = rbI[rn];

					if(VerboseMode) printf("RdReg2(): I%d: backup value used due to latency restriction.\n", rn);
				}else{
					val = rI[rn];
				}
			}
			break;
		case eM0:		/* Mx register */
		case eM1:
		case eM2:
		case eM3:
		case eM4:
		case eM5:
		case eM6:
		case eM7:
			{
				int rn = ridx - eM0;

				if((::Cycles - LastAccessMx[rn]) <= 1){		/* restriction: if too close, use old value */
					val = rbM[rn];

					if(VerboseMode) printf("RdReg2(): M%d: backup value used due to latency restriction.\n", rn);
				}else{
					val = rM[rn];
				}
			}
			break;
		case eL0:		/* Lx register */
		case eL1:
		case eL2:
		case eL3:
		case eL4:
		case eL5:
		case eL6:
		case eL7:
			{
				int rn = ridx - eL0;

				if((::Cycles - LastAccessLx[rn]) <= 1){		/* restriction: if too close, use old value */
					val = rbL[rn];

					if(VerboseMode) printf("RdReg2(): L%d: backup value used due to latency restriction.\n", rn);
				}else{
					val = rL[rn];
				}
			}
			break;
		case eB0:		/* Bx register */
		case eB1:
		case eB2:
		case eB3:
		case eB4:
		case eB5:
		case eB6:
		case eB7:
			{
				int rn = ridx - eB0;

				if((::Cycles - LastAccessBx[rn]) <= 1){		/* restriction: if too close, use old value */
					val = rbB[rn];

					if(VerboseMode) printf("RdReg2(): B%d: backup value used due to latency restriction.\n", rn);
				}else{
					val = rB[rn];
				}
			}
			break;
		default: 		/* other special registers should be added here */
			/* print error message */
			if(p){
				printRunTimeError(p->PMA, (char *)sOp[p->Index], 
					"Invalid case in RdReg2().\n");
			}else{
				printRunTimeError(UNDEFINED, "", 
					"Invalid case in RdReg2().\n");
			}
			return 0;
			break;
	}										
	return val;
}

/** 
* @brief Write data to the specified register (12-bit) 
* 
* @param p Pointer to current instruction
* @param ridx Register ID
* @param data Data to write (12-bit)
*/
void WrReg(sBCode *p, int ridx, int data)
{
	switch(ridx){
		case eR0:		/* Rx register */
		case eR1:
		case eR2:
		case eR3:
		case eR4:
		case eR5:
		case eR6:
		case eR7:
		case eR8:
		case eR9:
		case eR10:
		case eR11:
		case eR12:
		case eR13:
		case eR14:
		case eR15:
		case eR16:
		case eR17:
		case eR18:
		case eR19:
		case eR20:
		case eR21:
		case eR22:
		case eR23:
		case eR24:
		case eR25:
		case eR26:
		case eR27:
		case eR28:
		case eR29:
		case eR30:
		case eR31:
			rR[ridx - eR0] = 0xFFF & data;
			break;
		case eACC0_L:
			rAcc[0].L = 0xFFF & data;
			/* rAcc[x].value is updated here */
			rAcc[0].value =   (((rAcc[0].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[0].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[0].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC0_M:
			rAcc[0].M = 0xFFF & data;
			if(rAcc[0].M & 0x800){	/* sign extension */
				rAcc[0].H = 0xFF;
			}else{
				rAcc[0].H = 0x00;
			}
			/* rAcc[x].value is updated here */
			rAcc[0].value =   (((rAcc[0].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[0].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[0].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC0_H:
			rAcc[0].H = 0xFF & data;
			/* rAcc[x].value is updated here */
			rAcc[0].value =   (((rAcc[0].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[0].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[0].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC0:		/* ACC0 */
			rAcc[0].value = 0xFFFFFFFF & data;	
			/* rAcc[x].H/M/L is updated here */
			rAcc[0].H = ((data & 0xFF000000)>>24) & 0xFF ;
			rAcc[0].M = ((data & 0x00FFF000)>>12) & 0xFFF;
			rAcc[0].L =  (data & 0x00000FFF)      & 0xFFF;
			break;
		case eACC1_L:
			rAcc[1].L = 0xFFF & data;
			/* rAcc[x].value is updated here */
			rAcc[1].value =   (((rAcc[1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[1].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC1_M:
			rAcc[1].M = 0xFFF & data;
			if(rAcc[1].M & 0x800){	/* sign extension */
				rAcc[1].H = 0xFF;
			}else{
				rAcc[1].H = 0x00;
			}
			/* rAcc[x].value is updated here */
			rAcc[1].value =   (((rAcc[1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[1].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC1_H:
			rAcc[1].H = 0xFF & data;
			/* rAcc[x].value is updated here */
			rAcc[1].value =   (((rAcc[1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[1].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC1:		/* ACC1 */
			rAcc[1].value = 0xFFFFFFFF & data;	
			/* rAcc[x].H/M/L is updated here */
			rAcc[1].H = ((data & 0xFF000000)>>24) & 0xFF ;
			rAcc[1].M = ((data & 0x00FFF000)>>12) & 0xFFF;
			rAcc[1].L =  (data & 0x00000FFF)      & 0xFFF;
			break;
		case eACC2_L:
			rAcc[2].L = 0xFFF & data;
			/* rAcc[x].value is updated here */
			rAcc[2].value =   (((rAcc[2].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[2].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[2].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC2_M:
			rAcc[2].M = 0xFFF & data;
			if(rAcc[2].M & 0x800){	/* sign extension */
				rAcc[2].H = 0xFF;
			}else{
				rAcc[2].H = 0x00;
			}
			/* rAcc[x].value is updated here */
			rAcc[2].value =   (((rAcc[2].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[2].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[2].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC2_H:
			rAcc[2].H = 0xFF & data;
			/* rAcc[x].value is updated here */
			rAcc[2].value =   (((rAcc[2].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[2].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[2].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC2:		/* ACC2 */
			rAcc[2].value = 0xFFFFFFFF & data;	
			/* rAcc[x].H/M/L is updated here */
			rAcc[2].H = ((data & 0xFF000000)>>24) & 0xFF ;
			rAcc[2].M = ((data & 0x00FFF000)>>12) & 0xFFF;
			rAcc[2].L =  (data & 0x00000FFF)      & 0xFFF;
			break;
		case eACC3_L:
			rAcc[3].L = 0xFFF & data;
			/* rAcc[x].value is updated here */
			rAcc[3].value =   (((rAcc[3].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[3].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[3].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC3_M:
			rAcc[3].M = 0xFFF & data;
			if(rAcc[3].M & 0x800){	/* sign extension */
				rAcc[3].H = 0xFF;
			}else{
				rAcc[3].H = 0x00;
			}
			/* rAcc[x].value is updated here */
			rAcc[3].value =   (((rAcc[3].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[3].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[3].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC3_H:
			rAcc[3].H = 0xFF & data;
			/* rAcc[x].value is updated here */
			rAcc[3].value =   (((rAcc[3].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[3].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[3].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC3:		/* ACC3 */
			rAcc[3].value = 0xFFFFFFFF & data;	
			/* rAcc[x].H/M/L is updated here */
			rAcc[3].H = ((data & 0xFF000000)>>24) & 0xFF ;
			rAcc[3].M = ((data & 0x00FFF000)>>12) & 0xFFF;
			rAcc[3].L =  (data & 0x00000FFF)      & 0xFFF;
			break;
		case eACC4_L:
			rAcc[4].L = 0xFFF & data;
			/* rAcc[x].value is updated here */
			rAcc[4].value =   (((rAcc[4].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[4].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[4].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC4_M:
			rAcc[4].M = 0xFFF & data;
			if(rAcc[4].M & 0x800){	/* sign extension */
				rAcc[4].H = 0xFF;
			}else{
				rAcc[4].H = 0x00;
			}
			/* rAcc[x].value is updated here */
			rAcc[4].value =   (((rAcc[4].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[4].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[4].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC4_H:
			rAcc[4].H = 0xFF & data;
			/* rAcc[x].value is updated here */
			rAcc[4].value =   (((rAcc[4].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[4].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[4].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC4:		/* ACC4 */
			rAcc[4].value = 0xFFFFFFFF & data;	
			/* rAcc[x].H/M/L is updated here */
			rAcc[4].H = ((data & 0xFF000000)>>24) & 0xFF ;
			rAcc[4].M = ((data & 0x00FFF000)>>12) & 0xFFF;
			rAcc[4].L =  (data & 0x00000FFF)      & 0xFFF;
			break;
		case eACC5_L:
			rAcc[5].L = 0xFFF & data;
			/* rAcc[x].value is updated here */
			rAcc[5].value =   (((rAcc[5].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[5].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[5].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC5_M:
			rAcc[5].M = 0xFFF & data;
			if(rAcc[5].M & 0x800){	/* sign extension */
				rAcc[5].H = 0xFF;
			}else{
				rAcc[5].H = 0x00;
			}
			/* rAcc[x].value is updated here */
			rAcc[5].value =   (((rAcc[5].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[5].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[5].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC5_H:
			rAcc[5].H = 0xFF & data;
			/* rAcc[x].value is updated here */
			rAcc[5].value =   (((rAcc[5].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[5].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[5].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC5:		/* ACC5 */
			rAcc[5].value = 0xFFFFFFFF & data;	
			/* rAcc[x].H/M/L is updated here */
			rAcc[5].H = ((data & 0xFF000000)>>24) & 0xFF ;
			rAcc[5].M = ((data & 0x00FFF000)>>12) & 0xFFF;
			rAcc[5].L =  (data & 0x00000FFF)      & 0xFFF;
			break;
		case eACC6_L:
			rAcc[6].L = 0xFFF & data;
			/* rAcc[x].value is updated here */
			rAcc[6].value =   (((rAcc[6].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[6].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[6].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC6_M:
			rAcc[6].M = 0xFFF & data;
			if(rAcc[6].M & 0x800){	/* sign extension */
				rAcc[6].H = 0xFF;
			}else{
				rAcc[6].H = 0x00;
			}
			/* rAcc[x].value is updated here */
			rAcc[6].value =   (((rAcc[6].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[6].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[6].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC6_H:
			rAcc[6].H = 0xFF & data;
			/* rAcc[x].value is updated here */
			rAcc[6].value =   (((rAcc[6].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[6].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[6].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC6:		/* ACC6 */
			rAcc[6].value = 0xFFFFFFFF & data;	
			/* rAcc[x].H/M/L is updated here */
			rAcc[6].H = ((data & 0xFF000000)>>24) & 0xFF ;
			rAcc[6].M = ((data & 0x00FFF000)>>12) & 0xFFF;
			rAcc[6].L =  (data & 0x00000FFF)      & 0xFFF;
			break;
		case eACC7_L:
			rAcc[7].L = 0xFFF & data;
			/* rAcc[x].value is updated here */
			rAcc[7].value =   (((rAcc[7].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[7].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[7].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC7_M:
			rAcc[7].M = 0xFFF & data;
			if(rAcc[7].M & 0x800){	/* sign extension */
				rAcc[7].H = 0xFF;
			}else{
				rAcc[7].H = 0x00;
			}
			/* rAcc[x].value is updated here */
			rAcc[7].value =   (((rAcc[7].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[7].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[7].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC7_H:
			rAcc[7].H = 0xFF & data;
			/* rAcc[x].value is updated here */
			rAcc[7].value =   (((rAcc[7].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[7].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[7].L & 0xFFF)      & 0x00000FFF);
			break;
		case eACC7:		/* ACC7 */
			rAcc[7].value = 0xFFFFFFFF & data;	
			/* rAcc[x].H/M/L is updated here */
			rAcc[7].H = ((data & 0xFF000000)>>24) & 0xFF ;
			rAcc[7].M = ((data & 0x00FFF000)>>12) & 0xFFF;
			rAcc[7].L =  (data & 0x00000FFF)      & 0xFFF;
			break;
		case eI0:		/* Ix register */
		case eI1:
		case eI2:
		case eI3:
		case eI4:
		case eI5:
		case eI6:
		case eI7:
			{
				int rn = ridx - eI0;

				/* back up values */
				rbI[rn] = rI[rn];

				/* record when it's written: 2009.07.08 */
				LastAccessIx[rn] = ::Cycles;	

				/* update */
				rI[rn] = 0xFFFF & data;
			}
			break;
		case eM0:		/* Mx register */
		case eM1:
		case eM2:
		case eM3:
		case eM4:
		case eM5:
		case eM6:
		case eM7:
			{
				int rn = ridx - eM0;

				/* back up values */
				rbM[rn] = rM[rn];

				/* record when it's written: 2009.07.08 */
				LastAccessMx[rn] = ::Cycles;	

				/* update */
				rM[rn] = 0xFFFF & data;
			}
			break;
		case eL0:		/* Lx register */
		case eL1:
		case eL2:
		case eL3:
		case eL4:
		case eL5:
		case eL6:
		case eL7:
			{
				int rn = ridx - eL0;

				/* back up values */
				rbL[rn] = rL[rn];

				/* record when it's written: 2009.07.08 */
				LastAccessLx[rn] = ::Cycles;	

				/* update */
				rL[rn] = 0xFFFF & data;
			}
			break;
		case eB0:		/* Bx register */
		case eB1:
		case eB2:
		case eB3:
		case eB4:
		case eB5:
		case eB6:
		case eB7:
			{
				int rn = ridx - eB0;

				/* back up values */
				rbB[rn] = rB[rn];

				/* record when it's written: 2009.07.08 */
				LastAccessBx[rn] = ::Cycles;	

				/* update */
				rB[rn] = 0xFFFF & data;
			}
			break;
		case eCNTR:		/* CNTR */
			rCNTR = 0xFFFF & data;
			break;
		case eMSTAT:	/* MSTAT */
			/* back up values */
			rbMstat = rMstat;

			/* record when it's written: 2009.07.08 */
			LastAccessMstat = ::Cycles;	

			/* update */
			rMstat.SR = (data & 0x0001)? 1: 0;		/* bit 0 */
			rMstat.BR = (data & 0x0002)? 1: 0;		/* bit 1 */
			rMstat.OL = (data & 0x0004)? 1: 0;		/* bit 2 */
			rMstat.AS = (data & 0x0008)? 1: 0;		/* bit 3 */
			rMstat.MM = (data & 0x0010)? 1: 0;		/* bit 4 */
			rMstat.TI = (data & 0x0020)? 1: 0;		/* bit 5 */
			rMstat.SD = (data & 0x0040)? 1: 0;		/* bit 6 */
			rMstat.MB = (data & 0x0080)? 1: 0;		/* bit 7: different from ADSP-219x */
			break;
		case eSSTAT:	/* SSTAT */
			rSstat.PCE = (data & 0x0001)? 1: 0;		/* bit 0 */
			rSstat.PCF = (data & 0x0002)? 1: 0;		/* bit 1 */
			rSstat.PCL = (data & 0x0004)? 1: 0;		/* bit 2 */
			rSstat.LSE = (data & 0x0010)? 1: 0;		/* bit 4 */
			rSstat.LSF = (data & 0x0020)? 1: 0;		/* bit 5 */
			rSstat.SSE = (data & 0x0040)? 1: 0;		/* bit 6 */
			rSstat.SOV = (data & 0x0080)? 1: 0;		/* bit 7: different from ADSP-219x */
			break;
		case eLPSTACK:	/* LPSTACK */
			rLPSTACK = 0xFFFF & data;
			break;
		case ePCSTACK:	/* PCSTACK */
			rPCSTACK = 0xFFFF & data;
			break;
		case eASTAT_R:	/* ASTAT.R */
			rAstatR.AZ = (data & 0x0001)? 1: 0;		/* bit 0 */
			rAstatR.AN = (data & 0x0002)? 1: 0;		/* bit 1 */
			rAstatR.AV = (data & 0x0004)? 1: 0;		/* bit 2 */
			rAstatR.AC = (data & 0x0008)? 1: 0;		/* bit 3 */
			rAstatR.AS = (data & 0x0010)? 1: 0;		/* bit 4 */
			rAstatR.AQ = (data & 0x0020)? 1: 0;		/* bit 5 */
			rAstatR.MV = (data & 0x0040)? 1: 0;		/* bit 6 */
			rAstatR.SS = (data & 0x0080)? 1: 0;		/* bit 7 */
			rAstatR.SV = (data & 0x0100)? 1: 0;		/* bit 8 */
			break;
		case eASTAT_I:	/* ASTAT.I */
			rAstatI.AZ = (data & 0x0001)? 1: 0;		/* bit 0 */
			rAstatI.AN = (data & 0x0002)? 1: 0;		/* bit 1 */
			rAstatI.AV = (data & 0x0004)? 1: 0;		/* bit 2 */
			rAstatI.AC = (data & 0x0008)? 1: 0;		/* bit 3 */
			rAstatI.AS = (data & 0x0010)? 1: 0;		/* bit 4 */
			rAstatI.AQ = (data & 0x0020)? 1: 0;		/* bit 5 */
			rAstatI.MV = (data & 0x0040)? 1: 0;		/* bit 6 */
			rAstatI.SS = (data & 0x0080)? 1: 0;		/* bit 7 */
			rAstatI.SV = (data & 0x0100)? 1: 0;		/* bit 8 */
			break;
		case eASTAT_C:	/* ASTAT.C */
			rAstatC.AZ = (data & 0x0001)? 1: 0;		/* bit 0 */
			rAstatC.AN = (data & 0x0002)? 1: 0;		/* bit 1 */
			rAstatC.AV = (data & 0x0004)? 1: 0;		/* bit 2 */
			rAstatC.AC = (data & 0x0008)? 1: 0;		/* bit 3 */
			rAstatC.AS = (data & 0x0010)? 1: 0;		/* bit 4 */
			rAstatC.AQ = (data & 0x0020)? 1: 0;		/* bit 5 */
			rAstatC.MV = (data & 0x0040)? 1: 0;		/* bit 6 */
			rAstatC.SS = (data & 0x0080)? 1: 0;		/* bit 7 */
			rAstatC.SV = (data & 0x0100)? 1: 0;		/* bit 8 */
			break;
		case eICNTL:	/* ICNTL */
			rICNTL.INE = (data & 0x0010)? 1: 0;		/* bit 4 */
			rICNTL.GIE = (data & 0x0020)? 1: 0;		/* bit 5 */
			break;
		case eIMASK:	/* IMASK */
        	rIMASK.EMU         = (data & 0x0001)? 1: 0;     /* bit 0 */
        	rIMASK.PWDN        = (data & 0x0002)? 1: 0;     /* bit 1 */
        	rIMASK.SSTEP       = (data & 0x0004)? 1: 0;     /* bit 2 */
        	rIMASK.STACK       = (data & 0x0008)? 1: 0;     /* bit 3 */
        	rIMASK.UserDef[0]  = (data & 0x0010)? 1: 0;     /* bit 4 */
        	rIMASK.UserDef[1]  = (data & 0x0020)? 1: 0;     /* bit 5 */
        	rIMASK.UserDef[2]  = (data & 0x0040)? 1: 0;     /* bit 6 */
        	rIMASK.UserDef[3]  = (data & 0x0080)? 1: 0;     /* bit 7 */
        	rIMASK.UserDef[4]  = (data & 0x0100)? 1: 0;     /* bit 8 */
        	rIMASK.UserDef[5]  = (data & 0x0200)? 1: 0;     /* bit 9 */
        	rIMASK.UserDef[6]  = (data & 0x0400)? 1: 0;     /* bit 10 */
        	rIMASK.UserDef[7]  = (data & 0x0800)? 1: 0;     /* bit 11 */
        	rIMASK.UserDef[8]  = (data & 0x1000)? 1: 0;     /* bit 12 */
        	rIMASK.UserDef[9]  = (data & 0x2000)? 1: 0;     /* bit 13 */
        	rIMASK.UserDef[10] = (data & 0x4000)? 1: 0;     /* bit 14 */
        	rIMASK.UserDef[11] = (data & 0x8000)? 1: 0;     /* bit 15 */
			break;
		case eIRPTL:	/* IRPTL */
        	rIRPTL.EMU         = (data & 0x0001)? 1: 0;     /* bit 0 */
        	rIRPTL.PWDN        = (data & 0x0002)? 1: 0;     /* bit 1 */
        	rIRPTL.SSTEP       = (data & 0x0004)? 1: 0;     /* bit 2 */
        	rIRPTL.STACK       = (data & 0x0008)? 1: 0;     /* bit 3 */
        	rIRPTL.UserDef[0]  = (data & 0x0010)? 1: 0;     /* bit 4 */
        	rIRPTL.UserDef[1]  = (data & 0x0020)? 1: 0;     /* bit 5 */
        	rIRPTL.UserDef[2]  = (data & 0x0040)? 1: 0;     /* bit 6 */
        	rIRPTL.UserDef[3]  = (data & 0x0080)? 1: 0;     /* bit 7 */
        	rIRPTL.UserDef[4]  = (data & 0x0100)? 1: 0;     /* bit 8 */
        	rIRPTL.UserDef[5]  = (data & 0x0200)? 1: 0;     /* bit 9 */
        	rIRPTL.UserDef[6]  = (data & 0x0400)? 1: 0;     /* bit 10 */
        	rIRPTL.UserDef[7]  = (data & 0x0800)? 1: 0;     /* bit 11 */
        	rIRPTL.UserDef[8]  = (data & 0x1000)? 1: 0;     /* bit 12 */
        	rIRPTL.UserDef[9]  = (data & 0x2000)? 1: 0;     /* bit 13 */
        	rIRPTL.UserDef[10] = (data & 0x4000)? 1: 0;     /* bit 14 */
        	rIRPTL.UserDef[11] = (data & 0x8000)? 1: 0;     /* bit 15 */
			break;
		case eCACTL:	/* CACTL */
			//rCACTL = 0xFFFF & data;
			break;
		case ePX:		/* PX */
			//rPX = data;
			break;
		case eLPEVER:	/* LPEVER */
			rLPEVER = 0xFFFF & data;
			break;
		case eIVEC0:	/* IVECx register */
		case eIVEC1:
		case eIVEC2:
		case eIVEC3:
			rIVEC[ridx - eIVEC0] = 0xFFFF & data;
			break;
		case eNONE:		/* ALU NONE */
		default:
        	;   /* do nothing */
			break;
	}
}

/** 
* @brief Read AC of ASTAT.R 
* 
* @return 1 or 0
*/
int RdC1(void)
{
	return rAstatR.AC;
}

/**
* @brief Read AC of ASTAT.R & ASTAT.I
*
* @return AC pairs in cplx structure type
*/
cplx RdC2(void)
{
	cplx n;
	n.r = rAstatR.AC;
	n.i = rAstatI.AC;
	return n;
}
			   
/** 
* @brief Read complex register data (12-bit x2)
* 
* @param p Pointer to current instruction
* @param ridx Register ID
* 
* @return Complex register data (12-bit x2)
*/
cplx cRdReg(sBCode *p, int ridx)
{
	int addr;
	cplx n; 

	switch(ridx){
		case eR0:		/* Rx register */
		case eR2:
		case eR4:
		case eR6:
		case eR8:
		case eR10:
		case eR12:
		case eR14:
		case eR16:
		case eR18:
		case eR20:
		case eR22:
		case eR24:
		case eR26:
		case eR28:
		case eR30:
			addr = ridx - eR0;
			n.r = rR[addr];
			if(isNeg(n.r, 12)) n.r |= 0xFFFFF000;
			n.i = rR[addr+1];
			if(isNeg(n.i, 12)) n.i |= 0xFFFFF000;
			break;
		case eR1:
		case eR3:
		case eR5:
		case eR7:
		case eR9:
		case eR11:
		case eR13:
		case eR15:
		case eR17:
		case eR19:
		case eR21:
		case eR23:
		case eR25:
		case eR27:
		case eR29:
		case eR31:
			if(p){
				printRunTimeError(p->PMA, (char *)sOp[p->Index], 
					"Complex register pair should not end with an odd number.\n");
			}
			return n;
			break;
		case eACC0_L:
			addr = 0;
			n.r = rAcc[addr].L;
			if(isNeg(n.r, 12)) n.r |= 0xFFFFF000;
			n.i = rAcc[addr+1].L;
			if(isNeg(n.i, 12)) n.i |= 0xFFFFF000;
			break;
		case eACC0_M:
			addr = 0;
			n.r = rAcc[addr].M;
			if(isNeg(n.r, 12)) n.r |= 0xFFFFF000;
			n.i = rAcc[addr+1].M;
			if(isNeg(n.i, 12)) n.i |= 0xFFFFF000;
			break;
		case eACC0_H:
			addr = 0;
			n.r = rAcc[addr].H;
			if(isNeg(n.r, 8)) n.r |= 0xFFFFFF00;
			n.i = rAcc[addr+1].H;
			if(isNeg(n.i, 8)) n.i |= 0xFFFFFF00;
			break;
		case eACC0:		/* ACC0 */
			addr = 0;
			n.r = rAcc[addr].value;	
			n.i = rAcc[addr+1].value;	
			break;
		case eACC2_L:
			addr = 2;
			n.r = rAcc[addr].L;
			if(isNeg(n.r, 12)) n.r |= 0xFFFFF000;
			n.i = rAcc[addr+1].L;
			if(isNeg(n.i, 12)) n.i |= 0xFFFFF000;
			break;
		case eACC2_M:
			addr = 2;
			n.r = rAcc[addr].M;
			if(isNeg(n.r, 12)) n.r |= 0xFFFFF000;
			n.i = rAcc[addr+1].M;
			if(isNeg(n.i, 12)) n.i |= 0xFFFFF000;
			break;
		case eACC2_H:
			addr = 2;
			n.r = rAcc[addr].H;
			if(isNeg(n.r, 8)) n.r |= 0xFFFFFF00;
			n.i = rAcc[addr+1].H;
			if(isNeg(n.i, 8)) n.i |= 0xFFFFFF00;
			break;
		case eACC2:		/* ACC2 */
			addr = 2;
			n.r = rAcc[addr].value;	
			n.i = rAcc[addr+1].value;	
			break;
		case eACC4_L:
			addr = 4;
			n.r = rAcc[addr].L;
			if(isNeg(n.r, 12)) n.r |= 0xFFFFF000;
			n.i = rAcc[addr+1].L;
			if(isNeg(n.i, 12)) n.i |= 0xFFFFF000;
			break;
		case eACC4_M:
			addr = 4;
			n.r = rAcc[addr].M;
			if(isNeg(n.r, 12)) n.r |= 0xFFFFF000;
			n.i = rAcc[addr+1].M;
			if(isNeg(n.i, 12)) n.i |= 0xFFFFF000;
			break;
		case eACC4_H:
			addr = 4;
			n.r = rAcc[addr].H;
			if(isNeg(n.r, 8)) n.r |= 0xFFFFFF00;
			n.i = rAcc[addr+1].H;
			if(isNeg(n.i, 8)) n.i |= 0xFFFFFF00;
			break;
		case eACC4:		/* ACC4 */
			addr = 4;
			n.r = rAcc[addr].value;	
			n.i = rAcc[addr+1].value;	
			break;
		case eACC6_L:
			addr = 6;
			n.r = rAcc[addr].L;
			if(isNeg(n.r, 12)) n.r |= 0xFFFFF000;
			n.i = rAcc[addr+1].L;
			if(isNeg(n.i, 12)) n.i |= 0xFFFFF000;
			break;
		case eACC6_M:
			addr = 6;
			n.r = rAcc[addr].M;
			if(isNeg(n.r, 12)) n.r |= 0xFFFFF000;
			n.i = rAcc[addr+1].M;
			if(isNeg(n.i, 12)) n.i |= 0xFFFFF000;
			break;
		case eACC6_H:
			addr = 6;
			n.r = rAcc[addr].H;
			if(isNeg(n.r, 8)) n.r |= 0xFFFFFF00;
			n.i = rAcc[addr+1].H;
			if(isNeg(n.i, 8)) n.i |= 0xFFFFFF00;
			break;
		case eACC6:		/* ACC6 */
			addr = 6;
			n.r = rAcc[addr].value;	
			n.i = rAcc[addr+1].value;	
			break;
		case eACC1_L:
		case eACC1_M:
		case eACC1_H:
		case eACC1:		/* ACC1 */
		case eACC3_L:
		case eACC3_M:
		case eACC3_H:
		case eACC3:		/* ACC3 */
		case eACC5_L:
		case eACC5_M:
		case eACC5_H:
		case eACC5:		/* ACC5 */
		case eACC7_L:
		case eACC7_M:
		case eACC7_H:
		case eACC7:		/* ACC7 */
			if(p){
				printRunTimeError(p->PMA, (char *)sOp[p->Index], 
					"Complex register pair should not end with an odd number.\n");
			}
			return n;
			break;
		case eNONE:		/* ALU NONE */
			n.r = n.i = 0;
			break;
		default: 		/* other special registers should be added here */
			n.r = n.i = UNDEFINED;
			break;
	}
	return n;
}

/** 
* @brief Write complex data to the specified register pair (12-bit x2)
* 
* @param p Pointer to current instruction
* @param ridx Register ID
* @param data1 12-bit data for real part of complex data pair
* @param data2 12-bit data for imaginary part of complex data pair
*/
void cWrReg(sBCode *p, int ridx, int data1, int data2)
{
	int addr;

	switch(ridx){
		case eR0:		/* Rx register */
		case eR2:
		case eR4:
		case eR6:
		case eR8:
		case eR10:
		case eR12:
		case eR14:
		case eR16:
		case eR18:
		case eR20:
		case eR22:
		case eR24:
		case eR26:
		case eR28:
		case eR30:
			addr = ridx - eR0;
			rR[addr]   = 0xFFF & data1;
			rR[addr+1] = 0xFFF & data2;
			break;
		case eR1:
		case eR3:
		case eR5:
		case eR7:
		case eR9:
		case eR11:
		case eR13:
		case eR15:
		case eR17:
		case eR19:
		case eR21:
		case eR23:
		case eR25:
		case eR27:
		case eR29:
		case eR31:
			if(p){
				printRunTimeError(p->PMA, (char *)sOp[p->Index], 
					"Complex register pair should not end with an odd number.\n");
			}
			return;
			break;
		case eACC0_L:
			addr = 0;
			rAcc[addr].L   = 0xFFF & data1;
			rAcc[addr+1].L = 0xFFF & data2;
			/* rAcc[x].value is updated here */
			rAcc[addr].value =   (((rAcc[addr].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr].L & 0xFFF)      & 0x00000FFF);
			rAcc[addr+1].value =   (((rAcc[addr+1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr+1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr+1].L & 0xFFF)      & 0x00000FFF);
			break;	
		case eACC0_M:
			addr = 0;
			rAcc[addr].M   = 0xFFF & data1;
			if(rAcc[addr].M & 0x800){	/* sign extension */
				rAcc[addr].H = 0xFF;
			}else{
				rAcc[addr].H = 0x00;
			}
			rAcc[addr+1].M = 0xFFF & data2;
			if(rAcc[addr+1].M & 0x800){	/* sign extension */
				rAcc[addr+1].H = 0xFF;
			}else{
				rAcc[addr+1].H = 0x00;
			}
			/* rAcc[x].value is updated here */
			rAcc[addr].value =   (((rAcc[addr].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr].L & 0xFFF)      & 0x00000FFF);
			rAcc[addr+1].value =   (((rAcc[addr+1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr+1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr+1].L & 0xFFF)      & 0x00000FFF);
			break;	
		case eACC0_H:
			addr = 0;
			rAcc[addr].H   = 0xFF & data1;
			rAcc[addr+1].H = 0xFF & data2;
			/* rAcc[x].value is updated here */
			rAcc[addr].value =   (((rAcc[addr].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr].L & 0xFFF)      & 0x00000FFF);
			rAcc[addr+1].value =   (((rAcc[addr+1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr+1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr+1].L & 0xFFF)      & 0x00000FFF);
			break;	
		case eACC0:		/* ACC0 */
			addr = 0;
			rAcc[addr].value   = 0xFFFFFFFF & data1;	
			rAcc[addr+1].value = 0xFFFFFFFF & data2;	

			/* rAcc[x].H/M/L should be updated here */
			rAcc[addr].H =   ((data1 & 0xFF000000)>>24) & 0xFF ;
			rAcc[addr].M =   ((data1 & 0x00FFF000)>>12) & 0xFFF;
			rAcc[addr].L =    (data1 & 0x00000FFF)      & 0xFFF;
			rAcc[addr+1].H = ((data2 & 0xFF000000)>>24) & 0xFF ;
			rAcc[addr+1].M = ((data2 & 0x00FFF000)>>12) & 0xFFF;
			rAcc[addr+1].L =  (data2 & 0x00000FFF)      & 0xFFF;
			break;	
		case eACC2_L:
			addr = 2;
			rAcc[addr].L   = 0xFFF & data1;
			rAcc[addr+1].L = 0xFFF & data2;
			/* rAcc[x].value is updated here */
			rAcc[addr].value =   (((rAcc[addr].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr].L & 0xFFF)      & 0x00000FFF);
			rAcc[addr+1].value =   (((rAcc[addr+1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr+1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr+1].L & 0xFFF)      & 0x00000FFF);
			break;	
		case eACC2_M:
			addr = 2;
			rAcc[addr].M   = 0xFFF & data1;
			if(rAcc[addr].M & 0x800){	/* sign extension */
				rAcc[addr].H = 0xFF;
			}else{
				rAcc[addr].H = 0x00;
			}
			rAcc[addr+1].M = 0xFFF & data2;
			if(rAcc[addr+1].M & 0x800){	/* sign extension */
				rAcc[addr+1].H = 0xFF;
			}else{
				rAcc[addr+1].H = 0x00;
			}
			/* rAcc[x].value is updated here */
			rAcc[addr].value =   (((rAcc[addr].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr].L & 0xFFF)      & 0x00000FFF);
			rAcc[addr+1].value =   (((rAcc[addr+1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr+1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr+1].L & 0xFFF)      & 0x00000FFF);
			break;	
		case eACC2_H:
			addr = 2;
			rAcc[addr].H   = 0xFF & data1;
			rAcc[addr+1].H = 0xFF & data2;
			/* rAcc[x].value is updated here */
			rAcc[addr].value =   (((rAcc[addr].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr].L & 0xFFF)      & 0x00000FFF);
			rAcc[addr+1].value =   (((rAcc[addr+1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr+1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr+1].L & 0xFFF)      & 0x00000FFF);
			break;	
		case eACC2:		/* ACC2 */
			addr = 2;
			rAcc[addr].value   = 0xFFFFFFFF & data1;	
			rAcc[addr+1].value = 0xFFFFFFFF & data2;	

			/* rAcc[x].H/M/L should be updated here */
			rAcc[addr].H =   ((data1 & 0xFF000000)>>24) & 0xFF ;
			rAcc[addr].M =   ((data1 & 0x00FFF000)>>12) & 0xFFF;
			rAcc[addr].L =    (data1 & 0x00000FFF)      & 0xFFF;
			rAcc[addr+1].H = ((data2 & 0xFF000000)>>24) & 0xFF ;
			rAcc[addr+1].M = ((data2 & 0x00FFF000)>>12) & 0xFFF;
			rAcc[addr+1].L =  (data2 & 0x00000FFF)      & 0xFFF;
			break;	
		case eACC4_L:
			addr = 4;
			rAcc[addr].L   = 0xFFF & data1;
			rAcc[addr+1].L = 0xFFF & data2;
			/* rAcc[x].value is updated here */
			rAcc[addr].value =   (((rAcc[addr].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr].L & 0xFFF)      & 0x00000FFF);
			rAcc[addr+1].value =   (((rAcc[addr+1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr+1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr+1].L & 0xFFF)      & 0x00000FFF);
			break;	
		case eACC4_M:
			addr = 4;
			rAcc[addr].M   = 0xFFF & data1;
			if(rAcc[addr].M & 0x800){	/* sign extension */
				rAcc[addr].H = 0xFF;
			}else{
				rAcc[addr].H = 0x00;
			}
			rAcc[addr+1].M = 0xFFF & data2;
			if(rAcc[addr+1].M & 0x800){	/* sign extension */
				rAcc[addr+1].H = 0xFF;
			}else{
				rAcc[addr+1].H = 0x00;
			}
			/* rAcc[x].value is updated here */
			rAcc[addr].value =   (((rAcc[addr].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr].L & 0xFFF)      & 0x00000FFF);
			rAcc[addr+1].value =   (((rAcc[addr+1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr+1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr+1].L & 0xFFF)      & 0x00000FFF);
			break;	
		case eACC4_H:
			addr = 4;
			rAcc[addr].H   = 0xFF & data1;
			rAcc[addr+1].H = 0xFF & data2;
			/* rAcc[x].value is updated here */
			rAcc[addr].value =   (((rAcc[addr].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr].L & 0xFFF)      & 0x00000FFF);
			rAcc[addr+1].value =   (((rAcc[addr+1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr+1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr+1].L & 0xFFF)      & 0x00000FFF);
			break;	
		case eACC4:		/* ACC4 */
			addr = 4;
			rAcc[addr].value   = 0xFFFFFFFF & data1;	
			rAcc[addr+1].value = 0xFFFFFFFF & data2;	

			/* rAcc[x].H/M/L should be updated here */
			rAcc[addr].H =   ((data1 & 0xFF000000)>>24) & 0xFF ;
			rAcc[addr].M =   ((data1 & 0x00FFF000)>>12) & 0xFFF;
			rAcc[addr].L =    (data1 & 0x00000FFF)      & 0xFFF;
			rAcc[addr+1].H = ((data2 & 0xFF000000)>>24) & 0xFF ;
			rAcc[addr+1].M = ((data2 & 0x00FFF000)>>12) & 0xFFF;
			rAcc[addr+1].L =  (data2 & 0x00000FFF)      & 0xFFF;
			break;	
		case eACC6_L:
			addr = 6;
			rAcc[addr].L   = 0xFFF & data1;
			rAcc[addr+1].L = 0xFFF & data2;
			/* rAcc[x].value is updated here */
			rAcc[addr].value =   (((rAcc[addr].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr].L & 0xFFF)      & 0x00000FFF);
			rAcc[addr+1].value =   (((rAcc[addr+1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr+1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr+1].L & 0xFFF)      & 0x00000FFF);
			break;	
		case eACC6_M:
			addr = 6;
			rAcc[addr].M   = 0xFFF & data1;
			if(rAcc[addr].M & 0x800){	/* sign extension */
				rAcc[addr].H = 0xFF;
			}else{
				rAcc[addr].H = 0x00;
			}
			rAcc[addr+1].M = 0xFFF & data2;
			if(rAcc[addr+1].M & 0x800){	/* sign extension */
				rAcc[addr+1].H = 0xFF;
			}else{
				rAcc[addr+1].H = 0x00;
			}
			/* rAcc[x].value is updated here */
			rAcc[addr].value =   (((rAcc[addr].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr].L & 0xFFF)      & 0x00000FFF);
			rAcc[addr+1].value =   (((rAcc[addr+1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr+1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr+1].L & 0xFFF)      & 0x00000FFF);
			break;	
		case eACC6_H:
			addr = 6;
			rAcc[addr].H   = 0xFF & data1;
			rAcc[addr+1].H = 0xFF & data2;
			/* rAcc[x].value is updated here */
			rAcc[addr].value =   (((rAcc[addr].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr].L & 0xFFF)      & 0x00000FFF);
			rAcc[addr+1].value =   (((rAcc[addr+1].H & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr+1].M & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr+1].L & 0xFFF)      & 0x00000FFF);
			break;	
		case eACC6:		/* ACC6 */
			addr = 6;
			rAcc[addr].value   = 0xFFFFFFFF & data1;	
			rAcc[addr+1].value = 0xFFFFFFFF & data2;	

			/* rAcc[x].H/M/L should be updated here */
			rAcc[addr].H =   ((data1 & 0xFF000000)>>24) & 0xFF ;
			rAcc[addr].M =   ((data1 & 0x00FFF000)>>12) & 0xFFF;
			rAcc[addr].L =    (data1 & 0x00000FFF)      & 0xFFF;
			rAcc[addr+1].H = ((data2 & 0xFF000000)>>24) & 0xFF ;
			rAcc[addr+1].M = ((data2 & 0x00FFF000)>>12) & 0xFFF;
			rAcc[addr+1].L =  (data2 & 0x00000FFF)      & 0xFFF;
			break;	
		case eACC1_L:
		case eACC1_M:
		case eACC1_H:
		case eACC1:		/* ACC1 */
		case eACC3_L:
		case eACC3_M:
		case eACC3_H:
		case eACC3:		/* ACC3 */
		case eACC5_L:
		case eACC5_M:
		case eACC5_H:
		case eACC5:		/* ACC5 */
		case eACC7_L:
		case eACC7_M:
		case eACC7_H:
		case eACC7:		/* ACC7 */
			if(p){
				printRunTimeError(p->PMA, (char *)sOp[p->Index], 
					"Complex register pair should not end with an odd number.\n");
			}
			return;
			break;
		case eNONE:		/* ALU NONE */
		default:
        	;   /* do nothing */
			break;
    }
}

/** 
* Set/clear affected flags of status registers. 
* Note: this function accesses global variables.
* 
* @param type Instruction type of current instruction.
* @param z zero flag	
* @param n negative flag	
* @param v overflow flag	
* @param c carry flag	
*/
void flagEffect(int type, int z, int n, int v, int c)
{
	switch(type){
		case t09c:	/* ADD, SUB */
		case t09e:	
		case t09i:	
		case t09g:	/* ADD, SUB ACC32 */
		case t01c: /* ALU || LD || LD */
		case t04c: /* ALU || LD */
		case t04g: /* ALU || ST */
		case t08c: /* ALU || CP */
		case t43a: /* ALU || MAC */
		case t44b: /* ALU || SHIFT */
		case t44d: /* ALU || SHIFT */
		case t46a: /* SCR */
			rAstatR.AZ = z;
			rAstatR.AN = n;
			if(!isAVLatchedMode()){		/* if AV_LATCH is not set */
				rAstatR.AV = v;			/* work as usual */	
				if(v) OVCount++;
			} else {					/* if set, update only if AV is 0 */
				if(rAstatR.AV == 0 && v == 1){
					rAstatR.AV = 1;
					OVCount++;
				}
			}
			rAstatR.AC = c;
			break;
		case t41a:	/* RNDACC */
			rAstatR.MV = v;
			if(v) OVCount++;
			break;
		case t41c:	/* CLRACC */
			rAstatR.AZ = 1;
			rAstatR.AN = 0;
			if(!isAVLatchedMode())		/* update AV only if AV_LATCH is not set */
				rAstatR.AV = 0;
			rAstatR.AC = 0;
			break;
		case t11a:	/* DO UNTIL */
			//rSstat.LSE = 0;
			rSstat.LSE = stackEmpty(&LoopBeginStack);
			rSstat.LSF = stackFull(&LoopBeginStack); 
			rSstat.SOV |= rSstat.LSF;
			break;
		case t26a: 	/* PUSH/POP */
			rSstat.PCE = stackEmpty(&PCStack);
			rSstat.PCF = stackFull(&PCStack);
			rSstat.PCL = stackCheckLevel(&PCStack);
			rSstat.LSE = stackEmpty(&LoopBeginStack);
			rSstat.LSF = stackFull(&LoopBeginStack);
			rSstat.SSE = stackEmpty(&StatusStack[0]);
			rSstat.SOV = PCStack.Overflow + PCStack.Underflow
						+ LoopBeginStack.Overflow + LoopBeginStack.Underflow
						+ StatusStack[0].Overflow + StatusStack[0].Underflow;
			break;
		case t37a: 	/* SETINT */
			rSstat.PCE = stackEmpty(&PCStack);
			rSstat.PCF = stackFull(&PCStack);
			rSstat.PCL = stackCheckLevel(&PCStack);
			rSstat.SSE = stackEmpty(&StatusStack[0]);
			rSstat.SOV = PCStack.Overflow + PCStack.Underflow
						+ LoopBeginStack.Overflow + LoopBeginStack.Underflow
						+ StatusStack[0].Overflow + StatusStack[0].Underflow;
			break;
		case t40e: /* MAC, MPY */
		case t01a: /* MAC || LD || LD */	
		case t04a: /* MAC || LD */
		case t04e: /* MAC || ST */
		case t08a: /* MAC || CP */
		case t45b: /* MAC || SHIFT */
		case t45d: /* MAC || SHIFT */
			rAstatR.MV = v;
			if(v) OVCount++;
			break;
		case t16a:	/* ASHIFT, LSHIFT */
		case t16e:	/* ASHIFT, LSHIFT */
		case t15a:	/* ASHIFT, LSHIFT by imm */
		case t15c:	/* ASHIFT, LSHIFT by imm */
		case t15e:	/* ASHIFT, LSHIFT by imm */
		case t12e:	/* SHIFT || LD */
		case t12m:	/* SHIFT || LD */
		case t12g:	/* SHIFT || ST */
		case t12o:	/* SHIFT || ST */
		case t14c:	/* SHIFT || CP */
		case t14k:	/* SHIFT || CP */
			rAstatR.SV = v;
			if(v) OVCount++;
			break;
		case t10a:	/* CALL */
		case t10b:	/* CALL */
		case t19a:
			rSstat.PCE = stackEmpty(&PCStack);
			rSstat.PCF = stackFull(&PCStack);
			rSstat.PCL = stackCheckLevel(&PCStack);
			rSstat.SOV = PCStack.Overflow + PCStack.Underflow
						+ LoopBeginStack.Overflow + LoopBeginStack.Underflow
						+ StatusStack[0].Overflow + StatusStack[0].Underflow;
			break;
		case t20a:	/* RTI/RTS */
			rSstat.PCE = stackEmpty(&PCStack);
			rSstat.PCF = stackFull(&PCStack);
			rSstat.PCL = stackCheckLevel(&PCStack);
			break;
		default:
			break;
	}
}

/**
* Set/clear affected flags of status registers for complex operations.
* Note: this function accesses global variables.
*
* @param type Instruction type of current instruction.
* @param z zero flag
* @param n negative flag
* @param v overflow flag
* @param c carry flag
*/
void cFlagEffect(int type, cplx z, cplx n, cplx v, cplx c)
{
    int ovset1, ovset2;

    switch(type){
        case t09d:	/* ADD.C, SUB.C */
        case t09f:
		case t09g:	/* ADD.C, SUB.C ACC64 */
		case t04d: /* ALU.C || LD.C */
		case t04h: /* ALU.C || ST.C */
		case t08d: /* ALU.C || CP.C */
		case t46b: /* SCR.C */
            rAstatR.AZ = z.r;
            rAstatI.AZ = z.i;
            rAstatC.AZ = (z.r && z.i);

            rAstatR.AN = n.r;
            rAstatI.AN = n.i;
            rAstatC.AN = (n.r || n.i);  /* TBD */

			if(!isAVLatchedMode()){     /* if AV_LATCH is not set */
				rAstatR.AV = v.r;           /* work as usual */
				rAstatI.AV = v.i;           /* work as usual */
				rAstatC.AV = (v.r || v.i);
				if(rAstatC.AV) OVCount++;
			} else {                    /* if set, update only if AV is 0 */
				ovset1 = ovset2 = 0;

				if(rAstatR.AV == 0 && v.r == 1){
					rAstatR.AV = 1;
					ovset1 = TRUE;
				}
				if(rAstatI.AV == 0 && v.i == 1){
					rAstatI.AV = 1;
					ovset2 = TRUE;
				}

				rAstatC.AV = (rAstatR.AV || rAstatI.AV);
				if(ovset1 || ovset2) OVCount++;
			}

			rAstatR.AC = c.r;
			rAstatI.AC = c.i;
			rAstatC.AC = (c.r || c.i);
			break;
		case t41b:  /* RNDACC.C */
			rAstatR.MV = v.r;
			rAstatI.MV = v.i;
			rAstatC.MV = (v.r || v.i);
			if(v.r || v.i) OVCount++;
			break;
		case t41d:  /* CLRACC.C */
			rAstatR.AZ = 1;
			rAstatI.AZ = 1;
			rAstatC.AZ = 1;

			rAstatR.AN = 0;
			rAstatI.AN = 0;
			rAstatC.AN = 0;

			if(!isAVLatchedMode()){     /* update AV only if AV_LATCH is not set */
				rAstatR.AV = 0;
				rAstatI.AV = 0;
				rAstatC.AV = 0;
			}

			rAstatR.AC = 0;
			rAstatI.AC = 0;
			rAstatC.AC = 0;

			rAstatR.MV = 0;
			rAstatI.MV = 0;
			rAstatC.MV = 0;
 			break;
		case t40f: /* MAC.C, MAS.C, MPY.C */
		case t40g:
		case t01b: /* MAC.C || LD.C || LD.C */
		case t04b: /* MAC.C || LD.C */
		case t04f: /* MAC.C || ST.C */
		case t08b: /* MAC.C || CP.C */
			rAstatR.MV = v.r;
			rAstatI.MV = v.i;
			rAstatC.MV = (v.r || v.i);

			if(v.r || v.i) OVCount++;
			break;
		case t16b:  /* ASHIFT.C, LSHIFT.C */
		case t16f:  /* ASHIFT.C, LSHIFT.C */
		case t15b:  /* ASHIFT.C, LSHIFT.C by imm */
		case t15d:  /* ASHIFT.C, LSHIFT.C by imm */
		case t15f:  /* ASHIFT.C, LSHIFT.C by imm */
		case t12f:	/* SHIFT.C || LD.C */
		case t12n:	/* SHIFT.C || LD.C */
		case t12h:	/* SHIFT.C || ST.C */
		case t12p:	/* SHIFT.C || ST.C */
		case t14d:	/* SHIFT.C || CP.C */
		case t14l:	/* SHIFT.C || CP.C */
			rAstatR.SV = v.r;
			rAstatI.SV = v.i;
			rAstatC.SV = (v.r || v.i);

			if(v.r || v.i) OVCount++;
			break;
		default:
			break;
	}
}

/** 
* @brief Show current register data
* 
* @param p Pointer to current instruction
*/
void dumpRegister(sBCode *p)
{
	int i;

	printf("\nPC:%04X Opcode:%s Type:%s\n", p->PMA, (char *)sOp[p->Index], 
		sType[p->InstType]);

	for(i = 0; i < 8; i++){
		printf("R%02d:%03X ", i   , (unsigned int)(rR[i]   & 0xFFF));
		printf("R%02d:%03X ", i+ 8, (unsigned int)(rR[i+ 8] & 0xFFF));
		printf("R%02d:%03X ", i+16, (unsigned int)(rR[i+16] & 0xFFF));
		printf("R%02d:%03X ", i+24, (unsigned int)(rR[i+24] & 0xFFF));
		printf("\n");
	}
	for(i = 0; i < 8; i++){
		printf("ACC%d:%08X ",   i, (unsigned int)(rAcc[i].value & 0xFFFFFFFF));
		printf("ACC%d.H:%02X ", i, (unsigned int)(rAcc[i].H     & 0xFF ));
		printf("ACC%d.M:%03X ", i, (unsigned int)(rAcc[i].M     & 0xFFF));
		printf("ACC%d.L:%03X ", i, (unsigned int)(rAcc[i].L     & 0xFFF));
		printf("\n");
	}
	for(i = 0; i < 8; i++){
		printf("I%01d:%04X ", i,   (unsigned int)(rI[i]   & 0xFFFF));
		printf("M%01d:%04X ", i,   (unsigned int)(rM[i]   & 0xFFFF));
		printf("L%01d:%04X ", i,   (unsigned int)(rL[i]   & 0xFFFF));
		printf("B%01d:%04X ", i,   (unsigned int)(rB[i]   & 0xFFFF));
		printf("\n");
	}
	printf("PCSTACK:%04X\t", (unsigned int)(RdReg(p, ePCSTACK) & 0xFFFF));
	printf("LPSTACK:%04X\n", (unsigned int)(RdReg(p, eLPSTACK) & 0xFFFF));
	printf("CNTR   :%04X\t", (unsigned int)(rCNTR & 0xFFFF));
	printf("LPEVER :%04X\n", (unsigned int)(rLPEVER & 0xFFFF));

	/* print stacks: added 2009.05.25 */
	stackPrint(&PCStack);
	stackPrint(&LoopBeginStack);
	stackPrint(&LoopEndStack);
	stackPrint(&LoopCounterStack);
	stackPrint(&LPEVERStack);
	stackPrint(&StatusStack[0]);
	stackPrint(&StatusStack[1]);
	stackPrint(&StatusStack[2]);
	stackPrint(&StatusStack[3]);

	printf("SSTAT - PCE:%d PCF:%d PCL:%d LSE:%d LSF:%d SSE:%d SOV:%d\n",
		rSstat.PCE, rSstat.PCF, rSstat.PCL, 
		rSstat.LSE, rSstat.LSF, rSstat.SSE, rSstat.SOV);
	if(RdReg(p, eASTAT_R)){
		printf("ASTAT.R - AZ:%d AN:%d AV:%d AC:%d AS:%d AQ:%d MV:%d SS:%d SV:%d\n",
			rAstatR.AZ, rAstatR.AN, rAstatR.AV, rAstatR.AC, rAstatR.AS, 
			rAstatR.AQ, rAstatR.MV, rAstatR.SS, rAstatR.SV);
	}
	if(RdReg(p, eASTAT_I)){
		printf("ASTAT.I - AZ:%d AN:%d AV:%d AC:%d AS:%d AQ:%d MV:%d SS:%d SV:%d\n",
			rAstatI.AZ, rAstatI.AN, rAstatI.AV, rAstatI.AC, rAstatI.AS, 
			rAstatI.AQ, rAstatI.MV, rAstatI.SS, rAstatI.SV);
	}
	if(RdReg(p, eASTAT_C)){
		printf("ASTAT.C - AZ:%d AN:%d AV:%d AC:%d AS:%d AQ:%d MV:%d SS:%d SV:%d\n",
			rAstatC.AZ, rAstatC.AN, rAstatC.AV, rAstatC.AC, rAstatC.AS, 
			rAstatC.AQ, rAstatC.MV, rAstatC.SS, rAstatC.SV);
	}
	if(RdReg(p, eMSTAT)){
		printf("MSTAT - SR:%d BR:%d OL:%d AS:%d MM:%d TI:%d SD:%d MB:%d\n",
			rMstat.SR, rMstat.BR, rMstat.OL, rMstat.AS,
			rMstat.MM, rMstat.TI, rMstat.SD, rMstat.MB);
	}
	if(RdReg(p, eICNTL)){
		printf("ICNTL - INE:%d GIE:%d\n", rICNTL.INE, rICNTL.GIE);
	}
	if(RdReg(p, eIMASK)){
		printf("IMASK - EMU:%d PWDN:%d SSTEP:%d STACK:%d User:%d%d%d%d %d%d%d%d %d%d%d%d\n",
			rIMASK.EMU, rIMASK.PWDN, rIMASK.SSTEP, rIMASK.STACK,
			rIMASK.UserDef[ 0], rIMASK.UserDef[ 1], rIMASK.UserDef[ 2], 
			rIMASK.UserDef[ 3], rIMASK.UserDef[ 4], rIMASK.UserDef[ 5], 
			rIMASK.UserDef[ 6], rIMASK.UserDef[ 7], rIMASK.UserDef[ 8], 
			rIMASK.UserDef[ 9], rIMASK.UserDef[10], rIMASK.UserDef[11]);
	}
	if(RdReg(p, eIRPTL)){
		printf("IRPTL - EMU:%d PWDN:%d SSTEP:%d STACK:%d User:%d%d%d%d %d%d%d%d %d%d%d%d\n",
			rIRPTL.EMU, rIRPTL.PWDN, rIRPTL.SSTEP, rIRPTL.STACK,
			rIRPTL.UserDef[ 0], rIRPTL.UserDef[ 1], rIRPTL.UserDef[ 2], 
			rIRPTL.UserDef[ 3], rIRPTL.UserDef[ 4], rIRPTL.UserDef[ 5], 
			rIRPTL.UserDef[ 6], rIRPTL.UserDef[ 7], rIRPTL.UserDef[ 8], 
			rIRPTL.UserDef[ 9], rIRPTL.UserDef[10], rIRPTL.UserDef[11]);
	}
	if(RdReg(p, eIVEC0) + RdReg(p, eIVEC1) + RdReg(p, eIVEC2) + RdReg(p, eIVEC3)){
		for(i = 0; i < 4; i++){
			printf("IVEC%01d:%04X ", i,   (unsigned int)(rIVEC[i]   & 0xFFFF));
		}
		printf("\n");
	}

	printf("PC:%04X NextPC: %04X\n", oldPC, PC);
	//printf("CACTL:%04X\n", (unsigned int)(rCACTL & 0xFFFF));

	//added 2009.05.25
	printf("Time: %ld cycles\n", Cycles);
}


/** 
* @brief Get integer value of given fval according to immediate type
* 
* @param p Pointer to instruction
* @param fval Integer value of register field
* @param immtype eImmType value (i.e. eIMM_INT16, ...)
* 
* @return Integer value
*/
int getIntImm(sBCode *p, int fval, int immtype)
{
	int val;

	switch(immtype){
		case eIMM_INT4:
			val = 0xF & fval;
			if(isNeg(val, 4)) val |= 0xFFFFFFF0;
			break;
		case eIMM_INT5:
			val = 0x1F & fval;
			if(isNeg(val, 5)) val |= 0xFFFFFFE0;
			break;
		case eIMM_INT6:
			val = 0x3F & fval;
			if(isNeg(val, 6)) val |= 0xFFFFFFC0;
			break;
		case eIMM_INT8:
			val = 0xFF & fval;
			if(isNeg(val, 8)) val |= 0xFFFFFF00;
			break;
		case eIMM_INT12:
			val = 0xFFF & fval;
			if(isNeg(val, 12)) val |= 0xFFFFF000;
			break;
		case eIMM_INT13:
			val = 0x1FFF & fval;
			if(isNeg(val, 13)) val |= 0xFFFFE000;
			break;
		case eIMM_INT16:
			val = 0xFFFF & fval;
			if(isNeg(val, 16)) val |= 0xFFFF0000;
			break;
		case eIMM_INT24:
			val = 0xFFFFFF & fval;
			if(isNeg(val, 24)) val |= 0xFF000000;
			break;
		case eIMM_UINT4:
			val = 0xF & fval;
			break;
		case eIMM_UINT12:
			val = 0xFFF & fval;
			break;
		case eIMM_UINT16:
			val = 0xFFFF & fval;
			break;
		default:
			/* print error message */
			printRunTimeError(p->PMA, (char *)sOp[p->Index], 
				"Parse Error: getIntImm().\n");
			break;
	}
	return val;
}


/** 
* @brief Get complex value of given fval according to immediate type
* 
* @param p Pointer to instruction
* @param fval Integer value of register field
* @param immtype eImmType value (i.e. eIMM_COMPLEX8, ...)
* 
* @return Complex value
*/
cplx getCplxImm(sBCode *p, int fval, int immtype)
{
	int val_r, val_i;
	cplx cval;

	switch(immtype){
		case eIMM_COMPLEX8:
			val_r = (0xF0 & fval) >> 4;
			val_i = (0x0F & fval);
			if(isNeg(val_r, 4)) val_r |= 0xFFFFFFF0;
			if(isNeg(val_i, 4)) val_i |= 0xFFFFFFF0;
			break;
		case eIMM_COMPLEX24:
			val_r = (0xFFF000 & fval) >> 12;
			val_i = (0x000FFF & fval);
			if(isNeg(val_r, 12)) val_r |= 0xFFFFF000;
			if(isNeg(val_i, 12)) val_i |= 0xFFFFF000;
			break;
		case eIMM_COMPLEX32:
			val_r = (0xFFFF0000 & fval) >> 16;
			val_i = (0x0000FFFF & fval);
			if(isNeg(val_r, 16)) val_r |= 0xFFFF0000;
			if(isNeg(val_i, 16)) val_i |= 0xFFFF0000;
			break;
		default:
			/* print error message */
			printRunTimeError(p->PMA, (char *)sOp[p->Index], 
				"Parse Error: getCplxImm().\n");
			break;
	}

	cval.r = val_r;
	cval.i = val_i; 
	return cval;
}

/** 
* @brief Save PC value from current instruction 
* 
* @param p Pointer to old instruction
* @param n Pointer to new instruction
* 
* @return Next instruction which is not comment or not label
*/
sBCode *updatePC(sBCode *p, sBCode *n)
{
	if(p != NULL) oldPC = p->PMA;		/* save old PC */
	else
		oldPC = UNDEFINED;
	if(n != NULL) PC = n->PMA;	/* new PC */
	else
		PC = UNDEFINED;	/* end of program */

	return n;
}

/** 
* If MSTAT.M_BIAS(MB) is clear, the multiplier is in Unbiased Round mode.
* 
* @return 1 if MSTAT.M_BIAS is 0, 0 if MSTAT.M_BIAS is 1.
*/
int isUnbiasedRounding(void)
{
	int val = RdReg2(NULL, eMSTAT);

	///if(!rMstat.MB) return TRUE;	/* Unbiased: 0, Biased: 1 */
	if(!(val & 0x80)) return TRUE;	
	else return FALSE;
}

/** 
* If MSTAT.M_MODE(MM) is clear, the multiplier works in fractional mode.
* In this mode, multiplier output is shifted one bit left.
* 
* @return 1 if MSTAT.M_MODE is 0, 0 if MSTAT.M_MODE is 1.
*/
int isFractionalMode(void)
{
	int val = RdReg2(NULL, eMSTAT);

	//if(!rMstat.MM) return TRUE;	/* Fractional: 0, Integer: 1 */
	if(!(val & 0x10)) return TRUE;	
	else return FALSE;
}

/** 
* If MSTAT.AV_LATCH(OL) is 1, AV bit is "sticky". 
* Once AV is set, it remains set until the application explicitly clears it.
* 
* @return 1 if MSTAT.AV_LATCH is 1, 0 if not.
*/
int	isAVLatchedMode(void)
{
	int val = RdReg2(NULL, eMSTAT);

	//if(rMstat.OL) return TRUE;	/* disabled: 0, enabled: 1 */
	if((val & 0x04)) return TRUE;	
	else return FALSE;
}

/** 
* If MSTAT.AL_SAT(AS) is 1, saturation is enabled 
* for all subsequent ALU operations.
* 
* @return 1 if MSTAT.AL_SAT is 1, 0 if not.
*/
int	isALUSatMode(void)
{
	int val = RdReg2(NULL, eMSTAT);

	//if(rMstat.AS) return TRUE;	/* disabled: 0, enabled: 1 */
	if((val & 0x08)) return TRUE;	
	else return FALSE;
}

/** 
* Check if z = x + y results in overflow (for signed-integer arithmetic).
* Note: this function accesses global variable OVCount.
* 
* @param type Instruction type
* @param x 1st source 
* @param y 2nd source
* 
* @return 1 if overflow occurs, 0 if not
*/
int OVCheck(int type, int x, int y)
{
	long long lz;
	int z;

	switch(type){
		case t40e: /* MAC */
		case t40f: /* MAC.C */
		case t40g: /* MAC.RC */
		case t41a: /* RNDACC */
		case t41b: /* RNDACC.C */
		case t41c: /* CLRACC */
		case t41d: /* CLRACC.C */
		case t01a: /* MAC || LD || LD */	
		case t01b: /* MAC.C || LD.C || LD.C */
		case t04a: /* MAC || LD */
		case t04b: /* MAC.C || LD.C */
		case t04e: /* MAC || ST */
		case t04f: /* MAC.C || ST.C */
		case t08a: /* MAC || CP */
		case t08b: /* MAC.C || CP.C */
		case t45b: /* MAC || SHIFT */
		case t45d: /* MAC || SHIFT */
			lz = (long long)x + (long long)y;
			if(((lz & 0x0FF800000) == 0x0FF800000)
				||((lz & 0xFF800000) == 0)){
				return 0;	/* not overflow */
			}else{
				return 1;	/* overflow */
			}
			break;
		case t09c: /* ADD, SUB */
		case t09e:
		case t09i:	
        case t09d: /* ADD.C, SUB.C */
        case t09f:
		case t01c: /* ALU || LD || LD */
		case t04c: /* ALU || LD */
		case t04d: /* ALU.C || LD.C */
		case t04g: /* ALU || ST */
		case t04h: /* ALU.C || ST.C */
		case t08c: /* ALU || CP */
		case t08d: /* ALU.C || CP.C */
		case t43a: /* ALU || MAC */
		case t44b: /* ALU || SHIFT */
		case t44d: /* ALU || SHIFT */
		case t46a: /* SCR */
		case t46b: /* SCR.C */
			if(isNeg(x, 12))	x |= 0xFFFFF000;
			if(isNeg(y, 12))	y |= 0xFFFFF000;
			z = x + y;

			if(!isNeg(x, 12) && !isNeg(y, 12) && isNeg(z, 12)){
				return 1;	/* overflow */
			} else if(isNeg(x, 12) && isNeg(y, 12) && !isNeg(z, 12)){
				return 1;	/* overflow */
			} else {
				return 0;	/* not overflow */
			}
			break;
		case t09g:		/* ADD, ADD.C with ACC */
		case t09h:
			lz = (long long)x + (long long)y;

			if((x > 0) && (y > 0) && (lz < 0)){
				return 1;	/* overflow */
			} else if((x < 0) && (y < 0) && (lz > 0)){
				return 1;	/* overflow */
			} else {
				return 0;	/* not overflow */
			}
			break;
		case t16a:		/* ASHIFT, LSHIFT */
		case t16b:  	/* ASHIFT.C, LSHIFT.C */
		case t16e:		/* ASHIFT, LSHIFT */
		case t16f:  	/* ASHIFT.C, LSHIFT.C */
		case t15a:		/* ASHIFT, LSHIFT by imm */
		case t15b:  	/* ASHIFT.C, LSHIFT.C by imm */
		case t15c:		/* ASHIFT, LSHIFT by imm */
		case t15d:  	/* ASHIFT.C, LSHIFT.C by imm */
		case t15e:		/* ASHIFT, LSHIFT by imm */
		case t15f:  	/* ASHIFT.C, LSHIFT.C by imm */
		case t12e:		/* SHIFT || LD */
		case t12f:		/* SHIFT.C || LD.C */
		case t12m:		/* SHIFT || LD */
		case t12n:		/* SHIFT.C || LD.C */
		case t12g:		/* SHIFT || ST */
		case t12h:		/* SHIFT.C || ST.C */
		case t12o:		/* SHIFT || ST */
		case t12p:		/* SHIFT.C || ST.C */
		case t14c:		/* SHIFT || CP */
		case t14d:		/* SHIFT.C || CP.C */
		case t14k:		/* SHIFT || CP */
		case t14l:		/* SHIFT.C || CP.C */
			lz = (long long)x;
			if(((lz & 0x0FF800000) == 0x0FF800000)
				||((lz & 0xFF800000) == 0)){
				return 0;	/* not overflow */
			}else{
				return 1;	/* overflow */
			}
			break;
		default:
			break;
	}
	return 0;	/* default: must not happen */
}


/** 
* Check if z = x + y generates carry (for unsigned integer arithmetic).
* 
* @param p Pointer to current instruction
* @param x 1st source 
* @param y 2nd source
* @param c carry(input)
* 
* @return 1 if generates carry(output), 0 if not
*/
int CarryCheck(sBCode *p, int x, int y, int c)
{
	int z;
	long long lz;

	switch(p->InstType){
		case t09c: /* ADD, SUB */
		case t09e:
		case t09i:	
		case t09d: /* ADD.C, SUB.C */
		case t09f:
			switch(p->Index){
				case iADD:
				case iADD_C:
					z = (0xFFF & x) + (0xFFF & y);
					break;
				case iADDC:
				case iADDC_C:
					z = (0xFFF & x) + (0xFFF & y) + c;
					break;
				case iSUB:
				case iSUB_C:
					z = (0xFFF & x) + (0xFFF & (~y)) + 1;
					break;
				case iSUBC:
				case iSUBC_C:
					z = (0xFFF & x) + (0xFFF & (~y)) + c;
					break;
				case iSUBB:
				case iSUBB_C:
					z = (0xFFF & y) + (0xFFF & (~x)) + 1;
					break;
				case iSUBBC:
				case iSUBBC_C:
					z = (0xFFF & y) + (0xFFF & (~x)) + c;
					break;
				case iINC:
					z = (0xFFF & x) + 1;
					break;
				case iDEC:
					z = (0xFFF & x)  + (0xFFF & (-1));
					break;
				default:
					/* print error message */
					printRunTimeError(p->PMA, (char *)sOp[p->Index], 
					"Invalid case in CarryCheck().\n");
					return 0;
					break;
			}
			if(z > 0x0FFF || z < 0x0){
				return 1;
			}
			return 0;
			break;
		case t09g:		/* ADD, ADD.C with ACC */
		case t09h:
			switch(p->Index){
				case iADD:
				case iADD_C:
					lz =  (0xFFFFFFFF & (long long)x) 
						+ (0xFFFFFFFF & (long long)y);
					break;
				case iSUB:
				case iSUB_C:
					lz =  (0xFFFFFFFF & (long long)x) 
						+ (0xFFFFFFFF & (long long)(~y)) + 1;
					break;
				case iSUBB:
				case iSUBB_C:
					lz =  (0xFFFFFFFF & (long long)y) 
						+ (0xFFFFFFFF & (long long)(~x)) + 1;
					break;
				default:
					/* print error message */
					printRunTimeError(p->PMA, (char *)sOp[p->Index], 
					"Invalid case in CarryCheck().\n");
					return 0;
					break;
			}
			if(lz > 0x0FFFFFFFF || lz < 0x0){
				return 1;
			}
			return 0;
			break;
		default:
			break;
	}
	return 0;	/* default: must not happen */
}

/** 
* @brief if saturation should be executed for 12-b operation
* 
* @param ov overflow flag
* @param c carry flag
* 
* @return 0x7FF for max saturation, 0x800 for min saturation, 0 for no saturation
*/
int satCheck12b(int ov, int c)
{
	if(ov == 1 && c == 0) return 0x7FF;
	if(ov == 1 && c == 1) return 0x800;
	else return 0;
}

/** 
* Read 12-bit data from data memory. If undefined, add to dMem as a new entry.
* 
* @param p Pointer to current instruction
* @param addr Address
* 
* @return Data value (12-bit)
*/
int RdDataMem(sBCode *p, unsigned int addr)
{
	dMem *dp;
	int data;

	/* if bit-reversed addressing is enabled */
	/*
	if(rMstat.BR){ -> should be modified considering MSTAT latency restriction
		addr = getBitReversedAddr(addr);
	}
	*/

	//if dataSegAddr is defined
	//addr += dataSegAddr;

	if(!isValidMemoryAddr(p, (int)addr)){
		data = (0x0FFF & UNDEFINED);

		char tnum[10];
		sprintf(tnum, "0x%04X", addr);
		printRunTimeWarning(p->PMA, tnum, 
			"Invalid data memory address.\n");
		if(VerboseMode) printf("RdDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data);
		return data;
	}

	dp = dMemHashSearch(dataMem, addr);
	if(dp == NULL){	/* if not found */
		char tnum[10];
		sprintf(tnum, "0x%04X", addr);
		dMemHashAdd(dataMem, 0x0FFF & UNDEFINED, addr);
		/*
		printRunTimeWarning(p->PMA, tnum, 
			"Undefined data memory address. Use \".VAR\" to define a new data memory variable.\n");
		*/
		data = (0x0FFF & UNDEFINED);
	} else {
		data = (0x0FFF & (dp->Data));
	}

	if(VerboseMode) printf("RdDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data);
	return data;
}

/** 
* Read 12-bit data from data memory. If undefined, do not add to dMem.
* 
* @param addr Address
* 
* @return Data value (12-bit)
*/
int briefRdDataMem(unsigned int addr)
{
	dMem *dp;
	int data;

	/* if bit-reversed addressing is enabled */
	/*
	if(rMstat.BR){ -> should be modified considering MSTAT latency restriction
		addr = getBitReversedAddr(addr);
	}
	*/

	//if dataSegAddr is defined
	//addr += dataSegAddr;

	if(!isValidMemoryAddr(NULL, (int)addr)){
		data = (0x0FFF & UNDEFINED);

		/*
		if(VerboseMode) printf("briefRdDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data);
		*/
		return data;
	}

	dp = dMemHashSearch(dataMem, addr);
	if(dp == NULL){	/* if not found */
		dMemHashAdd(dataMem, 0x0FFF & UNDEFINED, addr);
		data = (0x0FFF & UNDEFINED);
	} else {
		data = (0x0FFF & (dp->Data));
	}

	/*
	if(VerboseMode) printf("briefRdDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data);
	*/
	return data;
}

/** 
* Write 12-bit data to data memory. If undefined, add to dMem as a new entry.
* 
* @param p Pointer to current instruction
* @param data Data to write
* @param addr Address
*/
void WrDataMem(sBCode *p, int data, unsigned int addr)
{
	dMem *dp;

	/* if bit-reversed addressing is enabled */
	/*
	if(rMstat.BR){ -> should be modified considering MSTAT latency restriction
		addr = getBitReversedAddr(addr);
	}
	*/

	//if dataSegAddr is defined
	//addr += dataSegAddr;

	if(p){
		if(!isValidMemoryAddr(p, (int)addr)){
			return;
		}
	}

	dp = dMemHashSearch(dataMem, addr);
	if(dp == NULL){	/* if not found: make new var */
		char tnum[10];
		sprintf(tnum, "0x%04X", addr);
		dMemHashAdd(dataMem, 0x0FFF & data, addr);
		/*
		printRunTimeWarning(p->PMA, tnum, 
			"Undefined data memory address. Use \".VAR\" to define a new data memory variable.\n");
		*/
	} else {		/* found: update content */
		dp->Data = data;
	}

	if(VerboseMode) printf("WrDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data);
}

/** 
* @brief Compute bit-reversed address for bit-reversed addressing 
* 
* @param addr 16-bit address 
* 
* @return 16-bit bit-reversed address
*/
//unsigned int getBitReversedAddr(unsigned int addr)
//{
//	unsigned int bitRevAddr = 0;
//	int mask = 1;
//	int hmask = 0x8000;

//	for(int i = 0; i < 16; i++){
//		if(addr & mask) bitRevAddr |= hmask;
//		mask <<= 1;
//		hmask >>= 1;
//	}

//	if(VerboseMode) {
//			printf("getBitReversedAddr: 0x%04X -> 0x%04X\n", addr, bitRevAddr);
//	}

//	return (0xFFFF & bitRevAddr);
//}

/*
//int RdProgMem(unsigned int addr)
//{
//	sBCode *sp;

//	if(!isValidMemoryAddr((int)addr)){
//		return (0xFFFF & UNDEFINED);
//	}

//	sp = sBCodeListSearch(&iCode, addr);
//	if(sp == NULL){	
//		sp = sBCodeListAdd(&iCode, iDATA, addr, lineno);
//		sp->Data = 0x0FFFF & UNDEFINED;
//		return (0x0FFFF & UNDEFINED);
//	} else
//		return (0x0FFFF & (sp->Data));
//}
*/

/*
//void WrProgMem(int data, unsigned int addr)
//{
//	sBCode *sp;

//	if(!isValidMemoryAddr((int)addr)){
//		return;
//	}

//	sp = sBCodeListSearch(&iCode, addr);
//	if(sp == NULL){	
//		sp = sBCodeListAdd(&iCode, iDATA, addr, lineno);
//		sp->Data = 0x0FFFF & data;
//	} else {	
//		sp->Data = 0x0FFFF & data;
//	}
//}
*/

/** 
* @brief Check if address is negative or above 16-bit boundary
* 
* @param p Pointer to current instruction
* @param addr Memory (PM or DM) address
* 
* @return 1 if valid, 0 if out of boundary
*/
int isValidMemoryAddr(sBCode *p, int addr)
{
	char tnum[10];

	if(dataSegSize){	//if dataSegSize read from .ldi
		if (addr < dataSegAddr || addr > (dataSegAddr + dataSegSize -1)){
			sprintf(tnum, "%04X", addr);
			if(p){
				printRunTimeWarning(p->PMA, tnum, 
					"Data memory reference out of range!!\n");
			}else{
				printRunTimeWarning(addr, tnum, 
					"Data memory reference out of range!!\n");
			}
			return FALSE;
		} else
			return TRUE;
	}else{				//if dataSegSize not read from .ldi
		if (addr < 0 || addr > 0x0FFFF){
			sprintf(tnum, "%04X", addr);
			if(p){
				printRunTimeWarning(p->PMA, tnum, 
					"Data memory reference out of range!!\n");
			}else{
				printRunTimeWarning(addr, tnum, 
					"Data memory reference out of range!!\n");
			}
			return FALSE;
		} else
			return TRUE;
	}
}

/** 
* @brief Check if MSB of n-bit data is 1 or 0
* 
* @param x n-bit data
* @param n Integer 
* 
* @return 1 if negative, 0 if not
*/
int isNeg(int x, int n)
{
	int val = 0;

	switch(n)
	{
		case 4:
			if(x & 0x8) val = 1;
			break;
		case 5:
			if(x & 0x10) val = 1;
			break;
		case 6:
			if(x & 0x20) val = 1;
			break;
		case 8:
			if(x & 0x80) val = 1;
			break;
		case 12:
			if(x & 0x800) val = 1;
			break;
		case 13:
			if(x & 0x1000) val = 1;
			break;
		case 16:
			if(x & 0x8000) val = 1;
			break;
		case 24:
			if(x & 0x800000) val = 1;
			break;
		case 32:
			if(x & 0x80000000) val = 1;
			break;
		default:
			break;
	}

	return val;
}

/** 
* @brief Check if breakpoint
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isBreakpoint(sBCode *p)
{
	if(BreakPoint == UNDEFINED) return FALSE;
	if(p->PMA == BreakPoint) return TRUE;
	else return FALSE;
}


/** 
* @brief Check given condition code
* 
* @param cond Condition code value
* 
* @return TRUE or FALSE
*/
int	ifCond(int cond)
{
	int val = FALSE;
	int ce;

	switch(cond){
		/* EQ ~ TRUE: checks only ASTAT.R */
		case 0x00:		/* EQ: 00000 */
			if(rAstatR.AZ) 
				val = TRUE;
			break;
		case 0x01:		/* NE: 00001 */
			if(!rAstatR.AZ)	
				val = TRUE;
			break;
		case 0x02:		/* GT: 00010 */
			if(!((rAstatR.AN ^ rAstatR.AV) || rAstatR.AZ))	
				val = TRUE;
			break;
		case 0x03:		/* LE: 00011 */
			if((rAstatR.AN ^ rAstatR.AV) || rAstatR.AZ)	
				val = TRUE;
			break;
		case 0x04:		/* LT: 00100 */
			if(rAstatR.AN ^ rAstatR.AV)	
				val = TRUE;
			break;
		case 0x05:		/* GE: 00101 */
			if(!(rAstatR.AN ^ rAstatR.AV))
				val = TRUE;
			break;
		case 0x06:		/* AV: 00110 */
			if(rAstatR.AV)	
				val = TRUE;
			break;
		case 0x07:		/* NOT AV: 00111 */
			if(!rAstatR.AV)	
				val = TRUE;
			break;
		case 0x08:		/* AC: 01000 */
			if(rAstatR.AC)	
				val = TRUE;
			break;
		case 0x09:		/* NOT AC: 01001 */
			if(!rAstatR.AC)	
				val = TRUE;
			break;
		case 0x0A:		/* SV: 01010 */
			if(rAstatR.SV)	
				val = TRUE;
			break;
		case 0x0B:		/* NOT SV: 01011 */
			if(!rAstatR.SV)	
				val = TRUE;
			break;
		case 0x0C:		/* MV: 01100 */
			if(rAstatR.MV)	
				val = TRUE;
			break;
		case 0x0D:		/* NOT MV: 01101 */
			if(!rAstatR.MV)	
				val = TRUE;
			break;
		case 0x0E:		/* NOT CE: 01110 */
			ce = RdReg(NULL, eCNTR);
			if(ce > 1)	
				val = TRUE;		/* test if CNTR > 1: Refer to */
			                    /* ADSP-219x Inst Set Reference 7-2 */
			break;
		case 0x0F:		/* TRUE: 01111 */
			val = TRUE;
			break;
		/* EQ.C ~ NOT MV.C: checks ASTAT.C */
		case 0x10:		/* EQ.C: 10000 */
			if(rAstatC.AZ)	
				val = TRUE;
			break;
		case 0x11:		/* NE.C: 10001 */
			if(!rAstatC.AZ)	
				val = TRUE;
			break;
		case 0x12:		/* GT.C: 10010 */
			if(!((rAstatC.AN ^ rAstatC.AV) || rAstatC.AZ))	
				val = TRUE;
			break;
		case 0x13:		/* LE.C: 10011 */
			if((rAstatC.AN ^ rAstatC.AV) || rAstatC.AZ)	
				val = TRUE;
			break;
		case 0x14:		/* LT.C: 10100 */
			if(rAstatC.AN ^ rAstatC.AV)	
				val = TRUE;
			break;
		case 0x15:		/* GE.C: 10101 */
			if(!(rAstatC.AN ^ rAstatC.AV))	
				val = TRUE;
			break;
		case 0x16:		/* AV.C: 10110 */
			if(rAstatC.AV)	
				val = TRUE;
			break;
		case 0x17:		/* NOT AV.C: 10111 */
			if(!rAstatC.AV)	
				val = TRUE;
			break;
		case 0x18:		/* AC.C: 11000 */
			if(rAstatC.AC)	
				val = TRUE;
			break;
		case 0x19:		/* NOT AC.C: 11001 */
			if(!rAstatC.AC)	
				val = TRUE;
			break;
		case 0x1A:		/* SV.C: 11010 */
			if(rAstatC.SV)	
				val = TRUE;
			break;
		case 0x1B:		/* NOT SV.C: 11011 */
			if(!rAstatC.SV)	
				val = TRUE;
			break;
		case 0x1C:		/* MV.C: 11100 */
			if(rAstatC.MV)
				val = TRUE;
			break;
		case 0x1D:		/* NOT MV.C: 11101 */
			if(!rAstatC.MV)	
				val = TRUE;
			break;
		default:
			break;
	}
	return val;
}


/** 
* @brief Check if multifunction instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isMultiFunc(sBCode *p)
{
	int val = FALSE;

	switch(p->InstType){
		case t01a:
		case t01b:
		case t01c:
		case t04a:
		case t04b:
		case t04c:
		case t04d:
		case t04e:
		case t04f:
		case t04g:
		case t04h:
		case t08a:
		case t08b:
		case t08c:
		case t08d:
		case t12e:
		case t12f:
		case t12m:
		case t12n:
		case t12g:
		case t12h:
		case t12o:
		case t12p:
		case t14c:
		case t14d:
		case t14k:
		case t14l:
		case t43a:
		case t44b:
		case t44d:
		case t45b:
		case t45d:
			val = TRUE;
			break;
		default:
			break;
	}
	return val;
}


/** 
* @brief Check if an LD instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isLD(sBCode *p)
{
	int val;
	switch(p->InstType){
		case t03a:
		case t03d:
		case t06a:
		case t06b:
		case t06d:
		case t32a:
		case t29a:
			val = TRUE;
			break;
		default:
			val = FALSE;
			break;
	}
	return val;
}


/** 
* @brief Check if an LD.C instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isLD_C(sBCode *p)
{
	int val;
	switch(p->InstType){
		case t03c:
		case t03e:
		case t06c:
		case t32b:
		case t29b:
			val = TRUE;
			break;
		default:
			val = FALSE;
			break;
	}
	return val;
}


/** 
* @brief Check if an ST instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isST(sBCode *p)
{
	int val;
	switch(p->InstType){
		case t03f:
		case t03h:
		case t32c:
		case t29c:
		case t22a:
			val = TRUE;
			break;
		default:
			val = FALSE;
			break;
	}
	return val;
}


/** 
* @brief Check if an ST.C instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isST_C(sBCode *p)
{
	int val;
	switch(p->InstType){
		case t03g:
		case t03i:
		case t32d:
		case t29d:
			val = TRUE;
			break;
		default:
			val = FALSE;
			break;
	}
	return val;
}


/** 
* @brief Check if a multifunction instruction with LD/ST/LD.C/ST.C operation
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isLDSTMulti(sBCode *p)
{
	int val;
	switch(p->InstType){
		case t01a:		/* MAC || LD || LD or LD || LD */
		case t01b:		/* MAC.C || LD.C || LD.C or LD.C || LD.C */
		case t01c:		/* ALU || LD || LD */
		case t04a:		/* MAC || LD */
		case t04b:		/* MAC.C || LD.C */
		case t04c:		/* ALU || LD */
		case t04d:		/* ALU.C || LD.C */
		case t12e:		/* SHIFT || LD */
		case t12f:		/* SHIFT.C || LD.C */
		case t12m:		/* SHIFT || LD */
		case t12n:		/* SHIFT.C || LD.C */
		case t04e:		/* MAC || ST */
		case t04f:		/* MAC.C || ST.C */
		case t04g:		/* ALU || ST */
		case t04h:		/* ALU.C || ST.C */
		case t12g:		/* SHIFT || ST */
		case t12h:		/* SHIFT.C || ST.C */
		case t12o:		/* SHIFT || ST */
		case t12p:		/* SHIFT.C || ST.C */
			val = TRUE;
			break;
		default:
			val = FALSE;
			break;
	}

	return val;
}

/** 
* @brief Print run-time warning message to message buffer
* 
* @param addr Program memory address
* @param s Pointer to warning string
* @param msg Warning message
*/
void printRunTimeWarning(int addr, char *s, char *msg)
{
	static int lastAddr = -1;
	if(lastAddr != addr) {
		sprintf(msgbuf, "\naddr %04X: Warning: %s - %s", addr, s, msg);
		printf("\naddr %04X: Warning: %s - %s", addr, s, msg);
		lastAddr = addr;
	}
}

/** 
* @brief Print run-time error message to message buffer
* 
* @param addr Program memory address
* @param s Pointer to error string
* @param msg Error message
*/
void printRunTimeError(int addr, char *s, char *msg)
{
	static int lastAddr = -1;
	if(lastAddr != addr) {	
		sprintf(msgbuf, "\naddr %04X: Error: %s - %s", addr, s, msg);
		printf("\naddr %04X: Error: %s - %s", addr, s, msg);
		lastAddr = addr;
	}
	closeSim();
	exit(1);
}

/** 
* @brief Print content of message buffer (given by warnings or errors)
*/
void printRunTimeMessage(void)
{
	if(msgbuf[0] != 0){
		printf("%s\n", msgbuf);
		strcpy(msgbuf, "");
	}
}


/** 
* @brief Check the multiplier rounding mode (biased or unbiased) and do appropriate rounding operation.
* 
* @param x 32-bit accumulator value
* 
* @return Rounded 32-bit number
*/
int calcRounding(int x)
{
	/* Multiplier rounding operation */
    /* when result is 0x800, add 1 to bit 11 */
    /* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
    /* Note that after rounding, the content of ACCx.L is INVALID. */
    if(isUnbiasedRounding() && ((x & 0xFFF) == 0x800)) {    /* unbiased rounding only */
        x += 0x800;
        x &= 0xFFFFEFFF;    /* force bit 12 to zero */
    } else {    
        x += 0x800;
    }

    return x;
}


/** 
* @brief Handle core multiplication (and MAC) algorithm
* 
* @param p Pointer to current instruction
* @param temp1 Integer value of first operand
* @param temp2 Integer value of second operand
* @param rdst Destination register index
* @param opt Enum eMul value of muliply option string
* @param fmn fMN field value (1 if MAC destination is NONE, 0 else)
*/
void processMACFunc(sBCode *p, int temp1, int temp2, int rdst, int opt, int fmn)
{
	int temp3, temp4, temp5;
	char z, n, v, c;

	int isMulti;
	int index;

	/* check if multifunction: when 2nd operation is a SHIFT */
	if(p->InstType == t43a){
		isMulti = TRUE;
	}else{
		isMulti = FALSE;
	}

	if(isMulti){
		index = p->IndexMulti[0];	/* if isMulti, 2nd operation is a SHIFT */
	}else{
		index = p->Index;
	}

	/* Note: system defaults to fractional mode after reset. */
	switch(opt){
		case emRND:
		case emSS:
			break;
		case emSU:
			temp2 &= 0x0FFF;
			break;
		case emUS:
			temp1 &= 0x0FFF;
			break;
		case emUU:
			temp1 &= 0x0FFF;
			temp2 &= 0x0FFF;
			break;
		default:
			printRunTimeError(p->PMA, (char *)sOp[index], 
						"Invalid MAC option found in processMACFunc().\n");
			return;
			break;
	}
	temp3 = 0x0FFFFFFFF & (temp1 * temp2);

	if(isFractionalMode()) temp3 <<= 1;

	switch(index){
		case iMAC:
			temp4 = (int)RdReg(p, rdst);			/* accumulate */
			break;
		case iMAS:
			temp4 = (int)RdReg(p, rdst);
			temp3 = -temp3;						/* subtract */
			break;
		case iMPY:
			temp4 = 0;							/* do nothing */
			break;
		default:
			printRunTimeError(p->PMA, (char *)sOp[index], 
				"Invalid MAC operation called in processMACFunc().\n");
			break;
	}
	temp5 = temp3 + temp4;
				
	if(opt == emRND){
		/* Rounding operation */
		/* when result is 0x800, add 1 to bit 11 */
		/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
		/* Note that after rounding, the content of ACCx.L is INVALID. */
		temp5 = calcRounding(temp5);
	}

	if(isMulti){
		v = OVCheck(t40e, temp3, temp4);		/* if MAC is the 2nd operation in a multifunction */
	}else{
		v = OVCheck(p->InstType, temp3, temp4);
	}

	if(!fmn){
		//write result only if destination register is NOT "NONE"
		WrReg(p, rdst, temp5);
	}

	if(isMulti){
		flagEffect(t40e, 0, 0, v, 0);			/* if MAC is the 2nd operation in a multifunction */
	}else{
		flagEffect(p->InstType, 0, 0, v, 0);
	}
}


/** 
* @brief Handle core multiplication (and MAC) algorithm (complex version)
* 
* @param p Pointer to current instruction
* @param ct1 Complex value of first operand
* @param ct2 Complex value of second operand
* @param rdst Destination register index
* @param opt Enum eMul value of muliply option string
* @param fmn fMN field value (1 if MAC destination is NONE, 0 else)
*/
void processMAC_CFunc(sBCode *p, cplx ct1, cplx ct2, int rdst, int opt, int fmn)
{
	cplx z2, n2, v2, c2;
	const cplx o2 = { 0, 0 };
	cplx ct3, ct4, ct5;
	cplx ct31, ct32;

	/* Note: system defaults to fractional mode after reset. */
	switch(opt){
		case emRND:
		case emSS:
			break;
		case emSU:
			ct2.r &= 0x0FFF;
			ct2.i &= 0x0FFF;
			break;
		case emUS:
			ct1.r &= 0x0FFF;
			ct1.i &= 0x0FFF;
			break;
		case emUU:
			ct1.r &= 0x0FFF;
			ct1.i &= 0x0FFF;
			ct2.r &= 0x0FFF;
			ct2.i &= 0x0FFF;
			break;
		default:
			printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Invalid MAC option found in processMAC_CFunc().\n");
			return;
			break;
	}
	//ct3.r = 0x0FFFFFFFF & (ct1.r * ct2.r - ct1.i * ct2.i);    /* multiply      */
	ct31.r = 0x0FFFFFFFF & (ct1.r * ct2.r);
	ct32.r = 0x0FFFFFFFF & (ct1.i * ct2.i);
	ct3.r = 0x0FFFFFFFF & (ct31.r - ct32.r);
	//ct3.i = 0x0FFFFFFFF & (ct1.r * ct2.i + ct1.i * ct2.r);    /* multiply      */
	ct31.i = 0x0FFFFFFFF & (ct1.r * ct2.i);
	ct32.i = 0x0FFFFFFFF & (ct1.i * ct2.r);
	ct3.i = 0x0FFFFFFFF & (ct31.i + ct32.i);


	if(isFractionalMode()){ 
		ct3.r <<= 1;
		ct3.i <<= 1;
	}

	switch(p->Index){
		case iMAC_C:
			ct4 = cRdReg(p, rdst);
			break;
		case iMAS_C:
			ct4 = cRdReg(p, rdst);
			ct3.r = - ct3.r;
			ct3.i = - ct3.i;
			break;
		case iMPY_C:
			ct4.r = 0;
			ct4.i = 0;
			break;
		default:
			printRunTimeError(p->PMA, (char *)sOp[p->Index], 
				"Invalid MAC.C operation called in processMAC_CFunc().\n");
			return;
			break;
	}
	ct5.r = ct3.r + ct4.r;		/* accumulate */
	ct5.i = ct3.i + ct4.i;

	if(opt == emRND){
		/* Rounding operation */
		/* when result is 0x800, add 1 to bit 11 */
		/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
		/* Note that after rounding, the content of ACCx.L is INVALID. */
		ct5.r = calcRounding(ct5.r);
		ct5.i = calcRounding(ct5.i);
	}

	v2.r = OVCheck(p->InstType, ct3.r, ct4.r);
	v2.i = OVCheck(p->InstType, ct3.i, ct4.i);

	if(!fmn){
		//write result only if destination register is NOT "NONE"
		cWrReg(p, rdst, ct5.r, ct5.i);
	}

	cFlagEffect(p->InstType, o2, o2, v2, o2);
}

/** 
* @brief Handle core multiplication (and MAC) algorithm (real & complex mixed version)
* 
* @param p Pointer to current instruction
* @param temp1 Integer value of first operand
* @param ct2 Complex value of second operand
* @param rdst Destination register index
* @param opt Enum eMul value of muliply option string
* @param fmn fMN field value (1 if MAC destination is NONE, 0 else)
*/
void processMAC_RCFunc(sBCode *p, int temp1, cplx ct2, int rdst, int opt, int fmn)
{
	cplx z2, n2, v2, c2;
	const cplx o2 = { 0, 0 };
	cplx ct3, ct4, ct5;

	/* Note: system defaults to fractional mode after reset. */
	switch(opt){
		case emRND:
		case emSS:
			break;
		case emSU:
			ct2.r &= 0x0FFF;
			ct2.i &= 0x0FFF;
			break;
		case emUS:
			temp1 &= 0x0FFF;
			break;
		case emUU:
			temp1 &= 0x0FFF;
			ct2.r &= 0x0FFF;
			ct2.i &= 0x0FFF;
			break;
		default:
			printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Invalid MAC option found in processMAC_RCFunc().\n");
			return;
			break;
	}
	ct3.r = 0x0FFFFFFFF & (temp1 * ct2.r);  /* multiply */
	ct3.i = 0x0FFFFFFFF & (temp1 * ct2.i);  /* multiply */

	if(isFractionalMode()){ 
		ct3.r <<= 1;
		ct3.i <<= 1;
	}

	switch(p->Index){
		case iMAC_RC:
			ct4 = cRdReg(p, rdst);
			break;
		case iMAS_RC:
			ct4 = cRdReg(p, rdst);
			ct3.r = - ct3.r;
			ct3.i = - ct3.i;
			break;
		case iMPY_RC:
			ct4.r = 0;
			ct4.i = 0;
			break;
		default:
			printRunTimeError(p->PMA, (char *)sOp[p->Index], 
				"Invalid MAC.RC operation called in processMAC_RCFunc().\n");
			return;
			break;
	}
	ct5.r = ct3.r + ct4.r;		/* accumulate */
	ct5.i = ct3.i + ct4.i;

	if(opt == emRND){
		/* Rounding operation */
		/* when result is 0x800, add 1 to bit 11 */
		/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
		/* Note that after rounding, the content of ACCx.L is INVALID. */
		ct5.r = calcRounding(ct5.r);
		ct5.i = calcRounding(ct5.i);
	}

	v2.r = OVCheck(p->InstType, ct3.r, ct4.r);
	v2.i = OVCheck(p->InstType, ct3.i, ct4.i);

	if(!fmn){
		//write result only if destination register is NOT "NONE"
		cWrReg(p, rdst, ct5.r, ct5.i);
	}

	cFlagEffect(p->InstType, o2, o2, v2, o2);
}

/** 
* @brief Handle core ALU instruction algorithm
* 
* @param p Pointer to current instruction
* @param vs1 Integer value of first operand
* @param vs2 Integer value of second operand
* @param rdst Destination register index
*/
void processALUFunc(sBCode *p, int vs1, int vs2, int rdst)
{
	int temp3, temp4, temp5;
	char z, n, v, c;
	char cdata;

	switch(p->Index){
		case iADD:
			temp5 = vs1 + vs2;
			break;
		case iADDC:
			cdata = RdC1(); 
			temp5 = vs1 + vs2 + cdata;
			break;
		case iSUB:
			temp5 = vs1 - vs2;
			break;
		case iSUBC:
			cdata = RdC1(); 
			temp5 = vs1 - vs2 + cdata - 1;
			break;
		case iSUBB:
			temp5 = - vs1 + vs2;
			break;
		case iSUBBC:
			cdata = RdC1(); 
			temp5 = - vs1 + vs2 + cdata - 1;
			break;
		case iAND:
			temp5 = vs1 & vs2;
			break;
		case iOR:
			temp5 = vs1 | vs2;
			break;
		case iXOR:
			temp5 = vs1 ^ vs2;
			break;
		case iNOT:
			temp5 = ~ vs1;
			break;
		case iABS:
			if(isNeg(vs1, 12))
				temp5 = - vs1;
			else
				temp5 = vs1;
			break;
		case iINC:
			temp5 = vs1 + 1;
			break;
		case iDEC:
			temp5 = vs1 - 1;
			break;
		case iTSTBIT:
			temp4 = 0x1 << vs2;   	/* generate bit mask */
			temp5 = vs1 & temp4;   	/* test bit */
			break;
		case iSETBIT:
			temp4 = 0x1 << vs2;   	/* generate bit mask */
			temp5 = vs1 | temp4;   	/* set bit */
			break;
		case iCLRBIT:
			temp4 = 0x1 << vs2;   	/* generate bit mask */
			temp5 = vs1 & (~temp4);   /* clear bit */
			break;
		case iTGLBIT:
			temp4 = 0x1 << vs2;   	/* generate bit mask */
			temp5 = vs1 ^ temp4;   	/* toggle bit */
			break;
		case iSCR:
			if(vs2 & 0x1){            /* if 2nd operand is odd  */
				temp5 = -vs1;
			}else{                      /* if 2nd operand is even */
				temp5 = vs1;
			}
			break;
		default:
			printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Invalid ALU operation called in processALUFunc().\n");
			return;
			break;
	}

	z = (!temp5)?1:0;
	n = isNeg(temp5, 12);

	/* to support type 9g */
	if(p->InstType == t09g){
		if(temp5 < 0) 
			n = 1;
		else 
			n = 0;
	}

	switch(p->Index){
		case iADD:
			v = OVCheck(p->InstType, vs1, vs2);
			c = CarryCheck(p, vs1, vs2, 0);
			break;
		case iADDC:
			v = OVCheck(p->InstType, vs1, vs2 + cdata);
			c = CarryCheck(p, vs1, vs2, cdata);
			break;
		case iSUB:
			v = OVCheck(p->InstType, vs1, ~vs2+1);
			c = CarryCheck(p, vs1, vs2, 0);
			break;
		case iSUBC:
			v = OVCheck(p->InstType, vs1, - vs2 + cdata - 1);
			c = CarryCheck(p, vs1, vs2, cdata);
			break;
		case iSUBB:
			v = OVCheck(p->InstType, vs2, - vs1);
			c = CarryCheck(p, vs1, vs2, 0);
			break;
		case iSUBBC:
			v = OVCheck(p->InstType, vs2, - vs1 + cdata - 1);
			c = CarryCheck(p, vs1, vs2, cdata);
			break;
		case iAND:
		case iOR:
		case iXOR:
		case iNOT:
		case iTSTBIT:
		case iSETBIT:
		case iCLRBIT:
		case iTGLBIT:
		case iSCR:
			v = 0;
			c = 0;
			break;
		case iABS:
			if(vs1 == 0x800){
				n = 1; v = 1;
			} else {
				n = 0; v = 0;
			}
			c = 0;
			break;
		case iINC:
			v = OVCheck(p->InstType, vs1, 1);
			c = CarryCheck(p, vs1, 0, 0);
			break;
		case iDEC:
			v = OVCheck(p->InstType, vs1, -1);
			c = CarryCheck(p, vs1, 0, 0);
			break;
		default:
			break;
	}

	flagEffect(p->InstType, z, n, v, c);

	if(p->Index == iABS){
		rAstatR.AS = isNeg(vs1, 12);
	}

	if(isALUSatMode()){ 			/* if saturation enabled  */
		temp3 = satCheck12b(v, c);	/* check if needed    */

		if(p->InstType == t09g){
			temp3 = 0;				/* type 09g & 09h: skip saturation logic */
		}

		if(temp3)					/* if so, */	
			WrReg(p, rdst, temp3);		/* write saturated value  */
		else						/* else */	
			WrReg(p, rdst, temp5);		/* write addition results */
	} else
		WrReg(p, rdst, temp5);		/* write addition results */
}

/** 
* @brief Handle core ALU instruction algorithm (complex version)
* 
* @param p Pointer to current instruction
* @param cvs1 Complex value of first operand
* @param cvs2 Complex value of second operand
* @param rdst Destination register index
*/
void processALU_CFunc(sBCode *p, cplx cvs1, cplx cvs2, int rdst)
{
	cplx ct3, ct4, ct5;
	cplx z2, n2, v2, c2;
	const cplx o2 = { 0, 0 };
	cplx cdata;

	switch(p->Index){
		case iADD_C:
			ct5.r = cvs1.r + cvs2.r;
		 	ct5.i = cvs1.i + cvs2.i;
			break;
		case iADDC_C:
			cdata = RdC2(); 
			ct5.r = cvs1.r + cvs2.r + cdata.r;
		 	ct5.i = cvs1.i + cvs2.i + cdata.i;
			break;
		case iSUB_C:
			ct5.r = cvs1.r - cvs2.r;
		 	ct5.i = cvs1.i - cvs2.i;
			break;
		case iSUBC_C:
			cdata = RdC2(); 
			ct5.r = cvs1.r - cvs2.r + cdata.r - 1;
		 	ct5.i = cvs1.i - cvs2.i + cdata.i - 1;
			break;
		case iSUBB_C:
			ct5.r = - cvs1.r + cvs2.r;
		 	ct5.i = - cvs1.i + cvs2.i;
			break;
		case iSUBBC_C:
			cdata = RdC2(); 
			ct5.r = - cvs1.r + cvs2.r + cdata.r - 1;
		 	ct5.i = - cvs1.i + cvs2.i + cdata.i - 1;
			break;
		case iNOT_C:
			ct5.r = ~cvs1.r;
			ct5.i = ~cvs1.i;
			break;
		case iABS_C:
			if(isNeg(cvs1.r, 12))
				ct5.r = - cvs1.r;
			else
				ct5.r = cvs1.r;
		   	if(isNeg(cvs1.i, 12))
				ct5.i = - cvs1.i;
			else
				ct5.i = cvs1.i;
			break;
		case iSCR_C:
			if(cvs2.i & 0x1){            /* if Im(2nd operand) is odd  */
				ct5.r = -cvs1.r;
			}else{                      /* if Im(2nd operand) is even */
				ct5.r = cvs1.r;
			}

			if(cvs2.i & 0x2){            /* if Im(2nd operand >> 1) is odd  */
				ct5.i = -cvs1.i;
			}else{                      /* if Im(2nd operand >> 1) is even */
				ct5.i = cvs1.i;
			}
			break;
		default:
			printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Invalid ALU.C operation called in processALU_CFunc().\n");
			return;
			break;
	}

	z2.r = (!ct5.r)?1:0;
	z2.i = (!ct5.i)?1:0;
	n2.r = isNeg(ct5.r, 12);
	n2.i = isNeg(ct5.i, 12);

	/* to support type 9h */
	if(p->InstType == t09h){
		if(ct5.r < 0) 
			n2.r = 1;
		else 
			n2.r = 0;

		if(ct5.i < 0) 
			n2.i = 1;
		else 
			n2.i = 0;
	}

	switch(p->Index){
		case iADD_C:
			v2.r = OVCheck(p->InstType, cvs1.r, cvs2.r);
			v2.i = OVCheck(p->InstType, cvs1.i, cvs2.i);
			c2.r = CarryCheck(p, cvs1.r, cvs2.r, 0);
			c2.i = CarryCheck(p, cvs1.i, cvs2.i, 0);
			break;
		case iADDC_C:
			v2.r = OVCheck(p->InstType, cvs1.r, cvs2.r + cdata.r);
			v2.i = OVCheck(p->InstType, cvs1.i, cvs2.i + cdata.i);
			c2.r = CarryCheck(p, cvs1.r, cvs2.r, cdata.r);
			c2.i = CarryCheck(p, cvs1.i, cvs2.i, cdata.i);
			break;
		case iSUB_C:
			v2.r = OVCheck(p->InstType, cvs1.r, - cvs2.r);
			v2.i = OVCheck(p->InstType, cvs1.i, - cvs2.i);
			c2.r = CarryCheck(p, cvs1.r, cvs2.r, 0);
			c2.i = CarryCheck(p, cvs1.i, cvs2.i, 0);
			break;
		case iSUBC_C:
			v2.r = OVCheck(p->InstType, cvs1.r, - cvs2.r + cdata.r - 1);
			v2.i = OVCheck(p->InstType, cvs1.i, - cvs2.i + cdata.i - 1);
			c2.r = CarryCheck(p, cvs1.r, cvs2.r, cdata.r);
			c2.i = CarryCheck(p, cvs1.i, cvs2.i, cdata.i);
			break;
		case iSUBB_C:
			v2.r = OVCheck(p->InstType, cvs2.r, - cvs1.r);
			v2.i = OVCheck(p->InstType, cvs2.i, - cvs1.i);
			c2.r = CarryCheck(p, cvs1.r, cvs2.r, 0);
			c2.i = CarryCheck(p, cvs1.i, cvs2.i, 0);
			break;
		case iSUBBC_C:
			v2.r = OVCheck(p->InstType, cvs2.r, - cvs1.r + cdata.r - 1);
			v2.i = OVCheck(p->InstType, cvs2.i, - cvs1.i + cdata.i - 1);
			c2.r = CarryCheck(p, cvs1.r, cvs2.r, cdata.r);
			c2.i = CarryCheck(p, cvs1.i, cvs2.i, cdata.i);
			break;
		case iNOT_C:
		case iSCR_C:
			v2.r = v2.i = 0;
			c2.r = c2.i = 0;
			break;
		case iABS_C:
			if(cvs1.r == 0x800){
				n2.r = 1; v2.r = 1;
			} else {
				n2.r = 0; v2.r = 0;
			}
			if(cvs1.i == 0x800){
				n2.i = 1; v2.i = 1;
			} else {
				n2.i = 0; v2.i = 0;
			}
			c2.r = c2.i = 0;
			break;
		default:
			break;
	}

	cFlagEffect(p->InstType, z2, n2, v2, c2);

	if(p->Index == iABS_C){
		rAstatR.AS = isNeg(cvs1.r, 12);
		rAstatI.AS = isNeg(cvs1.i, 12);
		rAstatC.AS = (rAstatR.AS || rAstatI.AS);
	}

	if(isALUSatMode()){            	 		/* if saturation enabled  */
		ct3.r = satCheck12b(v2.r, c2.r);    /* check if needed    */
		ct3.i = satCheck12b(v2.i, c2.i);

		if(p->InstType == t09h){
			ct3.r = ct3.i = 0;				/* type 09g & 09h: skip saturation logic */
		}

		if(ct3.r)               			/* if so, */
			ct5.r = ct3.r;      			/* write saturated value  */
		if(ct3.i)               			/* instead of addition result */
			ct5.i = ct3.i;
		cWrReg(p, rdst, ct5.r, ct5.i);
	} else
		cWrReg(p, rdst, ct5.r, ct5.i);     	/* write addition results */
}


/** 
* @brief Handle core shift algorithm
* 
* @param p Pointer to current instruction
* @param temp1 Integer value of first operand
* @param temp2 Integer value of second operand
* @param rdst Destination register index
* @param opt Enum eShift value of shift option string
*/
void processSHIFTFunc(sBCode *p, int temp1, int temp2, int rdst, int opt)
{
	int temp3, temp4, temp5;
	char z, n, v, c;
	unsigned int t4;

	int isMulti, isAccSrc;
	int index;
	int temp0;

	/* check if multifunction: when 2nd operation is a SHIFT */
	switch(p->InstType){
		case t44b:
		case t44d:
		case t45b:
		case t45d:
			isMulti = TRUE;
			break;
		default:
			isMulti = FALSE;
			break;
	}

	/* check if 1st source is an accumulator */
	switch(p->InstType){
		case t15c:
		case t12m:
		case t12o:
		case t14k:
		case t44d:
		case t45d:
			isAccSrc = TRUE;
			break;
		default:
			isAccSrc = FALSE;
			break;
	}

	if(isMulti){
		index = p->IndexMulti[0];	/* if isMulti, 2nd operation is a SHIFT */
	}else{
		index = p->Index;
	}

	switch(index){
		case iASHIFT:
		case iASHIFTOR:
			temp0 = temp1;

			if((opt == esHI)||(opt == esHIRND)){
				temp1 <<= 12;
			}
			if(isNeg(temp2, 12)){	/* shift right */
				if(temp2 == -32){
					if(isAccSrc && isNeg(temp0, 24)){
						temp5 = 0xFFFFFFFF;
					}else if(!isAccSrc && isNeg(temp0, 12)){
						temp5 = 0xFFFFFFFF;
					}else {
						temp5 = 0;
					}
				}else{
					/* shift right here */
					int numshift = (-temp2);    /* number of shift */

					/* shift (numshift -1) bits */
					temp5 = (temp1 >> (numshift - 1));
 
					/* if rounding should be performed, */
					if((opt == esLORND) || (opt == esHIRND) || (opt == esRND)) {
						/* add 1 to intermediate result */
						temp5 += 1;
					}

					/* shift last 1 bit */
					temp5 = (temp5 >> 1);
				}
			} else {				/* shift left */
				if(temp2 == 32){
					temp5 = 0;
				}else{
					temp5 = temp1 << temp2;
				}
			}
			break;
		case iLSHIFT:
		case iLSHIFTOR:
			/* check if 1st source is an accumulator */
			if(isAccSrc){
				;
			}else{
				temp1 &= 0xFFF;
			}

			t4 = temp1;
			if((opt == esHI)||(opt == esHIRND)){
				t4 <<= 12;
			}
			if(isNeg(temp2, 12)){
				if(temp2 == -32){
					temp5 = 0;
				}else{
					/* shift right here */
					int numshift = (-temp2);    /* number of shift */

					/* shift (numshift -1) bits */
					t4 = (t4 >> (numshift - 1));
 
					/* if rounding should be performed, */
					if((opt == esLORND) || (opt == esHIRND) || (opt == esRND)) {
						/* add 1 to intermediate result */
						t4 += 1;
					}

					/* shift last 1 bit */
					temp5 = (t4 >> 1);
				}
			} else {
				if(temp2 == 32){
					temp5 = 0;
				}else{
					temp5 = t4 << temp2;
				}
			}
			break;
		default:
			printRunTimeError(p->PMA, (char *)sOp[index], 
						"Invalid SHIFT operation called in processSHIFTFunc().\n");
			return;
			break;
	}
	
	if((index == iASHIFTOR) || (index == iLSHIFTOR)){
		temp5 |= RdReg(p, rdst);   /* OR */
	}
	
	WrReg(p, rdst, temp5);

	if(isMulti){		/* if this SHIFT is the 2nd operation in a multifunction */
		switch(p->InstType){
			case t44b:
			case t45b:
				v = OVCheck(t15a, temp5, 0);
				flagEffect(t15a, 0, 0, v, 0);
				break;
			case t44d:
			case t45d:
				v = OVCheck(t15c, temp5, 0);
				flagEffect(t15c, 0, 0, v, 0);
				break;
			default:
				break;
		}
	}else{
		v = OVCheck(p->InstType, temp5, 0);
		flagEffect(p->InstType, 0, 0, v, 0);
	}
}

/** 
* @brief Handle core shift algorithm (complex version)
* 
* @param p Pointer to current instruction
* @param ct1 Complex value of first operand
* @param ct2 Complex value of second operand
* @param rdst Destination register index
* @param opt Enum eShift value of shift option string
*/
void processSHIFT_CFunc(sBCode *p, cplx ct1, cplx ct2, int rdst, int opt)
{
	cplx ct3, ct4, ct5;
	cplx z2, n2, v2, c2;
	const cplx o2 = { 0, 0 };
	unsigned int t4r, t4i;
	int isAccSrc;
	cplx ct0;

	/* check if 1st source is an accumulator */
	switch(p->InstType){
		case t15d:
		case t12n:
		case t12p:
		case t14l:
			isAccSrc = TRUE;
			break;
		default:
			isAccSrc = FALSE;
			break;
	}

	switch(p->Index){
		case iASHIFT_C:
		case iASHIFTOR_C:
			ct0 = ct1;

			if((opt == esHI)||(opt == esHIRND)){
				ct1.r <<= 12;
				ct1.i <<= 12;
			}
			if(isNeg(ct2.r, 12)){	/* shift right */
				if(ct2.r == -32){
					if(isAccSrc && isNeg(ct0.r, 24)){
						ct5.r = 0xFFFFFFFF;
					}else if(!isAccSrc && isNeg(ct0.r, 12)){
						ct5.r = 0xFFFFFFFF;
					} else
						ct5.r = 0;
				} else {
					/* shift right here: real part */
					int numshift = (-ct2.r);    /* number of shift */

					/* shift (numshift -1) bits */
					ct5.r = (ct1.r >> (numshift - 1));

					/* if rounding should be performed, */
					if((opt == esLORND) || (opt == esHIRND) || (opt == esRND)) {
						/* add 1 to intermediate result */
						ct5.r += 1;
					}

					/* shift last 1 bit */
					ct5.r = (ct5.r >> 1);
				}
			} else {				/* shift left */
				if(ct2.r == 32){
					ct5.r = 0;
				}else{
					ct5.r = ct1.r << ct2.r;
				}
			}
			if(isNeg(ct2.i, 12)){	/* shift right */
				if(ct2.i == -32){
					if(isAccSrc && isNeg(ct0.i, 24)){
						ct5.i = 0xFFFFFFFF;
					}else if(!isAccSrc && isNeg(ct0.i, 12)){
						ct5.i = 0xFFFFFFFF;
					} else
						ct5.i = 0;
				} else {
					/* shift right here: imaginary part */
					int numshift = (-ct2.i);    /* number of shift */

					/* shift (numshift -1) bits */
					ct5.i = (ct1.i >> (numshift - 1));

					/* if rounding should be performed, */
					if((opt == esLORND) || (opt == esHIRND) || (opt == esRND)) {
						/* add 1 to intermediate result */
						ct5.i += 1;
					}

					/* shift last 1 bit */
					ct5.i = (ct5.i >> 1);
				}
			} else {				/* shift left */
				if(ct2.i == 32){
					ct5.i = 0;
				}else{
					ct5.i = ct1.i << ct2.i;
				}
			}
			break;
		case iLSHIFT_C:
		case iLSHIFTOR_C:
			/* check if 1st source is an accumulator */
			if(isAccSrc){
				;
			}else{
				ct1.r &= 0xFFF;
				ct1.i &= 0xFFF;
			}

			t4r = ct1.r;
			t4i = ct1.i;

			if((opt == esHI)||(opt == esHIRND)){
				t4r <<= 12;
				t4i <<= 12;
			}
			if(isNeg(ct2.r, 12)){	/* shift right */
				if(ct2.r == -32){
					ct5.r = 0;
				} else {
					/* shift right here: real part */
					int numshift = (-ct2.r);    /* number of shift */

					/* shift (numshift -1) bits */
					t4r = (t4r >> (numshift - 1));

					/* if rounding should be performed, */
					if((opt == esLORND) || (opt == esHIRND) || (opt == esRND)) {
						/* add 1 to intermediate result */
						t4r += 1;
					}

					/* shift last 1 bit */
					ct5.r = (t4r >> 1);
				}
			} else {				/* shift left */
				if(ct2.r == 32){
					ct5.r = 0;
				}else{
					ct5.r = t4r << ct2.r;
				}
			}
			if(isNeg(ct2.i, 12)){	/* shift right */
				if(ct2.i == -32){
					ct5.i = 0;
				} else {
					/* shift right here: imaginary part */
					int numshift = (-ct2.i);    /* number of shift */

					/* shift (numshift -1) bits */
					t4i = (t4i >> (numshift - 1));

					/* if rounding should be performed, */
					if((opt == esLORND) || (opt == esHIRND) || (opt == esRND)) {
						/* add 1 to intermediate result */
						t4i += 1;
					}

					/* shift last 1 bit */
					ct5.i = (t4i >> 1);
				}
			} else {				/* shift left */
				if(ct2.i == 32){
					ct5.i = 0;
				}else{
					ct5.i = t4i << ct2.i;
				}
			}
			break;
		default:
			printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Invalid SHIFT.C operation called in processSHIFT_CFunc().\n");
			return;
			break;
	}
	
	if((p->Index == iASHIFTOR_C) || (p->Index == iLSHIFTOR_C)){	
					ct3 = cRdReg(p, rdst);
					ct5.r |= ct3.r;     /* OR */
					ct5.i |= ct3.i;     /* OR */
	}
	
	v2.r = OVCheck(p->InstType, ct5.r, 0);
	v2.i = OVCheck(p->InstType, ct5.i, 0);
	cFlagEffect(p->InstType, o2, o2, v2, o2);

	cWrReg(p, rdst, ct5.r, ct5.i);
}

/** 
* @brief Update Ix register for post-modify ("+=") addressing mode.
* 
* @param p Pointer to current instruction
* @param ridx Register ID
* @param data Data to write (12-bit)
*/
void updateIReg(sBCode *p, int ridx, int data)
{
	int index = ridx - eI0;				/* index: should be 0 ~ 7 */
	int loopSize;
	int base;

	if((ridx >= eI0) && (ridx <= eI7)) {		/* Ix register */

		/* read Lx register */
		if((::Cycles - LastAccessLx[index]) <= 1){		/* restriction: if too close, use old value */
			loopSize = rbL[index];
		}else{
			loopSize = rL[index];
		}

		if(loopSize != 0){					/* if circular addressing */

			/* read Bx register */
			if((::Cycles - LastAccessBx[index]) <= 1){		/* restriction: if too close, use old value */
				base = rbB[index];
			}else{
				base = rB[index];
			}

			int uBound = base + loopSize;
			int lBound = base;

			if(data >= uBound){
				data -= loopSize;
			}else if(data < lBound){ 
				data += loopSize;
			}
		}

		/* back up values */
		/* Note: don't record when it's written: 2009.07.08 */
		rbI[index] = rI[index];

		/* update */
		rI[index] = 0xFFFF & data;
	}else{
		printRunTimeError(p->PMA, (char *)sOp[p->Index], 
			"updateIReg() called with an non-Ix register argument.\n");
	}
}

/** 
* @brief Handle CORDIC-related algorithms (for iPOLAR_C and iRECT_C)
* 
* @param p Pointer to current instruction
* @param ct1 Complex value of first operand (r: magnitude in integer, i: angle in 3.11 format)
* @param rdst Destination register index
* 
* @return cplx (r: x component, i: y component)
*/
void processCordicFunc(sBCode *p, cplx ct1, int rdst)
{
	cplx z2, n2;
	const cplx o2 = { 0, 0 };
	cplx ct5;

	switch(p->Index){
		case iRECT_C:
			ct5 = runCordicRotationMode(ct1.r, ct1.i);
			break;
		case iPOLAR_C:
			ct5 = runCordicVectoringMode(ct1.r, ct1.i);
			break;
		default:
			printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Invalid CORDIC operation called in processCordicFunc().\n");
			return;
			break;
	}

	z2.r = (!ct5.r)?1:0;
	z2.i = (!ct5.i)?1:0;
	n2.r = isNeg(ct5.r, 12);
	n2.i = isNeg(ct5.i, 12);

	cWrReg(p, rdst, ct5.r, ct5.i);

	cFlagEffect(p->InstType, z2, n2, o2, o2);
}

/** 
* @brief Reset program memory address to the given addr value.
* 
* @param addr New address
*/
//void resetPMA(int addr)
//{
//	curaddr = addr;
//}

/** 
* @brief Given field id, get value from binary code.
* 
* @param p Pointer to current binary code
* @param fval eField value of the field, or UNDEF_FIELD (if not found)
* 
* @return Integer value of the designated field
*/
int getIntField(sBCode *p, int fval)
{
	int code = UNDEF_FIELD;
	int found = FALSE;

	const sInstFormat *sp = &(sInstFormatTable[p->InstType]);

	int sum = 0;
	for(int i = 0; i < MAX_FIELD; i++){
		if(sp->fields[i] == fval){ 
			code = bin2int(p->Binary + sum, sp->widths[i]);
			found = TRUE;
			break;
		}
		sum += sp->widths[i];
	}

	if(found == FALSE){
		if(!(fval == fCOND || fval == fCONJ)){
			/* print error message */
			printRunTimeError(p->PMA, (char *)sField[fval], 
				"Parse Error: getIntField().\n");
		}
	}

	return code;
}

/** 
* @brief Given field id, get its bit width.
* 
* @param p Pointer to current binary code
* @param fval eField value
* 
* @return Number of bits of the designated field, or 0 (if not found)
*/
//int getWidthOfField(sBCode *p, int fval)
//{
//	int w = 0;
//
//	const sInstFormat *sp = &(sInstFormatTable[p->InstType]);
//
//	for(int i = 0; i < MAX_FIELD; i++){
//		if(sp->fields[i] == fval){ 
//			w = sp->widths[i];	/* found */
//			break;
//		}
//	}
//
//	return w;
//}

/** 
* @brief Decode AF field to get opcode index.
* 
* @param p Pointer to current binary code
* @param cflag 0 if real, 1 if complex (*.C)
* 
* @return Opcode index value
*/
int getIndexFromAF(sBCode *p, int cflag)
{
	int index;

	if(!cflag){		/* real, non-complex opcode */
		switch(getIntField(p, fAF)){
			case 0:         /* ADD */
				index = iADD;
				break;
			case 1:         /* ADDC */
				index = iADDC;
				break;
			case 2:         /* SUB */
				index = iSUB;
				break;
			case 3:         /* SUBC */
				index = iSUBC;
				break;
			case 4:         /* SUBB */
				index = iSUBB;
				break;
			case 5:         /* SUBBC */
				index = iSUBBC;
				break;
			case 6:         /* AND */
				index = iAND;
				break;
			case 7:         /* OR */
				index = iOR;
				break;
			case 8:         /* XOR */
				index = iXOR;
				break;
			case 9:         /* TSTBIT, CLRBIT */
				switch(getIntField(p, fYZ)){
					case 0:     /* YZ = 0: TSTBIT */
						index = iTSTBIT;
						break;
					case 1:     /* YZ = 1: CLRBIT */
						index = iCLRBIT;
						break;
					default:
						index = iUNKNOWN;
						break;
				}
				break;
			case 10:        /* SETBIT */
				index = iSETBIT;
				break;
			case 11:        /* TGLBIT */
				index = iTGLBIT;
				break;
			case 12:        /* NOT */
				index = iNOT;
				break;
			case 13:        /* ABS */
				index = iABS;
				break;
			case 14:        /* INC */
				index = iINC;
				break;
			case 15:        /* DEC */
				index = iDEC;
				break;
			default:
				index = iUNKNOWN;
				break;
		}
	}else{	/* complex opcode */
		switch(getIntField(p, fAF)){
			case 0:         /* ADD.C */
				index = iADD_C;
				break;
			case 1:         /* ADDC.C */
				index = iADDC_C;
				break;
			case 2:         /* SUB.C */
				index = iSUB_C;
				break;
			case 3:         /* SUBC.C */
				index = iSUBC_C;
				break;
			case 4:         /* SUBB.C */
				index = iSUBB_C;
				break;
			case 5:         /* SUBBC.C */
				index = iSUBBC_C;
				break;
			case 12:        /* NOT.C */
				index = iNOT_C;
				break;
			case 13:        /* ABS.C */
				index = iABS_C;
				break;
			default:
				index = iUNKNOWN;
				break;
		}
	}
	return index;
}

/** 
* @brief Decode AMF field to get opcode index.
* 
* @param p Pointer to current binary code
* @param cflag 0 if real, 1 if complex (*.C), 2 if mixed (*.RC)
* 
* @return Opcode index value, 0 if NONE
*/
int getIndexFromAMF(sBCode *p, int cflag)
{
	int index;

	if(cflag == 0){		/* real, non-complex opcode */
		switch(getIntField(p, fAMF)){
			case 0x00:		/* NOP */
				index = 0x0;
				break;
			case 0x01:      /* MPY */
			case 0x04:
			case 0x05:
			case 0x06:
			case 0x07:
				index = iMPY;
				break;
			case 0x02:      /* MAC */
			case 0x08:
			case 0x09:
			case 0x0A:
			case 0x0B:
				index = iMAC;
				break;
			case 0x03:      /* MAS */
			case 0x0C:
			case 0x0D:
			case 0x0E:
			case 0x0F:
				index = iMAS;
				break;
			case 0x10:      /* ADD */
				index = iADD;
				break;
			case 0x12:      /* SUB */
				index = iSUB;
				break;
			case 0x14:      /* SUBB */
				index = iSUBB;
				break;
			default:
				index = iUNKNOWN;
				break;
		}
	}else if (cflag == 1){	/* complex opcode (*.C) */
		switch(getIntField(p, fAMF)){
			case 0x00:		/* NOP */
				index = 0x0;
				break;
			case 0x01:      /* MPY.C */
			case 0x04:
			case 0x05:
			case 0x06:
			case 0x07:
				index = iMPY_C;
				break;
			case 0x02:      /* MAC.C */
			case 0x08:
			case 0x09:
			case 0x0A:
			case 0x0B:
				index = iMAC_C;
				break;
			case 0x03:      /* MAS.C */
			case 0x0C:
			case 0x0D:
			case 0x0E:
			case 0x0F:
				index = iMAS_C;
				break;
			case 0x10:      /* ADD.C */
				index = iADD_C;
				break;
			case 0x12:      /* SUB.C */
				index = iSUB_C;
				break;
			case 0x14:      /* SUBB.C */
				index = iSUBB_C;
				break;
			default:
				index = iUNKNOWN;
				break;
		}
	}else{					/* mixed opcode (*.RC) */
		switch(getIntField(p, fAMF)){
			case 0x00:		/* NOP */
				index = 0x0;
				break;
			case 0x01:      /* MPY.RC */
			case 0x04:
			case 0x05:
			case 0x06:
			case 0x07:
				index = iMPY_RC;
				break;
			case 0x02:      /* MAC.RC */
			case 0x08:
			case 0x09:
			case 0x0A:
			case 0x0B:
				index = iMAC_RC;
				break;
			case 0x03:      /* MAS.RC */
			case 0x0C:
			case 0x0D:
			case 0x0E:
			case 0x0F:
				index = iMAS_RC;
				break;
			default:
				index = iUNKNOWN;
				break;
		}
	}
	return index;
}

/** 
* @brief Decode SF field to get opcode index.
* 
* @param p Pointer to current binary code
* @param cflag 0 if real, 1 if complex (*.C)
* 
* @return Opcode index value
*/
int getIndexFromSF(sBCode *p, int cflag)
{
	int index;

	if(!cflag){		/* real, non-complex opcode */
		switch(getIntField(p, fSF)){
			case 0x0:       /* LSHIFT */
			case 0x2:
			case 0x8:
			case 0x10:
			case 0x12:
			case 0x18:
				index = iLSHIFT;
				break;
			case 0x1:       /* LSHIFTOR */
			case 0x3:
			case 0x9:
			case 0x11:
			case 0x13:
			case 0x19:
				index = iLSHIFTOR;
				break;
			case 0x4:       /* ASHIFT */
			case 0x6:
			case 0xA:
			case 0x14:
			case 0x16:
			case 0x1A:
				index = iASHIFT;
				break;
			case 0x5:       /* ASHIFTOR */
			case 0x7:
			case 0xB:
			case 0x15:
			case 0x17:
			case 0x1B:
				index = iASHIFTOR;
				break;
			case 0xC:       /* EXP */
			case 0xD:
			case 0xE:
				index = iEXP;
				break;
			case 0xF:       /* EXPADJ */
				index = iEXPADJ;
				break;
			default:
				index = iUNKNOWN;
				break;
		}
	}else{	/* complex opcode */
		switch(getIntField(p, fSF)){
			case 0x0:       /* LSHIFT.C */
			case 0x2:
			case 0x8:
			case 0x10:
			case 0x12:
			case 0x18:
				index = iLSHIFT_C;
				break;
			case 0x1:       /* LSHIFTOR.C */
			case 0x3:
			case 0x9:
			case 0x11:
			case 0x13:
			case 0x19:
				index = iLSHIFTOR_C;
				break;
			case 0x4:       /* ASHIFT.C */
			case 0x6:
			case 0xA:
			case 0x14:
			case 0x16:
			case 0x1A:
				index = iASHIFT_C;
				break;
			case 0x5:       /* ASHIFTOR.C */
			case 0x7:
			case 0xB:
			case 0x15:
			case 0x17:
			case 0x1B:
				index = iASHIFTOR_C;
				break;
			case 0xC:       /* EXP.C */
			case 0xD:
			case 0xE:
				index = iEXP_C;
				break;
			case 0xF:       /* EXPADJ.C */
				index = iEXPADJ_C;
				break;
			default:
				index = iUNKNOWN;
				break;
		}
	}
	return index;
}

/** 
* @brief Decode SF2 field to get opcode index.
* 
* @param p Pointer to current binary code
* @param cflag 0 if real, 1 if complex (*.C)
* 
* @return Opcode index value
*/
int getIndexFromSF2(sBCode *p, int cflag)
{
	int index;

	if(!cflag){		/* real, non-complex opcode */
		switch(getIntField(p, fSF2)){
			case 0x0:       /* LSHIFT */
			case 0x1:
			case 0x2:
			case 0x3:
			case 0x8:
			case 0x9:
				index = iLSHIFT;
				break;
			case 0x4:       /* ASHIFT */
			case 0x5:
			case 0x6:
			case 0x7:
			case 0xA:
			case 0xB:
				index = iASHIFT;
				break;
			default:
				index = iUNKNOWN;
				break;
		}
	}else{	/* complex opcode */
		switch(getIntField(p, fSF2)){
			case 0x0:       /* LSHIFT.C */
			case 0x1:
			case 0x2:
			case 0x3:
			case 0x8:
			case 0x9:
				index = iLSHIFT_C;
				break;
			case 0x4:       /* ASHIFT.C */
			case 0x5:
			case 0x6:
			case 0x7:
			case 0xA:
			case 0xB:
				index = iASHIFT_C;
				break;
			default:
				index = iUNKNOWN;
				break;
		}
	}
	return index;
}

/** 
* @brief Decode MF field to get opcode index.
* 
* @param p Pointer to current binary code
* @param cflag 0 if real, 1 if complex (*.C)
* 
* @return Opcode index value, 0 if NONE
*/
int getIndexFromMF(sBCode *p, int cflag)
{
	int index;

	if(!cflag){		/* real, non-complex opcode */
		switch(getIntField(p, fMF)){
			case 0x00:		/* NOP */
				index = 0x0;
				break;
			case 0x01:      /* MPY */
			case 0x04:
			case 0x05:
			case 0x06:
			case 0x07:
				index = iMPY;
				break;
			case 0x02:      /* MAC */
			case 0x08:
			case 0x09:
			case 0x0A:
			case 0x0B:
				index = iMAC;
				break;
			case 0x03:      /* MAS */
			case 0x0C:
			case 0x0D:
			case 0x0E:
			case 0x0F:
				index = iMAS;
				break;
			default:
				index = iUNKNOWN;
				break;
		}
	}else{	/* complex opcode */
		switch(getIntField(p, fMF)){
			case 0x00:		/* NOP */
				index = 0x0;
				break;
			case 0x01:      /* MPY.C */
			case 0x04:
			case 0x05:
			case 0x06:
			case 0x07:
				index = iMPY_C;
				break;
			case 0x02:      /* MAC.C */
			case 0x08:
			case 0x09:
			case 0x0A:
			case 0x0B:
				index = iMAC_C;
				break;
			case 0x03:      /* MAS.C */
			case 0x0C:
			case 0x0D:
			case 0x0E:
			case 0x0F:
				index = iMAS_C;
				break;
			default:
				index = iUNKNOWN;
				break;
		}
	}
	return index;
}

/** 
* @brief Decode DF field to get opcode index.
* 
* @param p Pointer to current binary code
* 
* @return Opcode index value
*/
int getIndexFromDF(sBCode *p)
{
	int index;

	switch(getIntField(p, fDF)){
		case 0:         /* DIVQ */
			index = iDIVQ;
			break;
		case 1:         /* DIVS */
			index = iDIVS;
			break;
		default:
			index = iUNKNOWN;
			break;
	}
	return index;
}


/** 
* @brief Decode AMF field to get multiply option.
* 
* @param p Pointer to current binary code
* 
* @return enum eMul value, 0 if NONE
*/
int getOptionFromAMF(sBCode *p)
{
	int opt = 0;

	switch(getIntField(p, fAMF)){
		case 0x01:      /* RND */
		case 0x02:
		case 0x03:
			opt = emRND;
			break;
		case 0x04:      /* SS */
		case 0x08:
		case 0x0C:
			opt = emSS;
			break;
		case 0x05:      /* SU */
		case 0x09:
		case 0x0D:
			opt = emSU;
			break;
		case 0x06:      /* US */
		case 0x0A:
		case 0x0E:
			opt = emUS;
			break;
		case 0x07:      /* UU */
		case 0x0B:
		case 0x0F:
			opt = emUU;
			break;
		default:
			opt = 0x0;
			break;
	}
	return opt;
}


/** 
* @brief Decode MF field to get multiply option.
* 
* @param p Pointer to current binary code
* 
* @return enum eMul value, 0 if NONE
*/
int getOptionFromMF(sBCode *p)
{
	int opt = 0;

	switch(getIntField(p, fMF)){
		case 0x01:      /* RND */
		case 0x02:
		case 0x03:
			opt = emRND;
			break;
		case 0x04:      /* SS */
		case 0x08:
		case 0x0C:
			opt = emSS;
			break;
		case 0x05:      /* SU */
		case 0x09:
		case 0x0D:
			opt = emSU;
			break;
		case 0x06:      /* US */
		case 0x0A:
		case 0x0E:
			opt = emUS;
			break;
		case 0x07:      /* UU */
		case 0x0B:
		case 0x0F:
			opt = emUU;
			break;
		default:
			opt = UNDEF_FIELD;
			break;
	}
	return opt;
}


/** 
* @brief Decode SF field to get shift option.
* 
* @param p Pointer to current binary code
* 
* @return enum eShift value, 0 if NONE
*/
int getOptionFromSF(sBCode *p)
{
	int opt = 0;

	switch(getIntField(p, fSF)){
		case 0x00:      /* HI */
		case 0x01:
		case 0x04:
		case 0x05:
		case 0x0C:
			opt = esHI;
			break;
		case 0x02:      /* LO */
		case 0x03:
		case 0x06:
		case 0x07:
		case 0x0E:
			opt = esLO;
			break;
		case 0x10:      /* HIRND */
		case 0x11:
		case 0x14:
		case 0x15:
			opt = esHIRND;
			break;
		case 0x12:      /* LORND */
		case 0x13:
		case 0x16:
		case 0x17:
			opt = esLORND;
			break;
		case 0x8:      /* NORND */
		case 0x9:
		case 0xA:
		case 0xB:
			opt = esNORND;
			break;
		case 0x18:      /* RND */
		case 0x19:
		case 0x1A:
		case 0x1B:
			opt = esRND;
			break;
		case 0x0D:      /* HIX */
			opt = esHIX;
			break;
		default:
			opt = UNDEF_FIELD;
			break;
	}
	return opt;
}

/** 
* @brief Decode SF2 field to get shift option.
* 
* @param p Pointer to current binary code
* 
* @return enum eShift value, 0 if NONE
*/
int getOptionFromSF2(sBCode *p)
{
	int opt = 0;

	switch(getIntField(p, fSF2)){
		case 0x0:      /* HI */
		case 0x4:
			opt = esHI;
			break;
		case 0x2:      /* LO */
		case 0x6:
			opt = esLO;
			break;
		case 0x1:      /* HIRND */
		case 0x5:
			opt = esHIRND;
			break;
		case 0x3:      /* LORND */
		case 0x7:
			opt = esLORND;
			break;
		case 0x8:      /* NORND */
		case 0xA:
			opt = esNORND;
			break;
		case 0x9:      /* RND */
		case 0xB:
			opt = esRND;
			break;
		default:
			opt = UNDEF_FIELD;
			break;
	}
	return opt;
}


/** 
* @brief Check if this register is read-only.
* 
* @param p Pointer to current instruction
* @param ridx Register ID
* @param s Name of register to write
*
* @return TRUE or FALSE
*/
int isReadOnlyReg(sBCode *p, int ridx)
{
	char treg[10];
	int val = FALSE;

	switch(ridx){
		case eSSTAT:	/* SSTAT */
			val = TRUE;
			strcpy(treg, "SSTAT");
			break;
		case eASTAT_C:	/* ASTAT.C */
			strcpy(treg, "ASTAT.C");
			val = TRUE;
			break;
		default:
        	;   /* do nothing */
			break;
	}

	if(val){
		printRunTimeWarning(p->PMA, treg, 
			"This register is read-only - cannot be written.\n");
	}
	return val;
}


