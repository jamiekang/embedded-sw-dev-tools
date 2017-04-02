/*
All Rights Reserved.
*/

/** 
* @file simsupport.cc
* @brief Misc supporting routines for simulator main
* @date 2008-09-29
*/

#include <stdio.h>	/* perror() */
#include <stdlib.h>	/* strtol(), exit() */
#include <string.h>	/* strcpy() */
#include <ctype.h>	/* isdigit() */
#include <assert.h> /* assert() */
#include "dspsim.h"
#include "symtab.h"
#include "icode.h"
#include "dmem.h"
#include "optab.h"
#include "simcore.h"
#include "simsupport.h"
#include "dspdef.h"

/** Register set definitions */
sint rR[32];		/**< Rx data registers: 12b */
sAcc rAcc[8];	/**< Accumulator registers: 32/36b */

int rI[8];		/**< Ix registers: 16b - not shared, no SIMD */
int rM[8];		/**< Mx registers: 16b - not shared, no SIMD */
int rL[8];		/**< Lx registers: 16b - not shared, no SIMD */
int rB[8];		/**< Bx registers: 16b - not shared, no SIMD */

/* backup copies of Ix/Mx/Lx/Bx (not real registers): 2009.07.08 */
int rbI[8][2];      /**< Ix registers: 16b - not shared, no SIMD */
int rbM[8][2];      /**< Mx registers: 16b - not shared, no SIMD */
int rbL[8][2];      /**< Lx registers: 16b - not shared, no SIMD */
int rbB[8][2];      /**< Bx registers: 16b - not shared, no SIMD */
 
/**< to record when Ix/Mx/Lx/Bx registers were written: 2009.07.08 */
int LastAccessIx[8];    /**< not shared, no SIMD */
int LastAccessMx[8];    /**< not shared, no SIMD */
int LastAccessLx[8];    /**< not shared, no SIMD */
int LastAccessBx[8];    /**< not shared, no SIMD */

/* backup copies of LastAccessIx/Mx/Lx/Bx (not real registers): 2010.07.21 */
int LAbIx[8][2];    /**< not shared, no SIMD */
int LAbMx[8][2];    /**< not shared, no SIMD */
int LAbLx[8][2];    /**< not shared, no SIMD */
int LAbBx[8][2];    /**< not shared, no SIMD */

int rLPSTACK;   /**< _LPSTACK */
int rPCSTACK;   /**< _PCSTACK */
sIcntl rICNTL;     /**< _ICNTL */
sIrptl rIMASK;     /**< _IMASK */
sIrptl rIRPTL;     /**< _IRPTL */

int rIVEC[4];	/**< _IVECx registers: 16b */

sAstat rAstatR;	/**< ASTAT.R: Note - Real instruction affects only ASTAT.R */
sAstat rAstatI;	/**< ASTAT.I */
sAstat rAstatC;	/**< ASTAT.C: Note - Complex instruction affects ASTAT.R, ASTAT.I, and ASTAT.C */
sMstat rMstat;	/**< _MSTAT */
sSstat rSstat;	/**< _SSTAT */
int rDstat0;	/**< _DSTAT0 */
int rDstat1;	/**< _DSTAT1 */

int rCNTR;		/**< _CNTR */
int	rLPEVER;	/**< _LPEVER: set if DO UNTIL FOREVER */

sint rUMCOUNT;	/**< UMCOUNT */

sint rDID; 		/**< Data Path ID: 0 for DP #0, 1 for DP #1, ... */
sint rDPENA;	/**< decoded version of _DSTAT0 - for simulator internal only */
sint rDPMST;	/**< decoded version of _DSTAT1 - for simulator internal only */

/* backup copy of MSTAT (not a real register): 2009.07.08 */
sMstat rbMstat;	

/**< to record when MSTAT register was written: 2009.07.08 */
int LastAccessMstat;

/* backup copy of CNTR (not a real register): 2010.07.20 */
int rbCNTR;	

/**< to record when CNTR register was written: 2010.07.20 */
int LastAccessCNTR;

/** Hardware stacks */
sStack PCStack;				/**< PC Stack */
sStack LoopBeginStack;		/**< Loop Begin Stack */
sStack LoopEndStack;		/**< Loop End Stack */
sStack LoopCounterStack;	/**< Loop Counter Stack */
ssStack ASTATStack[3];       /**< ASTAT Stack */
sStack MSTATStack;          /**< MSTAT Stack */
sStack LPEVERStack;			/**< LPEVER Stack */

/** Simulator internal variables */
int oldPC = UNDEFINED, PC;

/** 
* @brief Check if RREG16 (Ix, M, Lx, Bx, CNTR, and other 16-bit control registers)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if RREG16, 0 if not
*/
int isRReg16(sICode *p, char *s)
{
	//if(!strcasecmp("NONE", s)) return TRUE; /* NONE */

	if(((s[0] == 'I') || (s[0] == 'i')) && isdigit(s[1])) {		/* Ix register */
		return TRUE;
	}else if(((s[0] == 'M') || (s[0] == 'm')) && isdigit(s[1])) {		/* Mx register */
		return TRUE;
	}else if(((s[0] == 'L') || (s[0] == 'l')) && isdigit(s[1])) {		/* Lx register */
		return TRUE;
	}else if(((s[0] == 'B') || (s[0] == 'b')) && isdigit(s[1])) {		/* Bx register */
		return TRUE;
	}else if(!strcasecmp(s, "_CNTR")){		/* _CNTR */
		return TRUE;
	}else if(!strcasecmp(s, "_MSTAT")){		/* _MSTAT */
		return TRUE;
	}else if(!strcasecmp(s, "_SSTAT")){		/* _SSTAT */
		return TRUE;
	}else if(!strcasecmp(s, "_LPSTACK")){	/* _LPSTACK */
		return TRUE;
	}else if(!strcasecmp(s, "_PCSTACK")){	/* _PCSTACK */
		return TRUE;
	}else if(!strcasecmp(s, "ASTAT.R")){	/* ASTAT.R */
		return TRUE;
	}else if(!strcasecmp(s, "ASTAT.I")){	/* ASTAT.I */
		return TRUE;
	}else if(!strcasecmp(s, "ASTAT.C")){	/* ASTAT.C */
		return TRUE;
	}else if(!strcasecmp(s, "_ICNTL")){		/* _ICNTL */
		return TRUE;
	}else if(!strcasecmp(s, "_IMASK")){		/* _IMASK */
		return TRUE;
	}else if(!strcasecmp(s, "_IRPTL")){		/* _IRPTL */
		return TRUE;
	}else if(!strcasecmp(s, "_LPEVER")){	/* _LPEVER */
		return TRUE;
	}else if(!strcasecmp(s, "_DSTAT0")){	/* _DSTAT0 */
		return TRUE;
	}else if(!strcasecmp(s, "_DSTAT1")){	/* _DSTAT1 */
		return TRUE;
	}else if(!strcasecmp(s, "_IVEC0")){		/* _IVEC0 */
		return TRUE;
	}else if(!strcasecmp(s, "_IVEC1")){		/* _IVEC1 */
		return TRUE;
	}else if(!strcasecmp(s, "_IVEC2")){		/* _IVEC2 */
		return TRUE;
	}else if(!strcasecmp(s, "_IVEC3")){		/* _IVEC3 */
		return TRUE;
	}else if(!strcasecmp(s, "UMCOUNT")){	/* UMCOUNT */
		return TRUE;
	}else if(!strcasecmp(s, "DID")){			/* DID */
		return TRUE;
	}
	return FALSE;
}

/** 
* @brief Check if RREG (DREG12, RREG16)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if RREG, 0 if not
*/
int isRReg(sICode *p, char *s)
{
	if(isDReg12(p, s) || isRReg16(p, s))
		return TRUE;
	else
		return FALSE;
}

/** 
* @brief Check if CREG (DREG24)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if CREG, 0 if not
*/
int isCReg(sICode *p, char *s)
{
	if(isDReg24(p, s))
		return TRUE;
	else
		return FALSE;
}

/** 
* @brief Check if Ix Register, where x = 0, 1, 2, ..., 7
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if IREG, 0 if not
*/
int isIReg(sICode *p, char *s)
{
	if(((s[0] == 'I') || (s[0] == 'i')) && isdigit(s[1])){ 		/* I register */
		int	val = s[1] - '0';
		if(val >= 0 && val <= 7) return TRUE;
		else {
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of I0 ~ I7.\n");
			return FALSE;
		}
	}else
		return FALSE;
}

/** 
* @brief Check if Ix Register, where x = 0, 1, 2, 3
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if Ix, 0 if not
*/
int isIx(sICode *p, char *s)
{
	int val;

	if(((s[0] == 'I') || (s[0] == 'i')) && isdigit(s[1])) {		/* Ix register */
		val = s[1] - '0';
		if(val >= 0 && val <= 3) return TRUE;
		else{
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of I0/I1/I2/I3.\n");
			return FALSE;
		}
	} else
		return FALSE;
}

/** 
* @brief Check if Iy Register, where y = 4, 5, 6, 7
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if Iy, 0 if not
*/
int isIy(sICode *p, char *s)
{
	int val;

	if(((s[0] == 'I') || (s[0] == 'i')) && isdigit(s[1])) {		/* Iy register */
		val = s[1] - '0';
		if(val >= 4 && val <= 7) return TRUE;
		else{
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of I4/I5/I6/I7.\n");
			return FALSE;
		}
	} else
		return FALSE;
}

/** 
* @brief Check if Mx Register, where x = 0, 1, 2, 3
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if Mx, 0 if not
*/
int isMx(sICode *p, char *s)
{
	int val;

	if(((s[0] == 'M') || (s[0] == 'm')) && isdigit(s[1])) {		/* Mx register */
		val = s[1] - '0';
		if(val >= 0 && val <= 3) return TRUE;
		else{
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of M0/M1/M2/M3.\n");
			return FALSE;
		}
	} else
		return FALSE;
}

/** 
* @brief Check if My Register, where y = 4, 5, 6, 7
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if My, 0 if not
*/
int isMy(sICode *p, char *s)
{
	int val;

	if(((s[0] == 'M') || (s[0] == 'm')) && isdigit(s[1])) {		/* My register */
		val = s[1] - '0';
		if(val >= 4 && val <= 7) return TRUE;
		else{
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of M4/M5/M6/M7.\n");
			return FALSE;
		}
	} else
		return FALSE;
}

/** 
* @brief Check if Mx Register
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if MREG, 0 if not
*/
int isMReg(sICode *p, char *s)
{
	if(((s[0] == 'M') || (s[0] == 'm')) && isdigit(s[1])){ 		/* M register */
		int	val = s[1] - '0';
		if(val >= 0 && val <= 7) return TRUE;
		else {
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of M0 ~ M7.\n");
			return FALSE;
		}
	}else
		return FALSE;
}

/** 
* @brief Check if Lx Register
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if LREG, 0 if not
*/
int isLReg(sICode *p, char *s)
{
	if(((s[0] == 'L') || (s[0] == 'l')) && isdigit(s[1])){ 		/* Lx register */
		int	val = s[1] - '0';
		if(val >= 0 && val <= 7) return TRUE;
		else {
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of L0 ~ L7.\n");
			return FALSE;
		}
	}else
		return FALSE;
}

/** 
* @brief Check if NONE (MAC NONE)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if NONE, 0 if not
*/
int isNONE(sICode *p, char *s)
{
	if(!strcasecmp("NONE", s)) 
		return TRUE; /* NONE */
	else
		return FALSE;
}

/** 
* @brief Check if REG12 (R0, R1, ..., R31)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if REG12, 0 if not
*/
int isReg12(sICode *p, char *s)
{
	int len;

	//if(!strcasecmp("NONE", s)) return TRUE; /* NONE */

	if((s[0] == 'R') || (s[0] == 'r')) {		/* Rx register */
		len = strlen(s);
		if(len == 2){			/* R0  ~ R9  */
			if(isdigit(s[1])) return TRUE;
			else return FALSE;
		}else if(len == 3){		/* R10 ~ R31 */
			if(isdigit(s[1]) && isdigit(s[2])) return TRUE;
			else return FALSE;
		}else{					/* error! */
			return FALSE;
		}
	}
	return FALSE;
}

/** 
* @brief Check if REG12S (R0, R1, ..., R7)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if REG12S, 0 if not
*/
int isReg12S(sICode *p, char *s)
{
	int len;
	int val;

	//if(!strcasecmp("NONE", s)) return TRUE; /* NONE */

	if((s[0] == 'R') || (s[0] == 'r')) {		/* Rx register */
		len = strlen(s);
		if(len == 2){			/* R0  ~ R7  */
			val = s[1] - '0';
			if(val >= 0 && val <= 7) return TRUE;
			else {
				printRunTimeError(p->LineCntr, s, 
					"Invalid Register name - not one of R0 ~ R7.\n");
				return FALSE;
			}
		}else{					/* error! */
			return FALSE;
		}
	}
	return FALSE;
}

/** 
* @brief Check if REG24 (R0, R2, ... R30)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if REG24, 0 if not
*/
int isReg24(sICode *p, char *s)
{
	int len;

	//if(!strcasecmp("NONE", s)) return TRUE; /* NONE */

	if((s[0] == 'R') || (s[0] == 'r')) {		/* Rx register */
		len = strlen(s);
		if(len == 2){			/* R0, R2, ..., R8  */
			if(isdigit(s[1])) {
				if(!((s[1] - '0') & 0x01)) return TRUE;
				else {
					printRunTimeError(p->LineCntr, s, 
						"Complex register pair should not end with an odd number.\n");
					return FALSE;
				}
			}else return FALSE;
		}else if(len == 3){		/* R10, R12, ..., R30 */
			if(isdigit(s[1]) && isdigit(s[2])) {
				if(!((s[2] - '0') & 0x01)) return TRUE;
				else {
					printRunTimeError(p->LineCntr, s, 
						"Complex register pair should not end with an odd number.\n");
					return FALSE;
				}
			} else return FALSE;
		}else{					/* error! */
			return FALSE;
		}
	}
	return FALSE;
}

/** 
* @brief Check if REG24S (R0, R2, R4, R6)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if REG24S, 0 if not
*/
int isReg24S(sICode *p, char *s)
{
	int len;
	int val;

	//if(!strcasecmp("NONE", s)) return TRUE; /* NONE */

	if((s[0] == 'R') || (s[0] == 'r')) {		/* Rx register */
		len = strlen(s);
		if(len == 2){			/* R0, R2, ..., R6  */
			if(isdigit(s[1])) {
				val = s[1] - '0';
				if(val > 7){
					printRunTimeError(p->LineCntr, s, 
						"Invalid Register name - not one of R0/R2/R4/R6.\n");
					return FALSE;
				}
				if(val == 0 || val == 2 || val == 4 || val == 6) return TRUE;
				if(val & 0x01) {
					printRunTimeError(p->LineCntr, s, 
						"Complex register pair should not end with an odd number.\n");
					return FALSE;
				}
			}else return FALSE;
		}else{					/* error! */
			return FALSE;
		}
	}
	return FALSE;
}

/** 
* @brief Check if DREG12 (REG12, ACC12)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if DREG12, 0 if not
*/
int isDReg12(sICode *p, char *s)
{
	if(s){
		if(isReg12(p, s) || isACC12(p, s)) return TRUE;
	}
	return FALSE;
}

/** 
* @brief Check if XOP12 (= REG12)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if XOP12, 0 if not
*/
int isXOP12(sICode *p, char *s)
{
	if(s){
		if(isReg12(p, s)) return TRUE;
	}
	return FALSE;
}

/** 
* @brief Check if DREG12S (REG12S, ACC12S)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if DREG12S, 0 if not
*/
int isDReg12S(sICode *p, char *s)
{
	if(s){
		if(isReg12S(p, s) || isACC12S(p, s)) return TRUE;
	}
	return FALSE;
}

/** 
* @brief Check if DREG24 (REG24, ACC24)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if DREG24, 0 if not
*/
int isDReg24(sICode *p, char *s)
{
	if(s){
		if(isReg24(p, s) || isACC24(p, s)) return TRUE;
	}
	return FALSE;
}

/** 
* @brief Check if XOP24 (= REG24)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if XOP24, 0 if not
*/
int isXOP24(sICode *p, char *s)
{
	if(s){
		if(isReg24(p, s)) return TRUE;
	}
	return FALSE;
}

/** 
* @brief Check if DREG24S (REG24S, ACC24S)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if DREG24S, 0 if not
*/
int isDReg24S(sICode *p, char *s)
{
	if(s){
		if(isReg24S(p, s) || isACC24S(p, s)) return TRUE;
	}
	return FALSE;
}

/** 
* @brief Check if ACC12 (ACCx.H, ACCx.M, ACCx.L, x = 0~7)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if ACC12, 0 if not
*/
int isACC12(sICode *p, char *s)
{
	if(!strcasecmp("NONE", s)) return TRUE; /* NONE */

	if(!strncasecmp(s, "ACC", 3) && (isdigit(s[3])) 
		&& (s[4] == '.')) {	
		int val = s[3] - '0';
		if(val > 7){
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of ACC0.* ~ ACC7.*.\n");
			return FALSE;
		}
		if(!strcasecmp(s+5, "H") || !strcasecmp(s+5, "M") || !strcasecmp(s+5, "L"))
			return TRUE;
		else{
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of ACC0.* ~ ACC7.*.\n");
			return FALSE;
		}
	}
	return FALSE;
}

/** 
* @brief Check if ACC12S (ACCx.L, x = 0~7)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if ACC12S, 0 if not
*/
int isACC12S(sICode *p, char *s)
{
	//if(!strcasecmp("NONE", s)) return TRUE; /* NONE */

	if(!strncasecmp(s, "ACC", 3) && (isdigit(s[3])) 
		&& (s[4] == '.')) {	
		int val = s[3] - '0';
		if(val > 7){
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of ACC0.L ~ ACC7.L.\n");
			return FALSE;
		}
		if(!strcasecmp(s+5, "L"))
			return TRUE;
		else{
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of ACC0.L ~ ACC7.L.\n");
			return FALSE;
		}
	}
	return FALSE;
}

/** 
* @brief Check if ACC24 (ACCx.H, ACCx.M, ACCx.L, x = 0,2,4,6)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if ACC24, 0 if not
*/
int isACC24(sICode *p, char *s)
{
	if(!strcasecmp("NONE", s)) return TRUE; /* NONE */

	if(!strncasecmp(s, "ACC", 3) && (isdigit(s[3])) 
		&& (s[4] == '.')) {	
		int val = s[3] - '0';
		if(!(val & 0x01)) {
			if(val > 7){
				printRunTimeError(p->LineCntr, s, 
					"Invalid Register name - not one of ACC0.*/ACC2.*/ACC4.*/ACC6.*.\n");
				return FALSE;
			}
			if(!strcasecmp(s+5, "H") || !strcasecmp(s+5, "M") 
				|| !strcasecmp(s+5, "L"))
				return TRUE;
			else{
				printRunTimeError(p->LineCntr, s, 
					"Invalid Register name - not one of ACC0.*/ACC2.*/ACC4.*/ACC6.*.\n");
				return FALSE;
			}
		} else {
				printRunTimeError(p->LineCntr, s, 
					"Complex register pair should not end with an odd number.\n");
				return FALSE;
		}
	}
	return FALSE;
}

/** 
* @brief Check if ACC24S (ACCx.L, x = 0,2,4,6)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if ACC24S, 0 if not
*/
int isACC24S(sICode *p, char *s)
{
	//if(!strcasecmp("NONE", s)) return TRUE; /* NONE */

	if(!strncasecmp(s, "ACC", 3) && (isdigit(s[3])) 
		&& (s[4] == '.')) {	
		int val = s[3] - '0';
		if(!(val & 0x01)) {
			if(val > 7){
				printRunTimeError(p->LineCntr, s, 
					"Invalid Register name - not one of ACC0.L/ACC2.L/ACC4.L/ACC6.L.\n");
				return FALSE;
			}
			if(!strcasecmp(s+5, "L"))
				return TRUE;
			else{
				printRunTimeError(p->LineCntr, s, 
					"Invalid Register name - not one of ACC0.L/ACC2.L/ACC4.L/ACC6.L.\n");
				return FALSE;
			}
		} else {
				printRunTimeError(p->LineCntr, s, 
					"Complex register pair should not end with an odd number.\n");
				return FALSE;
		}
	}
	return FALSE;
}

/** 
* @brief Check if ACC32 (ACC0~ACC7)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if ACC32, 0 if not
*/
int isACC32(sICode *p, char *s)
{
	if(!strcasecmp("NONE", s)) return TRUE; /* NONE for MAC */

	if(!strncasecmp(s, "ACC", 3) && (isdigit(s[3]))) {	/* Accumulator */
		int val = s[3] - '0';
		if(val > 7){
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of ACC0 ~ ACC7.\n");
			return FALSE;
		}else
			return TRUE;
	}
	return FALSE;
}

/** 
* @brief Check if ACC32S (ACC0, ACC1, ACC2, ACC3)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if ACC32S, 0 if not
*/
int isACC32S(sICode *p, char *s)
{
	int n;

	//if(!strcasecmp("NONE", s)) return TRUE; /* NONE */

	if(!strncasecmp(s, "ACC", 3) && (isdigit(s[3]))) {	/* Accumulator */
		n = s[3] - '0';
		if(n >= 0 && n <= 3) 
			return TRUE;
		else {
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of ACC0/1/2/3.\n");
			return FALSE;
		}
	}
	return FALSE;
}

/** 
* @brief Check if ACC64 (ACC0, ACC2, ACC4, ACC6)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if ACC64, 0 if not
*/
int isACC64(sICode *p, char *s)
{
	if(!strcasecmp("NONE", s)) return TRUE; /* NONE */

	if(!strncasecmp(s, "ACC", 3) && (isdigit(s[3]))) {	/* Accumulator */
		int val = s[3] - '0';
		if(val > 7){
				printRunTimeError(p->LineCntr, s, 
					"Invalid Register name - not one of ACC0/2/4/6.\n");
				return FALSE;
		}

		if(!(val & 0x01)) return TRUE;		/* check for even number */
		else {
				printRunTimeError(p->LineCntr, s, 
					"Complex register pair should not end with an odd number.\n");
				return FALSE;
		}
	}
	return FALSE;
}

/** 
* @brief Check if ACC64S (ACC0, ACC2, ACC4, ACC6)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if ACC64S, 0 if not
*/
int isACC64S(sICode *p, char *s)
{
	//if(!strcasecmp("NONE", s)) return TRUE; /* NONE */

	if(!strncasecmp(s, "ACC", 3) && (isdigit(s[3]))) {	/* Accumulator */
		int val = s[3] - '0';
		if(val > 7){
				printRunTimeError(p->LineCntr, s, 
					"Invalid Register name - not one of ACC0/2/4/6.\n");
				return FALSE;
		}
		if(!(val & 0x01)) return TRUE;		/* check for even number */
		else {
			printRunTimeError(p->LineCntr, s, 
				"Complex register pair should not end with an odd number.\n");
			return FALSE;
		}
	}
	return FALSE;
}

/** 
* @brief Check if system control register (e.g. CACTL)
* 
* @param s Pointer to register mnemonic string
* 
* @return 1 if system control register, 0 if not
*/
int isSysCtlReg(char *s)
{
	if(!strcasecmp(s, "CACTL"))
		return TRUE;
	else
		return FALSE;
}

/** 
* @brief Check if integer constant(immediate)
* 
* @param s Pointer to string
* 
* @return 1 if integer, 0 if not
*/
int isInt(char *s)
{
	sTab *sp;

	if(s != NULL){
		if((isdigit(s[0])) || (s[0] == '-')){
			return TRUE;
		}else{
			sp = sTabHashSearch(symTable, s);	/* get symbol table pointer */
			if(sp) return TRUE;
		}
	}

	return FALSE;
}

/** 
* @brief Check if this integer constant is in the specified range(-(2^(n-1)) <= x <= (2^(n-1))-1).
* 
* @param p Pointer to instruction
* @param htable Symbol table
* @param s Pointer to integer string
* @param n Number of bits (assumption: signed integer)
* 
* @return 1 if integer in the specified range, 0 if not
*/
int isIntSignedN(sICode *p, sTabList htable[], char *s, int n)
{
	int ln = p->LineCntr;
	//return(isIntNM(p, htable, s, -(1<<(n-1)), (1<<(n-1))-1));
	return(isIntNM(p, htable, s, -(1<<(n-1)), (1<<n)-1));
}


/** 
* @brief Check if this integer constant is in the specified range(0 <= x <= (2^n)-1).
* 
* @param p Pointer to instruction
* @param htable Symbol table
* @param s Pointer to integer string
* @param n Number of bits (assumption: unsigned integer)
* 
* @return 1 if integer in the specified range, 0 if not
*/
int isIntUnsignedN(sICode *p, sTabList htable[], char *s, int n)
{
	int ln = p->LineCntr;
	return(isIntNM(p, htable, s, 0, (1<<n)-1));
}


/** 
* @brief Check if this integer constant is in the specified range(n <= x <= m).
* 
* @param p Pointer to instruction
* @param htable Symbol table
* @param s Pointer to integer string
* @param n Lower bound value
* @param m Upper bound value
* 
* @return 1 if integer in the specified range, 0 if not
*/
int isIntNM(sICode *p, sTabList htable[], char *s, int n, int m)
{
	int ln = p->LineCntr;
	int pc_relative = FALSE;

	if(isInt(s)){
		int x = getIntSymAddr(p, htable, s);

		if(p->InstType == t11a || p->InstType == t10a || p->InstType == t10b){
			pc_relative = TRUE;
		}
		if(pc_relative)
			x -= p->PMA;
		
		if((x >= n) && (x <= m)){	/* ok */	
			return TRUE;
		}else{		/* overflow or underflow: give warning/error message */
			char msg[MAX_LINEBUF];
			sprintf(msg, "Integer constant out of range: check if %d <= %d(0x%X) <= %d.\n",
				n, x, x, m); 
			printRunTimeError(ln, s, msg);
			return FALSE;
		}
	}else
		return FALSE;
}


/** 
* @brief Check if this integer constant is in the specified range(n <= x <= m).
* 
* @param p Pointer to instruction
* @param x Integer value
* @param n Lower bound value
* @param m Upper bound value
* 
* @return 1 if integer in the specified range, 0 if not
*/
int isIntNumNM(sICode *p, int x, int n, int m)
{
	int ln = p->LineCntr;

	if((x >= n) && (x <= m)){	/* ok */	
		return TRUE;
	}else{		/* overflow or underflow: give warning/error message */
		char msg[MAX_LINEBUF];
		sprintf(msg, "Integer constant out of range: check if %d <= %d(0x%X) <= %d.\n",
			n, x, x, m); 
		printRunTimeError(ln, p->Operand[2], msg);
		return FALSE;
	}
}


/** 
* @brief Check if XREG12 (R24, R25, ..., R31)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if XREG12, 0 if not
*/
int isXReg12(sICode *p, char *s)
{
	int len;

	//if(!strcasecmp("NONE", s)) return TRUE; /* NONE */

	if((s[0] == 'R') || (s[0] == 'r')) {		/* Rx register */
		len = strlen(s);
		if(len == 2){			/* R0  ~ R9  */
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of R24, R25, ..., R31.\n");
			return FALSE;
		}else if(len == 3){		/* R10 ~ R31 */
			if(isdigit(s[1]) && isdigit(s[2])) {
				int val = (s[1] - '0') * 10 + (s[2] - '0');
				if(val >= 24 && val <= 31)
					return TRUE;
				else {
					printRunTimeError(p->LineCntr, s, 
						"Invalid Register name - not one of R24, R25, ..., R31.\n");
					return FALSE;
				}
			} else {
				printRunTimeError(p->LineCntr, s, 
					"Invalid Register name - not one of R24, R25, ..., R31.\n");
				return FALSE;
			}
		}else{					/* error! */
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of R24, R25, ..., R31.\n");
			return FALSE;
		}
	}
	return FALSE;
}


/** 
* @brief Check if XREG24 (R24, R26, R28, R30)
* 
* @param p Pointer to instruction
* @param s Pointer to register mnemonic string
* 
* @return 1 if XREG24, 0 if not
*/
int isXReg24(sICode *p, char *s)
{
	int len;

	if((s[0] == 'R') || (s[0] == 'r')) {		/* Rx register */
		len = strlen(s);
		if(len == 2){			/* R0, R2, ..., R8  */
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of R24/R26/R28/R30.\n");
			return FALSE;
		}else if(len == 3){		/* R10, R12, ..., R30 */
			if(isdigit(s[1]) && isdigit(s[2])) {
				int val = (s[1] - '0') * 10 + (s[2] - '0');
				if(val >= 24 && val <= 31){
					if(!((s[2] - '0') & 0x01)) return TRUE;
					else {
						printRunTimeError(p->LineCntr, s, 
							"Complex register pair should not end with an odd number.\n");
						return FALSE;
					}
				} else {
					printRunTimeError(p->LineCntr, s, 
						"Invalid Register name - not one of R24/R26/R28/R30.\n");
					return FALSE;
				}
			} else {
				printRunTimeError(p->LineCntr, s, 
					"Invalid Register name - not one of R24/R26/R28/R30.\n");
				return FALSE;
			}
		}else{					/* error! */
			printRunTimeError(p->LineCntr, s, 
				"Invalid Register name - not one of R24/R26/R28/R30.\n");
			return FALSE;
		}
	}
	return FALSE;
}

/** 
* @brief Check if valid IDN constant (2 or 4)
* 
* @param s Pointer to string
* 
* @return 1 if integer, 0 if not
*/
int isIDN(char *s)
{
	int ret = FALSE;

	if(isInt(s)){
		int imm4 = atoi(s);
		switch(imm4){
			case 2:
			case 4:
				ret = TRUE;
				break;
			default:
				printRunTimeError(lineno, s, 
					"Invalide number of active data-paths (2 or 4).\n");
				ret = FALSE;
				break;
		}
	}
	return ret;
}


/** 
* @brief Read register data (12-bit)
* 
* @param s Name of register to read
* 
* @return Register data (12-bit)
*/
int RdReg(char *s)
{
	int val;

	if(!strcasecmp(s, "NONE")){     /* NONE */
        val = 0;
    }else if(!strcasecmp(s, "_CNTR")){			/* _CNTR */
		val = rCNTR;
	}else if(!strcasecmp(s, "_PCSTACK")){		/* _PCSTACK */
		val = rPCSTACK;
	}else if(!strcasecmp(s, "_LPSTACK")){		/* _LPSTACK */
		val = rLPSTACK;
	}else if(!strcasecmp(s, "_LPEVER")){		/* _LPEVER */
		val = rLPEVER;
	}else if(!strcasecmp(s, "_MSTAT")){	/* _MSTAT */
		val = 0;
		val |= rMstat.MB; val <<= 1;	/* bit 7: different from ADSP-219x */
		val |= rMstat.SD; val <<= 1;	/* bit 6 */
		val |= rMstat.TI; val <<= 1;	/* bit 5 */
		val |= rMstat.MM; val <<= 1;	/* bit 4 */
		val |= rMstat.AS; val <<= 1;	/* bit 3 */
		val |= rMstat.OL; val <<= 1;	/* bit 2 */
		val |= rMstat.BR; val <<= 1;	/* bit 1 */
		val |= rMstat.SR; 				/* bit 0 */
	}else if(!strcasecmp(s, "_SSTAT")){	/* _SSTAT */
		val = 0;
		val |= rSstat.SOV; val <<= 1;	/* bit 7 */
		val |= rSstat.SSE; val <<= 1;	/* bit 6 */
		val |= rSstat.LSF; val <<= 1;	/* bit 5 */
		val |= rSstat.LSE; val <<= 1;	/* bit 4 */
		val <<= 1;	/* bit 3 */
		val |= rSstat.PCL; val <<= 1;	/* bit 2 */
		val |= rSstat.PCF; val <<= 1;	/* bit 1 */
		val |= rSstat.PCE; 				/* bit 0 */
	}else if(!strcasecmp(s, "_DSTAT0")){	/* _DSTAT0 */
		val = rDstat0;
	}else if(!strcasecmp(s, "_DSTAT1")){	/* _DSTAT1 */
		val = rDstat1;
	}else if(!strcasecmp(s, "_ICNTL")){	/* _ICNTL */
		val = 0;
		val <<= 1;	/* bit 7 */
		val <<= 1;	/* bit 6 */
		val |= rICNTL.GIE; val <<= 1;	/* bit 5 */
		val |= rICNTL.INE; val <<= 1;	/* bit 4 */
	}else if(!strcasecmp(s, "_IMASK")){  /* _IMASK */
        val = 0;
        for(int i = 0; i < 12; i++){
            val |= rIMASK.UserDef[11-i]; val <<= 1; /* bit 15-i */
        }
        val |= rIMASK.STACK; val <<= 1; /* bit 3 */
        val |= rIMASK.SSTEP; val <<= 1; /* bit 2 */
        val |= rIMASK.PWDN;  val <<= 1; /* bit 1 */
        val |= rIMASK.EMU;              /* bit 0 */
	}else if(!strcasecmp(s, "_IRPTL")){  /* _IRPTL */
        val = 0;
        for(int i = 0; i < 12; i++){
            val |= rIRPTL.UserDef[11-i]; val <<= 1; /* bit 15-i */
        }
        val |= rIRPTL.STACK; val <<= 1; /* bit 3 */
        val |= rIRPTL.SSTEP; val <<= 1; /* bit 2 */
        val |= rIRPTL.PWDN;  val <<= 1; /* bit 1 */
        val |= rIRPTL.EMU;              /* bit 0 */
	}else if(!strncasecmp(s, "_IVEC", 5)) {	/* _IVECx register */
		int rn = atoi(s+5);
		val = rIVEC[rn];
	}else if(s[0] == 'I' || s[0] == 'i') {		/* Ix register */
		int rn = atoi(s+1);
		val = rI[rn];
	}else if(s[0] == 'M' || s[0] == 'm') {		/* Mx register */
		int rn = atoi(s+1);
		val = rM[rn];
	}else if(s[0] == 'L' || s[0] == 'l') {		/* Lx register */
		int rn = atoi(s+1);
		val = rL[rn];
	}else if(s[0] == 'B' || s[0] == 'b') {		/* Bx register */
		int rn = atoi(s+1);
		val = rB[rn];
	}else{
		printRunTimeError(lineno, s, 
			"Cannot use RdReg() to read SIMD registers.\n");
	}
	return val;
}

/** 
* @brief Read SIMD register data (12-bit x NUMDP)
* 
* @param s Name of register to read
* 
* @return SIMD register data (12-bit x NUMDP)
*/
sint sRdReg(char *s)
{
/*************************************************************************/
//
// 1. When we should check rDPENA?  
//		- write attempt to SIMD registers (and flags too)
//		- read attempt: don't care
// 2. When we should check rDPMST?
//		- write attempt to non-SIMD registers
//		- read attempt: don't care
//
/*************************************************************************/
	int val;
	sint sval;

	if(!strcasecmp(s, "NONE")){     /* NONE */
        val = 0;

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = val;
    }else if(!strcasecmp(s, "_CNTR")){			/* _CNTR */
		val = rCNTR;

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = val;
	}else if(!strcasecmp(s, "_PCSTACK")){		/* _PCSTACK */
		val = rPCSTACK;

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = val;
	}else if(!strcasecmp(s, "_LPSTACK")){		/* _LPSTACK */
		val = rLPSTACK;

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = val;
	}else if(!strcasecmp(s, "_LPEVER")){		/* _LPEVER */
		val = rLPEVER;

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = val;
	}else if(!strcasecmp(s, "ASTAT.R")){	/* ASTAT.R */
		for(int j = 0; j < NUMDP; j++) {
			val = 0;
			val |= rAstatR.UM.dp[j]; val <<= 1;	/* bit 9 */
			val |= rAstatR.SV.dp[j]; val <<= 1;	/* bit 8 */
			val |= rAstatR.SS.dp[j]; val <<= 1;	/* bit 7 */
			val |= rAstatR.MV.dp[j]; val <<= 1;	/* bit 6 */
			val |= rAstatR.AQ.dp[j]; val <<= 1;	/* bit 5 */
			val |= rAstatR.AS.dp[j]; val <<= 1;	/* bit 4 */
			val |= rAstatR.AC.dp[j]; val <<= 1;	/* bit 3 */
			val |= rAstatR.AV.dp[j]; val <<= 1;	/* bit 2 */
			val |= rAstatR.AN.dp[j]; val <<= 1;	/* bit 1 */
			val |= rAstatR.AZ.dp[j]; 			/* bit 0 */
			sval.dp[j] = val;
		}
	}else if(!strcasecmp(s, "ASTAT.I")){	/* ASTAT.I */
		for(int j = 0; j < NUMDP; j++) {
			val = 0;
			val |= rAstatI.UM.dp[j]; val <<= 1;	/* bit 9 */
			val |= rAstatI.SV.dp[j]; val <<= 1;	/* bit 8 */
			val |= rAstatI.SS.dp[j]; val <<= 1;	/* bit 7 */
			val |= rAstatI.MV.dp[j]; val <<= 1;	/* bit 6 */
			val |= rAstatI.AQ.dp[j]; val <<= 1;	/* bit 5 */
			val |= rAstatI.AS.dp[j]; val <<= 1;	/* bit 4 */
			val |= rAstatI.AC.dp[j]; val <<= 1;	/* bit 3 */
			val |= rAstatI.AV.dp[j]; val <<= 1;	/* bit 2 */
			val |= rAstatI.AN.dp[j]; val <<= 1;	/* bit 1 */
			val |= rAstatI.AZ.dp[j];			/* bit 0 */
			sval.dp[j] = val;
		}
	}else if(!strcasecmp(s, "ASTAT.C")){	/* ASTAT.C */
		for(int j = 0; j < NUMDP; j++) {
			val = 0;
			val |= rAstatC.UM.dp[j]; val <<= 1;	/* bit 9 */
			val |= rAstatC.SV.dp[j]; val <<= 1;	/* bit 8 */
			val |= rAstatC.SS.dp[j]; val <<= 1;	/* bit 7 */
			val |= rAstatC.MV.dp[j]; val <<= 1;	/* bit 6 */
			val |= rAstatC.AQ.dp[j]; val <<= 1;	/* bit 5 */
			val |= rAstatC.AS.dp[j]; val <<= 1;	/* bit 4 */
			val |= rAstatC.AC.dp[j]; val <<= 1;	/* bit 3 */
			val |= rAstatC.AV.dp[j]; val <<= 1;	/* bit 2 */
			val |= rAstatC.AN.dp[j]; val <<= 1;	/* bit 1 */
			val |= rAstatC.AZ.dp[j];			/* bit 0 */
			sval.dp[j] = val;
		}
	}else if(!strcasecmp(s, "_MSTAT")){	/* _MSTAT */
		val = 0;
		val |= rMstat.MB; val <<= 1;	/* bit 7: different from ADSP-219x */
		val |= rMstat.SD; val <<= 1;	/* bit 6 */
		val |= rMstat.TI; val <<= 1;	/* bit 5 */
		val |= rMstat.MM; val <<= 1;	/* bit 4 */
		val |= rMstat.AS; val <<= 1;	/* bit 3 */
		val |= rMstat.OL; val <<= 1;	/* bit 2 */
		val |= rMstat.BR; val <<= 1;	/* bit 1 */
		val |= rMstat.SR; 				/* bit 0 */

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = val;
	}else if(!strcasecmp(s, "_SSTAT")){	/* _SSTAT */
		val = 0;
		val |= rSstat.SOV; val <<= 1;	/* bit 7 */
		val |= rSstat.SSE; val <<= 1;	/* bit 6 */
		val |= rSstat.LSF; val <<= 1;	/* bit 5 */
		val |= rSstat.LSE; val <<= 1;	/* bit 4 */
		val <<= 1;	/* bit 3 */
		val |= rSstat.PCL; val <<= 1;	/* bit 2 */
		val |= rSstat.PCF; val <<= 1;	/* bit 1 */
		val |= rSstat.PCE; 				/* bit 0 */

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = val;
	}else if(!strcasecmp(s, "_DSTAT0")){	/* _DSTAT0 */
		val = rDstat0;

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = val;
	}else if(!strcasecmp(s, "_DSTAT1")){	/* _DSTAT1 */
		val = rDstat1;

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = val;
	}else if(!strcasecmp(s, "UMCOUNT")){	/* UMCOUNT */
		//val = rUMCOUNT;

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = rUMCOUNT.dp[j];
	}else if(!strcasecmp(s, "DID")){			/* DID */
		//val = rDID;

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = rDID.dp[j];
	}else if(!strcasecmp(s, "_ICNTL")){	/* _ICNTL */
		val = 0;
		val <<= 1;	/* bit 7 */
		val <<= 1;	/* bit 6 */
		val |= rICNTL.GIE; val <<= 1;	/* bit 5 */
		val |= rICNTL.INE; val <<= 1;	/* bit 4 */

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = val;
	}else if(!strcasecmp(s, "_IMASK")){  /* _IMASK */
        val = 0;
        for(int i = 0; i < 12; i++){
            val |= rIMASK.UserDef[11-i]; val <<= 1; /* bit 15-i */
        }
        val |= rIMASK.STACK; val <<= 1; /* bit 3 */
        val |= rIMASK.SSTEP; val <<= 1; /* bit 2 */
        val |= rIMASK.PWDN;  val <<= 1; /* bit 1 */
        val |= rIMASK.EMU;              /* bit 0 */

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = val;
	}else if(!strcasecmp(s, "_IRPTL")){  /* _IRPTL */
        val = 0;
        for(int i = 0; i < 12; i++){
            val |= rIRPTL.UserDef[11-i]; val <<= 1; /* bit 15-i */
        }
        val |= rIRPTL.STACK; val <<= 1; /* bit 3 */
        val |= rIRPTL.SSTEP; val <<= 1; /* bit 2 */
        val |= rIRPTL.PWDN;  val <<= 1; /* bit 1 */
        val |= rIRPTL.EMU;              /* bit 0 */

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = val;
	}else if(!strncasecmp(s, "_IVEC", 5)) {	/* _IVECx register */
		int rn = atoi(s+5);
		val = rIVEC[rn];

		for(int j = 0; j < NUMDP; j++) 
			sval.dp[j] = val;
	}else if(s[0] == 'R' || s[0] == 'r') {		/* Rx register */
		for(int j = 0; j < NUMDP; j++) {
			val = rR[atoi(s+1)].dp[j];
			if(isNeg12b(val))	val |= 0xFFFFF000;

			sval.dp[j] = val;
		}
	}else if(!strncasecmp(s, "ACC", 3)) {	/* Accumulator */
		int x = (int)(s[3] - '0');
		if(s[4] == '.'){	/* ACCx.H, M, L */
			switch(s[5]){
				case 'H':
				case 'h':
					for(int j = 0; j < NUMDP; j++) {
						val = rAcc[x].H.dp[j];
						if(isNeg8b(val))	val |= 0xFFFFFF00;
						sval.dp[j] = val;
					}
					break;
				case 'M':
				case 'm':
					for(int j = 0; j < NUMDP; j++) {
						val = rAcc[x].M.dp[j];
						if(isNeg12b(val))	val |= 0xFFFFF000;
						sval.dp[j] = val;
					}
					break;
				case 'L':
				case 'l':
					for(int j = 0; j < NUMDP; j++) {
						val = rAcc[x].L.dp[j];
						if(isNeg12b(val))	val |= 0xFFFFF000;
						sval.dp[j] = val;
					}
					break;
				default:		/* must not happen */
					for(int j = 0; j < NUMDP; j++) {
						val = 0;
						sval.dp[j] = val;
					}
					break;	
			}
		}else{				/* ACCx */
			for(int j = 0; j < NUMDP; j++) {
				val = rAcc[x].value.dp[j];	
				sval.dp[j] = val;
			}
		}

	}else if(s[0] == 'I' || s[0] == 'i') {		/* Ix register */
		int rn = atoi(s+1);
		val = rI[rn];

		for(int j = 0; j < NUMDP; j++) {
			sval.dp[j] = val;
		}
	}else if(s[0] == 'M' || s[0] == 'm') {		/* Mx register */
		int rn = atoi(s+1);
		val = rM[rn];

		for(int j = 0; j < NUMDP; j++) {
			sval.dp[j] = val;
		}
	}else if(s[0] == 'L' || s[0] == 'l') {		/* Lx register */
		int rn = atoi(s+1);
		val = rL[rn];

		for(int j = 0; j < NUMDP; j++) {
			sval.dp[j] = val;
		}
	}else if(s[0] == 'B' || s[0] == 'b') {		/* Bx register */
		int rn = atoi(s+1);
		val = rB[rn];

		for(int j = 0; j < NUMDP; j++) {
			sval.dp[j] = val;
		}
	}else{
		printRunTimeError(lineno, s, 
            "Parse Error: sRdReg() - Please report.\n");
	}										/* other special registers should be added here */
	return sval;
}

/** 
* @brief Read Ix/Mx/Lx/Mx register data (16-bit) considering latency restriction
* 
* @param p Pointer to instruction
* @param s Name of register to read
* 
* @return Register data (16-bit)
*/
int RdReg2(sICode *p, char *s)
{
	int val;

	if(!strcasecmp(s, "_MSTAT")){	/* _MSTAT */
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

			if(p){
				printRunTimeWarning(p->LineCntr, s, 
					"Backup value of this register used due to load-use latency restriction.\n");
			}

			if(VerboseMode) printf("RdReg2(): %s: backup value 0x%04X used due to latency restriction.\n", s, val);
			if(VerboseMode) printf("          Cycles: %d, LastAccess: %d\n", ::Cycles, ::LastAccessMstat);
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
	}else if(!strcasecmp(s, "_CNTR")){	/* _CNTR */
		if((::Cycles - LastAccessCNTR) <= 1){		/* restriction: if too close, use old value */
			val = rbCNTR;

			if(p){
				printRunTimeWarning(p->LineCntr, s, 
					"Backup value of this register used due to load-use latency restriction.\n");
			}

			if(VerboseMode) printf("RdReg2(): %s: backup value 0x%04X used due to latency restriction.\n", s, val);
			if(VerboseMode) printf("          Cycles: %d, LastAccess: %d\n", ::Cycles, ::LastAccessMstat);
		}else{
			val = rCNTR;
		}
	}else if(s[0] == 'I' || s[0] == 'i') {		/* Ix register */
		int rn = atoi(s+1);

		if((::Cycles - LastAccessIx[rn]) <= 1){				/* case: <= 1 - if too close, use old value */
			int diff_rbI0 = ::Cycles - LAbIx[rn][0];
			int diff_rbI1 = ::Cycles - LAbIx[rn][1];

			if(diff_rbI0 == 2){
				/* example:
				* [01]  LD  I0, 0x1000		; -> rbI[][1]**
				* [02]  LD  I0, 0x2000		; -> rbI[][0]
				* [03]  LD  I0, 0x3000		; -> rI[]
				* [04]  ST  DM(I0+M0), 1	; ** diff rI[] == 1, diff rbI[0] == 2, diff rbI[1] == 3
				*/
				val = rbI[rn][1];
			}else{
				/* example 1:
				* [01]  LD  I0, 0x1000		; -> rbI[][0]**
				* [02]  NOP
				* [03]  LD  I0, 0x3000		; -> rI[]
				* [04]  ST  DM(I0+M0), 1	; ** diff rI[] == 1, diff rbI[0] == 3, diff rbI[1] == >3
				*/
				/* example 2:
				* [01]  NOP
				* [02]  LD  I0, 0x2000		; -> rbI[][0]**
				* [03]  LD  I0, 0x3000		; -> rI[]
				* [04]  ST  DM(I0+M0), 1	; ** diff rI[] == 1, diff rbI[0] == 2, diff rbI[1] == >3
				*/
				val = rbI[rn][0];
			}

			if(p){
				printRunTimeWarning(p->LineCntr, s, 
					"Backup value of this register used due to load-use latency restriction.\n");
			}

			if(VerboseMode) printf("RdReg2(): %s: backup value 0x%04X used due to latency restriction.\n", s, val);
			if(VerboseMode) printf("          Cycles: %d, LastAccess: %d\n", ::Cycles, ::LastAccessIx[rn]);
		}else if((::Cycles - LastAccessIx[rn]) == 2){		/* case: 2 - if too close, use old value */
				/* example:
				* [01]  LD  I0, 0x1000		; -> rbI[][0]**
				* [02]  LD  I0, 0x2000		; -> rI[]
				* [03]  NOP
				* [04]  ST  DM(I0+M0), 1	; ** diff rI[] == 2, diff rbI[0] == 3, diff rbI[1] == >3
				*/
			val = rbI[rn][0];

			if(p){
				printRunTimeWarning(p->LineCntr, s, 
					"Backup value of this register used due to load-use latency restriction.\n");
			}

			if(VerboseMode) printf("RdReg2(): %s: backup value 0x%04X used due to latency restriction.\n", s, val);
			if(VerboseMode) printf("          Cycles: %d, LastAccess: %d\n", ::Cycles, ::LastAccessIx[rn]);
		}else{													/* case: >= 3 */
			val = rI[rn];
		}
	}else if(s[0] == 'M' || s[0] == 'm') {		/* Mx register */
		int rn = atoi(s+1);

		if((::Cycles - LastAccessMx[rn]) <= 1){				/* case: <= 1 - if too close, use old value */
			int diff_rbM0 = ::Cycles - LAbMx[rn][0];
			int diff_rbM1 = ::Cycles - LAbMx[rn][1];

			if(diff_rbM0 == 2){
				val = rbM[rn][1];
			}else{
				val = rbM[rn][0];
			}

			if(p){
				printRunTimeWarning(p->LineCntr, s, 
					"Backup value of this register used due to load-use latency restriction.\n");
			}

			if(VerboseMode) printf("RdReg2(): %s: backup value 0x%04X used due to latency restriction.\n", s, val);
			if(VerboseMode) printf("          Cycles: %d, LastAccess: %d\n", ::Cycles, ::LastAccessMx[rn]);
		}else if((::Cycles - LastAccessMx[rn]) == 2){		/* case: 2 - if too close, use old value */
			val = rbM[rn][0];

			if(p){
				printRunTimeWarning(p->LineCntr, s, 
					"Backup value of this register used due to load-use latency restriction.\n");
			}

			if(VerboseMode) printf("RdReg2(): %s: backup value 0x%04X used due to latency restriction.\n", s, val);
			if(VerboseMode) printf("          Cycles: %d, LastAccess: %d\n", ::Cycles, ::LastAccessMx[rn]);
		}else{													/* case: >= 3 */
			val = rM[rn];
		}
	}else if(s[0] == 'L' || s[0] == 'l') {		/* Lx register */
		int rn = atoi(s+1);

		if((::Cycles - LastAccessLx[rn]) <= 1){				/* case: <= 1 - if too close, use old value */
			int diff_rbL0 = ::Cycles - LAbLx[rn][0];
			int diff_rbL1 = ::Cycles - LAbLx[rn][1];

			if(diff_rbL0 == 2){
				val = rbL[rn][1];
			}else{
				val = rbL[rn][0];
			}

			if(p){
				printRunTimeWarning(p->LineCntr, s, 
					"Backup value of this register used due to load-use latency restriction.\n");
			}

			if(VerboseMode) printf("RdReg2(): %s: backup value 0x%04X used due to latency restriction.\n", s, val);
			if(VerboseMode) printf("          Cycles: %d, LastAccess: %d\n", ::Cycles, ::LastAccessLx[rn]);
		}else if((::Cycles - LastAccessLx[rn]) == 2){		/* case: 2 - if too close, use old value */
			val = rbL[rn][0];

			if(p){
				printRunTimeWarning(p->LineCntr, s, 
					"Backup value of this register used due to load-use latency restriction.\n");
			}

			if(VerboseMode) printf("RdReg2(): %s: backup value 0x%04X used due to latency restriction.\n", s, val);
			if(VerboseMode) printf("          Cycles: %d, LastAccess: %d\n", ::Cycles, ::LastAccessLx[rn]);
		}else{													/* case: >= 3 */
			val = rL[rn];
		}
	}else if(s[0] == 'B' || s[0] == 'b') {		/* Bx register */
		int rn = atoi(s+1);

		if((::Cycles - LastAccessBx[rn]) <= 1){				/* case: <= 1 - if too close, use old value */
			int diff_rbB0 = ::Cycles - LAbBx[rn][0];
			int diff_rbB1 = ::Cycles - LAbBx[rn][1];

			if(diff_rbB0 == 2){
				val = rbB[rn][1];
			}else{
				val = rbB[rn][0];
			}

			if(p){
				printRunTimeWarning(p->LineCntr, s, 
					"Backup value of this register used due to load-use latency restriction.\n");
			}

			if(VerboseMode) printf("RdReg2(): %s: backup value 0x%04X used due to latency restriction.\n", s, val);
			if(VerboseMode) printf("          Cycles: %d, LastAccess: %d\n", ::Cycles, ::LastAccessBx[rn]);
		}else if((::Cycles - LastAccessBx[rn]) == 2){		/* case: 2 - if too close, use old value */
			val = rbB[rn][0];

			if(p){
				printRunTimeWarning(p->LineCntr, s, 
					"Backup value of this register used due to load-use latency restriction.\n");
			}

			if(VerboseMode) printf("RdReg2(): %s: backup value 0x%04X used due to latency restriction.\n", s, val);
			if(VerboseMode) printf("          Cycles: %d, LastAccess: %d\n", ::Cycles, ::LastAccessBx[rn]);
		}else{													/* case: >= 3 */
			val = rB[rn];
		}
	}else{
		if(p){
			printRunTimeError(p->LineCntr, s, 
           		"Parse Error: RdReg2() - Please report.\n");
		}
	}										/* other special registers should be added here */
	return val;
}


/** 
* @brief Read SIMD register data (12-bit x NUMDP) from other SIMD data path.
* 
* @param s Name of register to read
* @param id_offset Relative offset of data-path ID 
* @param active_dp Number of active data paths
* 
* @return SIMD register data (12-bit x NUMDP)
*/
sint sRdXReg(char *s, int id_offset, int active_dp)
{
/*************************************************************************/
//
// 1. When we should check rDPENA?  
//		- write attempt to SIMD registers (and flags too)
//		- read attempt: don't care
// 2. When we should check rDPMST?
//		- write attempt to non-SIMD registers
//		- read attempt: don't care
//
/*************************************************************************/
	int val;
	sint sval = { 0, 0, 0, 0 };
	int reg = atoi(s+1);

	if(s[0] == 'R' || s[0] == 'r') {		/* Rx register */
		if(reg < REG_X_LO || reg > REG_X_HI){
			printRunTimeError(lineno, s, 
				"Source register operand is out of cross-path register window.\n");
		}
		if(id_offset < 0 || id_offset >= NUMDP){
			printRunTimeError(lineno, s, 
				"Invalid data path ID offset.\n");
		}
		if(active_dp != 2 && active_dp != 4){
			printRunTimeError(lineno, s, 
				"Invalide number of active data-paths (2 or 4).\n");
		}

		for(int j = 0; j < NUMDP; j++) {
			val = rR[reg].dp[(j+id_offset)%active_dp];
			if(isNeg12b(val))	val |= 0xFFFFF000;

			sval.dp[j] = val;
		}
	}else{
		printRunTimeError(lineno, s, 
            "Parse Error: sRdXReg() - Please report.\n");
	}										/* other special registers should be added here */
	return sval;
}

/** 
* @brief Write data to the specified register (12-bit) 
* 
* @param s Name of register to write
* @param data Data to write (12-bit)
*/
void WrReg(char *s, int data)
{
	if(!strcasecmp(s, "NONE")){     /* NONE */
        ;   /* do nothing */
    }else if(!strcasecmp(s, "_CNTR")){			/* _CNTR */
		/* back up values */
		rbCNTR = rCNTR;

		/* record when it's written: 2010.07.20 */
		LastAccessCNTR = ::Cycles;	

		/* update */
		rCNTR = data;
	}else if(!strcasecmp(s, "_PCSTACK")){		/* _PCSTACK */
		rPCSTACK = 0xFFFF & data;
	}else if(!strcasecmp(s, "_LPSTACK")){		/* _LPSTACK */
		rLPSTACK = 0xFFFF & data;
	}else if(!strcasecmp(s, "_LPEVER")){		/* _LPEVER */
		rLPEVER = 0xFFFF & data;
	}else if(!strcasecmp(s, "_MSTAT")){			/* _MSTAT */
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
	}else if(!strcasecmp(s, "_SSTAT")){			/* _SSTAT */
		rSstat.PCE = (data & 0x0001)? 1: 0;		/* bit 0 */
		rSstat.PCF = (data & 0x0002)? 1: 0;		/* bit 1 */
		rSstat.PCL = (data & 0x0004)? 1: 0;		/* bit 2 */
		rSstat.LSE = (data & 0x0010)? 1: 0;		/* bit 4 */
		rSstat.LSF = (data & 0x0020)? 1: 0;		/* bit 5 */
		rSstat.SSE = (data & 0x0040)? 1: 0;		/* bit 6 */
		rSstat.SOV = (data & 0x0080)? 1: 0;		/* bit 7: different from ADSP-219x */
	}else if(!strcasecmp(s, "_DSTAT0")){	/* _DSTAT0 */
		rDstat0 = 0x000F & data;

		if(0x1 & data) rDPENA.dp[0] = 1;
		else rDPENA.dp[0] = 0;

		if(0x2 & data) rDPENA.dp[1] = 1;
		else rDPENA.dp[1] = 0;

		if(0x4 & data) rDPENA.dp[2] = 1;
		else rDPENA.dp[2] = 0;

		if(0x8 & data) rDPENA.dp[3] = 1;
		else rDPENA.dp[3] = 0;
	}else if(!strcasecmp(s, "_DSTAT1")){	/* _DSTAT1 */
		rDstat1 = 0x000F & data;
		if(0x1 & data) {
			rDPMST.dp[0] = 1;
			rDPMST.dp[1] = rDPMST.dp[2] = rDPMST.dp[3] = 0;
		}else if(0x2 & data) {
			rDPMST.dp[1] = 1;
			rDPMST.dp[0] = rDPMST.dp[2] = rDPMST.dp[3] = 0;
		}else if(0x4 & data) {
			rDPMST.dp[2] = 1;
			rDPMST.dp[0] = rDPMST.dp[1] = rDPMST.dp[3] = 0;
		}else if(0x8 & data) {
			rDPMST.dp[3] = 1;
			rDPMST.dp[0] = rDPMST.dp[1] = rDPMST.dp[2] = 0;
		}
	}else if(!strcasecmp(s, "_ICNTL")){			/* _ICNTL */
		rICNTL.INE = (data & 0x0010)? 1: 0;		/* bit 4 */
		rICNTL.GIE = (data & 0x0020)? 1: 0;		/* bit 5 */
	}else if(!strcasecmp(s, "_IMASK")){          /* _IMASK */
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
	}else if(!strcasecmp(s, "_IRPTL")){          /* _IRPTL */
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
	}else if(!strncasecmp(s, "_IVEC", 5)) {	/* _IVECx register */
		rIVEC[atoi(s+5)] = data;
	}else if(s[0] == 'I' || s[0] == 'i') {		/* Ix register */
		int rn = atoi(s+1);

		/* back up values */
		rbI[rn][1] = rbI[rn][0];
		rbI[rn][0] = rI[rn];

		/* record when it's written: 2010.07.21 */
		LAbIx[rn][1] =  LAbIx[rn][0];
		LAbIx[rn][0] = LastAccessIx[rn];
		LastAccessIx[rn] = ::Cycles;	

		/* update */
		rI[rn] = data;
	}else if(s[0] == 'M' || s[0] == 'm') {		/* Mx register */
		int rn = atoi(s+1);

		/* back up values */
		rbM[rn][1] = rbM[rn][0];
		rbM[rn][0] = rM[rn];

		/* record when it's written: 2010.07.21 */
		LAbMx[rn][1] =  LAbMx[rn][0];
		LAbMx[rn][0] = LastAccessMx[rn];
		LastAccessMx[rn] = ::Cycles;	

		/* update */
		rM[rn] = data;
	}else if(s[0] == 'L' || s[0] == 'l') {		/* Lx register */
		int rn = atoi(s+1);

		/* back up values */
		rbL[rn][1] = rbL[rn][0];
		rbL[rn][0] = rL[rn];

		/* record when it's written: 2010.07.21 */
		LAbLx[rn][1] =  LAbLx[rn][0];
		LAbLx[rn][0] = LastAccessLx[rn];
		LastAccessLx[rn] = ::Cycles;	

		/* update */
		rL[rn] = data;
	}else if(s[0] == 'B' || s[0] == 'b') {		/* Bx register */
		int rn = atoi(s+1);

		/* back up values */
		rbB[rn][1] = rbB[rn][0];
		rbB[rn][0] = rB[rn];

		/* record when it's written: 2010.07.21 */
		LAbBx[rn][1] =  LAbBx[rn][0];
		LAbBx[rn][0] = LastAccessBx[rn];
		LastAccessBx[rn] = ::Cycles;	

		/* update */
		rB[rn] = data;
	}else{
		printRunTimeError(lineno, s, 
			"Cannot use WrReg() to write SIMD registers.\n");
	}
}

/** 
* @brief Write SIMD data to the specified register (12-bit x NUMDP) 
* 
* @param s Name of register to write
* @param data SIMD data to write (12-bit x NUMDP)
* @param mask conditional execution mask
*/
void sWrReg(char *s, sint data, sint mask)
{
	if(!strcasecmp(s, "NONE")){     /* NONE */
        return;   /* do nothing */
    }else if(!strcasecmp(s, "_CNTR")){			/* _CNTR */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				/* back up values */
				rbCNTR = rCNTR;

				/* record when it's written: 2010.07.20 */
				LastAccessCNTR = ::Cycles;	

				/* update */
				rCNTR = data.dp[j];
				break;
			}
		}
	}else if(!strcasecmp(s, "_PCSTACK")){		/* _PCSTACK */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				rPCSTACK = 0xFFFF & data.dp[j];
				break;
			}
		}
	}else if(!strcasecmp(s, "_LPSTACK")){		/* _LPSTACK */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				rLPSTACK = 0xFFFF & data.dp[j];
				break;
			}
		}
	}else if(!strcasecmp(s, "_LPEVER")){		/* _LPEVER */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				rLPEVER = 0xFFFF & data.dp[j];
				break;
			}
		}
	}else if(!strcasecmp(s, "ASTAT.R")){		/* ASTAT.R */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPENA.dp[j] && mask.dp[j]){
				rAstatR.AZ.dp[j] = (data.dp[j] & 0x0001)? 1: 0;		/* bit 0 */
				rAstatR.AN.dp[j] = (data.dp[j] & 0x0002)? 1: 0;		/* bit 1 */
				rAstatR.AV.dp[j] = (data.dp[j] & 0x0004)? 1: 0;		/* bit 2 */
				rAstatR.AC.dp[j] = (data.dp[j] & 0x0008)? 1: 0;		/* bit 3 */
				rAstatR.AS.dp[j] = (data.dp[j] & 0x0010)? 1: 0;		/* bit 4 */
				rAstatR.AQ.dp[j] = (data.dp[j] & 0x0020)? 1: 0;		/* bit 5 */
				rAstatR.MV.dp[j] = (data.dp[j] & 0x0040)? 1: 0;		/* bit 6 */
				rAstatR.SS.dp[j] = (data.dp[j] & 0x0080)? 1: 0;		/* bit 7 */
				rAstatR.SV.dp[j] = (data.dp[j] & 0x0100)? 1: 0;		/* bit 8 */
				rAstatR.UM.dp[j] = (data.dp[j] & 0x0200)? 1: 0;		/* bit 9 */
			}
		}
	}else if(!strcasecmp(s, "ASTAT.I")){		/* ASTAT.I */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPENA.dp[j] && mask.dp[j]){
				rAstatI.AZ.dp[j] = (data.dp[j] & 0x0001)? 1: 0;		/* bit 0 */
				rAstatI.AN.dp[j] = (data.dp[j] & 0x0002)? 1: 0;		/* bit 1 */
				rAstatI.AV.dp[j] = (data.dp[j] & 0x0004)? 1: 0;		/* bit 2 */
				rAstatI.AC.dp[j] = (data.dp[j] & 0x0008)? 1: 0;		/* bit 3 */
				rAstatI.AS.dp[j] = (data.dp[j] & 0x0010)? 1: 0;		/* bit 4 */
				rAstatI.AQ.dp[j] = (data.dp[j] & 0x0020)? 1: 0;		/* bit 5 */
				rAstatI.MV.dp[j] = (data.dp[j] & 0x0040)? 1: 0;		/* bit 6 */
				rAstatI.SS.dp[j] = (data.dp[j] & 0x0080)? 1: 0;		/* bit 7 */
				rAstatI.SV.dp[j] = (data.dp[j] & 0x0100)? 1: 0;		/* bit 8 */
				rAstatI.UM.dp[j] = (data.dp[j] & 0x0200)? 1: 0;		/* bit 9 */
			}
		}
	}else if(!strcasecmp(s, "ASTAT.C")){		/* ASTAT.C */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPENA.dp[j] && mask.dp[j]){
				rAstatC.AZ.dp[j] = (data.dp[j] & 0x0001)? 1: 0;		/* bit 0 */
				rAstatC.AN.dp[j] = (data.dp[j] & 0x0002)? 1: 0;		/* bit 1 */
				rAstatC.AV.dp[j] = (data.dp[j] & 0x0004)? 1: 0;		/* bit 2 */
				rAstatC.AC.dp[j] = (data.dp[j] & 0x0008)? 1: 0;		/* bit 3 */
				rAstatC.AS.dp[j] = (data.dp[j] & 0x0010)? 1: 0;		/* bit 4 */
				rAstatC.AQ.dp[j] = (data.dp[j] & 0x0020)? 1: 0;		/* bit 5 */
				rAstatC.MV.dp[j] = (data.dp[j] & 0x0040)? 1: 0;		/* bit 6 */
				rAstatC.SS.dp[j] = (data.dp[j] & 0x0080)? 1: 0;		/* bit 7 */
				rAstatC.SV.dp[j] = (data.dp[j] & 0x0100)? 1: 0;		/* bit 8 */
				rAstatC.UM.dp[j] = (data.dp[j] & 0x0200)? 1: 0;		/* bit 9 */
			}
		}
	}else if(!strcasecmp(s, "_MSTAT")){			/* _MSTAT */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				/* back up values */
				rbMstat = rMstat;

				/* record when it's written: 2009.07.08 */
				LastAccessMstat = ::Cycles;	

				/* update */
				rMstat.SR = (data.dp[j] & 0x0001)? 1: 0;		/* bit 0 */
				rMstat.BR = (data.dp[j] & 0x0002)? 1: 0;		/* bit 1 */
				rMstat.OL = (data.dp[j] & 0x0004)? 1: 0;		/* bit 2 */
				rMstat.AS = (data.dp[j] & 0x0008)? 1: 0;		/* bit 3 */
				rMstat.MM = (data.dp[j] & 0x0010)? 1: 0;		/* bit 4 */
				rMstat.TI = (data.dp[j] & 0x0020)? 1: 0;		/* bit 5 */
				rMstat.SD = (data.dp[j] & 0x0040)? 1: 0;		/* bit 6 */
				rMstat.MB = (data.dp[j] & 0x0080)? 1: 0;		/* bit 7: different from ADSP-219x */
				break;
			}
		}
	}else if(!strcasecmp(s, "_SSTAT")){			/* _SSTAT */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				rSstat.PCE = (data.dp[j] & 0x0001)? 1: 0;		/* bit 0 */
				rSstat.PCF = (data.dp[j] & 0x0002)? 1: 0;		/* bit 1 */
				rSstat.PCL = (data.dp[j] & 0x0004)? 1: 0;		/* bit 2 */
				rSstat.LSE = (data.dp[j] & 0x0010)? 1: 0;		/* bit 4 */
				rSstat.LSF = (data.dp[j] & 0x0020)? 1: 0;		/* bit 5 */
				rSstat.SSE = (data.dp[j] & 0x0040)? 1: 0;		/* bit 6 */
				rSstat.SOV = (data.dp[j] & 0x0080)? 1: 0;		/* bit 7: different from ADSP-219x */
				break;
			}
		}
	}else if(!strcasecmp(s, "_DSTAT0")){	/* _DSTAT0 */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				rDstat0 = 0x000F & data.dp[j];
				if(0x1 & data.dp[j]) rDPENA.dp[0] = 1;
				if(0x2 & data.dp[j]) rDPENA.dp[1] = 1;
				if(0x4 & data.dp[j]) rDPENA.dp[2] = 1;
				if(0x8 & data.dp[j]) rDPENA.dp[3] = 1;
				break;
			}
		}
	}else if(!strcasecmp(s, "_DSTAT1")){	/* _DSTAT1 */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				rDstat1 = 0x000F & data.dp[j];
				if(0x1 & data.dp[j]) rDPMST.dp[0] = 1;
				else if(0x2 & data.dp[j]) rDPMST.dp[1] = 1;
				else if(0x4 & data.dp[j]) rDPMST.dp[2] = 1;
				else if(0x8 & data.dp[j]) rDPMST.dp[3] = 1;
				break;
			}
		}
	}else if(!strcasecmp(s, "UMCOUNT")){	/* UMCOUNT */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPENA.dp[j] && mask.dp[j]){
				//rUMCOUNT.dp[j] = 0xFFFF & data.dp[j];
				rUMCOUNT.dp[j] = 0;								/* modified 2010.05.04: any write attempt (LD/CP) to UMCOUNT works as clearing to zero */
			}
		}
	}else if(!strcasecmp(s, "DID")){			/* DID */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPENA.dp[j] && mask.dp[j]){
				rDID.dp[j] = 0x03 & data.dp[j];
			}
		}
	}else if(!strcasecmp(s, "_ICNTL")){			/* _ICNTL */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				rICNTL.INE = (data.dp[j] & 0x0010)? 1: 0;		/* bit 4 */
				rICNTL.GIE = (data.dp[j] & 0x0020)? 1: 0;		/* bit 5 */
				break;
			}
		}
	}else if(!strcasecmp(s, "_IMASK")){          /* _IMASK */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j]){
        		rIMASK.EMU         = (data.dp[j] & 0x0001)? 1: 0;     /* bit 0 */
        		rIMASK.PWDN        = (data.dp[j] & 0x0002)? 1: 0;     /* bit 1 */
        		rIMASK.SSTEP       = (data.dp[j] & 0x0004)? 1: 0;     /* bit 2 */
        		rIMASK.STACK       = (data.dp[j] & 0x0008)? 1: 0;     /* bit 3 */
        		rIMASK.UserDef[0]  = (data.dp[j] & 0x0010)? 1: 0;     /* bit 4 */
        		rIMASK.UserDef[1]  = (data.dp[j] & 0x0020)? 1: 0;     /* bit 5 */
        		rIMASK.UserDef[2]  = (data.dp[j] & 0x0040)? 1: 0;     /* bit 6 */
        		rIMASK.UserDef[3]  = (data.dp[j] & 0x0080)? 1: 0;     /* bit 7 */
        		rIMASK.UserDef[4]  = (data.dp[j] & 0x0100)? 1: 0;     /* bit 8 */
        		rIMASK.UserDef[5]  = (data.dp[j] & 0x0200)? 1: 0;     /* bit 9 */
        		rIMASK.UserDef[6]  = (data.dp[j] & 0x0400)? 1: 0;     /* bit 10 */
        		rIMASK.UserDef[7]  = (data.dp[j] & 0x0800)? 1: 0;     /* bit 11 */
        		rIMASK.UserDef[8]  = (data.dp[j] & 0x1000)? 1: 0;     /* bit 12 */
        		rIMASK.UserDef[9]  = (data.dp[j] & 0x2000)? 1: 0;     /* bit 13 */
        		rIMASK.UserDef[10] = (data.dp[j] & 0x4000)? 1: 0;     /* bit 14 */
        		rIMASK.UserDef[11] = (data.dp[j] & 0x8000)? 1: 0;     /* bit 15 */
				break;
			}
		}
	}else if(!strcasecmp(s, "_IRPTL")){          /* _IRPTL */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j]){
        		rIRPTL.EMU         = (data.dp[j] & 0x0001)? 1: 0;     /* bit 0 */
        		rIRPTL.PWDN        = (data.dp[j] & 0x0002)? 1: 0;     /* bit 1 */
        		rIRPTL.SSTEP       = (data.dp[j] & 0x0004)? 1: 0;     /* bit 2 */
        		rIRPTL.STACK       = (data.dp[j] & 0x0008)? 1: 0;     /* bit 3 */
        		rIRPTL.UserDef[0]  = (data.dp[j] & 0x0010)? 1: 0;     /* bit 4 */
        		rIRPTL.UserDef[1]  = (data.dp[j] & 0x0020)? 1: 0;     /* bit 5 */
        		rIRPTL.UserDef[2]  = (data.dp[j] & 0x0040)? 1: 0;     /* bit 6 */
        		rIRPTL.UserDef[3]  = (data.dp[j] & 0x0080)? 1: 0;     /* bit 7 */
        		rIRPTL.UserDef[4]  = (data.dp[j] & 0x0100)? 1: 0;     /* bit 8 */
        		rIRPTL.UserDef[5]  = (data.dp[j] & 0x0200)? 1: 0;     /* bit 9 */
        		rIRPTL.UserDef[6]  = (data.dp[j] & 0x0400)? 1: 0;     /* bit 10 */
        		rIRPTL.UserDef[7]  = (data.dp[j] & 0x0800)? 1: 0;     /* bit 11 */
        		rIRPTL.UserDef[8]  = (data.dp[j] & 0x1000)? 1: 0;     /* bit 12 */
        		rIRPTL.UserDef[9]  = (data.dp[j] & 0x2000)? 1: 0;     /* bit 13 */
        		rIRPTL.UserDef[10] = (data.dp[j] & 0x4000)? 1: 0;     /* bit 14 */
        		rIRPTL.UserDef[11] = (data.dp[j] & 0x8000)? 1: 0;     /* bit 15 */
				break;
			}
		}
	}else if(!strncasecmp(s, "_IVEC", 5)) {	/* _IVECx register */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				rIVEC[atoi(s+5)] = data.dp[j];
				break;
			}
		}
	}else if(s[0] == 'R' || s[0] == 'r') {		/* Rx register */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPENA.dp[j] && mask.dp[j]){
				rR[atoi(s+1)].dp[j] = 0xFFF & data.dp[j];
			}
		}
	}else if(!strncasecmp(s, "ACC", 3)) {	/* Accumulator */
			int x = (int)(s[3] - '0');
			if(s[4] == '.'){	/* ACCx.H, M, L */
				switch(s[5]){
					case 'H':
					case 'h':
						for(int j = 0; j < NUMDP; j++) {
							if(rDPENA.dp[j] && mask.dp[j]){
								rAcc[x].H.dp[j] = data.dp[j];
							}
						}
						break;
					case 'M':
					case 'm':
						for(int j = 0; j < NUMDP; j++) {
							if(rDPENA.dp[j] && mask.dp[j]){
								rAcc[x].M.dp[j] = data.dp[j];
								if(rAcc[x].M.dp[j] & 0x800){	/* sign extension */
									rAcc[x].H.dp[j] = 0xFF;
								}else{
									rAcc[x].H.dp[j] = 0x00;
								}
							}
						}
						break;
					case 'L':
					case 'l':
						for(int j = 0; j < NUMDP; j++) {
							if(rDPENA.dp[j] && mask.dp[j]){
								rAcc[x].L.dp[j] = data.dp[j];
							}
						}
						break;
					default:		/* must not happen */
						break;	
				}
				for(int j = 0; j < NUMDP; j++) {
					if(rDPENA.dp[j] && mask.dp[j]){
						/* rAcc[x].value should be updated here */
						rAcc[x].value.dp[j] =   (((rAcc[x].H.dp[j] & 0xFF )<<24) & 0xFF000000)
								| (((rAcc[x].M.dp[j] & 0xFFF)<<12) & 0x00FFF000)
								|  ((rAcc[x].L.dp[j] & 0xFFF)      & 0x00000FFF);
					}
				}
			}else{				/* ACCx */
				for(int j = 0; j < NUMDP; j++) {
					if(rDPENA.dp[j] && mask.dp[j]){
						rAcc[x].value.dp[j] = data.dp[j];	

						/* rAcc[x].H/M/L should be updated here */
						rAcc[x].H.dp[j] = ((data.dp[j] & 0xFF000000)>>24) & 0xFF ;
						rAcc[x].M.dp[j] = ((data.dp[j] & 0x00FFF000)>>12) & 0xFFF;
						rAcc[x].L.dp[j] =  (data.dp[j] & 0x00000FFF)      & 0xFFF;
					}
				}
			}
	}else if(s[0] == 'I' || s[0] == 'i') {		/* Ix register */
		int rn = atoi(s+1);
	
		/* back up values */
		rbI[rn][1] = rbI[rn][0];
		rbI[rn][0] = rI[rn];

		/* record when it's written: 2010.07.21 */
		LAbIx[rn][1] =  LAbIx[rn][0];
		LAbIx[rn][0] = LastAccessIx[rn];
		LastAccessIx[rn] = ::Cycles;	

		/* update */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				rI[rn] = data.dp[j];
				break;
			}
		}
	}else if(s[0] == 'M' || s[0] == 'm') {		/* Mx register */
		int rn = atoi(s+1);

		/* back up values */
		rbM[rn][1] = rbM[rn][0];
		rbM[rn][0] = rM[rn];

		/* record when it's written: 2010.07.21 */
		LAbMx[rn][1] =  LAbMx[rn][0];
		LAbMx[rn][0] = LastAccessMx[rn];
		LastAccessMx[rn] = ::Cycles;	

		/* update */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				rM[rn] = data.dp[j];
				break;
			}
		}
	}else if(s[0] == 'L' || s[0] == 'l') {		/* Lx register */
		int rn = atoi(s+1);

		/* back up values */
		rbL[rn][1] = rbL[rn][0];
		rbL[rn][0] = rL[rn];

		/* record when it's written: 2010.07.21 */
		LAbLx[rn][1] =  LAbLx[rn][0];
		LAbLx[rn][0] = LastAccessLx[rn];
		LastAccessLx[rn] = ::Cycles;	

		/* update */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				rL[rn] = data.dp[j];
				break;
			}
		}
	}else if(s[0] == 'B' || s[0] == 'b') {		/* Bx register */
		int rn = atoi(s+1);

		/* back up values */
		rbB[rn][1] = rbB[rn][0];
		rbB[rn][0] = rB[rn];

		/* record when it's written: 2009.07.08 */
		LAbBx[rn][1] =  LAbBx[rn][0];
		LAbBx[rn][0] = LastAccessBx[rn];
		LastAccessBx[rn] = ::Cycles;	

		/* update */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPMST.dp[j] && mask.dp[j]){
				rB[rn] = data.dp[j];
				break;
			}
		}
	}else{
		printRunTimeError(lineno, s, 
            "Parse Error: sWrReg() - Please report.\n");
	}
}

/** 
* @brief Write SIMD data to the specified register (12-bit x NUMDP) to other SIMD data path.
* 
* @param s Name of register to write
* @param data SIMD data to write (12-bit x NUMDP)
* @param id_offset Relative offset of data-path ID 
* @param active_dp Number of active data paths
* @param mask conditional execution mask
*/
void sWrXReg(char *s, sint data, int id_offset, int active_dp, sint mask)
{
	int reg = atoi(s+1);

	if(s[0] == 'R' || s[0] == 'r') {		/* Rx register */
		if(reg < REG_X_LO || reg > REG_X_HI){
			printRunTimeError(lineno, s, 
				"Destination register operand is out of cross-path register window.\n");
		}
		if(id_offset < 0 || id_offset >= NUMDP){
			printRunTimeError(lineno, s, 
				"Invalid data path ID offset.\n");
		}
		if(active_dp != 2 && active_dp != 4){
			printRunTimeError(lineno, s, 
				"Invalide number of active data-paths (2 or 4).\n");
		}

		for(int j = 0; j < NUMDP; j++) {
			int k = (j+id_offset)%active_dp;
			if(rDPENA.dp[j] && rDPENA.dp[k] && mask.dp[j]){
				rR[reg].dp[k] = 0xFFF & data.dp[j];
			}
		}
	}else{
		printRunTimeError(lineno, s, 
            "Parse Error: sWrXReg() - Please report.\n");
	}
}

/** 
* @brief Read AC of ASTAT.R 
* 
* @return 1 or 0
*/
/*
int RdC1(void)
{
	return rAstatR.AC;
}
*/

/** 
* @brief Read SIMD AC of ASTAT.R 
* 
* @return 1 or 0
*/
sint sRdC1(void)
{
	sint ret;

	for(int j = 0; j < NUMDP; j++) {
		ret.dp[j] = rAstatR.AC.dp[j];
	}
	return ret;
}

/**
* @brief Read AC of ASTAT.R & ASTAT.I
*
* @return AC pairs in cplx structure type
*/
/*
cplx RdC2(void)
{
	cplx n;
	n.r = rAstatR.AC;
	n.i = rAstatI.AC;
	return n;
}
*/
			   
/**
* @brief Read SIMD AC of ASTAT.R & ASTAT.I
*
* @return AC pairs in scplx structure type
*/
scplx RdC2(void)
{
	scplx n;
	for(int j = 0; j < NUMDP; j++) {
		n.r.dp[j] = rAstatR.AC.dp[j];
		n.i.dp[j] = rAstatI.AC.dp[j];
	}
	return n;
}
			   
/** 
* @brief Read complex register data (12-bit x2)
* 
* @param reg Name of register to read
* 
* @return Complex register data (12-bit x2)
*/
//cplx cRdReg(char *reg)
//{
//	int addr;
//	cplx n; 
//	n.r = 0; n.i = 0;
//
///*****************************************************************************/
//	printRunTimeError(lineno, reg, 
//		"Cannot use cRdReg() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	if(!strcasecmp(reg, "NONE")){       /* NONE */
//        return n;
//    }
//
//	/* function body here */
//	if(reg[0] == 'R' || reg[0] == 'r') {		/* Rx register */
//		addr = atoi(reg+1);
//		if(addr & 0x1) {
//			printRunTimeError(lineno, reg, 
//				"Complex register pair should not end with an odd number.\n");
//			return n;
//		}
//
//		n.r = rR[addr];
//		if(isNeg12b(n.r)) n.r |= 0xFFFFF000;
//		n.i = rR[addr+1];
//		if(isNeg12b(n.i)) n.i |= 0xFFFFF000;
//
//	}else if(!strncasecmp(reg, "ACC", 3)) {	/* Accumulator */
//		addr = (int)(reg[3] - '0');
//		if(addr & 0x1) {
//			printRunTimeError(lineno, reg, 
//				"Complex register pair should not end with an odd number.\n");
//			return n;
//		}
//
//		if(reg[4] == '.'){	/* ACCx.H, M, L */
//			switch(reg[5]){
//				case 'H':
//				case 'h':
//					n.r = rAcc[addr].H;
//					if(isNeg8b(n.r)) n.r |= 0xFFFFFF00;
//					n.i = rAcc[addr+1].H;
//					if(isNeg8b(n.i)) n.i |= 0xFFFFFF00;
//					break;
//				case 'M':
//				case 'm':
//					n.r = rAcc[addr].M;
//					if(isNeg12b(n.r)) n.r |= 0xFFFFF000;
//					n.i = rAcc[addr+1].M;
//					if(isNeg12b(n.i)) n.i |= 0xFFFFF000;
//					break;
//				case 'L':
//				case 'l':
//					n.r = rAcc[addr].L;
//					if(isNeg12b(n.r)) n.r |= 0xFFFFF000;
//					n.i = rAcc[addr+1].L;
//					if(isNeg12b(n.i)) n.i |= 0xFFFFF000;
//					break;
//				default:		/* must not happen */
//					n.r = n.i = 0;
//					break;	
//			}
//		}else{				/* ACCx */
//			n.r = rAcc[addr].value;	
//			n.i = rAcc[addr+1].value;	
//		}
//	}										/* other special registers should be added here */
//	return n;
//}

/** 
* @brief Read SIMD complex register data (12-bit x 2 x NUMDP)
* 
* @param reg Name of register to read
* 
* @return SIMD complex register data (12-bit x 2 x NUMDP)
*/
scplx scRdReg(char *reg)
{
	int addr;
	scplx n; 
	for(int j = 0; j < NUMDP; j++){
		n.r.dp[j] = 0; n.i.dp[j] = 0;
	}

	if(!strcasecmp(reg, "NONE")){       /* NONE */
        return n;
    }

	/* function body here */
	if(reg[0] == 'R' || reg[0] == 'r') {		/* Rx register */
		addr = atoi(reg+1);
		if(addr & 0x1) {
			printRunTimeError(lineno, reg, 
				"Complex register pair should not end with an odd number.\n");
			return n;
		}

		for(int j = 0; j < NUMDP; j++){
			n.r.dp[j] = rR[addr].dp[j];
			if(isNeg12b(n.r.dp[j])) n.r.dp[j] |= 0xFFFFF000;
			n.i.dp[j] = rR[addr+1].dp[j];
			if(isNeg12b(n.i.dp[j])) n.i.dp[j] |= 0xFFFFF000;
		}

	}else if(!strncasecmp(reg, "ACC", 3)) {	/* Accumulator */
		addr = (int)(reg[3] - '0');
		if(addr & 0x1) {
			printRunTimeError(lineno, reg, 
				"Complex register pair should not end with an odd number.\n");
			return n;
		}

		if(reg[4] == '.'){	/* ACCx.H, M, L */
			switch(reg[5]){
				case 'H':
				case 'h':
					for(int j = 0; j < NUMDP; j++){
						n.r.dp[j] = rAcc[addr].H.dp[j];
						if(isNeg8b(n.r.dp[j])) n.r.dp[j] |= 0xFFFFFF00;
						n.i.dp[j] = rAcc[addr+1].H.dp[j];
						if(isNeg8b(n.i.dp[j])) n.i.dp[j] |= 0xFFFFFF00;
					}
					break;
				case 'M':
				case 'm':
					for(int j = 0; j < NUMDP; j++){
						n.r.dp[j] = rAcc[addr].M.dp[j];
						if(isNeg12b(n.r.dp[j])) n.r.dp[j] |= 0xFFFFF000;
						n.i.dp[j] = rAcc[addr+1].M.dp[j];
						if(isNeg12b(n.i.dp[j])) n.i.dp[j] |= 0xFFFFF000;
					}
					break;
				case 'L':
				case 'l':
					for(int j = 0; j < NUMDP; j++){
						n.r.dp[j] = rAcc[addr].L.dp[j];
						if(isNeg12b(n.r.dp[j])) n.r.dp[j] |= 0xFFFFF000;
						n.i.dp[j] = rAcc[addr+1].L.dp[j];
						if(isNeg12b(n.i.dp[j])) n.i.dp[j] |= 0xFFFFF000;
					}
					break;
				default:		/* must not happen */
					for(int j = 0; j < NUMDP; j++){
						n.r.dp[j] = n.i.dp[j] = 0;
					}
					break;	
			}
		}else{				/* ACCx */
			for(int j = 0; j < NUMDP; j++){
				n.r.dp[j] = rAcc[addr].value.dp[j];	
				n.i.dp[j] = rAcc[addr+1].value.dp[j];	
			}
		}
	}else{
		printRunTimeError(lineno, reg, 
            "Parse Error: scRdReg() - Please report.\n");
	}										/* other special registers should be added here */
	return n;
}

/** 
* @brief Read SIMD complex register data (12-bit x 2 x NUMDP) from other SIMD data path.
* 
* @param s Name of register to read
* @param id_offset Relative offset of data-path ID 
* @param active_dp Number of active data paths
* 
* @return SIMD complex register data (12-bit x 2 x NUMDP)
*/
scplx scRdXReg(char *s, int id_offset, int active_dp)
{
	cplx val;
	scplx sval; 
	for(int j = 0; j < NUMDP; j++){
		sval.r.dp[j] = 0; sval.i.dp[j] = 0;
	}
	int reg = atoi(s+1);

	/* function body here */
	if(s[0] == 'R' || s[0] == 'r') {		/* Rx register */
		if(reg & 0x1) {
			printRunTimeError(lineno, s, 
				"Complex register pair should not end with an odd number.\n");
		}
		if(reg < REG_X_LO || reg > REG_X_HI){
			printRunTimeError(lineno, s, 
				"Source register operand is out of cross-path register window.\n");
		}
		if(id_offset < 0 || id_offset >= NUMDP){
			printRunTimeError(lineno, s, 
				"Invalid data path ID offset.\n");
		}
		if(active_dp != 2 && active_dp != 4){
			printRunTimeError(lineno, s, 
				"Invalide number of active data-paths (2 or 4).\n");
		}

		for(int j = 0; j < NUMDP; j++){
			val.r = rR[reg  ].dp[(j+id_offset)%active_dp];
			if(isNeg12b(val.r)) val.r |= 0xFFFFF000;
			val.i = rR[reg+1].dp[(j+id_offset)%active_dp];
			if(isNeg12b(val.i)) val.i |= 0xFFFFF000;

			sval.r.dp[j] = val.r;
			sval.i.dp[j] = val.i;
		}
	}else{
		printRunTimeError(lineno, s, 
            "Parse Error: scRdXReg() - Please report.\n");
	}										/* other special registers should be added here */
	return sval;
}

/** 
* @brief Write complex data to the specified register pair (12-bit x2)
* 
* @param reg Name of register to write
* @param data1 12-bit data for real part of complex data pair
* @param data2 12-bit data for imaginary part of complex data pair
*/
//void cWrReg(char *reg, int data1, int data2)
//{
//	int addr;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use cWrReg() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	if(!strcasecmp(reg, "NONE")){       /* NONE */
//        return;   /* do nothing */
//    }
//
//	if(reg[0] == 'R' || reg[0] == 'r') {		/* Rx register */
//		addr = atoi(reg+1);
//		if(addr & 0x1) {
//			printRunTimeError(lineno, reg, 
//				"Complex register pair should not end with an odd number.\n");
//			return;
//		}
//		rR[addr]   = 0xFFF & data1;
//		rR[addr+1] = 0xFFF & data2;
//	}else if(!strncasecmp(reg, "ACC", 3)) {	/* Accumulator */
//		addr = (int)(reg[3] - '0');
//		if(addr & 0x1) {
//			printRunTimeError(lineno, reg, 
//				"Complex register pair should not end with an odd number.\n");
//			return;
//		}
//
//		if(reg[4] == '.'){	/* ACCx.H, M, L */
//			switch(reg[5]){
//				case 'H':
//				case 'h':
//					rAcc[addr].H   = data1;
//					rAcc[addr+1].H = data2;
//					break;
//				case 'M':
//				case 'm':
//					rAcc[addr].M   = data1;
//					if(rAcc[addr].M & 0x800){	/* sign extension */
//						rAcc[addr].H = 0xFF;
//					}else{
//						rAcc[addr].H = 0x00;
//					}
//
//					rAcc[addr+1].M = data2;
//					if(rAcc[addr+1].M & 0x800){	/* sign extension */
//						rAcc[addr+1].H = 0xFF;
//					}else{
//						rAcc[addr+1].H = 0x00;
//					}
//					break;
//				case 'L':
//				case 'l':
//					rAcc[addr].L   = data1;
//					rAcc[addr+1].L = data2;
//					break;
//				default:		/* must not happen */
//					break;	
//			}
//			/* rAcc[x].value should be updated here */
//			rAcc[addr].value =   (((rAcc[addr].H & 0xFF )<<24) & 0xFF000000)
//							| (((rAcc[addr].M & 0xFFF)<<12) & 0x00FFF000)
//							|  ((rAcc[addr].L & 0xFFF)      & 0x00000FFF);
//			rAcc[addr+1].value =   (((rAcc[addr+1].H & 0xFF )<<24) & 0xFF000000)
//							| (((rAcc[addr+1].M & 0xFFF)<<12) & 0x00FFF000)
//							|  ((rAcc[addr+1].L & 0xFFF)      & 0x00000FFF);
//			;
//		}else{				/* ACCx */
//			rAcc[addr].value   = data1;	
//			rAcc[addr+1].value = data2;	
//
//			/* rAcc[x].H/M/L should be updated here */
//			rAcc[addr].H =   ((data1 & 0xFF000000)>>24) & 0xFF ;
//			rAcc[addr].M =   ((data1 & 0x00FFF000)>>12) & 0xFFF;
//			rAcc[addr].L =    (data1 & 0x00000FFF)      & 0xFFF;
//			rAcc[addr+1].H = ((data2 & 0xFF000000)>>24) & 0xFF ;
//			rAcc[addr+1].M = ((data2 & 0x00FFF000)>>12) & 0xFFF;
//			rAcc[addr+1].L =  (data2 & 0x00000FFF)      & 0xFFF;
//		}
//	}
//}

/** 
* @brief Write complex SIMD data to the specified register pair (12-bit x2 x NUMDP)
* 
* @param reg Name of register to write
* @param data1 SIMD data for real part of complex data pair (12-bit x NUMDP)
* @param data2 SIMD data for imaginary part of complex data pair (12-bit x NUMDP)
* @param mask conditional execution mask
*/
void scWrReg(char *reg, sint data1, sint data2, sint mask)
{
	int addr;

	if(!strcasecmp(reg, "NONE")){       /* NONE */
        return;   /* do nothing */
    }

	if(reg[0] == 'R' || reg[0] == 'r') {		/* Rx register */
		addr = atoi(reg+1);
		if(addr & 0x1) {
			printRunTimeError(lineno, reg, 
				"Complex register pair should not end with an odd number.\n");
			return;
		}
		for(int j = 0; j < NUMDP; j++) {
			if(rDPENA.dp[j] & mask.dp[j]){
				rR[addr].dp[j]   = 0xFFF & data1.dp[j];
				rR[addr+1].dp[j] = 0xFFF & data2.dp[j];
			}
		}
	}else if(!strncasecmp(reg, "ACC", 3)) {	/* Accumulator */
		addr = (int)(reg[3] - '0');
		if(addr & 0x1) {
			printRunTimeError(lineno, reg, 
				"Complex register pair should not end with an odd number.\n");
			return;
		}

		if(reg[4] == '.'){	/* ACCx.H, M, L */
			switch(reg[5]){
				case 'H':
				case 'h':
					for(int j = 0; j < NUMDP; j++) {
						if(rDPENA.dp[j] & mask.dp[j]){
							rAcc[addr].H.dp[j]   = data1.dp[j];
							rAcc[addr+1].H.dp[j] = data2.dp[j];
						}
					}
					break;
				case 'M':
				case 'm':
					for(int j = 0; j < NUMDP; j++) {
						if(rDPENA.dp[j] & mask.dp[j]){
							rAcc[addr].M.dp[j]   = data1.dp[j];
							if(rAcc[addr].M.dp[j] & 0x800){	/* sign extension */
								rAcc[addr].H.dp[j] = 0xFF;
							}else{
								rAcc[addr].H.dp[j] = 0x00;
							}

							rAcc[addr+1].M.dp[j] = data2.dp[j];
							if(rAcc[addr+1].M.dp[j] & 0x800){	/* sign extension */
								rAcc[addr+1].H.dp[j] = 0xFF;
							}else{
								rAcc[addr+1].H.dp[j] = 0x00;
							}
						}
					}
					break;
				case 'L':
				case 'l':
					for(int j = 0; j < NUMDP; j++) {
						if(rDPENA.dp[j] & mask.dp[j]){
							rAcc[addr].L.dp[j]   = data1.dp[j];
							rAcc[addr+1].L.dp[j] = data2.dp[j];
						}
					}
					break;
				default:		/* must not happen */
					break;	
			}

			for(int j = 0; j < NUMDP; j++) {
				if(rDPENA.dp[j] & mask.dp[j]){
					/* rAcc[x].value should be updated here */
					rAcc[addr].value.dp[j] =   (((rAcc[addr].H.dp[j] & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr].M.dp[j] & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr].L.dp[j] & 0xFFF)      & 0x00000FFF);
					rAcc[addr+1].value.dp[j] =   (((rAcc[addr+1].H.dp[j] & 0xFF )<<24) & 0xFF000000)
							| (((rAcc[addr+1].M.dp[j] & 0xFFF)<<12) & 0x00FFF000)
							|  ((rAcc[addr+1].L.dp[j] & 0xFFF)      & 0x00000FFF);
				}
			}
		}else{				/* ACCx */
			for(int j = 0; j < NUMDP; j++) {
				if(rDPENA.dp[j] & mask.dp[j]){
					rAcc[addr].value.dp[j]   = data1.dp[j];	
					rAcc[addr+1].value.dp[j] = data2.dp[j];	

					/* rAcc[x].H/M/L should be updated here */
					rAcc[addr].H.dp[j] =   ((data1.dp[j] & 0xFF000000)>>24) & 0xFF ;
					rAcc[addr].M.dp[j] =   ((data1.dp[j] & 0x00FFF000)>>12) & 0xFFF;
					rAcc[addr].L.dp[j] =    (data1.dp[j] & 0x00000FFF)      & 0xFFF;
					rAcc[addr+1].H.dp[j] = ((data2.dp[j] & 0xFF000000)>>24) & 0xFF ;
					rAcc[addr+1].M.dp[j] = ((data2.dp[j] & 0x00FFF000)>>12) & 0xFFF;
					rAcc[addr+1].L.dp[j] =  (data2.dp[j] & 0x00000FFF)      & 0xFFF;
				}
			}
		}
	}else{
		printRunTimeError(lineno, reg, 
            "Parse Error: scWrReg() - Please report.\n");
	}
}

/** 
* @brief Write complex SIMD data to the specified register pair (12-bit x2 x NUMDP) to other SIMD data path.
* 
* @param s Name of register to write
* @param data1 SIMD data for real part of complex data pair (12-bit x NUMDP)
* @param data2 SIMD data for imaginary part of complex data pair (12-bit x NUMDP)
* @param id_offset Relative offset of data-path ID 
* @param active_dp Number of active data paths
* @param mask conditional execution mask
*/
void scWrXReg(char *s, sint data1, sint data2, int id_offset, int active_dp, sint mask)
{
	cplx val;
	scplx sval; 
	for(int j = 0; j < NUMDP; j++){
		sval.r.dp[j] = 0; sval.i.dp[j] = 0;
	}
	int reg = atoi(s+1);

	if(s[0] == 'R' || s[0] == 'r') {		/* Rx register */
		if(reg & 0x1) {
			printRunTimeError(lineno, s, 
				"Complex register pair should not end with an odd number.\n");
		}
		if(reg < REG_X_LO || reg > REG_X_HI){
			printRunTimeError(lineno, s, 
				"Destination register operand is out of cross-path register window.\n");
		}
		if(id_offset < 0 || id_offset >= NUMDP){
			printRunTimeError(lineno, s, 
				"Invalid data path ID offset.\n");
		}
		if(active_dp != 2 && active_dp != 4){
			printRunTimeError(lineno, s, 
				"Invalide number of active data-paths (2 or 4).\n");
		}

		for(int j = 0; j < NUMDP; j++) {
			int k = (j+id_offset)%active_dp;
			if(rDPENA.dp[j] && rDPENA.dp[k] && mask.dp[j]){
				rR[reg  ].dp[k] = 0xFFF & data1.dp[j];
				rR[reg+1].dp[k] = 0xFFF & data2.dp[j];
			}
		}
	}else{
		printRunTimeError(lineno, s, 
            "Parse Error: scWrXReg() - Please report.\n");
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
//void flagEffect(int type, int z, int n, int v, int c)
//{
//	switch(type){
//		case t09c:	/* ADD, SUB */
//		case t09e:	
//		case t09i:	
//		case t09g:	/* ADD, SUB ACC32 */
//		case t01c: /* ALU || LD || LD */
//		case t04c: /* ALU || LD */
//		case t04g: /* ALU || ST */
//		case t08c: /* ALU || CP */
//		case t43a: /* ALU || MAC */
//		case t44b: /* ALU || SHIFT */
//		case t44d: /* ALU || SHIFT */
//		case t46a: /* SCR */
//			rAstatR.AZ = z;
//			rAstatR.AN = n;
//			if(!isAVLatchedMode()){		/* if AV_LATCH is not set */
//				rAstatR.AV = v;			/* work as usual */	
//				if(v) OVCount++;
//			} else {					/* if set, update only if AV is 0 */
//				if(rAstatR.AV == 0 && v == 1){
//					rAstatR.AV = 1;
//					OVCount++;
//				}
//			}
//			rAstatR.AC = c;
//			break;
//		case t41a:	/* RNDACC */
//			rAstatR.MV = v;
//			if(v) OVCount++;
//			break;
//		case t41c:	/* CLRACC */
//			/*
//			rAstatR.AZ = 1;
//			rAstatR.AN = 0;
//			if(!isAVLatchedMode())	
//				rAstatR.AV = 0;
//			rAstatR.AC = 0;
//			*/
//			rAstatR.MV = 0;
//			break;
//		case t11a:	/* DO UNTIL */
//			//rSstat.LSE = 0;
//			rSstat.LSE = stackEmpty(&LoopBeginStack);
//			rSstat.LSF = stackFull(&LoopBeginStack); 
//			rSstat.SOV |= rSstat.LSF;
//			break;
//		case t26a: 	/* PUSH/POP */
//			rSstat.PCE = stackEmpty(&PCStack);
//			rSstat.PCF = stackFull(&PCStack);
//			rSstat.PCL = stackCheckLevel(&PCStack);
//			rSstat.LSE = stackEmpty(&LoopBeginStack);
//			rSstat.LSF = stackFull(&LoopBeginStack);
//			rSstat.SSE = stackEmpty(&ASTATStack[0]);
//			rSstat.SOV = PCStack.Overflow + PCStack.Underflow
//						+ LoopBeginStack.Overflow + LoopBeginStack.Underflow
//						+ ASTATStack[0].Overflow + ASTATStack[0].Underflow;
//			break;
//		case t37a: 	/* SETINT */
//			rSstat.PCE = stackEmpty(&PCStack);
//			rSstat.PCF = stackFull(&PCStack);
//			rSstat.PCL = stackCheckLevel(&PCStack);
//			rSstat.SSE = stackEmpty(&ASTATStack[0]);
//			rSstat.SOV = PCStack.Overflow + PCStack.Underflow
//						+ LoopBeginStack.Overflow + LoopBeginStack.Underflow
//						+ ASTATStack[0].Overflow + ASTATStack[0].Underflow;
//			break;
//		case t40e: /* MAC, MPY */
//		case t01a: /* MAC || LD || LD */	
//		case t04a: /* MAC || LD */
//		case t04e: /* MAC || ST */
//		case t08a: /* MAC || CP */
//		case t45b: /* MAC || SHIFT */
//		case t45d: /* MAC || SHIFT */
//			rAstatR.MV = v;
//			if(v) OVCount++;
//			break;
//		case t16a:	/* ASHIFT, LSHIFT */
//		case t16e:	/* ASHIFT, LSHIFT */
//		case t15a:	/* ASHIFT, LSHIFT by imm */
//		case t15c:	/* ASHIFT, LSHIFT by imm */
//		case t15e:	/* ASHIFT, LSHIFT by imm */
//		case t12e:	/* SHIFT || LD */
//		case t12m:	/* SHIFT || LD */
//		case t12g:	/* SHIFT || ST */
//		case t12o:	/* SHIFT || ST */
//		case t14c:	/* SHIFT || CP */
//		case t14k:	/* SHIFT || CP */
//			rAstatR.SV = v;
//			if(v) OVCount++;
//			break;
//		case t10a:	/* CALL */
//		case t10b:	/* CALL */
//		case t19a:
//			rSstat.PCE = stackEmpty(&PCStack);
//			rSstat.PCF = stackFull(&PCStack);
//			rSstat.PCL = stackCheckLevel(&PCStack);
//			rSstat.SOV = PCStack.Overflow + PCStack.Underflow
//						+ LoopBeginStack.Overflow + LoopBeginStack.Underflow
//						+ ASTATStack[0].Overflow + ASTATStack[0].Underflow;
//			break;
//		case t20a:	/* RTI/RTS */
//			rSstat.PCE = stackEmpty(&PCStack);
//			rSstat.PCF = stackFull(&PCStack);
//			rSstat.PCL = stackCheckLevel(&PCStack);
//			break;
//		default:	/* non-primary instruction in multifunction: ignored here */
//			break;
//	}
//}

/** 
* Set/clear affected flags of status registers. (SIMD version)
* Note: this function accesses global variables.
* 
* @param type Instruction type of current instruction.
* @param z zero flag	
* @param n negative flag	
* @param v overflow flag	
* @param c carry flag	
* @param mask conditional execution mask
*/
void sFlagEffect(int type, sint z, sint n, sint v, sint c, sint mask)
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
			for(int j = 0; j < NUMDP; j++){
				if(rDPENA.dp[j] && mask.dp[j]){
					rAstatR.AZ.dp[j] = z.dp[j];
					rAstatR.AN.dp[j] = n.dp[j];
					if(!isAVLatchedMode()){		/* if AV_LATCH is not set */
						rAstatR.AV.dp[j] = v.dp[j];			/* work as usual */	
						if(v.dp[j]) OVCount.dp[j]++;
					} else {					/* if set, update only if AV is 0 */
						if(rAstatR.AV.dp[j] == 0 && v.dp[j] == 1){
							rAstatR.AV.dp[j] = 1;
							OVCount.dp[j]++;
						}
					}
					rAstatR.AC.dp[j] = c.dp[j];
				}
			}
			break;
		case t41a:	/* RNDACC */
			for(int j = 0; j < NUMDP; j++){
				if(rDPENA.dp[j] && mask.dp[j]){
					rAstatR.MV.dp[j] = v.dp[j];
					if(v.dp[j]) OVCount.dp[j]++;
				}
			}
			break;
		case t41c:	/* CLRACC */
			for(int j = 0; j < NUMDP; j++){
				if(rDPENA.dp[j] && mask.dp[j]){
					/*
					rAstatR.AZ.dp[j] = 1;
					rAstatR.AN.dp[j] = 0;
					if(!isAVLatchedMode())	
						rAstatR.AV.dp[j] = 0;
					rAstatR.AC.dp[j] = 0;
					*/
					rAstatR.MV.dp[j] = 0;
				}
			}
			break;
		case t11a:	/* DO UNTIL */
			//for(int j = 0; j < NUMDP; j++) {
			//	if(rDPMST.dp[j]){
					rSstat.LSE = stackEmpty(&LoopBeginStack);
					rSstat.LSF = stackFull(&LoopBeginStack); 
					rSstat.SOV |= rSstat.LSF;
			//		break;
			//	}
			//}
			break;
		case t26a: 	/* PUSH/POP */
			//for(int j = 0; j < NUMDP; j++) {
			//	if(rDPMST.dp[j]){
					rSstat.PCE = stackEmpty(&PCStack);
					rSstat.PCF = stackFull(&PCStack);
					rSstat.PCL = stackCheckLevel(&PCStack);
					rSstat.LSE = stackEmpty(&LoopBeginStack);
					rSstat.LSF = stackFull(&LoopBeginStack);
					rSstat.SSE = sStackEmpty(&ASTATStack[0]);
					rSstat.SOV = PCStack.Overflow + PCStack.Underflow
						+ LoopBeginStack.Overflow + LoopBeginStack.Underflow
						+ ASTATStack[0].Overflow + ASTATStack[0].Underflow;
			//		break;
			//	}
			//}
			break;
		case t37a: 	/* SETINT */
			//for(int j = 0; j < NUMDP; j++) {
			//	if(rDPMST.dp[j]){
					rSstat.PCE = stackEmpty(&PCStack);
					rSstat.PCF = stackFull(&PCStack);
					rSstat.PCL = stackCheckLevel(&PCStack);
					rSstat.SSE = sStackEmpty(&ASTATStack[0]);
					rSstat.SOV = PCStack.Overflow + PCStack.Underflow
						+ LoopBeginStack.Overflow + LoopBeginStack.Underflow
						+ ASTATStack[0].Overflow + ASTATStack[0].Underflow;
			//		break;
			//	}
			//}
			break;
		case t40e: /* MAC, MPY */
		case t01a: /* MAC || LD || LD */	
		case t04a: /* MAC || LD */
		case t04e: /* MAC || ST */
		case t08a: /* MAC || CP */
		case t45b: /* MAC || SHIFT */
		case t45d: /* MAC || SHIFT */
			for(int j = 0; j < NUMDP; j++){
				if(rDPENA.dp[j] && mask.dp[j]){
					rAstatR.MV.dp[j] = v.dp[j];
					if(v.dp[j]) OVCount.dp[j]++;
				}
			}
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
			for(int j = 0; j < NUMDP; j++){
				if(rDPENA.dp[j] && mask.dp[j]){
					rAstatR.SV.dp[j] = v.dp[j];
					if(v.dp[j]) OVCount.dp[j]++;
				}
			}
			break;
		case t10a:	/* CALL */
		case t10b:	/* CALL */
		case t19a:
			//for(int j = 0; j < NUMDP; j++) {
			//	if(rDPMST.dp[j]){
					rSstat.PCE = stackEmpty(&PCStack);
					rSstat.PCF = stackFull(&PCStack);
					rSstat.PCL = stackCheckLevel(&PCStack);
					rSstat.SOV = PCStack.Overflow + PCStack.Underflow
						+ LoopBeginStack.Overflow + LoopBeginStack.Underflow
						+ ASTATStack[0].Overflow + ASTATStack[0].Underflow;
			//		break;
			//	}
			//}
			break;
		case t20a:	/* RTI/RTS */
			//for(int j = 0; j < NUMDP; j++) {
			//	if(rDPMST.dp[j]){
					rSstat.PCE = stackEmpty(&PCStack);
					rSstat.PCF = stackFull(&PCStack);
					rSstat.PCL = stackCheckLevel(&PCStack);
			//		break;
			//	}
			//}
			break;
		default:	/* non-primary instruction in multifunction: ignored here */
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
//void cFlagEffect(int type, cplx z, cplx n, cplx v, cplx c)
//{
//    int ovset1, ovset2;
//
//    switch(type){
//        case t09d:	/* ADD.C, SUB.C */
//        case t09f:
//		case t09g:	/* ADD.C, SUB.C ACC64 */
//		case t04d: /* ALU.C || LD.C */
//		case t04h: /* ALU.C || ST.C */
//		case t08d: /* ALU.C || CP.C */
//		case t46b: /* SCR.C */
//            rAstatR.AZ = z.r;
//            rAstatI.AZ = z.i;
//            rAstatC.AZ = (z.r && z.i);
//
//            rAstatR.AN = n.r;
//            rAstatI.AN = n.i;
//            rAstatC.AN = (n.r || n.i);  /* TBD */
//
//			if(!isAVLatchedMode()){     /* if AV_LATCH is not set */
//				rAstatR.AV = v.r;           /* work as usual */
//				rAstatI.AV = v.i;           /* work as usual */
//				rAstatC.AV = (v.r || v.i);
//				if(rAstatC.AV) OVCount++;
//			} else {                    /* if set, update only if AV is 0 */
//				ovset1 = ovset2 = 0;
//
//				if(rAstatR.AV == 0 && v.r == 1){
//					rAstatR.AV = 1;
//					ovset1 = TRUE;
//				}
//				if(rAstatI.AV == 0 && v.i == 1){
//					rAstatI.AV = 1;
//					ovset2 = TRUE;
//				}
//
//				rAstatC.AV = (rAstatR.AV || rAstatI.AV);
//				if(ovset1 || ovset2) OVCount++;
//			}
//
//			rAstatR.AC = c.r;
//			rAstatI.AC = c.i;
//			rAstatC.AC = (c.r || c.i);
//			break;
//		case t41b:  /* RNDACC.C */
//			rAstatR.MV = v.r;
//			rAstatI.MV = v.i;
//			rAstatC.MV = (v.r || v.i);
//			if(v.r || v.i) OVCount++;
//			break;
//		case t41d:  /* CLRACC.C */
//			/*
//			rAstatR.AZ = 1;
//			rAstatI.AZ = 1;
//			rAstatC.AZ = 1;
//
//			rAstatR.AN = 0;
//			rAstatI.AN = 0;
//			rAstatC.AN = 0;
//
//			if(!isAVLatchedMode()){     
//				rAstatR.AV = 0;
//				rAstatI.AV = 0;
//				rAstatC.AV = 0;
//			}
//
//			rAstatR.AC = 0;
//			rAstatI.AC = 0;
//			rAstatC.AC = 0;
//			*/
//
//			rAstatR.MV = 0;
//			rAstatI.MV = 0;
//			rAstatC.MV = 0;
// 			break;
//		case t40f: /* MAC.C, MAS.C, MPY.C */
//		case t40g:
//		case t01b: /* MAC.C || LD.C || LD.C */
//		case t04b: /* MAC.C || LD.C */
//		case t04f: /* MAC.C || ST.C */
//		case t08b: /* MAC.C || CP.C */
//			rAstatR.MV = v.r;
//			rAstatI.MV = v.i;
//			rAstatC.MV = (v.r || v.i);
//
//			if(v.r || v.i) OVCount++;
//			break;
//		case t16b:  /* ASHIFT.C, LSHIFT.C */
//		case t16f:  /* ASHIFT.C, LSHIFT.C */
//		case t15b:  /* ASHIFT.C, LSHIFT.C by imm */
//		case t15d:  /* ASHIFT.C, LSHIFT.C by imm */
//		case t15f:  /* ASHIFT.C, LSHIFT.C by imm */
//		case t12f:	/* SHIFT.C || LD.C */
//		case t12n:	/* SHIFT.C || LD.C */
//		case t12h:	/* SHIFT.C || ST.C */
//		case t12p:	/* SHIFT.C || ST.C */
//		case t14d:	/* SHIFT.C || CP.C */
//		case t14l:	/* SHIFT.C || CP.C */
//			rAstatR.SV = v.r;
//			rAstatI.SV = v.i;
//			rAstatC.SV = (v.r || v.i);
//
//			if(v.r || v.i) OVCount++;
//			break;
//		case t42a:	/* POLAR.C, RECT.C */
//		case t47a:	/* MAG.C */
//            rAstatR.AZ = z.r;
//            rAstatI.AZ = z.i;
//            rAstatC.AZ = (z.r && z.i);
//
//            rAstatR.AN = n.r;
//            rAstatI.AN = n.i;
//            rAstatC.AN = (n.r || n.i);  
//			break;
//		default:	/* non-primary instruction in multifunction: ignored here */
//			break;
//	}
//}

/**
* Set/clear affected flags of status registers for complex operations. (SIMD version)
* Note: this function accesses global variables.
*
* @param type Instruction type of current instruction.
* @param z zero flag
* @param n negative flag
* @param v overflow flag
* @param c carry flag
* @param mask conditional execution mask
*/
void scFlagEffect(int type, scplx z, scplx n, scplx v, scplx c, sint mask)
{
    int ovset1, ovset2;

    switch(type){
        case t09d:	/* ADD.C, SUB.C */
        case t09f:
		case t09g:	/* ADD.C, SUB.C ACC64 */
		case t04d:	/* ALU.C || LD.C */
		case t04h: 	/* ALU.C || ST.C */
		case t08d: 	/* ALU.C || CP.C */
		case t46b: 	/* SCR.C */
		case t47a:	/* MAG.C */
		case t50a: /* RCCW.C, RCW.C */
			for(int j = 0; j < NUMDP; j++){
				if(rDPENA.dp[j] && mask.dp[j]){
            		rAstatR.AZ.dp[j] = z.r.dp[j];
            		rAstatI.AZ.dp[j] = z.i.dp[j];
            		rAstatC.AZ.dp[j] = (z.r.dp[j] && z.i.dp[j]);

            		rAstatR.AN.dp[j] = n.r.dp[j];
            		rAstatI.AN.dp[j] = n.i.dp[j];
            		rAstatC.AN.dp[j] = (n.r.dp[j] || n.i.dp[j]);  /* TBD */

					if(!isAVLatchedMode()){     /* if AV_LATCH is not set */
						rAstatR.AV.dp[j] = v.r.dp[j];           /* work as usual */
						rAstatI.AV.dp[j] = v.i.dp[j];           /* work as usual */
						rAstatC.AV.dp[j] = (v.r.dp[j] || v.i.dp[j]);
						if(rAstatC.AV.dp[j]) OVCount.dp[j]++;
					} else {                    /* if set, update only if AV is 0 */
						ovset1 = ovset2 = 0;

						if(rAstatR.AV.dp[j] == 0 && v.r.dp[j] == 1){
							rAstatR.AV.dp[j] = 1;
							ovset1 = TRUE;
						}
						if(rAstatI.AV.dp[j] == 0 && v.i.dp[j] == 1){
							rAstatI.AV.dp[j] = 1;
							ovset2 = TRUE;
						}

						rAstatC.AV.dp[j] = (rAstatR.AV.dp[j] || rAstatI.AV.dp[j]);
						if(ovset1 || ovset2) OVCount.dp[j]++;
					}

					rAstatR.AC.dp[j] = c.r.dp[j];
					rAstatI.AC.dp[j] = c.i.dp[j];
					rAstatC.AC.dp[j] = (c.r.dp[j] || c.i.dp[j]);
				}
			}
			break;
		case t41b:  /* RNDACC.C */
			for(int j = 0; j < NUMDP; j++){
				if(rDPENA.dp[j] && mask.dp[j]){
					rAstatR.MV.dp[j] = v.r.dp[j];
					rAstatI.MV.dp[j] = v.i.dp[j];
					rAstatC.MV.dp[j] = (v.r.dp[j] || v.i.dp[j]);
					if(v.r.dp[j] || v.i.dp[j]) OVCount.dp[j]++;
				}
			}
			break;
		case t41d:  /* CLRACC.C */
			for(int j = 0; j < NUMDP; j++){
				if(rDPENA.dp[j] && mask.dp[j]){
					/*
					rAstatR.AZ.dp[j] = 1;
					rAstatI.AZ.dp[j] = 1;
					rAstatC.AZ.dp[j] = 1;

					rAstatR.AN.dp[j] = 0;
					rAstatI.AN.dp[j] = 0;
					rAstatC.AN.dp[j] = 0;

					if(!isAVLatchedMode()){     
						rAstatR.AV.dp[j] = 0;
						rAstatI.AV.dp[j] = 0;
						rAstatC.AV.dp[j] = 0;
					}

					rAstatR.AC.dp[j] = 0;
					rAstatI.AC.dp[j] = 0;
					rAstatC.AC.dp[j] = 0;
					*/

					rAstatR.MV.dp[j] = 0;
					rAstatI.MV.dp[j] = 0;
					rAstatC.MV.dp[j] = 0;
				}
			}
 			break;
		case t40f: /* MAC.C, MAS.C, MPY.C */
		case t40g:
		case t01b: /* MAC.C || LD.C || LD.C */
		case t04b: /* MAC.C || LD.C */
		case t04f: /* MAC.C || ST.C */
		case t08b: /* MAC.C || CP.C */
			for(int j = 0; j < NUMDP; j++){
				if(rDPENA.dp[j] && mask.dp[j]){
					rAstatR.MV.dp[j] = v.r.dp[j];
					rAstatI.MV.dp[j] = v.i.dp[j];
					rAstatC.MV.dp[j] = (v.r.dp[j] || v.i.dp[j]);

					if(v.r.dp[j] || v.i.dp[j]) OVCount.dp[j]++;
				}
			}
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
			for(int j = 0; j < NUMDP; j++){
				if(rDPENA.dp[j] && mask.dp[j]){
					rAstatR.SV.dp[j] = v.r.dp[j];
					rAstatI.SV.dp[j] = v.i.dp[j];
					rAstatC.SV.dp[j] = (v.r.dp[j] || v.i.dp[j]);

					if(v.r.dp[j] || v.i.dp[j]) OVCount.dp[j]++;
				}
			}
			break;
		case t42a:	/* POLAR.C, RECT.C */
			for(int j = 0; j < NUMDP; j++){
				if(rDPENA.dp[j] && mask.dp[j]){
            		rAstatR.AZ.dp[j] = z.r.dp[j];
            		rAstatI.AZ.dp[j] = z.i.dp[j];
            		rAstatC.AZ.dp[j] = (z.r.dp[j] && z.i.dp[j]);

            		rAstatR.AN.dp[j] = n.r.dp[j];
            		rAstatI.AN.dp[j] = n.i.dp[j];
            		rAstatC.AN.dp[j] = (n.r.dp[j] || n.i.dp[j]);  
				}
			}
			break;
		default:	/* non-primary instruction in multifunction: ignored here */
			break;
	}
}

/** 
* @brief Show current register data
* 
* @param p Pointer to current instruction
*/
void dumpRegister(sICode *p)
{
	int i;

	int master_id;

	/* find master DP ID */
	for(int j = 0; j < NUMDP; j++) {
		if(rDPMST.dp[j]){
			master_id = j;
			break;
		}
	}


	if(p){		//if p is not NULL
		if(isCommentLabelInst(p->Index))
			return;     /* no need to dump register */

		printf("\n_PC:%04X Line:%d Opcode:%s Type:%s\n", p->PMA, p->LineCntr, 
			(char *)sOp[p->Index], sType[p->InstType]);
		printf("----\n");
	}

	for(int j = 0; j < NUMDP; j++) {
		char *s;
		if(j == master_id){			/* if master */
			printf("[*]                                 \t");
		}else if(rDPENA.dp[j]){		/* else, if enabled */
			printf("[%d]                                 \t", j);
		}else if(SIMD4ForceMode){
			printf("[.]                                 \t");
		}
	}
	printf("\n");

	for(i = 0; i < 8; i++){
		for(int j = 0; j < NUMDP; j++) {
			if(rDPENA.dp[j] || SIMD4ForceMode){
				printf("[%01d] ", j);
				printf("R%02d:%03X ", i   , (unsigned int)(rR[i   ].dp[j]   & 0xFFF));
				printf("R%02d:%03X ", i+ 8, (unsigned int)(rR[i+ 8].dp[j] & 0xFFF));
				printf("R%02d:%03X ", i+16, (unsigned int)(rR[i+16].dp[j] & 0xFFF));
				printf("R%02d:%03X ", i+24, (unsigned int)(rR[i+24].dp[j] & 0xFFF));
				printf("\t");
			}
		}
		printf("\n");
	}

	for(i = 0; i < 8; i++){
		for(int j = 0; j < NUMDP; j++) {
			if(rDPENA.dp[j] || SIMD4ForceMode){
				printf("[%01d] ", j);
				printf("ACC%d:%08X ",   i, (unsigned int)(rAcc[i].value.dp[j] & 0xFFFFFFFF));
				//printf("ACC%d.H:%02X ", i, (unsigned int)(rAcc[i].H.dp[j]     & 0xFF ));
				//printf("ACC%d.M:%03X ", i, (unsigned int)(rAcc[i].M.dp[j]     & 0xFFF));
				//printf("ACC%d.L:%03X ", i, (unsigned int)(rAcc[i].L.dp[j]     & 0xFFF));
				printf("(H:%02X ", (unsigned int)(rAcc[i].H.dp[j]     & 0xFF ));
				printf("M:%03X ", (unsigned int)(rAcc[i].M.dp[j]     & 0xFFF));
				printf("L:%03X)", (unsigned int)(rAcc[i].L.dp[j]     & 0xFFF));
				printf("\t");
			}
		}
		printf("\n");
	}

	printf("----\n");
	//printf("ASTAT:\t");
	if(rDPENA.dp[0] || SIMD4ForceMode)
		printf("[0] AZ AN AV AC AS AQ MV SS SV UM\t");
	if(rDPENA.dp[1] || SIMD4ForceMode)
		printf("[1] AZ AN AV AC AS AQ MV SS SV UM\t");
	if(rDPENA.dp[2] || SIMD4ForceMode)
		printf("[2] AZ AN AV AC AS AQ MV SS SV UM\t");
	if(rDPENA.dp[3] || SIMD4ForceMode)
		printf("[3] AZ AN AV AC AS AQ MV SS SV UMt");
	printf("\n");

	for(int j = 0; j < NUMDP; j++) {
		if(rDPENA.dp[j] || SIMD4ForceMode){
			//printf("ASTAT.R - AZ:%d AN:%d AV:%d AC:%d AS:%d AQ:%d MV:%d SS:%d SV:%d\t",
			//	rAstatR.AZ.dp[j], rAstatR.AN.dp[j], rAstatR.AV.dp[j], rAstatR.AC.dp[j], rAstatR.AS.dp[j], 
			//	rAstatR.AQ.dp[j], rAstatR.MV.dp[j], rAstatR.SS.dp[j], rAstatR.SV.dp[j]);
			//printf("[%01d] ", j);
			printf(".R   ");
			printf("%d  %d  %d  %d  %d  %d  %d  %d  %d  .     \t",
				rAstatR.AZ.dp[j], rAstatR.AN.dp[j], rAstatR.AV.dp[j], rAstatR.AC.dp[j], rAstatR.AS.dp[j], 
				rAstatR.AQ.dp[j], rAstatR.MV.dp[j], rAstatR.SS.dp[j], rAstatR.SV.dp[j]);
		}
	}
	printf("\n");

	for(int j = 0; j < NUMDP; j++) {
		if(rDPENA.dp[j] || SIMD4ForceMode){
			//printf("ASTAT.I - AZ:%d AN:%d AV:%d AC:%d AS:%d AQ:%d MV:%d SS:%d SV:%d\n",
			//	rAstatI.AZ.dp[j], rAstatI.AN.dp[j], rAstatI.AV.dp[j], rAstatI.AC.dp[j], rAstatI.AS.dp[j], 
			//	rAstatI.AQ.dp[j], rAstatI.MV.dp[j], rAstatI.SS.dp[j], rAstatI.SV.dp[j]);
			//printf("[%01d] ", j);
			printf(".I   ");
			printf("%d  %d  %d  %d  %d  %d  %d  %d  %d  .     \t",
				rAstatI.AZ.dp[j], rAstatI.AN.dp[j], rAstatI.AV.dp[j], rAstatI.AC.dp[j], rAstatI.AS.dp[j], 
				rAstatI.AQ.dp[j], rAstatI.MV.dp[j], rAstatI.SS.dp[j], rAstatI.SV.dp[j]);
		}
	}
	printf("\n");

	for(int j = 0; j < NUMDP; j++) {
		if(rDPENA.dp[j] || SIMD4ForceMode){
			//printf("ASTAT.C - AZ:%d AN:%d AV:%d AC:%d AS:%d AQ:%d MV:%d SS:%d SV:%d UM:%d\n",
			//	rAstatC.AZ.dp[j], rAstatC.AN.dp[j], rAstatC.AV.dp[j], rAstatC.AC.dp[j], rAstatC.AS.dp[j], 
			//	rAstatC.AQ.dp[j], rAstatC.MV.dp[j], rAstatC.SS.dp[j], rAstatC.SV.dp[j], rAstatC.UM.dp[j]);
			//printf("[%01d] ", j);
			printf(".C   ");
			printf("%d  %d  %d  %d  %d  %d  %d  %d  %d  %d    \t",
				rAstatC.AZ.dp[j], rAstatC.AN.dp[j], rAstatC.AV.dp[j], rAstatC.AC.dp[j], rAstatC.AS.dp[j], 
				rAstatC.AQ.dp[j], rAstatC.MV.dp[j], rAstatC.SS.dp[j], rAstatC.SV.dp[j], rAstatC.UM.dp[j]);
		}
	}
	printf("\n");

	if(VerboseMode){
		for(int j = 0; j < NUMDP; j++) {
			if(rDPENA.dp[j] || SIMD4ForceMode){
				printf("[%01d] DID:%01X                          \t", j, rDID.dp[j]);
			}
		}
		printf("\n");
	}

	int usum = 0;
	for(int j = 0; j < NUMDP; j++) {
		if(rDPENA.dp[j]){
			usum += rUMCOUNT.dp[j];
		}
	}
	if(usum){		/* print if any UMCOUNT value is non-zero */
		for(int j = 0; j < NUMDP; j++) {
			if(rDPENA.dp[j] || SIMD4ForceMode){
				printf("[%01d] UMCOUNT:%04X                       \t", j, rUMCOUNT.dp[j]);
			}
		}
		printf("\n");
	}

	printf("----\n");
	//for(i = 0; i < 8; i++){
	//	printf("[%01d] ", master_id);
	//	printf("I%01d:%04X ", i,   (unsigned int)(rI[i]   & 0xFFFF));
	//	printf("M%01d:%04X ", i,   (unsigned int)(rM[i]   & 0xFFFF));
	//	printf("L%01d:%04X ", i,   (unsigned int)(rL[i]   & 0xFFFF));
	//	printf("B%01d:%04X ", i,   (unsigned int)(rB[i]   & 0xFFFF));
	//	printf("\n");
	//}

	printf("[%01d] ", master_id);
	for(i = 0; i < 8; i++){
		printf("I%01d:%04X ", i,   (unsigned int)(rI[i]   & 0xFFFF));
	}
	printf("\n");
	printf("[%01d] ", master_id);
	for(i = 0; i < 8; i++){
		printf("M%01d:%04X ", i,   (unsigned int)(rM[i]   & 0xFFFF));
	}
	printf("\n");
	printf("[%01d] ", master_id);
	for(i = 0; i < 8; i++){
		printf("L%01d:%04X ", i,   (unsigned int)(rL[i]   & 0xFFFF));
	}
	printf("\n");
	printf("[%01d] ", master_id);
	for(i = 0; i < 8; i++){
		printf("B%01d:%04X ", i,   (unsigned int)(rB[i]   & 0xFFFF));
	}
	printf("\n");

	printf("[%01d] ", master_id);
	printf("_CNTR:%04X ", (unsigned int)(rCNTR & 0xFFFF));
	printf("_LPEVER:%04X ", (unsigned int)(rLPEVER & 0xFFFF));
	printf("_PCSTACK:%04X ", (unsigned int)(RdReg("_PCSTACK") & 0xFFFF));
	printf("_LPSTACK:%04X\n", (unsigned int)(RdReg("_LPSTACK") & 0xFFFF));

	//if(RdReg("_MSTAT")){
		printf("[%01d] ", master_id);
		printf("_MSTAT - SR:%d BR:%d OL:%d AS:%d MM:%d TI:%d SD:%d MB:%d\n",
			rMstat.SR, rMstat.BR, rMstat.OL, rMstat.AS,
			rMstat.MM, rMstat.TI, rMstat.SD, rMstat.MB);
	//}

	/* print stacks: added 2009.05.25 */
	stackPrint(&PCStack);
	stackPrint(&LoopBeginStack);
	stackPrint(&LoopEndStack);
	stackPrint(&LoopCounterStack);
	stackPrint(&LPEVERStack);
	sStackPrint(&ASTATStack[0]);
	sStackPrint(&ASTATStack[1]);
	sStackPrint(&ASTATStack[2]);
	stackPrint(&MSTATStack);

	printf("[%01d] ", master_id);
	printf("_SSTAT - PCE:%d PCF:%d PCL:%d LSE:%d LSF:%d SSE:%d SOV:%d\n",
		rSstat.PCE, rSstat.PCF, rSstat.PCL, 
		rSstat.LSE, rSstat.LSF, rSstat.SSE, rSstat.SOV);

	printf("[%01d] ", master_id);
	printf("_DSTAT0:%01X ", (unsigned int)(rDstat0 & 0x000F));
	printf("_DSTAT1:%01X ", (unsigned int)(rDstat1 & 0x000F));
	printf("\n");

	if(RdReg("_ICNTL")){
		printf("[%01d] ", master_id);
		printf("_ICNTL - INE:%d GIE:%d\n", rICNTL.INE, rICNTL.GIE);
	}
	if(RdReg("_IMASK")){
		printf("[%01d] ", master_id);
		printf("_IMASK - EMU:%d PWDN:%d SSTEP:%d STACK:%d User:%d%d%d%d %d%d%d%d %d%d%d%d\n",
			rIMASK.EMU, rIMASK.PWDN, rIMASK.SSTEP, rIMASK.STACK,
			rIMASK.UserDef[ 0], rIMASK.UserDef[ 1], rIMASK.UserDef[ 2],
			rIMASK.UserDef[ 3], rIMASK.UserDef[ 4], rIMASK.UserDef[ 5],
			rIMASK.UserDef[ 6], rIMASK.UserDef[ 7], rIMASK.UserDef[ 8],
			rIMASK.UserDef[ 9], rIMASK.UserDef[10], rIMASK.UserDef[11]);
	}
	if(RdReg("_IRPTL")){
		printf("[%01d] ", master_id);
		printf("_IRPTL - EMU:%d PWDN:%d SSTEP:%d STACK:%d User:%d%d%d%d %d%d%d%d %d%d%d%d\n",
			rIRPTL.EMU, rIRPTL.PWDN, rIRPTL.SSTEP, rIRPTL.STACK,
			rIRPTL.UserDef[ 0], rIRPTL.UserDef[ 1], rIRPTL.UserDef[ 2],
			rIRPTL.UserDef[ 3], rIRPTL.UserDef[ 4], rIRPTL.UserDef[ 5],
			rIRPTL.UserDef[ 6], rIRPTL.UserDef[ 7], rIRPTL.UserDef[ 8],
			rIRPTL.UserDef[ 9], rIRPTL.UserDef[10], rIRPTL.UserDef[11]);
	}
	if(RdReg("_IVEC0") + RdReg("_IVEC1") + RdReg("_IVEC2") + RdReg("_IVEC3")){
		printf("[%01d] ", master_id);
		for(i = 0; i < 4; i++){
			printf("_IVEC%01d:%04X ", i,   (unsigned int)(rIVEC[i]   & 0xFFFF));
		}
		printf("\n");
	}

	printf("----\n");
	printf("_PC:%04X NextPC: %04X ", oldPC, PC);
	//printf("CACTL:%04X\n", (unsigned int)(rCACTL & 0xFFFF));

	//added 2009.05.25
	printf("Time: %ld(cycles)\n", Cycles);
}

/** 
* @brief Get address of given symbol label
* 
* @param p Pointer to instruction
* @param htable Symbol table
* @param s Symbol label string
* 
* @return Address of the symbol
*/
int getLabelAddr(sICode *p, sTabList htable[], char *s)
{
	sTab *sp;
	int addr = UNDEFINED;

	sp = sTabHashSearch(htable, s);	/* get symbol table pointer */

	if(sp) addr = sp->Addr;
	else addr = UNDEFINED;
	if(sp) {
		if(sp->Type == tEXTERN){
			/* add to extern reference list */
			if(!sMemRefListSearch(&(sp->MemRefList), p->PMA)){
				sMemRefListAdd(&(sp->MemRefList), p->PMA);
				p->MemoryRefExtern = TRUE;
			}

			/* give warning */
			if(!AssemblerMode){
				printRunTimeWarning(p->LineCntr, sp->Name, 
					"Address of this EXTERN variable is unknown. 0x0 is assumed.\n");
			}
			return (0x0);
		}
		return (sp->Addr);
	}

	return addr;
}

/** 
* @brief Get integer value if integer string OR get symbol address if symbol name.
* 
* @param p Pointer to instruction
* @param htable Symbol table
* @param s Symbol label string
* 
* @return Address of the symbol or integer value
*/
int getIntSymAddr(sICode *p, sTabList htable[], char *s)
{
	sTab *sp;
	int addr = UNDEFINED;

	if(s != NULL){
		if((isdigit(s[0])) || (s[0] == '-')){
			addr = atoi(s);

			if(p->MemoryRefExtern == TRUE){
				/* give warning */

				char *symname = sTabSymNameSearchByReferredAddr(htable, p->PMA);
				if(!AssemblerMode){
					printRunTimeWarning(p->LineCntr, symname, 
						"Address of this EXTERN variable is unknown. 0x0 is assumed.\n");
				}
			}
		}else{
			sp = sTabHashSearch(htable, s);	/* get symbol table pointer */

			/* this is to support .EQU */
			if(sp->isConst){
				if(p->MemoryRefType == tREL)	/* adjust memory reference type to ABS since it's constant */
					p->MemoryRefType = tABS;
			}

			if(sp) {
				if(sp->Type == tEXTERN){
					/* add to extern reference list */
					if(!sMemRefListSearch(&(sp->MemRefList), p->PMA)){
						sMemRefListAdd(&(sp->MemRefList), p->PMA);
						p->MemoryRefExtern = TRUE;
					}

					/* give warning */
					if(!AssemblerMode){
						printRunTimeWarning(p->LineCntr, sp->Name, 
							"Address of this EXTERN variable is unknown. 0x0 is assumed.\n");
					}
					addr = 0x0;
				}else if(sp->Addr == UNDEFINED){
					if(!SuppressUndefinedDMMode)
						printRunTimeError(p->LineCntr, s, 
							"Undefined data memory address."
							"Use \".VAR\" to define a new data memory variable.\n");
				}else
					addr = sp->Addr;
			}
		}
	}
	return addr;
}

/** 
* @brief Get integer value of given fval according to immediate type
* 
* @param p Pointer to instruction
* @param fval Integer value 
* @param immtype eImmType value (i.e. eIMM_INT16, ...)
* 
* @return Integer value
*/
int getIntImm(sICode *p, int fval, int immtype)
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
        case eIMM_UINT2:
            val = 0x3 & fval;
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
                "Parse Error: getIntImm() - Please report.\n");
            break;
    }
    return val;
}


/** 
* @brief Get complex value of given fval according to immediate type
* 
* @param p Pointer to instruction
* @param fval Integer value 
* @param immtype eImmType value (i.e. eIMM_COMPLEX8, ...)
* 
* @return Complex value
*/
cplx getCplxImm(sICode *p, int fval, int immtype)
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
                "Parse Error: getCplxImm() - Please report.\n");
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
sICode *updatePC(sICode *p, sICode *n)
{
	/* skip comment and/or label only lines */
	if(n != NULL){
		while(n != NULL && isCommentLabelInst(n->Index)){
			n = n->Next;
		}
	}

	if(p != NULL) oldPC = p->PMA;		/* save old PC */
	else
		oldPC = UNDEFINED;
	if(n != NULL) PC = n->PMA;	/* new PC */
	else
		PC = UNDEFINED;	/* end of program */

	/* update lineno */
	if(n != NULL) lineno = n->LineCntr;

	return n;
}

/** 
* If MSTAT.M_BIAS(MB) is clear, the multiplier is in Unbiased Round mode.
* 
* @return 1 if MSTAT.M_BIAS is 0, 0 if MSTAT.M_BIAS is 1.
*/
int isUnbiasedRounding(void)
{
	int val = RdReg2((sICode *)NULL, "_MSTAT");

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
	int val = RdReg2((sICode *)NULL, "_MSTAT");

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
	int val = RdReg2((sICode *)NULL, "_MSTAT");

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
	int val = RdReg2((sICode *)NULL, "_MSTAT");

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
//int OVCheck(int type, int x, int y)
//{
//	long long lz;
//	int z;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use CarryCheck() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	switch(type){
//		case t40e: /* MAC */
//		case t40f: /* MAC.C */
//		case t40g: /* MAC.RC */
//		case t41a: /* RNDACC */
//		case t41b: /* RNDACC.C */
//		case t41c: /* CLRACC */
//		case t41d: /* CLRACC.C */
//		case t01a: /* MAC || LD || LD */	
//		case t01b: /* MAC.C || LD.C || LD.C */
//		case t04a: /* MAC || LD */
//		case t04b: /* MAC.C || LD.C */
//		case t04e: /* MAC || ST */
//		case t04f: /* MAC.C || ST.C */
//		case t08a: /* MAC || CP */
//		case t08b: /* MAC.C || CP.C */
//		case t45b: /* MAC || SHIFT */
//		case t45d: /* MAC || SHIFT */
//			lz = (long long)x + (long long)y;
//			if(((lz & 0x0FF800000) == 0x0FF800000)
//				||((lz & 0xFF800000) == 0)){
//				return 0;	/* not overflow */
//			}else{
//				return 1;	/* overflow */
//			}
//			break;
//		case t09c: /* ADD, SUB */
//		case t09e:
//		case t09i:	
//        case t09d: /* ADD.C, SUB.C */
//        case t09f:
//		case t01c: /* ALU || LD || LD */
//		case t04c: /* ALU || LD */
//		case t04d: /* ALU.C || LD.C */
//		case t04g: /* ALU || ST */
//		case t04h: /* ALU.C || ST.C */
//		case t08c: /* ALU || CP */
//		case t08d: /* ALU.C || CP.C */
//		case t43a: /* ALU || MAC */
//		case t44b: /* ALU || SHIFT */
//		case t44d: /* ALU || SHIFT */
//		case t46a: /* SCR */
//		case t46b: /* SCR.C */
//			if(isNeg12b(x))	x |= 0xFFFFF000;
//			if(isNeg12b(y))	y |= 0xFFFFF000;
//			z = x + y;
//
//			if(!isNeg12b(x) && !isNeg12b(y) && isNeg12b(z)){
//				return 1;	/* overflow */
//			} else if(isNeg12b(x) && isNeg12b(y) && !isNeg12b(z)){
//				return 1;	/* overflow */
//			} else {
//				return 0;	/* not overflow */
//			}
//			break;
//		case t09g:		/* ADD, ADD.C with ACC */
//		case t09h:
//			lz = (long long)x + (long long)y;
//
//			if((x > 0) && (y > 0) && (lz < 0)){
//				return 1;	/* overflow */
//			} else if((x < 0) && (y < 0) && (lz > 0)){
//				return 1;	/* overflow */
//			} else {
//				return 0;	/* not overflow */
//			}
//			break;
//		case t16a:		/* ASHIFT, LSHIFT */
//		case t16b:  	/* ASHIFT.C, LSHIFT.C */
//		case t16e:		/* ASHIFT, LSHIFT */
//		case t16f:  	/* ASHIFT.C, LSHIFT.C */
//		case t15a:		/* ASHIFT, LSHIFT by imm */
//		case t15b:  	/* ASHIFT.C, LSHIFT.C by imm */
//		case t15c:		/* ASHIFT, LSHIFT by imm */
//		case t15d:  	/* ASHIFT.C, LSHIFT.C by imm */
//		case t15e:		/* ASHIFT, LSHIFT by imm */
//		case t15f:  	/* ASHIFT.C, LSHIFT.C by imm */
//		case t12e:		/* SHIFT || LD */
//		case t12f:		/* SHIFT.C || LD.C */
//		case t12m:		/* SHIFT || LD */
//		case t12n:		/* SHIFT.C || LD.C */
//		case t12g:		/* SHIFT || ST */
//		case t12h:		/* SHIFT.C || ST.C */
//		case t12o:		/* SHIFT || ST */
//		case t12p:		/* SHIFT.C || ST.C */
//		case t14c:		/* SHIFT || CP */
//		case t14d:		/* SHIFT.C || CP.C */
//		case t14k:		/* SHIFT || CP */
//		case t14l:		/* SHIFT.C || CP.C */
//			lz = (long long)x;
//			if(((lz & 0x0FF800000) == 0x0FF800000)
//				||((lz & 0xFF800000) == 0)){
//				return 0;	/* not overflow */
//			}else{
//				return 1;	/* overflow */
//			}
//			break;
//		default:
//			break;
//	}
//	return 0;	/* default: must not happen */
//}


/** 
* Check if z = x + y results in overflow (for signed-integer arithmetic). (SIMD version)
* Note: this function accesses global variable OVCount.
* 
* @param type Instruction type
* @param x 1st source 
* @param y 2nd source
* 
* @return 1 if overflow occurs, 0 if not
*/
sint sOVCheck(int type, sint x, sint y)
{
	long long lz;
	int z;
	sint ret;

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
			for(int j = 0; j < NUMDP; j++){
				lz = (long long)x.dp[j] + (long long)y.dp[j];
				if(((lz & 0x0FF800000) == 0x0FF800000)
					||((lz & 0xFF800000) == 0)){
					ret.dp[j] = 0;
					//return 0;	/* not overflow */
				}else{
					ret.dp[j] = 1;
					//return 1;	/* overflow */
				}
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
		case t47a:	/* MAG.C */
			for(int j = 0; j < NUMDP; j++){
				if(isNeg12b(x.dp[j]))	x.dp[j] |= 0xFFFFF000;
				if(isNeg12b(y.dp[j]))	y.dp[j] |= 0xFFFFF000;
				z = x.dp[j] + y.dp[j];

				if(!isNeg12b(x.dp[j]) && !isNeg12b(y.dp[j]) && isNeg12b(z)){
					//return 1;	/* overflow */
					ret.dp[j] = 1;
				} else if(isNeg12b(x.dp[j]) && isNeg12b(y.dp[j]) && !isNeg12b(z)){
					//return 1;	/* overflow */
					ret.dp[j] = 1;
				} else {
					//return 0;	/* not overflow */
					ret.dp[j] = 0;
				}
			}
			break;
		case t09g:		/* ADD, ADD.C with ACC */
		case t09h:
			for(int j = 0; j < NUMDP; j++){
				lz = (long long)x.dp[j] + (long long)y.dp[j];

				if((x.dp[j] > 0) && (y.dp[j] > 0) && (lz < 0)){
					//return 1;	/* overflow */
					ret.dp[j] = 1;
				} else if((x.dp[j] < 0) && (y.dp[j] < 0) && (lz > 0)){
					//return 1;	/* overflow */
					ret.dp[j] = 1;
				} else {
					//return 0;	/* not overflow */
					ret.dp[j] = 0;
				}
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
			for(int j = 0; j < NUMDP; j++){
				lz = (long long)x.dp[j];
				if(((lz & 0x0FF800000) == 0x0FF800000)
					||((lz & 0xFF800000) == 0)){
					//return 0;	/* not overflow */
					ret.dp[j] = 0;
				}else{
					//return 1;	/* overflow */
					ret.dp[j] = 1;
				}
			}
			break;
		default:
			break;
	}
	return ret;	
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
//int CarryCheck(sICode *p, int x, int y, int c)
//{
//	int z;
//	long long lz;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use CarryCheck() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	switch(p->InstType){
//		case t09c: /* ADD, SUB */
//		case t09e:
//		case t09i:	
//		case t09d: /* ADD.C, SUB.C */
//		case t09f:
//			switch(p->Index){
//				case iADD:
//				case iADD_C:
//					z = (0xFFF & x) + (0xFFF & y);
//					break;
//				case iADDC:
//				case iADDC_C:
//					z = (0xFFF & x) + (0xFFF & y) + c;
//					break;
//				case iSUB:
//				case iSUB_C:
//					z = (0xFFF & x) + (0xFFF & (~y)) + 1;
//					break;
//				case iSUBC:
//				case iSUBC_C:
//					z = (0xFFF & x) + (0xFFF & (~y)) + c;
//					break;
//				case iSUBB:
//				case iSUBB_C:
//					z = (0xFFF & y) + (0xFFF & (~x)) + 1;
//					break;
//				case iSUBBC:
//				case iSUBBC_C:
//					z = (0xFFF & y) + (0xFFF & (~x)) + c;
//					break;
//				case iINC:
//					z = (0xFFF & x) + 1;
//					break;
//				case iDEC:
//					z = (0xFFF & x)  + (0xFFF & (-1));
//					break;
//				default:
//					/* print error message */
//					printRunTimeError(p->LineCntr, (char *)sOp[p->Index],
//					"Invalid case in CarryCheck() - Please report.\n");
//					return 0;
//					break;
//			}
//			if(z > 0x0FFF || z < 0x0){
//				return 1;
//			}
//			return 0;
//			break;
//		case t09g:		/* ADD, ADD.C with ACC */
//		case t09h:
//			switch(p->Index){
//				case iADD:
//				case iADD_C:
//					lz =  (0xFFFFFFFF & (long long)x) 
//						+ (0xFFFFFFFF & (long long)y);
//					break;
//				case iSUB:
//				case iSUB_C:
//					lz =  (0xFFFFFFFF & (long long)x) 
//						+ (0xFFFFFFFF & (long long)(~y)) + 1;
//					break;
//				case iSUBB:
//				case iSUBB_C:
//					lz =  (0xFFFFFFFF & (long long)y) 
//						+ (0xFFFFFFFF & (long long)(~x)) + 1;
//					break;
//				default:
//					/* print error message */
//					printRunTimeError(p->LineCntr, (char *)sOp[p->Index],
//					"Invalid case in CarryCheck() - Please report.\n");
//					return 0;
//					break;
//			}
//			if(lz > 0x0FFFFFFFF || lz < 0x0){
//				return 1;
//			}
//			return 0;
//			break;
//		default:
//			break;
//	}
//	return 0;	/* default: must not happen */
//}

/** 
* Check if z = x + y generates carry (for unsigned integer arithmetic). (SIMD version)
* 
* @param p Pointer to current instruction
* @param x 1st source 
* @param y 2nd source
* @param c carry(input)
* 
* @return 1 if generates carry(output), 0 if not
*/
sint sCarryCheck(sICode *p, sint x, sint y, sint c)
{
	sint z;
	long long lz[4];
	sint ret1 = { 1, 1, 1, 1 };
	sint ret0 = { 0, 0, 0, 0 };
	sint ret;

	switch(p->InstType){
		case t09c: /* ADD, SUB */
		case t09e:
		case t09i:	
		case t09d: /* ADD.C, SUB.C */
		case t09f:
		case t47a:	/* MAG.C */
			switch(p->Index){
				case iADD:
				case iADD_C:
					for(int j = 0; j < NUMDP; j++) {
						z.dp[j] = (0xFFF & x.dp[j]) + (0xFFF & y.dp[j]);
					}
					break;
				case iADDC:
				case iADDC_C:
					for(int j = 0; j < NUMDP; j++) {
						z.dp[j] = (0xFFF & x.dp[j]) + (0xFFF & y.dp[j]) + c.dp[j];
					}
					break;
				case iSUB:
				case iSUB_C:
					for(int j = 0; j < NUMDP; j++) {
						z.dp[j] = (0xFFF & x.dp[j]) + (0xFFF & (~y.dp[j])) + 1;
					}
					break;
				case iSUBC:
				case iSUBC_C:
					for(int j = 0; j < NUMDP; j++) {
						z.dp[j] = (0xFFF & x.dp[j]) + (0xFFF & (~y.dp[j])) + c.dp[j];
					}
					break;
				case iSUBB:
				case iSUBB_C:
					for(int j = 0; j < NUMDP; j++) {
						z.dp[j] = (0xFFF & y.dp[j]) + (0xFFF & (~x.dp[j])) + 1;
					}
					break;
				case iSUBBC:
				case iSUBBC_C:
					for(int j = 0; j < NUMDP; j++) {
						z.dp[j] = (0xFFF & y.dp[j]) + (0xFFF & (~x.dp[j])) + c.dp[j];
					}
					break;
				case iINC:
					for(int j = 0; j < NUMDP; j++) {
						z.dp[j] = (0xFFF & x.dp[j]) + 1;
					}
					break;
				case iDEC:
					for(int j = 0; j < NUMDP; j++) {
						z.dp[j] = (0xFFF & x.dp[j])  + (0xFFF & (-1));
					}
					break;
				case iMAG_C:
					for(int j = 0; j < NUMDP; j++) {
						z.dp[j] = (0xFFF & x.dp[j]) + (0xFFF & y.dp[j]);
					}
					break;
				default:
					/* print error message */
					printRunTimeError(p->LineCntr, (char *)sOp[p->Index],
					"Invalid case in sCarryCheck() - Please report.\n");
					return ret0;
					break;
			}
			for(int j = 0; j < NUMDP; j++) {
				if(z.dp[j] > 0x0FFF || z.dp[j] < 0x0){
					//return 1;
					ret.dp[j] = 1;
				}else{
					//return 0;
					ret.dp[j] = 0;
				}
			}
			break;
		case t09g:		/* ADD, ADD.C with ACC */
		case t09h:
			switch(p->Index){
				case iADD:
				case iADD_C:
					for(int j = 0; j < NUMDP; j++) {
						lz[j] =  (0xFFFFFFFF & (long long)x.dp[j]) 
							+ (0xFFFFFFFF & (long long)y.dp[j]);
					}
					break;
				case iSUB:
				case iSUB_C:
					for(int j = 0; j < NUMDP; j++) {
						lz[j] =  (0xFFFFFFFF & (long long)x.dp[j]) 
							+ (0xFFFFFFFF & (long long)(~y.dp[j])) + 1;
					}
					break;
				case iSUBB:
				case iSUBB_C:
					for(int j = 0; j < NUMDP; j++) {
						lz[j] =  (0xFFFFFFFF & (long long)y.dp[j]) 
							+ (0xFFFFFFFF & (long long)(~x.dp[j])) + 1;
					}
					break;
				default:
					/* print error message */
					printRunTimeError(p->LineCntr, (char *)sOp[p->Index],
					"Invalid case in CarryCheck() - Please report.\n");
					return ret0;
					break;
			}
			for(int j = 0; j < NUMDP; j++) {
				if(lz[j] > 0x0FFFFFFFF || lz[j] < 0x0){
					//return 1;
					ret.dp[j] = 1;
				}else{
					//return 0;
					ret.dp[j] = 0;
				}
			}
			break;
		default:
			break;
	}
	return ret;
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
* @param addr Address
* 
* @return Data value (12-bit)
*/
//int RdDataMem(unsigned int addr)
//{
//	dMem *dp;
//	int data;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use RdDataMem() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	/* if bit-reversed addressing is enabled */
//	/*
//	if(rMstat.BR){	-> should be modified considering MSTAT latency restriction
//		addr = getBitReversedAddr(addr);
//	}
//	*/
//
//	//if dataSegAddr is defined
//	//addr += dataSegAddr;
//
//	if(!isValidMemoryAddr((int)addr)){
//		data = (0x0FFF & UNDEFINED);
//
//		char tnum[10];
//		sprintf(tnum, "0x%04X", addr);
//		printRunTimeWarning(lineno, tnum, 
//			"Invalid data memory address. Please report.\n");
//		if(VerboseMode) printf("RdDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data);
//		return data;
//	}
//
//	dp = dMemHashSearch(dataMem, addr);
//	if(dp == NULL){	/* if not found */
//		char tnum[10];
//		sprintf(tnum, "0x%04X", addr);
//		dMemHashAdd(dataMem, 0x0FFF & UNDEFINED, addr);
//		printRunTimeWarning(lineno, tnum, 
//			"Undefined data memory address. Use \".VAR\" to define a new data memory variable.\n");
//		data = (0x0FFF & UNDEFINED);
//	} else {
//		data = (0x0FFF & (dp->Data));
//	}
//
//	if(VerboseMode) printf("RdDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data);
//	return data;
//}

/** 
* Read SIMD data from data memory (12-bit x NUMDP). If undefined, add to dMem as a new entry.
* 
* @param addr Address
* 
* @return Data value (12-bit x NUMDP)
*/
sint sRdDataMem(unsigned int addr)
{
	dMem *dp;
	sint data;

	/* if bit-reversed addressing is enabled */
	/*
	if(rMstat.BR){	-> should be modified considering MSTAT latency restriction
		addr = getBitReversedAddr(addr);
	}
	*/

	//if dataSegAddr is defined
	//addr += dataSegAddr;

	if(!isValidMemoryAddr((int)addr)){
		for(int j = 0; j < NUMDP; j++) {
			data.dp[j] = (0x0FFF & UNDEFINED);

			char tnum[10];
			sprintf(tnum, "0x%04X", addr);
			printRunTimeWarning(lineno, tnum, 
				"Invalid data memory address. Please report.\n");
			if(VerboseMode) printf("RdDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data.dp[j]);
		}
		return data;
	}

	dp = dMemHashSearch(dataMem, addr);
	if(dp == NULL){	/* if not found */
		char tnum[10];
		sprintf(tnum, "0x%04X", addr);

		sint sUNDEFINED;
		for(int j = 0; j < NUMDP; j++) {
			sUNDEFINED.dp[j] = 0x0FFF & UNDEFINED;
		}

		dMemHashAdd(dataMem, sUNDEFINED, addr);
		if(!SuppressUndefinedDMMode)
			printRunTimeWarning(lineno, tnum, 
				"Undefined data memory address. Use \".VAR\" to define a new data memory variable.\n");

		data = sUNDEFINED;
	} else {
		for(int j = 0; j < NUMDP; j++) {
			data.dp[j] = (0x0FFF & (dp->Data.dp[j]));
		}
	}

	for(int j = 0; j < NUMDP; j++) {
		if(VerboseMode) printf("RdDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data.dp[j]);
	}

	return data;
}

/** 
* Read 12-bit data from data memory. If undefined, do not add to dMem.
* 
* @param addr Address
* 
* @return Data value (12-bit)
*/
//int briefRdDataMem(unsigned int addr)
//{
//	dMem *dp;
//	int data;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use briefRdDataMem() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	/* if bit-reversed addressing is enabled */
//	/*
//	if(rMstat.BR){	-> should be modified considering MSTAT latency restriction
//		addr = getBitReversedAddr(addr);
//	}
//	*/
//
//	//if dataSegAddr is defined
//	//addr += dataSegAddr;
//
//	if(!isValidMemoryAddr((int)addr)){
//		data = (0x0FFF & UNDEFINED);
//
//		if(VerboseMode) printf("briefRdDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data);
//		return data;
//	}
//
//	dp = dMemHashSearch(dataMem, addr);
//	if(dp == NULL){	/* if not found */
//		//dMemHashAdd(dataMem, 0x0FFF & UNDEFINED, addr);
//		data = (0x0FFF & UNDEFINED);
//	} else {
//		data = (0x0FFF & (dp->Data));
//	}
//
//	if(VerboseMode) printf("briefRdDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data);
//	return data;
//}

/** 
* Read SIMD data from data memory. If undefined, do not add to dMem. (12-bit x NUMDP)
* 
* @param addr Address
* 
* @return Data value (12-bit x NUMDP)
*/
sint sBriefRdDataMem(unsigned int addr)
{
	dMem *dp;
	sint data;

	/* if bit-reversed addressing is enabled */
	/*
	if(rMstat.BR){	-> should be modified considering MSTAT latency restriction
		addr = getBitReversedAddr(addr);
	}
	*/

	//if dataSegAddr is defined
	//addr += dataSegAddr;

	if(!isValidMemoryAddr((int)addr)){
		for(int j = 0; j < NUMDP; j++) {
			data.dp[j] = (0x0FFF & UNDEFINED);

			if(VerboseMode) printf("sBriefRdDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data.dp[j]);
		}
		return data;
	}

	dp = dMemHashSearch(dataMem, addr);
	if(dp == NULL){	/* if not found */
		//dMemHashAdd(dataMem, 0x0FFF & UNDEFINED, addr);
		for(int j = 0; j < NUMDP; j++) {
			data.dp[j] = (0x0FFF & UNDEFINED);
		}
	} else {
		for(int j = 0; j < NUMDP; j++) {
			data.dp[j] = (0x0FFF & (dp->Data.dp[j]));
		}
	}

	for(int j = 0; j < NUMDP; j++) {
		if(VerboseMode) printf("sBriefRdDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data.dp[j]);
	}
	return data;
}

/** 
* Write 12-bit data to data memory. If undefined, add to dMem as a new entry.
* 
* @param data Data to write
* @param addr Address
*/
//void WrDataMem(int data, unsigned int addr)
//{
//	dMem *dp;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use WrDataMem() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	/* if bit-reversed addressing is enabled */
//	/*
//	if(rMstat.BR){	-> should be modified considering MSTAT latency restriction
//		addr = getBitReversedAddr(addr);
//	}
//	*/
//
//	//if dataSegAddr is defined
//	//addr += dataSegAddr;
//
//	if(!isValidMemoryAddr((int)addr)){
//		return;
//	}
//
//	dp = dMemHashSearch(dataMem, addr);
//	if(dp == NULL){	/* if not found: make new var */
//		char tnum[10];
//		sprintf(tnum, "0x%04X", addr);
//		dMemHashAdd(dataMem, 0x0FFF & data, addr);
//		printRunTimeWarning(lineno, tnum, 
//			"Undefined data memory address. Use \".VAR\" to define a new data memory variable.\n");
//	} else {		/* found: update content */
//		dp->Data = data;
//	}
//
//	if(VerboseMode) printf("WrDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data);
//}

/** 
* Write SIMD data to data memory. If undefined, add to dMem as a new entry. (12-bit x NUMDP)
* 
* @param data Data to write
* @param addr Address
* @param mask conditional execution mask
*/
void sWrDataMem(sint data, unsigned int addr, sint mask)
{
	dMem *dp;

	/* if bit-reversed addressing is enabled */
	/*
	if(rMstat.BR){	-> should be modified considering MSTAT latency restriction
		addr = getBitReversedAddr(addr);
	}
	*/

	//if dataSegAddr is defined
	//addr += dataSegAddr;

	if(!isValidMemoryAddr((int)addr)){
		return;
	}

	dp = dMemHashSearch(dataMem, addr);
	if(dp == NULL){	/* if not found: make new var */
		char tnum[10];
		sprintf(tnum, "0x%04X", addr);

		for(int j = 0; j < NUMDP; j++) {
			//if(isScratchPadMemoryAddr((int)addr)){		/* if scratchpad memory */
			//	if(rDPMST.dp[j]){
			//		copy data to all data.dp[k]
			//	}
			//}else 
			if((rDPENA.dp[j] || InitSimMode) && mask.dp[j]){		/* if enabled OR for initDumpIn(): write data */
				data.dp[j] = 0x0FFF & data.dp[j];
			}else{					/* if disabled: write undefined */
				data.dp[j] = UNDEFINED;
			}
		}

		dMemHashAdd(dataMem, data, addr);
		if(!SuppressUndefinedDMMode)
			printRunTimeWarning(lineno, tnum, 
				"Undefined data memory address. Use \".VAR\" to define a new data memory variable.\n");
	} else {		/* found: update content */
		for(int j = 0; j < NUMDP; j++) {
			if((rDPENA.dp[j] || InitSimMode) && mask.dp[j]){		/* if enabled OR for initDumpIn(): write data */
				dp->Data.dp[j] = 0x0FFF & data.dp[j];
			}
		}
	}

	for(int j = 0; j < NUMDP; j++) {
		if((rDPENA.dp[j] || InitSimMode) && mask.dp[j]){		/* if enabled OR for initDumpIn(): write data */
			if(VerboseMode) printf("WrDataMem: (addr: 0x%04X, data 0x%03X)\n", addr, data.dp[j]);
		}
	}
}

/** 
* @brief Compute bit-reversed address for bit-reversed addressing 
* 
* @param addr 16-bit address 
* 
* @return 16-bit bit-reversed address
*/
unsigned int getBitReversedAddr(unsigned int addr)
{
	unsigned int bitRevAddr = 0;
	int mask = 1;
	int hmask = 0x8000;

	for(int i = 0; i < 16; i++){
		if(addr & mask) bitRevAddr |= hmask;
		mask <<= 1;
		hmask >>= 1;
	}

	if(VerboseMode) {
			printf("getBitReversedAddr: 0x%04X -> 0x%04X\n", addr, bitRevAddr);
	}

	return (0xFFFF & bitRevAddr);
}

/*
int RdProgMem(unsigned int addr)
{
	sICode *sp;

	if(!isValidMemoryAddr((int)addr)){
		return (0xFFFF & UNDEFINED);
	}

	sp = sICodeListSearch(&iCode, addr);
	if(sp == NULL){	
		sp = sICodeListAdd(&iCode, iDATA, addr, lineno);
		sp->Data = 0x0FFFF & UNDEFINED;
		return (0x0FFFF & UNDEFINED);
	} else
		return (0x0FFFF & (sp->Data));
}
*/

/*
void WrProgMem(int data, unsigned int addr)
{
	sICode *sp;

	if(!isValidMemoryAddr((int)addr)){
		return;
	}

	sp = sICodeListSearch(&iCode, addr);
	if(sp == NULL){	
		sp = sICodeListAdd(&iCode, iDATA, addr, lineno);
		sp->Data = 0x0FFFF & data;
	} else {	
		sp->Data = 0x0FFFF & data;
	}
}
*/

/** 
* @brief Check if address is negative or above 16-bit boundary
* 
* @param addr data memory address
* 
* @return 1 if valid, 0 if out of boundary
*/
int isValidMemoryAddr(int addr)
{
	char tnum[10];

	if (addr < 0 || addr > 0x0FFFF){
		sprintf(tnum, "%04X", addr);
		printRunTimeError(lineno, tnum, 
			"This address is out of memory space!!\n");
		return FALSE;
	} else
		return TRUE;
}

/** 
* @brief Check if address is in the scratchpad memory region
* 
* @param addr data memory address
* 
* @return 1 if scratchpad, 0 if not
*/
int isScratchPadMemoryAddr(int addr)
{
	char tnum[10];

	if (addr < SCRPAD_LO || addr > SCRPAD_HI){
		return FALSE;
	} else
		return TRUE;
}

/** 
* @brief Check if MSB of 12-bit data is 1 or 0
* 
* @param x 12-bit data
* 
* @return 1 if negative, 0 if not
*/
int isNeg12b(int x)
{
	if(x & 0x800) return 1;
	return 0;
}

/** 
* @brief Check if MSB of 16-bit data is 1 or 0
* 
* @param x 16-bit data
* 
* @return 1 if negative, 0 if not
*/
int isNeg16b(int x)
{
	if(x & 0x8000) return 1;
	return 0;
}

/** 
* @brief Check if MSB of 24-bit data is 1 or 0
* 
* @param x 24-bit data
* 
* @return 1 if negative, 0 if not
*/
int isNeg24b(int x)
{
	if(x & 0x800000) return 1;
	return 0;
}

/** 
* @brief Check if MSB of 8-bit data is 1 or 0
* 
* @param x 8-bit data
* 
* @return 1 if negative, 0 if not
*/
int isNeg8b(int x)
{
	if(x & 0x80) return 1;
	return 0;
}

/** 
* @brief Check if MSB of 32-bit data is 1 or 0
* 
* @param x 32-bit data
* 
* @return 1 if negative, 0 if not
*/
int isNeg32b(int x)
{
	if(x & 0x80000000) return 1;
	return 0;
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
int isBreakpoint(sICode *p)
{
	if(BreakPoint == UNDEFINED) return FALSE;
	if(p->PMA == BreakPoint) return TRUE;
	else return FALSE;
}


/** 
* @brief Check given condition code
* 
* @param s Condition code string
* 
* @return TRUE or FALSE
*/
int	ifCond(char *s)
{
	int j;
	/* first, find MASTER data path */
	for(j = 0; j < NUMDP; j++) {
		if(rDPMST.dp[j]){
			break;
		}
	}

	/* EQ ~ TRUE: checks only ASTAT.R */
	if(!strcasecmp("EQ", s)) {				/* 00000 */
		if(rAstatR.AZ.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("NE", s)) {		/* 00001 */
		if(!rAstatR.AZ.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("GT", s)) {		/* 00010 */
		if(!((rAstatR.AN.dp[j] ^ rAstatR.AV.dp[j]) || rAstatR.AZ.dp[j]))	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("LE", s)) {		/* 00011 */
		if((rAstatR.AN.dp[j] ^ rAstatR.AV.dp[j]) || rAstatR.AZ.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("LT", s)) {		/* 00100 */
		if(rAstatR.AN.dp[j] ^ rAstatR.AV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("GE", s)) {		/* 00101 */
		if(!(rAstatR.AN.dp[j] ^ rAstatR.AV.dp[j]))	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("AV", s)) {		/* 00110 */
		if(rAstatR.AV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("NOT AV", s)) {	/* 00111 */
		if(!rAstatR.AV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("AC", s)) {		/* 01000 */
		if(rAstatR.AC.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("NOT AC", s)) {	/* 01001 */
		if(!rAstatR.AC.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("SV", s)) {		/* 01010 */
		if(rAstatR.SV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("NOT SV", s)) {	/* 01011 */
		if(!rAstatR.SV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("MV", s)) {		/* 01100 */
		if(rAstatR.MV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("NOT MV", s)) {	/* 01101 */
		if(!rAstatR.MV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("NOT CE", s)) {	/* 01110 */
		int ce = RdReg("_CNTR");

		if(ce > 1)	return TRUE;	/* test if CNTR > 1: Refer to */
			                        /* ADSP-219x Inst Set Reference 7-2 */
		else	return FALSE;
	}else if(!strcasecmp("TRUE", s)) {		/* 01111 */
		return TRUE;
	/* EQ.C ~ NOT MV.C: checks ASTAT.C */
	}else if(!strcasecmp("EQ.C", s)) {		/* 10000 */
		if(rAstatC.AZ.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("NE.C", s)) {		/* 10001 */
		if(!rAstatC.AZ.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("GT.C", s)) {		/* 10010 */
		if(!((rAstatC.AN.dp[j] ^ rAstatC.AV.dp[j]) || rAstatC.AZ.dp[j]))	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("LE.C", s)) {		/* 10011 */
		if((rAstatC.AN.dp[j] ^ rAstatC.AV.dp[j]) || rAstatC.AZ.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("LT.C", s)) {		/* 10100 */
		if(rAstatC.AN.dp[j] ^ rAstatC.AV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("GE.C", s)) {		/* 10101 */
		if(!(rAstatC.AN.dp[j] ^ rAstatC.AV.dp[j]))	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("AV.C", s)) {		/* 10110 */
		if(rAstatC.AV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("NOT AV.C", s)) {	/* 10111 */
		if(!rAstatC.AV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("AC.C", s)) {		/* 11000 */
		if(rAstatC.AC.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("NOT AC.C", s)) {	/* 11001 */
		if(!rAstatC.AC.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("SV.C", s)) {		/* 11010 */
		if(rAstatC.SV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("NOT SV.C", s)) {	/* 11011 */
		if(!rAstatC.SV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("MV.C", s)) {		/* 11100 */
		if(rAstatC.MV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("NOT MV.C", s)) {	/* 11101 */
		if(!rAstatC.MV.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("UM.C", s)) {		/* 11110 */
		if(rAstatC.UM.dp[j])	return TRUE;
		else	return FALSE;
	}else if(!strcasecmp("NOT UM.C", s)) {	/* 11111 */
		if(!rAstatC.UM.dp[j])	return TRUE;
		else	return FALSE;
	}
	return FALSE;	/* default */
}


/** 
* @brief Check given condition code for SIMD data paths.
* 
* @param s Condition code string
* @param mask Pointer to condition mask array to store evaluation results
* 
* @return Number of TRUE condition(s), 0 if all FALSE.
*/
int	sIfCond(char *s, sint *mask)
{
	int ret = 0;		/* ret: number of TRUE condition(s) */

	int j;

	for(j = 0; j < NUMDP; j++) {
		if(rDPENA.dp[j]){
			/* EQ ~ TRUE: checks only ASTAT.R */
			if(!strcasecmp("TRUE", s)) {		/* 01111 */
				(*mask).dp[j] = 1;
			}else if(!strcasecmp("EQ", s)) {				/* 00000 */
				if(rAstatR.AZ.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("NE", s)) {		/* 00001 */
				if(!rAstatR.AZ.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("GT", s)) {		/* 00010 */
				if(!((rAstatR.AN.dp[j] ^ rAstatR.AV.dp[j]) || rAstatR.AZ.dp[j]))	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("LE", s)) {		/* 00011 */
				if((rAstatR.AN.dp[j] ^ rAstatR.AV.dp[j]) || rAstatR.AZ.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("LT", s)) {		/* 00100 */
				if(rAstatR.AN.dp[j] ^ rAstatR.AV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("GE", s)) {		/* 00101 */
				if(!(rAstatR.AN.dp[j] ^ rAstatR.AV.dp[j]))	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("AV", s)) {		/* 00110 */
				if(rAstatR.AV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("NOT AV", s)) {	/* 00111 */
				if(!rAstatR.AV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("AC", s)) {		/* 01000 */
				if(rAstatR.AC.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("NOT AC", s)) {	/* 01001 */
				if(!rAstatR.AC.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("SV", s)) {		/* 01010 */
				if(rAstatR.SV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("NOT SV", s)) {	/* 01011 */
				if(!rAstatR.SV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("MV", s)) {		/* 01100 */
				if(rAstatR.MV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("NOT MV", s)) {	/* 01101 */
				if(!rAstatR.MV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("NOT CE", s)) {	/* 01110 */
				int ce = RdReg("_CNTR");
	
				if(ce > 1)	
					(*mask).dp[j] = 1;
										/* test if CNTR > 1: Refer to */
				                        /* ADSP-219x Inst Set Reference 7-2 */
				else	
					(*mask).dp[j] = 0;

			/* EQ.C ~ NOT MV.C: checks ASTAT.C */
			}else if(!strcasecmp("EQ.C", s)) {		/* 10000 */
				if(rAstatC.AZ.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("NE.C", s)) {		/* 10001 */
				if(!rAstatC.AZ.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("GT.C", s)) {		/* 10010 */
				if(!((rAstatC.AN.dp[j] ^ rAstatC.AV.dp[j]) || rAstatC.AZ.dp[j]))	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("LE.C", s)) {		/* 10011 */
				if((rAstatC.AN.dp[j] ^ rAstatC.AV.dp[j]) || rAstatC.AZ.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("LT.C", s)) {		/* 10100 */
				if(rAstatC.AN.dp[j] ^ rAstatC.AV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("GE.C", s)) {		/* 10101 */
				if(!(rAstatC.AN.dp[j] ^ rAstatC.AV.dp[j]))	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("AV.C", s)) {		/* 10110 */
				if(rAstatC.AV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("NOT AV.C", s)) {	/* 10111 */
				if(!rAstatC.AV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("AC.C", s)) {		/* 11000 */
				if(rAstatC.AC.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("NOT AC.C", s)) {	/* 11001 */
				if(!rAstatC.AC.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("SV.C", s)) {		/* 11010 */
				if(rAstatC.SV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("NOT SV.C", s)) {	/* 11011 */
				if(!rAstatC.SV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("MV.C", s)) {		/* 11100 */
				if(rAstatC.MV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("NOT MV.C", s)) {	/* 11101 */
				if(!rAstatC.MV.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("UM.C", s)) {		/* 11110 */
				if(rAstatC.UM.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}else if(!strcasecmp("NOT UM.C", s)) {	/* 11111 */
				if(!rAstatC.UM.dp[j])	
					(*mask).dp[j] = 1;
				else	
					(*mask).dp[j] = 0;
			}
		}
	}

	for(j = 0; j < NUMDP; j++) {
		if(rDPENA.dp[j]){
			ret += (*mask).dp[j];
		}
	}
	return ret;
}


/** 
* @brief Check if multifunction instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isMultiFunc(sICode *p)
{
	if(p->MultiCounter) return TRUE;
	else return FALSE;
}


/** 
* @brief Check if an ALU instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isALU(sICode *p)
{
	int val = FALSE;

	switch(p->Index){
		case iADD:
		case iADDC:
		case iSUB:
		case iSUBC:
		case iSUBB:
		case iSUBBC:
		case iAND:
		case iOR:
		case iXOR:
		case iTSTBIT:
		case iSETBIT:
		case iCLRBIT:
		case iTGLBIT:
		case iNOT:
		case iABS:
		case iINC:
		case iDEC:
		case iSCR:
			val = TRUE;
			break;
		default:
			break;
	}
	return val;
}


/** 
* @brief Check if an ALU.C instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isALU_C(sICode *p)
{
	int val = FALSE;

	switch(p->Index){
		case iADD_C:
		case iADDC_C:
		case iSUB_C:
		case iSUBC_C:
		case iSUBB_C:
		case iSUBBC_C:
		case iNOT_C:
		case iABS_C:
		case iSCR_C:
			val = TRUE;
			break;
		default:
			break;
	}
	return val;
}


/** 
* @brief Check if an MAC instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isMAC(sICode *p)
{
	int val = FALSE;

	switch(p->Index){
		case iMPY:
		case iMAC:
		case iMAS:
			val = TRUE;
			break;
		default:
			break;
	}
	return val;
}


/** 
* @brief Check if an MAC.C instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isMAC_C(sICode *p)
{
	int val = FALSE;

	switch(p->Index){
		case iMPY_C:
		case iMPY_RC:
		case iMAC_C:
		case iMAC_RC:
		case iMAS_C:
		case iMAS_RC:
			val = TRUE;
			break;
		default:
			break;
	}
	return val;
}


/** 
* @brief Check if an MAC instruction (MAC/MAC.C/Multifunction)
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isAnyMAC(sICode *p)
{
	int val = FALSE;

	switch(p->Index){
		case iMPY:
		case iMAC:
		case iMAS:
		case iMPY_C:
		case iMPY_RC:
		case iMAC_C:
		case iMAC_RC:
		case iMAS_C:
		case iMAS_RC:
			val = TRUE;
			break;
		default:
			if(isMACMulti(p))
				val = TRUE;
			break;
	}
	return val;
}


/** 
* @brief Check if a multifunction instruction with MAC/MAC.C operation
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isMACMulti(sICode *p)
{
	int val;
	switch(p->InstType){
		case t01a:		/* MAC || LD || LD or LD || LD */
		case t01b:		/* MAC.C || LD.C || LD.C or LD.C || LD.C */
		case t04a:		/* MAC || LD */
		case t04b:		/* MAC.C || LD.C */
		case t04e:		/* MAC || ST */
		case t04f:		/* MAC.C || ST.C */
		case t08a:		/* MAC || CP */
		case t08b:		/* MAC.C || CP.C */
		case t43a:		/* ALU || MAC */
			val = TRUE;
			break;
		default:
			val = FALSE;
			break;
	}

	return val;
}


/** 
* @brief Check if an SHIFT instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isSHIFT(sICode *p)
{
	int val = FALSE;

	switch(p->Index){
		case iASHIFT:
		case iASHIFTOR:
		case iLSHIFT:
		case iLSHIFTOR:
		/*
		case iNORM:
		case iNORMOR:
		*/
		case iEXP:
		case iEXPADJ:
			val = TRUE;
			break;
		default:
			break;
	}
	return val;
}


/** 
* @brief Check if an SHIFT.C instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isSHIFT_C(sICode *p)
{
	int val = FALSE;

	switch(p->Index){
		case iASHIFT_C:
		case iASHIFTOR_C:
		case iLSHIFT_C:
		case iLSHIFTOR_C:
		/*
		case iNORM_C:
		case iNORMOR_C:
		*/
		case iEXP_C:
		case iEXPADJ_C:
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
int isLD(sICode *p)
{
	int val = FALSE;
	if(p->Index == iLD) val = TRUE;
	return val;
}


/** 
* @brief Check if an LD.C instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isLD_C(sICode *p)
{
	int val = FALSE;
	if(p->Index == iLD_C) val = TRUE;
	return val;
}


/** 
* @brief Check if an ST instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isST(sICode *p)
{
	int val = FALSE;
	if(p->Index == iST) val = TRUE;
	return val;
}


/** 
* @brief Check if an ST.C instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isST_C(sICode *p)
{
	int val = FALSE;
	if(p->Index == iST_C) val = TRUE;
	return val;
}


/** 
* @brief Check if an CP instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isCP(sICode *p)
{
	int val = FALSE;
	if(p->Index == iCP) val = TRUE;
	return val;
}


/** 
* @brief Check if an CP.C instruction
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isCP_C(sICode *p)
{
	int val = FALSE;
	if(p->Index == iCP_C) val = TRUE;
	return val;
}

/** 
* @brief Check if a multifunction instruction with LD/ST/LD.C/ST.C operation
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isLDSTMulti(sICode *p)
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
* @brief Check if a multifunction instruction with LD/LD.C operation
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isLDMulti(sICode *p)
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
			val = TRUE;
			break;
		default:
			val = FALSE;
			break;
	}

	return val;
}

/** 
* @brief Check if a multifunction instruction with ST/ST.C operation
* 
* @param p pointer to instruction
* 
* @return TRUE or FALSE
*/
int isSTMulti(sICode *p)
{
	int val;
	switch(p->InstType){
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
* @param ln Line number
* @param s Pointer to warning string
* @param msg Warning message
*/
void printRunTimeWarning(int ln, char *s, char *msg)
{
	static int lastLn = -1;
	if(lastLn != ln) {
		sprintf(msgbuf, "\nLine %d: Warning: %s - %s", ln, s, msg);
		printf("\nLine %d: Warning: %s - %s", ln, s, msg);
		fprintf(dumpErrFP, "\nLine %d: Warning: %s - %s", ln, s, msg);
		lastLn = ln;
	}
}

/** 
* @brief Print run-time error message to message buffer
* 
* @param ln Line number
* @param s Pointer to error string
* @param msg Error message
*/
void printRunTimeError(int ln, char *s, char *msg)
{
	static int lastLn = -1;
	if(lastLn != ln) {	
		sprintf(msgbuf, "\nLine %d: Error: %s - %s", ln, s, msg);
		printf("\nLine %d: Error: %s - %s", ln, s, msg);
		fprintf(dumpErrFP, "\nLine %d: Error: %s - %s", ln, s, msg);
		lastLn = ln;
	}

	if(!AssemblerMode){
		/* added 2009.11.04 */
		printf("\n----\nPC:%04X Line %d\n", oldPC, ln);
		dumpRegister((sICode *)NULL);
	}
	printf("\n----\nRun-Time Error: Program ended unexpectedly.\n");
	printf("Line %d: %s - %s\n\n", ln, s, msg);

	fprintf(dumpErrFP, "\n----\nRun-Time Error: Program ended unexpectedly.\n");
	fprintf(dumpErrFP, "Line %d: %s - %s\n\n", ln, s, msg);

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
* @brief Given operand string, get DREG12 code and string.
* 
* @param ret String buffer to write DREG12 code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return DREG12 code (6 bits)
*/
int getCodeDReg12(char *ret, char *s)
{
	int val;

	if(s[0] == 'R' || s[0] == 'r') {		/* REG12: Rx register */
		val = atoi(s+1);
	}else if(!strncasecmp(s, "ACC", 3)) {	/* ACC12: Accumulator */
		val = 32;

		int x = (int)(s[3] - '0');			/* x: 0/1/2/3/4/5/6/7 */

		switch(x){
			case 0:
				val += 0;
				break;
			case 1:
				val += 1;
				break;
			case 2:
				val += 8;
				break;
			case 3:
				val += 9;
				break;
			case 4:
				val += 16;
				break;
			case 5:
				val += 17;
				break;
			case 6:
				val += 24;
				break;
			case 7:
				val += 25;
				break;
		}

		if(s[4] == '.'){	/* ACCx.H, M, L */
			switch(s[5]){
				case 'L':
				case 'l':
					val += 0;
					break;
				case 'M':
				case 'm':
					val += 2;
					break;
				case 'H':
				case 'h':
					val += 4;
					break;
				default:		/* must not happen */
					val = 0;
					break;	
			}
		} else if(s[4] == '\0'){	/* ACCx */
			val += 6;
		}
	} else if(!strcasecmp("NONE", s)) {		/* NONE */
		val = 63;
	}				

	if(ret) int2bin(ret, val, 6);
	return val;
}

/** 
* @brief Given XOP12 string, get 5-bit code and string.
* 
* @param ret String buffer to write XOP12 code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return XOP12 code (5 bits)
*/
int getCodeXOP12(char *ret, char *s)
{
	int val;

	val = getCodeReg12(NULL, s);
	
	if(ret) int2bin(ret, val, 5);
	return val;
}

/** 
* @brief Given operand string, get DREG12S code and string.
* 
* @param ret String buffer to write DREG12S code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return DREG12S code (4 bits)
*/
int getCodeDReg12S(char *ret, char *s)
{
	int val;

	if(s[0] == 'R' || s[0] == 'r') {		/* REG12S: R0~R7 */
		val = atoi(s+1);
	}else if(!strncasecmp(s, "ACC", 3)) {	/* ACC12S: ACC0.L, ACC1.L, ..., ACC7.L */
		val = 8;

		int x = (int)(s[3] - '0');
		val += x;
	}				

	if(ret) int2bin(ret, val, 4);
	return val;
}

/** 
* @brief Given operand string, get REG12 code and string.
* 
* @param ret String buffer to write REG12 code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return REG12 code (5 bits)
*/
int getCodeReg12(char *ret, char *s)
{
	int val;

	if(s[0] == 'R' || s[0] == 'r') {		/* REG12: Rx register */
		val = atoi(s+1);
	}				

	if(ret) int2bin(ret, val, 5);
	return val;
}

/** 
* @brief Given operand string, get REG12S code and string.
* 
* @param ret String buffer to write REG12S code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return REG12S code (3 bits)
*/
int getCodeReg12S(char *ret, char *s)
{
	int val;

	if(s[0] == 'R' || s[0] == 'r') {		/* REG12S: Rx register */
		val = atoi(s+1);
	}				

	if(ret) int2bin(ret, val, 3);
	return val;
}

/** 
* @brief Given operand string, get REG24 code and string.
* 
* @param ret String buffer to write REG24 code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return REG24 code (4 bits)
*/
int getCodeReg24(char *ret, char *s)
{
	int val;

	if(s[0] == 'R' || s[0] == 'r') {		/* REG24: Rx register */
		val = atoi(s+1);
	}				
	val >>= 1;

	if(ret) int2bin(ret, val, 4);
	return val;
}

/** 
* @brief Given operand string, get REG24S code and string.
* 
* @param ret String buffer to write REG24S code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return REG24S code (2 bits)
*/
int getCodeReg24S(char *ret, char *s)
{
	int val;

	if(s[0] == 'R' || s[0] == 'r') {		/* REG24S: Rx register */
		val = atoi(s+1);
	}				
	val >>= 1;

	if(ret) int2bin(ret, val, 2);
	return val;
}

/** 
* @brief Given operand string, get DREG24 code and string.
* 
* @param ret String buffer to write DREG24 code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return DREG24 code (5 bits)
*/
int getCodeDReg24(char *ret, char *s)
{
	int val;

	if(s[0] == 'R' || s[0] == 'r') {		/* REG24: Rx register */
		val = atoi(s+1);
		val >>= 1;
	}else if(!strncasecmp(s, "ACC", 3)) {	/* ACC24: Accumulator */
		val = 16;

		int x = (int)(s[3] - '0');
		val += (x << 1);

		if(s[4] == '.'){	/* ACCx.H, M, L */
			switch(s[5]){
				case 'H':
				case 'h':
					val += 2;
					break;
				case 'M':
				case 'm':
					val += 1;
					break;
				case 'L':
				case 'l':
					val += 0;
					break;
				default:		/* must not happen */
					val = 0;
					break;	
			}
		}
	} else if(!strcasecmp("NONE", s)) {		/* NONE */
		val = 31;
	}				

	if(ret) int2bin(ret, val, 5);
	return val;
}

/** 
* @brief Given operand string, get DREG24S code and string.
* 
* @param ret String buffer to write DREG24S code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return DREG24S code (3 bits)
*/
int getCodeDReg24S(char *ret, char *s)
{
	int val;

	if(s[0] == 'R' || s[0] == 'r') {		/* REG24S: Rx register */
		val = atoi(s+1);
		val >>= 1;
	}else if(!strncasecmp(s, "ACC", 3)) {	/* ACC24S: Accumulator */
		val = 4;

		int x = (int)(s[3] - '0');
		val += (x << 1);
	}				

	if(ret) int2bin(ret, val, 3);
	return val;
}

/** 
* @brief Given XOP24 string, get 4-bit code and string.
* 
* @param ret String buffer to write XOP24 code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return XOP24 code (4 bits)
*/
int getCodeXOP24(char *ret, char *s)
{
	int val;

	val = getCodeReg24(NULL, s);
	
	if(ret) int2bin(ret, val, 4);
	return val;
}

/** 
* @brief Given operand string, get ACC32 code and string.
* 
* @param ret String buffer to write ACC32 code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return ACC32 code (3 bits)
*/
int getCodeACC32(char *ret, char *s)
{
	int val;

	if(!strncasecmp(s, "ACC", 3)) {	/* ACC32: Accumulator */
		val = (int)(s[3] - '0');
	}				

	if(ret) int2bin(ret, val, 3);
	return val;
}

/** 
* @brief Given operand string, get ACC32S code and string.
* 
* @param ret String buffer to write ACC32S code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return ACC32S code (2 bits)
*/
int getCodeACC32S(char *ret, char *s)
{
	int val;

	if(!strncasecmp(s, "ACC", 3)) {	/* ACC32S: Accumulator */
		val = (int)(s[3] - '0');
	}				

	if(ret) int2bin(ret, val, 2);
	return val;
}

/** 
* @brief Given operand string, get ACC64 code and string.
* 
* @param ret String buffer to write ACC64 code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return ACC64 code (2 bits)
*/
int getCodeACC64(char *ret, char *s)
{
	int val;

	if(!strncasecmp(s, "ACC", 3)) {	/* ACC64: complex accumulator */
		val = (int)(s[3] - '0');
	}				
	val >>= 1;

	if(ret) int2bin(ret, val, 2);
	return val;
}

/** 
* @brief Given operand string, get ACC64S code and string.
* 
* @param ret String buffer to write ACC64S code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return ACC64S code (2 bit)
*/
int getCodeACC64S(char *ret, char *s)
{
	int val;

	if(!strncasecmp(s, "ACC", 3)) {	/* ACC64S: complex accumulator */
		val = (int)(s[3] - '0');
	}				
	val >>= 1;

	if(ret) int2bin(ret, val, 2);
	return val;
}

/** 
* @brief Given COND string, get 5-bit code and string.
* 
* @param ret String buffer to write COND code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return COND code (5 bits)
*/
int	getCodeCOND(char *ret, char *s)
{
	int val;

	/* EQ ~ TRUE: checks only ASTAT.R */
	if(!s) {								/* if COND == NULL, TRUE: 01111 */
		val = 15;
	} else if(!strcasecmp("EQ", s)) {		/* 00000 */
		val = 0;
	}else if(!strcasecmp("NE", s)) {		/* 00001 */
		val = 1;
	}else if(!strcasecmp("GT", s)) {		/* 00010 */
		val = 2;
	}else if(!strcasecmp("LE", s)) {		/* 00011 */
		val = 3;
	}else if(!strcasecmp("LT", s)) {		/* 00100 */
		val = 4;
	}else if(!strcasecmp("GE", s)) {		/* 00101 */
		val = 5;
	}else if(!strcasecmp("AV", s)) {		/* 00110 */
		val = 6;
	}else if(!strcasecmp("NOT AV", s)) {	/* 00111 */
		val = 7;
	}else if(!strcasecmp("AC", s)) {		/* 01000 */
		val = 8;
	}else if(!strcasecmp("NOT AC", s)) {	/* 01001 */
		val = 9;
	}else if(!strcasecmp("SV", s)) {		/* 01010 */
		val = 10;
	}else if(!strcasecmp("NOT SV", s)) {	/* 01011 */
		val = 11;
	}else if(!strcasecmp("MV", s)) {		/* 01100 */
		val = 12;
	}else if(!strcasecmp("NOT MV", s)) {	/* 01101 */
		val = 13;
	}else if(!strcasecmp("NOT CE", s)) {	/* 01110 */
		val = 14;
	}else if(!strcasecmp("TRUE", s)) {		/* 01111 */
		val = 15;
	}else if(!strcasecmp("EQ.C", s)) {		/* 10000 */
		val = 16;
	}else if(!strcasecmp("NE.C", s)) {		/* 10001 */
		val = 17;
	}else if(!strcasecmp("GT.C", s)) {		/* 10010 */
		val = 18;
	}else if(!strcasecmp("LE.C", s)) {		/* 10011 */
		val = 19;
	}else if(!strcasecmp("LT.C", s)) {		/* 10100 */
		val = 20;
	}else if(!strcasecmp("GE.C", s)) {		/* 10101 */
		val = 21;
	}else if(!strcasecmp("AV.C", s)) {		/* 10110 */
		val = 22;
	}else if(!strcasecmp("NOT AV.C", s)) {	/* 10111 */
		val = 23;
	}else if(!strcasecmp("AC.C", s)) {		/* 11000 */
		val = 24;
	}else if(!strcasecmp("NOT AC.C", s)) {	/* 11001 */
		val = 25;
	}else if(!strcasecmp("SV.C", s)) {		/* 11010 */
		val = 26;
	}else if(!strcasecmp("NOT SV.C", s)) {	/* 11011 */
		val = 27;
	}else if(!strcasecmp("MV.C", s)) {		/* 11100 */
		val = 28;
	}else if(!strcasecmp("NOT MV.C", s)) {	/* 11101 */
		val = 29;
	}else if(!strcasecmp("UM.C", s)) {		/* 11110 */
		val = 30;
	}else if(!strcasecmp("NOT UM.C", s)) {	/* 11111 */
		val = 31;
	}

	if(ret) int2bin(ret, val, 5);
	return val;
}

/** 
* @brief Given RREG16 string, get 6-bit code and string.
* 
* @param ret String buffer to write RREG16 code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return RREG16 code (6 bits)
*/
int getCodeRReg16(char *ret, char *s)
{
	int val;

	if(((s[0] == 'I') || (s[0] == 'i')) && isdigit(s[1])) {		/* Ix register */
		val = 0;
		val += (int)(s[1] - '0');
	}else if(((s[0] == 'M') || (s[0] == 'm')) && isdigit(s[1])) {		/* Mx register */
		val = 8;
		val += (int)(s[1] - '0');
	}else if(((s[0] == 'L') || (s[0] == 'l')) && isdigit(s[1])) {		/* Lx register */
		val = 16;
		val += (int)(s[1] - '0');
	}else if(((s[0] == 'B') || (s[0] == 'b')) && isdigit(s[1])) {		/* Bx register */
		val = 24;
		val += (int)(s[1] - '0');
	}else if(!strcasecmp(s, "_CNTR")){			/* _CNTR */
		val = 32;
	}else if(!strcasecmp(s, "_MSTAT")){			/* _MSTAT */
		val = 33;
	}else if(!strcasecmp(s, "_SSTAT")){			/* _SSTAT */
		val = 34;
	}else if(!strcasecmp(s, "_LPSTACK")){		/* _LPSTACK */
		val = 35;
	}else if(!strcasecmp(s, "_PCSTACK")){		/* _PCSTACK */
		val = 36;
	}else if(!strcasecmp(s, "ASTAT.R")){		/* ASTAT.R */
		val = 37;
	}else if(!strcasecmp(s, "ASTAT.I")){		/* ASTAT.I */
		val = 38;
	}else if(!strcasecmp(s, "ASTAT.C")){		/* ASTAT.C */
		val = 39;
	}else if(!strcasecmp(s, "_ICNTL")){			/* _ICNTL */
		val = 40;
	}else if(!strcasecmp(s, "_IMASK")){			/* _IMASK */
		val = 41;
	}else if(!strcasecmp(s, "_IRPTL")){			/* _IRPTL */
		val = 42;
	//}else if(!strcasecmp(s, "_CACTL")){		/* _CACTL */
	//	val = 43;
	//}else if(!strcasecmp(s, "_PX")){			/* _PX */
	//	val = 44;
	}else if(!strcasecmp(s, "_LPEVER")){		/* _LPEVER */
		val = 45;
	}else if(!strcasecmp(s, "_DSTAT0")){		/* _DSTAT0 */
		val = 46;
	}else if(!strcasecmp(s, "_DSTAT1")){		/* _DSTAT1 */
		val = 47;
	}else if(!strcasecmp(s, "_IVEC0")){			/* _IVEC0 */
		val = 48;
	}else if(!strcasecmp(s, "_IVEC1")){			/* _IVEC1 */
		val = 49;
	}else if(!strcasecmp(s, "_IVEC2")){			/* _IVEC2 */
		val = 50;
	}else if(!strcasecmp(s, "_IVEC3")){			/* _IVEC3 */
		val = 51;
	}else if(!strcasecmp(s, "UMCOUNT")){		/* UMCOUNT */
		val = 56;
	}else if(!strcasecmp(s, "DID")){				/* DID */
		val = 57;
	}

	if(ret) int2bin(ret, val, 6);
	return val;
}

/** 
* @brief Given RREG string, get 7-bit code and string.
* 
* @param p Pointer to instruction
* @param ret String buffer to write RREG code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return RREG code (7 bits)
*/
int getCodeRReg(sICode *p, char *ret, char *s)
{
	int val;

	if(isDReg12(p, s)){		/* DREG12 */
		val = getCodeDReg12(NULL, s);
	}else{					/* RREG16 */
		val = getCodeRReg16(NULL, s);
		val += 0x40;
	}
	
	if(ret) int2bin(ret, val, 7);
	return val;
}

/** 
* @brief Given CReg string, get 5-bit code and string.
* 
* @param ret String buffer to write CReg code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return CReg code (5 bits)
*/
int getCodeCReg(char *ret, char *s)
{
	int val;

	val = getCodeDReg24(NULL, s);
	
	if(ret) int2bin(ret, val, 5);
	return val;
}

/** 
* @brief Given operand string, get XREG12 code and string.
* 
* @param ret String buffer to write XREG12 code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return XREG12 code (3 bits)
*/
int getCodeXReg12(char *ret, char *s)
{
	int val;

	if(s[0] == 'R' || s[0] == 'r') {		/* REG12: Rx register */
		val = atoi(s+1);
		val -= 24;							/* XREG12: R24 ~ R31 */
	}				

	if(ret) int2bin(ret, val, 3);
	return val;
}

/** 
* @brief Given operand string, get XREG24 code and string.
* 
* @param ret String buffer to write XREG24 code
* @param s Given operand string (e.g. Opr0, Opr1, ...)
* 
* @return XREG24 code (2 bits)
*/
int getCodeXReg24(char *ret, char *s)
{
	int val;

	if(s[0] == 'R' || s[0] == 'r') {		/* REG24: Rx register */
		val = atoi(s+1);
		val -= 24;							/* XREG24: R24, R26, R28, R30 */
		val >>= 1;
	}				

	if(ret) int2bin(ret, val, 2);
	return val;
}

/** 
* @brief Given opcode index, get AMF code and string.
* 
* @param ret String buffer to write AMF code
* @param p Pointer to current instruction
*/
void getCodeAMF(char *ret, sICode *p)
{
	switch(p->Index){
		case iMPY:
		case iMPY_C:
		case iMPY_RC:
			if(!strcasecmp(p->Operand[3], "RND")){
				strcpy(ret, "00001");			
			} else if(!strcasecmp(p->Operand[3], "SS")){
				strcpy(ret, "00100");			
			} else if(!strcasecmp(p->Operand[3], "SU")){
				strcpy(ret, "00101");			
			} else if(!strcasecmp(p->Operand[3], "US")){
				strcpy(ret, "00110");			
			} else if(!strcasecmp(p->Operand[3], "UU")){
				strcpy(ret, "00111");			
			} 
			break;
		case iMAC:
		case iMAC_C:
		case iMAC_RC:
			if(!strcasecmp(p->Operand[3], "RND")){
				strcpy(ret, "00010");			
			} else if(!strcasecmp(p->Operand[3], "SS")){
				strcpy(ret, "01000");			
			} else if(!strcasecmp(p->Operand[3], "SU")){
				strcpy(ret, "01001");			
			} else if(!strcasecmp(p->Operand[3], "US")){
				strcpy(ret, "01010");			
			} else if(!strcasecmp(p->Operand[3], "UU")){
				strcpy(ret, "01011");			
			} 
			break;
		case iMAS:
		case iMAS_C:
		case iMAS_RC:
			if(!strcasecmp(p->Operand[3], "RND")){
				strcpy(ret, "00011");			
			} else if(!strcasecmp(p->Operand[3], "SS")){
				strcpy(ret, "01100");			
			} else if(!strcasecmp(p->Operand[3], "SU")){
				strcpy(ret, "01101");			
			} else if(!strcasecmp(p->Operand[3], "US")){
				strcpy(ret, "01110");			
			} else if(!strcasecmp(p->Operand[3], "UU")){
				strcpy(ret, "01111");			
			} 
			break;
		case iADD:
		case iADD_C:
			strcpy(ret, "10000");			
			break;
		case iADDC:
		case iADDC_C:
			strcpy(ret, "10001");			
			break;
		case iSUB:
		case iSUB_C:
			strcpy(ret, "10010");			
			break;
		case iSUBC:
		case iSUBC_C:
			strcpy(ret, "10011");			
			break;
		case iSUBB:
		case iSUBB_C:
			strcpy(ret, "10100");			
			break;
		case iSUBBC:
		case iSUBBC_C:
			strcpy(ret, "10101");			
			break;
		case iAND:
			strcpy(ret, "10110");			
			break;
		case iOR:
			strcpy(ret, "10111");			
			break;
		case iXOR:						
			strcpy(ret, "11000");			
			break;
		case iTSTBIT:
		case iCLRBIT:
			strcpy(ret, "11001");			
			break;
		case iSETBIT:
			strcpy(ret, "11010");			
			break;
		case iTGLBIT:
			strcpy(ret, "11011");			
			break;
		case iNOT:
		case iNOT_C:
			strcpy(ret, "11100");			
			break;
		case iABS:
		case iABS_C:
			strcpy(ret, "11101");			
			break;
		case iINC:
			strcpy(ret, "11110");			
			break;
		case iDEC:
			strcpy(ret, "11111");			
			break;
		default:
			strcpy(ret, "00000");	/* Multiplier NOP */
			break;
	}
}

/** 
* @brief Given opcode index, get AF code and string.
* 
* @param ret String buffer to write AF code
* @param p Pointer to current instruction
*/
void getCodeAF(char *ret, sICode *p)
{
	switch(p->Index){
		case iADD:
		case iADD_C:
			strcpy(ret, "0000");			
			break;
		case iADDC:
		case iADDC_C:
			strcpy(ret, "0001");			
			break;
		case iSUB:
		case iSUB_C:
			strcpy(ret, "0010");			
			break;
		case iSUBC:
		case iSUBC_C:
			strcpy(ret, "0011");			
			break;
		case iSUBB:
		case iSUBB_C:
			strcpy(ret, "0100");			
			break;
		case iSUBBC:
		case iSUBBC_C:
			strcpy(ret, "0101");			
			break;
		case iAND:
			strcpy(ret, "0110");			
			break;
		case iOR:
			strcpy(ret, "0111");			
			break;
		case iXOR:						
			strcpy(ret, "1000");			
			break;
		case iTSTBIT:
		case iCLRBIT:
			strcpy(ret, "1001");			
			break;
		case iSETBIT:
			strcpy(ret, "1010");			
			break;
		case iTGLBIT:
			strcpy(ret, "1011");			
			break;
		case iNOT:
		case iNOT_C:
			strcpy(ret, "1100");			
			break;
		case iABS:
		case iABS_C:
			strcpy(ret, "1101");			
			break;
		case iINC:
			strcpy(ret, "1110");			
			break;
		case iDEC:
			strcpy(ret, "1111");			
			break;
		default:
			break;
	}
}

/** 
* @brief Given opcode index, get MF code and string.
* 
* @param ret String buffer to write MF code
* @param p Pointer to current instruction
*/
void getCodeMF(char *ret, sICode *p)
{
	switch(p->Index){
		case iMPY:
		case iMPY_C:
		case iMPY_RC:
			if(!strcasecmp(p->Operand[3], "RND")){
				strcpy(ret, "0001");			
			} else if(!strcasecmp(p->Operand[3], "SS")){
				strcpy(ret, "0100");			
			} else if(!strcasecmp(p->Operand[3], "SU")){
				strcpy(ret, "0101");			
			} else if(!strcasecmp(p->Operand[3], "US")){
				strcpy(ret, "0110");			
			} else if(!strcasecmp(p->Operand[3], "UU")){
				strcpy(ret, "0111");			
			} 
			break;
		case iMAC:
		case iMAC_C:
		case iMAC_RC:
			if(!strcasecmp(p->Operand[3], "RND")){
				strcpy(ret, "0010");			
			} else if(!strcasecmp(p->Operand[3], "SS")){
				strcpy(ret, "1000");			
			} else if(!strcasecmp(p->Operand[3], "SU")){
				strcpy(ret, "1001");			
			} else if(!strcasecmp(p->Operand[3], "US")){
				strcpy(ret, "1010");			
			} else if(!strcasecmp(p->Operand[3], "UU")){
				strcpy(ret, "1011");			
			} 
			break;
		case iMAS:
		case iMAS_C:
		case iMAS_RC:
			if(!strcasecmp(p->Operand[3], "RND")){
				strcpy(ret, "0011");			
			} else if(!strcasecmp(p->Operand[3], "SS")){
				strcpy(ret, "1100");			
			} else if(!strcasecmp(p->Operand[3], "SU")){
				strcpy(ret, "1101");			
			} else if(!strcasecmp(p->Operand[3], "US")){
				strcpy(ret, "1110");			
			} else if(!strcasecmp(p->Operand[3], "UU")){
				strcpy(ret, "1111");			
			} 
			break;
		default:
			strcpy(ret, "0000");	/* Multiplier NOP */
			break;
	}
}

/** 
* @brief Given opcode index, get SF code and string.
* 
* @param ret String buffer to write SF code
* @param opr Pointer to operand string
* @param index Instruction index
*/
void getCodeSF(char *ret, char *opr, unsigned int index)
{
	switch(index){
		case iLSHIFT:
		case iLSHIFT_C:
			if(!strcasecmp(opr, "HI"))		
				strcpy(ret, "00000");			
			else if(!strcasecmp(opr, "LO"))
				strcpy(ret, "00010");			
			else if(!strcasecmp(opr, "NORND"))
				strcpy(ret, "01000");			
			else if(!strcasecmp(opr, "HIRND"))
				strcpy(ret, "10000");			
			else if(!strcasecmp(opr, "LORND"))
				strcpy(ret, "10010");			
			else if(!strcasecmp(opr, "RND"))
				strcpy(ret, "11000");			
			break;
		case iLSHIFTOR:
		case iLSHIFTOR_C:
			if(!strcasecmp(opr, "HI"))
				strcpy(ret, "00001");			
			else if(!strcasecmp(opr, "LO"))
				strcpy(ret, "00011");			
			else if(!strcasecmp(opr, "NORND"))
				strcpy(ret, "01001");			
			else if(!strcasecmp(opr, "HIRND"))
				strcpy(ret, "10001");			
			else if(!strcasecmp(opr, "LORND"))
				strcpy(ret, "10011");			
			else if(!strcasecmp(opr, "RND"))
				strcpy(ret, "11001");			
			break;
		case iASHIFT:
		case iASHIFT_C:
			if(!strcasecmp(opr, "HI"))		
				strcpy(ret, "00100");			
			else if(!strcasecmp(opr, "LO"))
				strcpy(ret, "00110");			
			else if(!strcasecmp(opr, "NORND"))
				strcpy(ret, "01010");			
			else if(!strcasecmp(opr, "HIRND"))
				strcpy(ret, "10100");			
			else if(!strcasecmp(opr, "LORND"))
				strcpy(ret, "10110");			
			else if(!strcasecmp(opr, "RND"))
				strcpy(ret, "11010");			
			break;
		case iASHIFTOR:
		case iASHIFTOR_C:
			if(!strcasecmp(opr, "HI"))
				strcpy(ret, "00101");			
			else if(!strcasecmp(opr, "LO"))
				strcpy(ret, "00111");			
			else if(!strcasecmp(opr, "NORND"))
				strcpy(ret, "01011");			
			else if(!strcasecmp(opr, "HIRND"))
				strcpy(ret, "10101");			
			else if(!strcasecmp(opr, "LORND"))
				strcpy(ret, "10111");			
			else if(!strcasecmp(opr, "RND"))
				strcpy(ret, "11011");			
			break;
		case iEXP:
		case iEXP_C:
			if(!strcasecmp(opr, "HI"))
				strcpy(ret, "01100");			
			else if(!strcasecmp(opr, "HIX"))
				strcpy(ret, "01101");			
			else if(!strcasecmp(opr, "LO"))
				strcpy(ret, "01110");			
			break;
		case iEXPADJ:
		case iEXPADJ_C:
			strcpy(ret, "01111");
			break;
		default:
			break;
	}
}

/** 
* @brief Given opcode index, get SF2 code and string.
* 
* @param ret String buffer to write SF2 code
* @param opr Pointer to operand string
* @param index Instruction index
*/
void getCodeSF2(char *ret, char *opr, unsigned int index)
{
	switch(index){
		case iLSHIFT:
		case iLSHIFT_C:
			if(!strcasecmp(opr, "HI"))		
				strcpy(ret, "0000");			
			else if(!strcasecmp(opr, "HIRND"))
				strcpy(ret, "0001");			
			else if(!strcasecmp(opr, "LO"))
				strcpy(ret, "0010");			
			else if(!strcasecmp(opr, "LORND"))
				strcpy(ret, "0011");			
			else if(!strcasecmp(opr, "NORND"))
				strcpy(ret, "1000");			
			else if(!strcasecmp(opr, "RND"))
				strcpy(ret, "1001");			
			break;
		case iASHIFT:
		case iASHIFT_C:
			if(!strcasecmp(opr, "HI"))		
				strcpy(ret, "0100");			
			else if(!strcasecmp(opr, "HIRND"))
				strcpy(ret, "0101");			
			else if(!strcasecmp(opr, "LO"))
				strcpy(ret, "0110");			
			else if(!strcasecmp(opr, "LORND"))
				strcpy(ret, "0111");			
			else if(!strcasecmp(opr, "NORND"))
				strcpy(ret, "1010");			
			else if(!strcasecmp(opr, "RND"))
				strcpy(ret, "1011");			
			break;
		default:
			break;
	}
}

/** 
* @brief Given opcode index, get LSF code and string.
* 
* @param ret String buffer to write LSF code
* @param opr Pointer to operand string
* @param index Instruction index
*/
void getCodeLSF(char *ret, char *opr, unsigned int index)
{
	if(!strcasecmp(opr, "HI"))
		strcpy(ret, "1");			
	else if(!strcasecmp(opr, "LO"))
		strcpy(ret, "0");			
}

/** 
* @brief Check if given operand is zero constant. Return 1 if opr is null.
* 
* @param ret String buffer to write YZ code
* @param opr Pointer to operand string
* 
* @return YZ code (1 bit)
*/
int getCodeYZ(char *ret, char *opr)
{
	int val = FALSE;

	if(opr){
		if(isInt(opr)){
			if(!atoi(opr)) {
				val = TRUE;
			}
		}
	}else	/* return 1 if opr is null */
		val = TRUE;

	if(val) strcpy(ret, "1");
	else strcpy(ret, "0");
	return val;
}

/** 
* @brief Check if given operand is zero constant (complex number).
* 
* @param ret String buffer to write YZ code
* @param opr1 Pointer to real part of operand string
* @param opr2 Pointer to imaginary part of operand string
* 
* @return YZ code (1 bit)
*/
int getCodeComplexYZ(char *ret, char *opr1, char *opr2)
{
	int val = FALSE;

	if(opr1 && opr2){
		if(isInt(opr1) && isInt(opr2)){
			if(!atoi(opr1) && !atoi(opr2)) {
				val = TRUE;
			}
		}
	}

	if(val) strcpy(ret, "1");
	else strcpy(ret, "0");
	return val;
}

/** 
* @brief Check if given operand is DM or PM.
* 
* @param ret String buffer to write MS code
* @param opr Pointer to operand string
* 
* @return MS code (1 bit)
*/
int getCodeMS(char *ret, char *opr)
{
	int val = FALSE;

	if(opr){
		if(!strcasecmp(opr, "PM")) 	/* 0 for DM, 1 for PM */
			val = TRUE;
	}

	if(val) strcpy(ret, "1");	
	else strcpy(ret, "0");
	return val;
}

/** 
* @brief Check if given operand is "+=" or "+".
* 
* @param ret String buffer to write U code
* @param opr Pointer to operand string
* 
* @return U code (1 bit)
*/
int getCodeU(char *ret, char *opr)
{
	int val = FALSE;

	if(opr){
		if(!strcasecmp(opr, "+=")) 	/* 0 for "+", 1 for "+=" */
			val = TRUE;
	}

	if(val) strcpy(ret, "1");	
	else strcpy(ret, "0");
	return val;
}

/** 
* @brief Get the 3-bit code of Ix register.
* 
* @param ret String buffer to write I code
* @param opr Pointer to operand string
* 
* @return I code (3 bits)
*/
int getCodeIReg3b(char *ret, char *opr)
{
	int val = 0;

	if(opr){
		if(opr[0] == 'I' || opr[0] == 'i'){
			val = opr[1] - '0'; 	/* val: 0~7 */
		}
	}

	int2bin(ret, val, 3);	
	return val;
}

/** 
* @brief Get the least significant 2 bits of Ix register.
* 
* @param ret String buffer to write I code
* @param opr Pointer to operand string
* 
* @return I code (2 bits)
*/
int getCodeIReg2b(char *ret, char *opr)
{
	int val = 0;

	if(opr){
		if(opr[0] == 'I' || opr[0] == 'i'){
			int c = opr[1] - '0'; 	/* c: 0~7 */
			val = c & 0x3;			/* val: 0~3 */
		}
	}

	int2bin(ret, val, 2);	
	return val;
}

/** 
* @brief Get the 3-bit code of Mx register.
* 
* @param ret String buffer to write M code
* @param opr Pointer to operand string
* 
* @return M code (3 bits)
*/
int getCodeMReg3b(char *ret, char *opr)
{
	int val = 0;

	if(opr){
		if(opr[0] == 'M' || opr[0] == 'm'){
			val = opr[1] - '0'; 	/* val: 0~7 */
		}
	}

	int2bin(ret, val, 3);	
	return val;
}

/** 
* @brief Get the least significant 2 bits of Mx register.
* 
* @param ret String buffer to write M code
* @param opr Pointer to operand string
* 
* @return M code (2 bits)
*/
int getCodeMReg2b(char *ret, char *opr)
{
	int val = 0;

	if(opr){
		if(opr[0] == 'M' || opr[0] == 'm'){
			int c = opr[1] - '0'; 	/* c: 0~7 */
			val = c & 0x3;			/* val: 0~3 */
		}
	}

	int2bin(ret, val, 2);	
	return val;
}

/** 
* @brief Get termination code of DO-UNTIL instruction
* 
* @param ret String buffer to write TERM code
* @param opr Pointer to operand string
* 
* @return TERM code (4 bits)
*/
int getCodeTERM(char *ret, char *opr)
{
	int val = 0;

	if(opr){
		if(!strcasecmp(opr, "CE")) 	/* 1110 for CE */
			val = 14;
		else if(!strcasecmp(opr, "FOREVER")) 	/* 1111 for FOREVER */
			val = 15;
	}

	int2bin(ret, val, 4);	
	return val;
}

/** 
* @brief Check if PUSH/POP of loop stacks specified
* 
* @param ret String buffer to write LPP code
* @param opr Pointer to operand string
* @param index Instruction index (iPUSH or iPOP)
* 
* @return LPP code (2 bits)
*/
int getCodeLPP(char *ret, char *opr, unsigned int index)
{
	int val = 0;

	if(opr){
		if(!strcasecmp(opr, "LOOP")){ 	/* LOOP */
			if(index == iPUSH) val = 1;
			else if(index == iPOP) val = 3;
		}
	}

	int2bin(ret, val, 2);	
	return val;
}

/** 
* @brief Check if PUSH/POP of PC stacks specified
* 
* @param ret String buffer to write PPP code
* @param opr Pointer to operand string
* @param index Instruction index (iPUSH or iPOP)
* 
* @return PPP code (2 bits)
*/
int getCodePPP(char *ret, char *opr, unsigned int index)
{
	int val = 0;

	if(opr){
		if(!strcasecmp(opr, "PC")){ 	/* PC */
			if(index == iPUSH) val = 1;
			else if(index == iPOP) val = 3;
		}
	}

	int2bin(ret, val, 2);	
	return val;
}

/** 
* @brief Check if PUSH/POP of Status stacks specified
* 
* @param ret String buffer to write SPP code
* @param opr0 Pointer to operand-0 string
* @param opr1 Pointer to operand-1 string
* @param index Instruction index (iPUSH or iPOP)
* 
* @return SPP code (2 bits)
*/
int getCodeSPP(char *ret, char *opr0, char *opr1, unsigned int index)
{
	int val = 0;

	if(opr0){
		if(!strcasecmp(opr0, "STS")){ 	/* STS */
			if(index == iPUSH) val = 1;
			else if(index == iPOP) val = 3;
		} 
	} else if(opr1){
		if(!strcasecmp(opr1, "STS")){ 	/* STS */
			if(index == iPUSH) val = 1;
			else if(index == iPOP) val = 3;
		}
	}

	int2bin(ret, val, 2);	
	return val;
}

/** 
* @brief Check if Conjugate modifier (*) is set.
* 
* @param ret String buffer to write CONJ code
* @param conj Conj field of current instruction
* 
* @return CONJ code (1 bit)
*/
int getCodeCONJ(char *ret, int conj)
{
	if(conj) strcpy(ret, "1");
	else strcpy(ret, "0");
	return conj;
}

/** 
* @brief Check if DREG12 <- RREG16 or RREG16 <- DREG12 (CP instruction)
* 
* @param p Pointer to instruction
* @param ret String buffer to write DC code
* @param opr0 Pointer to 1st operand string
* @param opr1 Pointer to 2nd operand string
* 
* @return DC code (1 bit)
*/
int getCodeDC(sICode *p, char *ret, char *opr0, char *opr1)
{
	int val = FALSE;

	if(opr0 && opr1){
		if(isRReg16(p, opr0) && isDReg12(p, opr1)){
			val = TRUE;
		}
	}

	if(val) strcpy(ret, "1");
	else strcpy(ret, "0");
	return val;
}

/** 
* @brief Get binary representation of ENA/DIS operands
* 
* @param ret String buffer to write ENA code
* @param p Pointer to current instruction
* 
* @return ENA code (18 bits)
*/
void getCodeENA(char *ret, sICode *p)
{
	int val = 0;
	int i;

	/* init to all "don't care"s */
	int2bin(ret, 0, 18);	

	switch(p->Index){
		case iENA:
			val = 3;
			for(i = 0; i < p->OperandCounter; i++){
				if(!strcasecmp(p->Operand[i], "SR")){
					int2bin(ret, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "BR")){
					int2bin(ret+2, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "OL")){
					int2bin(ret+4, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "AS")){
					int2bin(ret+6, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "MM")){
					int2bin(ret+8, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "TI")){
					int2bin(ret+10, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "SD")){
					int2bin(ret+12, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "MB")){
					int2bin(ret+14, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "INT")){
					int2bin(ret+16, val, 2);	
				}
			}
			break;
		case iDIS:
			val = 1;
			for(i = 0; i < p->OperandCounter; i++){
				if(!strcasecmp(p->Operand[i], "SR")){
					int2bin(ret, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "BR")){
					int2bin(ret+2, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "OL")){
					int2bin(ret+4, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "AS")){
					int2bin(ret+6, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "MM")){
					int2bin(ret+8, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "TI")){
					int2bin(ret+10, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "SD")){
					int2bin(ret+12, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "MB")){
					int2bin(ret+14, val, 2);	
				} else if(!strcasecmp(p->Operand[i], "INT")){
					int2bin(ret+16, val, 2);	
				}
			}
			break;
		default:
			break;
	}
}

/** 
* @brief Check if given operand is NONE.
* 
* @param ret String buffer to write MN code
* @param opr Pointer to operand string
* 
* @return MN code (1 bit)
*/
int getCodeMN(char *ret, char *opr)
{
	int val = FALSE;

	if(opr){
		if(!strcasecmp(opr, "NONE")) 	/* 1 if NONE, else 0 */
			val = TRUE;
	}

	if(val) strcpy(ret, "1");	
	else strcpy(ret, "0");
	return val;
}

/** 
* @brief Get binary representation of IDE operands
* 
* @param ret String buffer to write IDE code
* @param p Pointer to current instruction
* 
* @return IDE code (4 bits)
*/
void getCodeIDE(char *ret, sICode *p)
{
	int val = 0;
	int num;
	int i;

	/* init to all "don't care"s */
	int2bin(ret, 0, 4);	

	for(i = 0; i < p->OperandCounter; i++){
		num = atoi(p->Operand[i]);
		val += (1<<num);
	}
	int2bin(ret, val, 4);	
}

/** 
* @brief Simulate binary printf function, for example, something like '%b'
* 
* @param ret String buffer to write in binary format, e.g. "0000"
* @param val Binary integer number to convert
* @param n Number of output characters
*/
void int2bin(char *ret, int val, int n)
{
	int i;
	int mask = 0x1;

	if(ret){
		/* ret[0]: MSB, ret[n-1]: LSB */
		for(i = 0; i < n; i++){
			if(val & mask){
				ret[n-1-i] = '1';
			}else
				ret[n-1-i] = '0';
			mask <<= 1;
		}
	}else{
		assert(ret != NULL);
	}
}


/** 
* @brief Handle core multiplication (and MAC) algorithm
* 
* @param p Pointer to current instruction
* @param temp1 Integer value of first operand
* @param temp2 Integer value of second operand
* @param Opr0 Pointer to destination operand string
* @param Opr3 Pointer to muliplier option string
*/
//void processMACFunc(sICode *p, int temp1, int temp2, char *Opr0, char *Opr3)
//{
//	int temp3, temp4, temp5;
//	char z, n, v, c;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use processMACFunc() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	/* Note: system defaults to fractional mode after reset. */
//	if(!strcasecmp(Opr3, "RND")){
//		;
//	} else if(!strcasecmp(Opr3, "SS")){
//		;
//	} else if(!strcasecmp(Opr3, "SU")){
//		temp2 &= 0x0FFF;
//	} else if(!strcasecmp(Opr3, "US")){
//		temp1 &= 0x0FFF;
//	} else if(!strcasecmp(Opr3, "UU")){
//		temp1 &= 0x0FFF;
//		temp2 &= 0x0FFF;
//	}
//	temp3 = 0x0FFFFFFFF & (temp1 * temp2);
//
//	if(isFractionalMode()) temp3 <<= 1;
//
//	switch(p->Index){
//		case iMAC:
//			temp4 = (int)RdReg(Opr0);			/* accumulate */
//			break;
//		case iMAS:
//			temp4 = (int)RdReg(Opr0);
//			temp3 = -temp3;						/* subtract */
//			break;
//		case iMPY:
//			temp4 = 0;							/* do nothing */
//			break;
//		default:
//			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
//						"Invalid MAC operation called in processMACFunc().\n");
//			return;
//			break;
//	}
//	temp5 = temp3 + temp4;
//				
//	if(!strcasecmp(Opr3, "RND")){
//		/* Rounding operation */
//		/* when result is 0x800, add 1 to bit 11 */
//		/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
//		/* Note that after rounding, the content of ACCx.L is INVALID. */
//		temp5 = calcRounding(temp5);
//	}
//	v = OVCheck(p->InstType, temp3, temp4);
//
//	if(strcasecmp(Opr0, "NONE")){	//write result only if destination register is NOT "NONE"
//		WrReg(Opr0, temp5);
//	}
//
//	flagEffect(p->InstType, 0, 0, v, 0);
//
//	if(VerboseMode){
//		printf("processMACFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
//	}
//}

/** 
* @brief Handle core SIMD multiplication (and MAC) algorithm
* 
* @param p Pointer to current instruction
* @param temp1 SIMD integer value of first operand
* @param temp2 SIMD integer value of second operand
* @param Opr0 Pointer to destination operand string
* @param Opr3 Pointer to muliplier option string
* @param mask conditional execution mask
*/
void sProcessMACFunc(sICode *p, sint temp1, sint temp2, char *Opr0, char *Opr3, sint mask)
{
	sint temp3, temp4, temp5;
	sint z, n, v, c;
	sint o2 = { 0, 0, 0, 0 };

	/* Note: system defaults to fractional mode after reset. */
	if(!strcasecmp(Opr3, "RND")){
		;
	} else if(!strcasecmp(Opr3, "SS")){
		;
	} else if(!strcasecmp(Opr3, "SU")){
		for(int j = 0; j < NUMDP; j++) {
			temp2.dp[j] &= 0x0FFF;
		}
	} else if(!strcasecmp(Opr3, "US")){
		for(int j = 0; j < NUMDP; j++) {
			temp1.dp[j] &= 0x0FFF;
		}
	} else if(!strcasecmp(Opr3, "UU")){
		for(int j = 0; j < NUMDP; j++) {
			temp1.dp[j] &= 0x0FFF;
			temp2.dp[j] &= 0x0FFF;
		}
	}

	for(int j = 0; j < NUMDP; j++) {
		temp3.dp[j] = 0x0FFFFFFFF & (temp1.dp[j] * temp2.dp[j]);
		if(isFractionalMode()) temp3.dp[j] <<= 1;
	}

	switch(p->Index){
		case iMAC:
			temp4 = (sint)sRdReg(Opr0);			/* accumulate */
			break;
		case iMAS:
			temp4 = (sint)sRdReg(Opr0);
			for(int j = 0; j < NUMDP; j++) {
				temp3.dp[j] = -temp3.dp[j];		/* subtract */
			}
			break;
		case iMPY:
			for(int j = 0; j < NUMDP; j++) {
				temp4.dp[j] = 0;				/* do nothing */
			}
			break;
		default:
			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid MAC operation called in processMACFunc().\n");
			return;
			break;
	}

	for(int j = 0; j < NUMDP; j++) {
		temp5.dp[j] = temp3.dp[j] + temp4.dp[j];
	}
				
	if(!strcasecmp(Opr3, "RND")){
		/* Rounding operation */
		/* when result is 0x800, add 1 to bit 11 */
		/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
		/* Note that after rounding, the content of ACCx.L is INVALID. */
		temp5 = sCalcRounding(temp5);
	}
	v = sOVCheck(p->InstType, temp3, temp4);

	if(strcasecmp(Opr0, "NONE")){	//write result only if destination register is NOT "NONE"
		sWrReg(Opr0, temp5, mask);
	}

	sFlagEffect(p->InstType, o2, o2, v, o2, mask);

	if(VerboseMode){
		printf("sProcessMACFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
	}
}

/** 
* @brief Check the multiplier rounding mode (biased or unbiased) and do appropriate rounding operation.
* 
* @param x 32-bit accumulator value
* 
* @return Rounded 32-bit number
*/
//int calcRounding(int x)
//{
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use calcRounding() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	/* Multiplier rounding operation */
//	/* when result is 0x800, add 1 to bit 11 */
//	/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
//	/* Note that after rounding, the content of ACCx.L is INVALID. */
//	if(isUnbiasedRounding() && ((x & 0xFFF) == 0x800)) {	/* unbiased rounding only */
//		x += 0x800;
//		x &= 0xFFFFEFFF;	/* force bit 12 to zero */
//	} else {	
//		x += 0x800;
//	}
//
//	return x;
//}


/** 
* @brief Check the multiplier rounding mode (biased or unbiased) and do appropriate rounding operation. (SIMD version)
* 
* @param x 32-bit accumulator value (32-bit x NUMDP)
* 
* @return Rounded 32-bit number (32-bit x NUMDP)
*/
sint sCalcRounding(sint x)
{
	/* Multiplier rounding operation */
	/* when result is 0x800, add 1 to bit 11 */
	/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
	/* Note that after rounding, the content of ACCx.L is INVALID. */
	for(int j = 0; j < NUMDP; j++) {
		if(isUnbiasedRounding() && ((x.dp[j] & 0xFFF) == 0x800)) {	/* unbiased rounding only */
			x.dp[j] += 0x800;
			x.dp[j] &= 0xFFFFEFFF;	/* force bit 12 to zero */
		} else {	
			x.dp[j] += 0x800;
		}
	}

	return x;
}


/** 
* @brief Handle core multiplication (and MAC) algorithm (complex version)
* 
* @param p Pointer to current instruction
* @param ct1 Complex value of first operand
* @param ct2 Complex value of second operand
* @param Opr0 Pointer to destination operand string
* @param Opr3 Pointer to muliplier option string
*/
//void processMAC_CFunc(sICode *p, cplx ct1, cplx ct2, char *Opr0, char *Opr3)
//{
//	cplx z2, n2, v2, c2;
//	const cplx o2 = { 0, 0 };
//	cplx ct3, ct4, ct5;
//	cplx ct31, ct32;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use processMAC_CFunc() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	/* Note: system defaults to fractional mode after reset. */
//	if(!strcasecmp(Opr3, "RND")){
//		;
//	} else if(!strcasecmp(Opr3, "SS")){
//		;
//	} else if(!strcasecmp(Opr3, "SU")){
//		ct2.r &= 0x0FFF;
//		ct2.i &= 0x0FFF;
//	} else if(!strcasecmp(Opr3, "US")){
//		ct1.r &= 0x0FFF;
//		ct1.i &= 0x0FFF;
//	} else if(!strcasecmp(Opr3, "UU")){
//		ct1.r &= 0x0FFF;
//		ct1.i &= 0x0FFF;
//		ct2.r &= 0x0FFF;
//		ct2.i &= 0x0FFF;
//	}
//	//ct3.r = 0x0FFFFFFFF & (ct1.r * ct2.r - ct1.i * ct2.i);	/* multiply */
//	ct31.r = 0x0FFFFFFFF & (ct1.r * ct2.r);
//	ct32.r = 0x0FFFFFFFF & (ct1.i * ct2.i);
//	ct3.r = 0x0FFFFFFFF & (ct31.r - ct32.r);	
//	//ct3.i = 0x0FFFFFFFF & (ct1.r * ct2.i + ct1.i * ct2.r);	/* multiply */
//	ct31.i = 0x0FFFFFFFF & (ct1.r * ct2.i);
//	ct32.i = 0x0FFFFFFFF & (ct1.i * ct2.r);	
//	ct3.i = 0x0FFFFFFFF & (ct31.i + ct32.i);
//
//	if(isFractionalMode()){ 
//		ct3.r <<= 1;
//		ct3.i <<= 1;
//	}
//
//	switch(p->Index){
//		case iMAC_C:
//			ct4 = cRdReg(Opr0);
//			break;
//		case iMAS_C:
//			ct4 = cRdReg(Opr0);
//			ct3.r = - ct3.r;
//			ct3.i = - ct3.i;
//			break;
//		case iMPY_C:
//			ct4.r = 0;
//			ct4.i = 0;
//			break;
//		default:
//			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
//						"Invalid MAC.C operation called in processMAC_CFunc().\n");
//			return;
//			break;
//	}
//	ct5.r = ct3.r + ct4.r;		/* accumulate */
//	ct5.i = ct3.i + ct4.i;
//
//	if(!strcasecmp(Opr3, "RND")){
//		/* Rounding operation */
//		/* when result is 0x800, add 1 to bit 11 */
//		/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
//		/* Note that after rounding, the content of ACCx.L is INVALID. */
//		ct5.r = calcRounding(ct5.r);
//		/*
//		if(isUnbiasedRounding() && (ct5.r == 0x800)) {	//unbiased rounding only
//			ct5.r = 0x0;
//		} else {	
//			ct5.r += 0x800;
//		}
//		*/
//		ct5.i = calcRounding(ct5.i);
//		/*
//		if(isUnbiasedRounding() && (ct5.i == 0x800)) {	//unbiased rounding only 
//			ct5.i = 0x0;
//		} else {	
//			ct5.i += 0x800;
//		}
//		*/
//	}
//
//	v2.r = OVCheck(p->InstType, ct3.r, ct4.r);
//	v2.i = OVCheck(p->InstType, ct3.i, ct4.i);
//
//	if(strcasecmp(Opr0, "NONE")){	//write result only if destination register is NOT "NONE"
//		cWrReg(Opr0, ct5.r, ct5.i);
//	}
//
//	cFlagEffect(p->InstType, o2, o2, v2, o2);
//
//	if(VerboseMode){
//		printf("processMAC_CFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
//	}
//}

/** 
* @brief Handle core multiplication (and MAC) algorithm (complex SIMD version)
* 
* @param p Pointer to current instruction
* @param ct1 Complex value of first operand
* @param ct2 Complex value of second operand
* @param Opr0 Pointer to destination operand string
* @param Opr3 Pointer to muliplier option string
* @param mask conditional execution mask
*/
void sProcessMAC_CFunc(sICode *p, scplx ct1, scplx ct2, char *Opr0, char *Opr3, sint mask)
{
	scplx z2, n2, v2, c2;
	scplx o2;
	scplx ct3, ct4, ct5;
	scplx ct31, ct32;

	/* init o2 constant */
	for(int j = 0; j < NUMDP; j++) {
		o2.r.dp[j] = 0;
		o2.i.dp[j] = 0;
	}

	/* Note: system defaults to fractional mode after reset. */
	if(!strcasecmp(Opr3, "RND")){
		;
	} else if(!strcasecmp(Opr3, "SS")){
		;
	} else if(!strcasecmp(Opr3, "SU")){
		for(int j = 0; j < NUMDP; j++) {
			ct2.r.dp[j] &= 0x0FFF;
			ct2.i.dp[j] &= 0x0FFF;
		}
	} else if(!strcasecmp(Opr3, "US")){
		for(int j = 0; j < NUMDP; j++) {
			ct1.r.dp[j] &= 0x0FFF;
			ct1.i.dp[j] &= 0x0FFF;
		}
	} else if(!strcasecmp(Opr3, "UU")){
		for(int j = 0; j < NUMDP; j++) {
			ct1.r.dp[j] &= 0x0FFF;
			ct1.i.dp[j] &= 0x0FFF;
			ct2.r.dp[j] &= 0x0FFF;
			ct2.i.dp[j] &= 0x0FFF;
		}
	}

	for(int j = 0; j < NUMDP; j++) {
		//ct3.r = 0x0FFFFFFFF & (ct1.r * ct2.r - ct1.i * ct2.i);	/* multiply */
		ct31.r.dp[j] = 0x0FFFFFFFF & (ct1.r.dp[j] * ct2.r.dp[j]);
		ct32.r.dp[j] = 0x0FFFFFFFF & (ct1.i.dp[j] * ct2.i.dp[j]);
		ct3.r.dp[j] = 0x0FFFFFFFF & (ct31.r.dp[j] - ct32.r.dp[j]);	
		//ct3.i = 0x0FFFFFFFF & (ct1.r * ct2.i + ct1.i * ct2.r);	/* multiply */
		ct31.i.dp[j] = 0x0FFFFFFFF & (ct1.r.dp[j] * ct2.i.dp[j]);
		ct32.i.dp[j] = 0x0FFFFFFFF & (ct1.i.dp[j] * ct2.r.dp[j]);	
		ct3.i.dp[j] = 0x0FFFFFFFF & (ct31.i.dp[j] + ct32.i.dp[j]);
	}

	if(isFractionalMode()){ 
		for(int j = 0; j < NUMDP; j++) {
			ct3.r.dp[j] <<= 1;
			ct3.i.dp[j] <<= 1;
		}
	}

	switch(p->Index){
		case iMAC_C:
			ct4 = scRdReg(Opr0);
			break;
		case iMAS_C:
			ct4 = scRdReg(Opr0);
			for(int j = 0; j < NUMDP; j++) {
				ct3.r.dp[j] = - ct3.r.dp[j];
				ct3.i.dp[j] = - ct3.i.dp[j];
			}
			break;
		case iMPY_C:
			for(int j = 0; j < NUMDP; j++) {
				ct4.r.dp[j] = 0;
				ct4.i.dp[j] = 0;
			}
			break;
		default:
			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid MAC.C operation called in sProcessMAC_CFunc().\n");
			return;
			break;
	}

	for(int j = 0; j < NUMDP; j++) {
		ct5.r.dp[j] = ct3.r.dp[j] + ct4.r.dp[j];		/* accumulate */
		ct5.i.dp[j] = ct3.i.dp[j] + ct4.i.dp[j];
	}

	if(!strcasecmp(Opr3, "RND")){
		/* Rounding operation */
		/* when result is 0x800, add 1 to bit 11 */
		/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
		/* Note that after rounding, the content of ACCx.L is INVALID. */
		ct5.r = sCalcRounding(ct5.r);
		/*
		if(isUnbiasedRounding() && (ct5.r == 0x800)) {	//unbiased rounding only
			ct5.r = 0x0;
		} else {	
			ct5.r += 0x800;
		}
		*/
		ct5.i = sCalcRounding(ct5.i);
		/*
		if(isUnbiasedRounding() && (ct5.i == 0x800)) {	//unbiased rounding only 
			ct5.i = 0x0;
		} else {	
			ct5.i += 0x800;
		}
		*/
	}

	v2.r = sOVCheck(p->InstType, ct3.r, ct4.r);
	v2.i = sOVCheck(p->InstType, ct3.i, ct4.i);

	if(strcasecmp(Opr0, "NONE")){	//write result only if destination register is NOT "NONE"
		scWrReg(Opr0, ct5.r, ct5.i, mask);
	}

	scFlagEffect(p->InstType, o2, o2, v2, o2, mask);

	if(VerboseMode){
		printf("sProcessMAC_CFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
	}
}

/** 
* @brief Handle core multiplication (and MAC) algorithm (real-complex mixed version)
* 
* @param p Pointer to current instruction
* @param temp1 Integer value of first operand
* @param ct2 Complex value of second operand
* @param Opr0 Pointer to destination operand string
* @param Opr3 Pointer to muliplier option string
*/
//void processMAC_RCFunc(sICode *p, int temp1, cplx ct2, char *Opr0, char *Opr3)
//{
//	cplx z2, n2, v2, c2;
//	const cplx o2 = { 0, 0 };
//	cplx ct3, ct4, ct5;
//	cplx ct31, ct32;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use processMAC_RCFunc() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	/* Note: system defaults to fractional mode after reset. */
//	if(!strcasecmp(Opr3, "RND")){
//		;
//	} else if(!strcasecmp(Opr3, "SS")){
//		;
//	} else if(!strcasecmp(Opr3, "SU")){
//		ct2.r &= 0x0FFF;
//		ct2.i &= 0x0FFF;
//	} else if(!strcasecmp(Opr3, "US")){
//		temp1 &= 0x0FFF;
//	} else if(!strcasecmp(Opr3, "UU")){
//		temp1 &= 0x0FFF;
//		ct2.r &= 0x0FFF;
//		ct2.i &= 0x0FFF;
//	}
//	ct3.r = 0x0FFFFFFFF & (temp1 * ct2.r);	/* multiply */
//	ct3.i = 0x0FFFFFFFF & (temp1 * ct2.i);	/* multiply */
//
//	if(isFractionalMode()){ 
//		ct3.r <<= 1;
//		ct3.i <<= 1;
//	}
//
//	switch(p->Index){
//		case iMAC_RC:
//			ct4 = cRdReg(Opr0);
//			break;
//		case iMAS_RC:
//			ct4 = cRdReg(Opr0);
//			ct3.r = - ct3.r;
//			ct3.i = - ct3.i;
//			break;
//		case iMPY_RC:
//			ct4.r = 0;
//			ct4.i = 0;
//			break;
//		default:
//			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
//						"Invalid MAC.C operation called in processMAC_RCFunc().\n");
//			return;
//			break;
//	}
//	ct5.r = ct3.r + ct4.r;		/* accumulate */
//	ct5.i = ct3.i + ct4.i;
//
//	if(!strcasecmp(Opr3, "RND")){
//		/* Rounding operation */
//		/* when result is 0x800, add 1 to bit 11 */
//		/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
//		/* Note that after rounding, the content of ACCx.L is INVALID. */
//		ct5.r = calcRounding(ct5.r);
//		/*
//		if(isUnbiasedRounding() && (ct5.r == 0x800)) {	//unbiased rounding only
//			ct5.r = 0x0;
//		} else {	
//			ct5.r += 0x800;
//		}
//		*/
//		ct5.i = calcRounding(ct5.i);
//		/*
//		if(isUnbiasedRounding() && (ct5.i == 0x800)) {	//unbiased rounding only 
//			ct5.i = 0x0;
//		} else {	
//			ct5.i += 0x800;
//		}
//		*/
//	}
//
//	v2.r = OVCheck(p->InstType, ct3.r, ct4.r);
//	v2.i = OVCheck(p->InstType, ct3.i, ct4.i);
//
//	if(strcasecmp(Opr0, "NONE")){	//write result only if destination register is NOT "NONE"
//		cWrReg(Opr0, ct5.r, ct5.i);
//	}
//
//	cFlagEffect(p->InstType, o2, o2, v2, o2);
//
//	if(VerboseMode){
//		printf("processMAC_RCFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
//	}
//}

/** 
* @brief Handle core multiplication (and MAC) algorithm (real-complex mixed SIMD version)
* 
* @param p Pointer to current instruction
* @param temp1 Integer value of first operand
* @param ct2 Complex value of second operand
* @param Opr0 Pointer to destination operand string
* @param Opr3 Pointer to muliplier option string
* @param mask conditional execution mask
*/
void sProcessMAC_RCFunc(sICode *p, sint temp1, scplx ct2, char *Opr0, char *Opr3, sint mask)
{
	scplx z2, n2, v2, c2;
	scplx o2;
	scplx ct3, ct4, ct5;

	/* init o2 constant */
	for(int j = 0; j < NUMDP; j++) {
		o2.r.dp[j] = 0;
		o2.i.dp[j] = 0;
	}

	/* Note: system defaults to fractional mode after reset. */
	if(!strcasecmp(Opr3, "RND")){
		;
	} else if(!strcasecmp(Opr3, "SS")){
		;
	} else if(!strcasecmp(Opr3, "SU")){
		for(int j = 0; j < NUMDP; j++) {
			ct2.r.dp[j] &= 0x0FFF;
			ct2.i.dp[j] &= 0x0FFF;
		}
	} else if(!strcasecmp(Opr3, "US")){
		for(int j = 0; j < NUMDP; j++) {
			temp1.dp[j] &= 0x0FFF;
		}
	} else if(!strcasecmp(Opr3, "UU")){
		for(int j = 0; j < NUMDP; j++) {
			temp1.dp[j] &= 0x0FFF;
			ct2.r.dp[j] &= 0x0FFF;
			ct2.i.dp[j] &= 0x0FFF;
		}
	}

	for(int j = 0; j < NUMDP; j++) {
		ct3.r.dp[j] = 0x0FFFFFFFF & (temp1.dp[j] * ct2.r.dp[j]);	/* multiply */
		ct3.i.dp[j] = 0x0FFFFFFFF & (temp1.dp[j] * ct2.i.dp[j]);	/* multiply */
	}

	if(isFractionalMode()){ 
		for(int j = 0; j < NUMDP; j++) {
			ct3.r.dp[j] <<= 1;
			ct3.i.dp[j] <<= 1;
		}
	}

	switch(p->Index){
		case iMAC_RC:
			ct4 = scRdReg(Opr0);
			break;
		case iMAS_RC:
			ct4 = scRdReg(Opr0);
			for(int j = 0; j < NUMDP; j++) {
				ct3.r.dp[j] = - ct3.r.dp[j];
				ct3.i.dp[j] = - ct3.i.dp[j];
			}
			break;
		case iMPY_RC:
			for(int j = 0; j < NUMDP; j++) {
				ct4.r.dp[j] = 0;
				ct4.i.dp[j] = 0;
			}
			break;
		default:
			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid MAC.C operation called in sProcessMAC_RCFunc().\n");
			return;
			break;
	}

	for(int j = 0; j < NUMDP; j++) {
		ct5.r.dp[j] = ct3.r.dp[j] + ct4.r.dp[j];		/* accumulate */
		ct5.i.dp[j] = ct3.i.dp[j] + ct4.i.dp[j];
	}

	if(!strcasecmp(Opr3, "RND")){
		/* Rounding operation */
		/* when result is 0x800, add 1 to bit 11 */
		/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
		/* Note that after rounding, the content of ACCx.L is INVALID. */
		ct5.r = sCalcRounding(ct5.r);
		/*
		if(isUnbiasedRounding() && (ct5.r == 0x800)) {	//unbiased rounding only
			ct5.r = 0x0;
		} else {	
			ct5.r += 0x800;
		}
		*/
		ct5.i = sCalcRounding(ct5.i);
		/*
		if(isUnbiasedRounding() && (ct5.i == 0x800)) {	//unbiased rounding only 
			ct5.i = 0x0;
		} else {	
			ct5.i += 0x800;
		}
		*/
	}

	v2.r = sOVCheck(p->InstType, ct3.r, ct4.r);
	v2.i = sOVCheck(p->InstType, ct3.i, ct4.i);

	if(strcasecmp(Opr0, "NONE")){	//write result only if destination register is NOT "NONE"
		scWrReg(Opr0, ct5.r, ct5.i, mask);
	}

	scFlagEffect(p->InstType, o2, o2, v2, o2, mask);

	if(VerboseMode){
		printf("sProcessMAC_RCFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
	}
}

/** 
* @brief Handle core ALU instruction algorithm
* 
* @param p Pointer to current instruction
* @param temp1 Integer value of first operand
* @param temp2 Integer value of second operand
* @param Opr0 Pointer to destination operand string
*/
//void processALUFunc(sICode *p, int temp1, int temp2, char *Opr0)
//{
//	int temp3, temp4, temp5;
//	char z, n, v, c;
//	char cdata;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use processALUFunc() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	switch(p->Index){
//		case iADD:
//			temp5 = temp1 + temp2;
//			break;
//		case iADDC:
//			cdata = RdC1(); 
//			temp5 = temp1 + temp2 + cdata;
//			break;
//		case iSUB:
//			temp5 = temp1 - temp2;
//			break;
//		case iSUBC:
//			cdata = RdC1(); 
//			temp5 = temp1 - temp2 + cdata - 1;
//			break;
//		case iSUBB:
//			temp5 = - temp1 + temp2;
//			break;
//		case iSUBBC:
//			cdata = RdC1(); 
//			temp5 = - temp1 + temp2 + cdata - 1;
//			break;
//		case iAND:
//			temp5 = temp1 & temp2;
//			break;
//		case iOR:
//			temp5 = temp1 | temp2;
//			break;
//		case iXOR:
//			temp5 = temp1 ^ temp2;
//			break;
//		case iNOT:
//			temp5 = ~ temp1;
//			break;
//		case iABS:
//			if(isNeg12b(temp1))
//				temp5 = - temp1;
//			else
//				temp5 = temp1;
//			break;
//		case iINC:
//			temp5 = temp1 + 1;
//			break;
//		case iDEC:
//			temp5 = temp1 - 1;
//			break;
//		case iTSTBIT:
//			temp4 = 0x1 << temp2;   	/* generate bit mask */
//			temp5 = temp1 & temp4;   	/* test bit */
//			break;
//		case iSETBIT:
//			temp4 = 0x1 << temp2;   	/* generate bit mask */
//			temp5 = temp1 | temp4;   	/* set bit */
//			break;
//		case iCLRBIT:
//			temp4 = 0x1 << temp2;   	/* generate bit mask */
//			temp5 = temp1 & (~temp4);   /* clear bit */
//			break;
//		case iTGLBIT:
//			temp4 = 0x1 << temp2;   	/* generate bit mask */
//			temp5 = temp1 ^ temp4;   	/* toggle bit */
//			break;
//		case iSCR:
//			if(temp2 & 0x1){			/* if 2nd operand is odd  */
//				temp5 = -temp1;
//			}else{						/* if 2nd operand is even */
//				temp5 = temp1;
//			}
//			break;
//		default:
//			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
//						"Invalid ALU operation called in processALUFunc().\n");
//			return;
//			break;
//	}
//
//	z = (!temp5)?1:0;
//	n = isNeg12b(temp5);
//
//	/* to support type 9g */
//	if(p->InstType == t09g){
//		if(temp5 < 0) 
//			n = 1;
//		else 
//			n = 0;
//	}
//
//	switch(p->Index){
//		case iADD:
//			v = OVCheck(p->InstType, temp1, temp2);
//			c = CarryCheck(p, temp1, temp2, 0);
//			break;
//		case iADDC:
//			v = OVCheck(p->InstType, temp1, temp2 + cdata);
//			c = CarryCheck(p, temp1, temp2, cdata);
//			break;
//		case iSUB:
//			v = OVCheck(p->InstType, temp1, ~temp2+1);
//			c = CarryCheck(p, temp1, temp2, 0);
//			break;
//		case iSUBC:
//			v = OVCheck(p->InstType, temp1, - temp2 + cdata - 1);
//			c = CarryCheck(p, temp1, temp2, cdata);
//			break;
//		case iSUBB:
//			v = OVCheck(p->InstType, temp2, - temp1);
//			c = CarryCheck(p, temp1, temp2, 0);
//			break;
//		case iSUBBC:
//			v = OVCheck(p->InstType, temp2, - temp1 + cdata - 1);
//			c = CarryCheck(p, temp1, temp2, cdata);
//			break;
//		case iAND:
//		case iOR:
//		case iXOR:
//		case iNOT:
//		case iTSTBIT:
//		case iSETBIT:
//		case iCLRBIT:
//		case iTGLBIT:
//		case iSCR:
//			v = 0;
//			c = 0;
//			break;
//		case iABS:
//			if(temp1 == 0x800){
//				n = 1; v = 1;
//			} else {
//				n = 0; v = 0;
//			}
//			c = 0;
//			break;
//		case iINC:
//			v = OVCheck(p->InstType, temp1, 1);
//			c = CarryCheck(p, temp1, 0, 0);
//			break;
//		case iDEC:
//			v = OVCheck(p->InstType, temp1, -1);
//			c = CarryCheck(p, temp1, 0, 0);
//			break;
//		default:
//			break;
//	}
//
//	flagEffect(p->InstType, z, n, v, c);
//
//	if(p->Index == iABS){
//		rAstatR.AS = isNeg12b(temp1);
//	}
//
//	if(isALUSatMode()){ 			/* if saturation enabled  */
//		temp3 = satCheck12b(v, c);	/* check if needed    */
//
//		if(p->InstType == t09g){
//			temp3 = 0;				/* type 09g & 09h: skip saturation logic */
//		}
//
//		if(temp3)					/* if so, */	
//			WrReg(Opr0, temp3);		/* write saturated value  */
//		else						/* else */	
//			WrReg(Opr0, temp5);		/* write addition results */
//	} else
//		WrReg(Opr0, temp5);		/* write addition results */
//
//	if(VerboseMode){
//		printf("processALUFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
//	}
//}

/** 
* @brief Handle core ALU instruction algorithm (SIMD version)
* 
* @param p Pointer to current instruction
* @param temp1 Integer value of first operand
* @param temp2 Integer value of second operand
* @param Opr0 Pointer to destination operand string
* @param mask conditional execution mask
*/
void sProcessALUFunc(sICode *p, sint temp1, sint temp2, char *Opr0, sint mask)
{
	sint temp3, temp4, temp5;
	sint z, n, v, c;
	sint cdata;
	sint temp6;
	sint sconst;		/* SIMD constant */

	switch(p->Index){
		case iADD:
			for(int j = 0; j < NUMDP; j++) {
				temp5.dp[j] = temp1.dp[j] + temp2.dp[j];
			}
			break;
		case iADDC:
			cdata = sRdC1(); 

			for(int j = 0; j < NUMDP; j++) {
				temp5.dp[j] = temp1.dp[j] + temp2.dp[j] + cdata.dp[j];
			}
			break;
		case iSUB:
			for(int j = 0; j < NUMDP; j++) {
				temp5.dp[j] = temp1.dp[j] - temp2.dp[j];
			}
			break;
		case iSUBC:
			cdata = sRdC1(); 

			for(int j = 0; j < NUMDP; j++) {
				temp5.dp[j] = temp1.dp[j] - temp2.dp[j] + cdata.dp[j] - 1;
			}
			break;
		case iSUBB:
			for(int j = 0; j < NUMDP; j++) {
				temp5.dp[j] = - temp1.dp[j] + temp2.dp[j];
			}
			break;
		case iSUBBC:
			cdata = sRdC1(); 

			for(int j = 0; j < NUMDP; j++) {
				temp5.dp[j] = - temp1.dp[j] + temp2.dp[j] + cdata.dp[j] - 1;
			}
			break;
		case iAND:
			for(int j = 0; j < NUMDP; j++) {
				temp5.dp[j] = temp1.dp[j] & temp2.dp[j];
			}
			break;
		case iOR:
			for(int j = 0; j < NUMDP; j++) {
				temp5.dp[j] = temp1.dp[j] | temp2.dp[j];
			}
			break;
		case iXOR:
			for(int j = 0; j < NUMDP; j++) {
				temp5.dp[j] = temp1.dp[j] ^ temp2.dp[j];
			}
			break;
		case iNOT:
			for(int j = 0; j < NUMDP; j++) {
				temp5.dp[j] = ~ temp1.dp[j];
			}
			break;
		case iABS:
			for(int j = 0; j < NUMDP; j++) {
				if(isNeg12b(temp1.dp[j])){
					temp5.dp[j] = - temp1.dp[j];
				}else{
					temp5.dp[j] = temp1.dp[j];
				}
			}
			break;
		case iINC:
			for(int j = 0; j < NUMDP; j++) {
				temp5.dp[j] = temp1.dp[j] + 1;
			}
			break;
		case iDEC:
			for(int j = 0; j < NUMDP; j++) {
				temp5.dp[j] = temp1.dp[j] - 1;
			}
			break;
		case iTSTBIT:
			for(int j = 0; j < NUMDP; j++) {
				temp4.dp[j] = 0x1 << temp2.dp[j];   	/* generate bit mask */
				temp5.dp[j] = temp1.dp[j] & temp4.dp[j];   	/* test bit */
			}
			break;
		case iSETBIT:
			for(int j = 0; j < NUMDP; j++) {
				temp4.dp[j] = 0x1 << temp2.dp[j];   	/* generate bit mask */
				temp5.dp[j] = temp1.dp[j] | temp4.dp[j];   	/* set bit */
			}
			break;
		case iCLRBIT:
			for(int j = 0; j < NUMDP; j++) {
				temp4.dp[j] = 0x1 << temp2.dp[j];   	/* generate bit mask */
				temp5.dp[j] = temp1.dp[j] & (~temp4.dp[j]);   /* clear bit */
			}
			break;
		case iTGLBIT:
			for(int j = 0; j < NUMDP; j++) {
				temp4.dp[j] = 0x1 << temp2.dp[j];   	/* generate bit mask */
				temp5.dp[j] = temp1.dp[j] ^ temp4.dp[j];   	/* toggle bit */
			}
			break;
		case iSCR:
			for(int j = 0; j < NUMDP; j++) {
				if(temp2.dp[j] & 0x1){			/* if 2nd operand is odd  */
					temp5.dp[j] = -temp1.dp[j];
				}else{						/* if 2nd operand is even */
					temp5.dp[j] = temp1.dp[j];
				}
			}
			break;
		default:
			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid ALU operation called in sProcessALUFunc().\n");
			return;
			break;
	}

	for(int j = 0; j < NUMDP; j++){
		z.dp[j] = (!temp5.dp[j])?1:0;
		n.dp[j] = isNeg12b(temp5.dp[j]);
	}

	/* to support type 9g */
	if(p->InstType == t09g){
		for(int j = 0; j < NUMDP; j++){
			if(temp5.dp[j] < 0) 
				n.dp[j] = 1;
			else 
				n.dp[j] = 0;
		}
	}

	switch(p->Index){
		case iADD:
			for(int j = 0; j < NUMDP; j++) {
				sconst.dp[j] = 0;
			}

			v = sOVCheck(p->InstType, temp1, temp2);
			//c = CarryCheck(p, temp1, temp2, 0);
			c = sCarryCheck(p, temp1, temp2, sconst);
			break;
		case iADDC:
			for(int j = 0; j < NUMDP; j++) {
				temp6.dp[j] = temp2.dp[j] + cdata.dp[j];
			}

			//v = OVCheck(p->InstType, temp1, temp2 + cdata);
			v = sOVCheck(p->InstType, temp1, temp6);
			c = sCarryCheck(p, temp1, temp2, cdata);
			break;
		case iSUB:
			for(int j = 0; j < NUMDP; j++) {
				sconst.dp[j] = 0;
				temp6.dp[j] = ~temp2.dp[j] + 1;
			}

			//v = OVCheck(p->InstType, temp1, ~temp2+1);
			v = sOVCheck(p->InstType, temp1, temp6);
			//c = CarryCheck(p, temp1, temp2, 0);
			c = sCarryCheck(p, temp1, temp2, sconst);
			break;
		case iSUBC:
			for(int j = 0; j < NUMDP; j++) {
				temp6.dp[j] = -temp2.dp[j] + cdata.dp[j] - 1;
			}

			//v = OVCheck(p->InstType, temp1, - temp2 + cdata - 1);
			v = sOVCheck(p->InstType, temp1, temp6);
			c = sCarryCheck(p, temp1, temp2, cdata);
			break;
		case iSUBB:
			for(int j = 0; j < NUMDP; j++) {
				sconst.dp[j] = 0;
				temp6.dp[j] = -temp1.dp[j];
			}

			//v = OVCheck(p->InstType, temp2, - temp1);
			v = sOVCheck(p->InstType, temp2, temp6);
			//c = CarryCheck(p, temp1, temp2, 0);
			c = sCarryCheck(p, temp1, temp2, sconst);
			break;
		case iSUBBC:
			for(int j = 0; j < NUMDP; j++) {
				temp6.dp[j] = -temp1.dp[j] + cdata.dp[j] - 1;
			}

			//v = OVCheck(p->InstType, temp2, - temp1 + cdata - 1);
			v = sOVCheck(p->InstType, temp2, temp6);
			c = sCarryCheck(p, temp1, temp2, cdata);
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
			//v = 0;
			//c = 0;

			for(int j = 0; j < NUMDP; j++) {
				v.dp[j] = 0;
				c.dp[j] = 0;
			}
			break;
		case iABS:
			for(int j = 0; j < NUMDP; j++) {
				if(temp1.dp[j] == 0x800){
					n.dp[j] = 1; v.dp[j] = 1;
				} else {
					n.dp[j] = 0; v.dp[j] = 0;
				}
				c.dp[j] = 0;
			}
			break;
		case iINC:
			for(int j = 0; j < NUMDP; j++) {
				sconst.dp[j] = 1;
			}

			//v = OVCheck(p->InstType, temp1, 1);
			v = sOVCheck(p->InstType, temp1, sconst);

			for(int j = 0; j < NUMDP; j++) {
				sconst.dp[j] = 0;
			}

			//c = CarryCheck(p, temp1, 0, 0);
			c = sCarryCheck(p, temp1, sconst, sconst);
			break;
		case iDEC:
			for(int j = 0; j < NUMDP; j++) {
				sconst.dp[j] = -1;
			}

			//v = OVCheck(p->InstType, temp1, -1);
			v = sOVCheck(p->InstType, temp1, sconst);

			for(int j = 0; j < NUMDP; j++) {
				sconst.dp[j] = 0;
			}

			//c = CarryCheck(p, temp1, 0, 0);
			c = sCarryCheck(p, temp1, sconst, sconst);
			break;
		default:
			break;
	}

	sFlagEffect(p->InstType, z, n, v, c, mask);

	if(p->Index == iABS){
		for(int j = 0; j < NUMDP; j++) {
			if(mask.dp[j]){
				rAstatR.AS.dp[j] = isNeg12b(temp1.dp[j]);
			}
		}
	}

	if(isALUSatMode()){ 			/* if saturation enabled  */
		for(int j = 0; j < NUMDP; j++) {
			temp3.dp[j] = satCheck12b(v.dp[j], c.dp[j]);	/* check if needed    */
		}

		if(p->InstType == t09g){
			for(int j = 0; j < NUMDP; j++) {
				temp3.dp[j] = 0;				/* type 09g & 09h: skip saturation logic */
			}
		}

		for(int j = 0; j < NUMDP; j++) {
			if(temp3.dp[j]){					/* if so, */	
				//sWrReg(Opr0, temp3);		/* write saturated value  */
				temp5.dp[j] = temp3.dp[j];
			}
		}

		sWrReg(Opr0, temp5, mask);		/* write addition results */
	} else
		sWrReg(Opr0, temp5, mask);		/* write addition results */

	if(VerboseMode){
		printf("sProcessALUFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
	}
}

/** 
* @brief Handle core ALU instruction algorithm (complex version)
* 
* @param p Pointer to current instruction
* @param ct1 Complex value of first operand
* @param ct2 Complex value of second operand
* @param Opr0 Pointer to destination operand string
*/
//void processALU_CFunc(sICode *p, cplx ct1, cplx ct2, char *Opr0)
//{
//	cplx ct3, ct4, ct5;
//	cplx z2, n2, v2, c2;
//	const cplx o2 = { 0, 0 };
//	cplx cdata;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use processALU_CFunc() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	switch(p->Index){
//		case iADD_C:
//			ct5.r = ct1.r + ct2.r;
//		 	ct5.i = ct1.i + ct2.i;
//			break;
//		case iADDC_C:
//			cdata = RdC2(); 
//			ct5.r = ct1.r + ct2.r + cdata.r;
//		 	ct5.i = ct1.i + ct2.i + cdata.i;
//			break;
//		case iSUB_C:
//			ct5.r = ct1.r - ct2.r;
//		 	ct5.i = ct1.i - ct2.i;
//			break;
//		case iSUBC_C:
//			cdata = RdC2(); 
//			ct5.r = ct1.r - ct2.r + cdata.r - 1;
//		 	ct5.i = ct1.i - ct2.i + cdata.i - 1;
//			break;
//		case iSUBB_C:
//			ct5.r = - ct1.r + ct2.r;
//		 	ct5.i = - ct1.i + ct2.i;
//			break;
//		case iSUBBC_C:
//			cdata = RdC2(); 
//			ct5.r = - ct1.r + ct2.r + cdata.r - 1;
//		 	ct5.i = - ct1.i + ct2.i + cdata.i - 1;
//			break;
//		case iNOT_C:
//			ct5.r = ~ct1.r;
//			ct5.i = ~ct1.i;
//			break;
//		case iABS_C:
//			if(isNeg12b(ct1.r))
//				ct5.r = - ct1.r;
//			else
//				ct5.r = ct1.r;
//		   	if(isNeg12b(ct1.i))
//				ct5.i = - ct1.i;
//			else
//				ct5.i = ct1.i;
//			break;
//		case iSCR_C:
//			if(ct2.i & 0x1){			/* if Im(2nd operand) is odd  */
//				ct5.r = -ct1.r;
//			}else{						/* if Im(2nd operand) is even */
//				ct5.r = ct1.r;
//			}
//
//			if(ct2.i & 0x2){			/* if Im(2nd operand >> 1) is odd  */
//				ct5.i = -ct1.i;
//			}else{						/* if Im(2nd operand >> 1) is even */
//				ct5.i = ct1.i;
//			}
//			break;
//		default:
//			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
//						"Invalid ALU.C operation called in processALU_CFunc().\n");
//			return;
//			break;
//	}
//
//	z2.r = (!ct5.r)?1:0;
//	z2.i = (!ct5.i)?1:0;
//	n2.r = isNeg12b(ct5.r);
//	n2.i = isNeg12b(ct5.i);
//
//	/* to support type 9h */
//	if(p->InstType == t09h){
//		if(ct5.r < 0) 
//			n2.r = 1;
//		else 
//			n2.r = 0;
//
//		if(ct5.i < 0) 
//			n2.i = 1;
//		else 
//			n2.i = 0;
//	}
//
//	switch(p->Index){
//		case iADD_C:
//			v2.r = OVCheck(p->InstType, ct1.r, ct2.r);
//			v2.i = OVCheck(p->InstType, ct1.i, ct2.i);
//			c2.r = CarryCheck(p, ct1.r, ct2.r, 0);
//			c2.i = CarryCheck(p, ct1.i, ct2.i, 0);
//			break;
//		case iADDC_C:
//			v2.r = OVCheck(p->InstType, ct1.r, ct2.r + cdata.r);
//			v2.i = OVCheck(p->InstType, ct1.i, ct2.i + cdata.i);
//			c2.r = CarryCheck(p, ct1.r, ct2.r, cdata.r);
//			c2.i = CarryCheck(p, ct1.i, ct2.i, cdata.i);
//			break;
//		case iSUB_C:
//			v2.r = OVCheck(p->InstType, ct1.r, - ct2.r);
//			v2.i = OVCheck(p->InstType, ct1.i, - ct2.i);
//			c2.r = CarryCheck(p, ct1.r, ct2.r, 0);
//			c2.i = CarryCheck(p, ct1.i, ct2.i, 0);
//			break;
//		case iSUBC_C:
//			v2.r = OVCheck(p->InstType, ct1.r, - ct2.r + cdata.r - 1);
//			v2.i = OVCheck(p->InstType, ct1.i, - ct2.i + cdata.i - 1);
//			c2.r = CarryCheck(p, ct1.r, ct2.r, cdata.r);
//			c2.i = CarryCheck(p, ct1.i, ct2.i, cdata.i);
//			break;
//		case iSUBB_C:
//			v2.r = OVCheck(p->InstType, ct2.r, - ct1.r);
//			v2.i = OVCheck(p->InstType, ct2.i, - ct1.i);
//			c2.r = CarryCheck(p, ct1.r, ct2.r, 0);
//			c2.i = CarryCheck(p, ct1.i, ct2.i, 0);
//			break;
//		case iSUBBC_C:
//			v2.r = OVCheck(p->InstType, ct2.r, - ct1.r + cdata.r - 1);
//			v2.i = OVCheck(p->InstType, ct2.i, - ct1.i + cdata.i - 1);
//			c2.r = CarryCheck(p, ct1.r, ct2.r, cdata.r);
//			c2.i = CarryCheck(p, ct1.i, ct2.i, cdata.i);
//			break;
//		case iNOT_C:
//		case iSCR_C:
//			v2.r = v2.i = 0;
//			c2.r = c2.i = 0;
//			break;
//		case iABS_C:
//			if(ct1.r == 0x800){
//				n2.r = 1; v2.r = 1;
//			} else {
//				n2.r = 0; v2.r = 0;
//			}
//			if(ct1.i == 0x800){
//				n2.i = 1; v2.i = 1;
//			} else {
//				n2.i = 0; v2.i = 0;
//			}
//			c2.r = c2.i = 0;
//			break;
//		default:
//			break;
//	}
//
//	cFlagEffect(p->InstType, z2, n2, v2, c2);
//
//	if(p->Index == iABS_C){
//		rAstatR.AS = isNeg12b(ct1.r);
//		rAstatI.AS = isNeg12b(ct1.i);
//		rAstatC.AS = (rAstatR.AS || rAstatI.AS);
//	}
//
//	if(isALUSatMode()){            	 		/* if saturation enabled  */
//		ct3.r = satCheck12b(v2.r, c2.r);    /* check if needed    */
//		ct3.i = satCheck12b(v2.i, c2.i);
//
//		if(p->InstType == t09h){
//			ct3.r = ct3.i = 0;				/* type 09g & 09h: skip saturation logic */
//		}
//
//		if(ct3.r)               			/* if so, */
//			ct5.r = ct3.r;      			/* write saturated value  */
//		if(ct3.i)               			/* instead of addition result */
//			ct5.i = ct3.i;
//		cWrReg(Opr0, ct5.r, ct5.i);
//	} else
//		cWrReg(Opr0, ct5.r, ct5.i);     	/* write addition results */
//
//	if(VerboseMode){
//		printf("processALU_CFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
//	}
//}


/** 
* @brief Handle core ALU instruction algorithm (complex SIMD version)
* 
* @param p Pointer to current instruction
* @param ct1 Complex SIMD value of first operand
* @param ct2 Complex SIMD value of second operand
* @param Opr0 Pointer to destination operand string
* @param mask conditional execution mask
*/
void sProcessALU_CFunc(sICode *p, scplx ct1, scplx ct2, char *Opr0, sint mask)
{
	scplx ct3, ct4, ct5;
	scplx z2, n2, v2, c2;
	scplx o2;
	scplx cdata;
	sint sconst;
	scplx ct6;

	/* init o2 constant */
	for(int j = 0; j < NUMDP; j++) {
		o2.r.dp[j] = 0;
		o2.i.dp[j] = 0;
	}

	switch(p->Index){
		case iADD_C:
			for(int j = 0; j < NUMDP; j++) {
				ct5.r.dp[j] = ct1.r.dp[j] + ct2.r.dp[j];
		 		ct5.i.dp[j] = ct1.i.dp[j] + ct2.i.dp[j];
			}
			break;
		case iADDC_C:
			cdata = RdC2(); 

			for(int j = 0; j < NUMDP; j++) {
				ct5.r.dp[j] = ct1.r.dp[j] + ct2.r.dp[j] + cdata.r.dp[j];
		 		ct5.i.dp[j] = ct1.i.dp[j] + ct2.i.dp[j] + cdata.i.dp[j];
			}
			break;
		case iSUB_C:
			for(int j = 0; j < NUMDP; j++) {
				ct5.r.dp[j] = ct1.r.dp[j] - ct2.r.dp[j];
		 		ct5.i.dp[j] = ct1.i.dp[j] - ct2.i.dp[j];
			}
			break;
		case iSUBC_C:
			cdata = RdC2(); 

			for(int j = 0; j < NUMDP; j++) {
				ct5.r.dp[j] = ct1.r.dp[j] - ct2.r.dp[j] + cdata.r.dp[j] - 1;
		 		ct5.i.dp[j] = ct1.i.dp[j] - ct2.i.dp[j] + cdata.i.dp[j] - 1;
			}
			break;
		case iSUBB_C:
			for(int j = 0; j < NUMDP; j++) {
				ct5.r.dp[j] = - ct1.r.dp[j] + ct2.r.dp[j];
		 		ct5.i.dp[j] = - ct1.i.dp[j] + ct2.i.dp[j];
			}
			break;
		case iSUBBC_C:
			cdata = RdC2(); 

			for(int j = 0; j < NUMDP; j++) {
				ct5.r.dp[j] = - ct1.r.dp[j] + ct2.r.dp[j] + cdata.r.dp[j] - 1;
		 		ct5.i.dp[j] = - ct1.i.dp[j] + ct2.i.dp[j] + cdata.i.dp[j] - 1;
			}
			break;
		case iNOT_C:
			for(int j = 0; j < NUMDP; j++) {
				ct5.r.dp[j] = ~ct1.r.dp[j];
				ct5.i.dp[j] = ~ct1.i.dp[j];
			}
			break;
		case iABS_C:
			for(int j = 0; j < NUMDP; j++) {
				if(isNeg12b(ct1.r.dp[j]))
					ct5.r.dp[j] = - ct1.r.dp[j];
				else
					ct5.r.dp[j] = ct1.r.dp[j];
		   		if(isNeg12b(ct1.i.dp[j]))
					ct5.i.dp[j] = - ct1.i.dp[j];
				else
					ct5.i.dp[j] = ct1.i.dp[j];
			}
			break;
		case iSCR_C:
			for(int j = 0; j < NUMDP; j++) {
				if(ct2.i.dp[j] & 0x1){			/* if Im(2nd operand) is odd  */
					ct5.r.dp[j] = -ct1.r.dp[j];
				}else{						/* if Im(2nd operand) is even */
					ct5.r.dp[j] = ct1.r.dp[j];
				}

				if(ct2.i.dp[j] & 0x2){			/* if Im(2nd operand >> 1) is odd  */
					ct5.i.dp[j] = -ct1.i.dp[j];
				}else{						/* if Im(2nd operand >> 1) is even */
					ct5.i.dp[j] = ct1.i.dp[j];
				}
			}
			break;
		case iRCCW_C:
			for(int j = 0; j < NUMDP; j++) {
				switch(ct2.r.dp[j] & 0x3){		/* read LSB 2 bits of 2nd operand */
					case 0:		/* angle += 0; no change */
						ct5.r.dp[j] = ct1.r.dp[j];
						ct5.i.dp[j] = ct1.i.dp[j];
						break;
					case 1:		/* angle += +pi/2 */	/* (x+j*y)*j = -y + j*x */
						ct5.r.dp[j] = (-1)*ct1.i.dp[j];
						ct5.i.dp[j] = ct1.r.dp[j];
						break;
					case 2:		/* angle += +pi */		/* (x+j*y)*(-1) = -x -j*y */
						ct5.r.dp[j] = (-1)*ct1.r.dp[j];
						ct5.i.dp[j] = (-1)*ct1.i.dp[j];
						break;
					case 3:		/* angle += +3*pi/2 */	/* (x+j*y)*(-j) = y -j*x */
						ct5.r.dp[j] = ct1.i.dp[j];
						ct5.i.dp[j] = (-1)*ct1.r.dp[j];
						break;
					default:
						break;
				}
			}
			break;
		case iRCW_C:
			for(int j = 0; j < NUMDP; j++) {
				switch(ct2.r.dp[j] & 0x3){		/* read LSB 2 bits of 2nd operand */
					case 0:		/* angle += 0; no change */
						ct5.r.dp[j] = ct1.r.dp[j];
						ct5.i.dp[j] = ct1.i.dp[j];
						break;
					case 1:		/* angle += -pi/2 */	/* (x+j*y)*(-j) = y -j*x */
						ct5.r.dp[j] = ct1.i.dp[j];
						ct5.i.dp[j] = (-1)*ct1.r.dp[j];
						break;
					case 2:		/* angle += -pi */		/* (x+j*y)*(-1) = -x -j*y */
						ct5.r.dp[j] = (-1)*ct1.r.dp[j];
						ct5.i.dp[j] = (-1)*ct1.i.dp[j];
						break;
					case 3:		/* angle += -3*pi/2 */	/* (x+j*y)*j = -y + j*x */
						ct5.r.dp[j] = (-1)*ct1.i.dp[j];
						ct5.i.dp[j] = ct1.r.dp[j];
						break;
					default:
						break;
				}
			}
			break;
		default:
			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid ALU.C operation called in sProcessALU_CFunc().\n");
			return;
			break;
	}

	for(int j = 0; j < NUMDP; j++) {
		z2.r.dp[j] = (!ct5.r.dp[j])?1:0;
		z2.i.dp[j] = (!ct5.i.dp[j])?1:0;
		n2.r.dp[j] = isNeg12b(ct5.r.dp[j]);
		n2.i.dp[j] = isNeg12b(ct5.i.dp[j]);
	}

	/* to support type 9h */
	if(p->InstType == t09h){
		for(int j = 0; j < NUMDP; j++) {
			if(ct5.r.dp[j] < 0) {
				n2.r.dp[j] = 1;
			} else {
				n2.r.dp[j] = 0;
			}

			if(ct5.i.dp[j] < 0) {
				n2.i.dp[j] = 1;
			} else {
				n2.i.dp[j] = 0;
			}
		}
	}

	switch(p->Index){
		case iADD_C:
			for(int j = 0; j < NUMDP; j++) {
				sconst.dp[j] = 0;
			}

			v2.r = sOVCheck(p->InstType, ct1.r, ct2.r);
			v2.i = sOVCheck(p->InstType, ct1.i, ct2.i);
			//c2.r = CarryCheck(p, ct1.r, ct2.r, 0);
			c2.r = sCarryCheck(p, ct1.r, ct2.r, sconst);
			//c2.i = CarryCheck(p, ct1.i, ct2.i, 0);
			c2.i = sCarryCheck(p, ct1.i, ct2.i, sconst);
			break;
		case iADDC_C:
			for(int j = 0; j < NUMDP; j++) {
				ct6.r.dp[j] = ct2.r.dp[j] + cdata.r.dp[j];
				ct6.i.dp[j] = ct2.i.dp[j] + cdata.i.dp[j];
			}

			//v2.r = sOVCheck(p->InstType, ct1.r, ct2.r + cdata.r);
			v2.r = sOVCheck(p->InstType, ct1.r, ct6.r);
			//v2.i = sOVCheck(p->InstType, ct1.i, ct2.i + cdata.i);
			v2.i = sOVCheck(p->InstType, ct1.i, ct6.i);
			c2.r = sCarryCheck(p, ct1.r, ct2.r, cdata.r);
			c2.i = sCarryCheck(p, ct1.i, ct2.i, cdata.i);
			break;
		case iSUB_C:
			for(int j = 0; j < NUMDP; j++) {
				sconst.dp[j] = 0;
				ct6.r.dp[j] = -ct2.r.dp[j];
				ct6.i.dp[j] = -ct2.i.dp[j];
			}

			//v2.r = sOVCheck(p->InstType, ct1.r, - ct2.r);
			v2.r = sOVCheck(p->InstType, ct1.r, ct6.r);
			//v2.i = sOVCheck(p->InstType, ct1.i, - ct2.i);
			v2.i = sOVCheck(p->InstType, ct1.i, ct6.i);
			//c2.r = sCarryCheck(p, ct1.r, ct2.r, 0);
			c2.r = sCarryCheck(p, ct1.r, ct2.r, sconst);
			//c2.i = sCarryCheck(p, ct1.i, ct2.i, 0);
			c2.i = sCarryCheck(p, ct1.i, ct2.i, sconst);
			break;
		case iSUBC_C:
			for(int j = 0; j < NUMDP; j++) {
				ct6.r.dp[j] = -ct2.r.dp[j] + cdata.r.dp[j] - 1;
				ct6.i.dp[j] = -ct2.i.dp[j] + cdata.i.dp[j] - 1;
			}

			//v2.r = sOVCheck(p->InstType, ct1.r, - ct2.r + cdata.r - 1);
			v2.r = sOVCheck(p->InstType, ct1.r, ct6.r);
			//v2.i = sOVCheck(p->InstType, ct1.i, - ct2.i + cdata.i - 1);
			v2.i = sOVCheck(p->InstType, ct1.i, ct6.i);
			//c2.r = sCarryCheck(p, ct1.r, ct2.r, cdata.r);
			c2.r = sCarryCheck(p, ct1.r, ct2.r, cdata.r);
			//c2.i = sCarryCheck(p, ct1.i, ct2.i, cdata.i);
			c2.i = sCarryCheck(p, ct1.i, ct2.i, cdata.i);
			break;
		case iSUBB_C:
			for(int j = 0; j < NUMDP; j++) {
				sconst.dp[j] = 0;
				ct6.r.dp[j] = -ct1.r.dp[j];
				ct6.i.dp[j] = -ct1.i.dp[j];
			}

			//v2.r = sOVCheck(p->InstType, ct2.r, - ct1.r);
			v2.r = sOVCheck(p->InstType, ct2.r, ct6.r);
			//v2.i = sOVCheck(p->InstType, ct2.i, - ct1.i);
			v2.i = sOVCheck(p->InstType, ct2.i, ct6.i);
			//c2.r = sCarryCheck(p, ct1.r, ct2.r, 0);
			c2.r = sCarryCheck(p, ct1.r, ct2.r, sconst);
			//c2.i = sCarryCheck(p, ct1.i, ct2.i, 0);
			c2.i = sCarryCheck(p, ct1.i, ct2.i, sconst);
			break;
		case iSUBBC_C:
			for(int j = 0; j < NUMDP; j++) {
				ct6.r.dp[j] = -ct1.r.dp[j] + cdata.r.dp[j] - 1;
				ct6.i.dp[j] = -ct1.i.dp[j] + cdata.i.dp[j] - 1;
			}

			//v2.r = sOVCheck(p->InstType, ct2.r, - ct1.r + cdata.r - 1);
			v2.r = sOVCheck(p->InstType, ct2.r, ct6.r);
			//v2.i = sOVCheck(p->InstType, ct2.i, - ct1.i + cdata.i - 1);
			v2.i = sOVCheck(p->InstType, ct2.i, ct6.i);
			c2.r = sCarryCheck(p, ct1.r, ct2.r, cdata.r);
			c2.i = sCarryCheck(p, ct1.i, ct2.i, cdata.i);
			break;
		case iNOT_C:
		case iSCR_C:
		case iRCCW_C:
		case iRCW_C:
			for(int j = 0; j < NUMDP; j++) {
				v2.r.dp[j] = v2.i.dp[j] = 0;
				c2.r.dp[j] = c2.i.dp[j] = 0;
			}
			break;
		case iABS_C:
			for(int j = 0; j < NUMDP; j++) {
				if(ct1.r.dp[j] == 0x800){
					n2.r.dp[j] = 1; v2.r.dp[j] = 1;
				} else {
					n2.r.dp[j] = 0; v2.r.dp[j] = 0;
				}
				if(ct1.i.dp[j] == 0x800){
					n2.i.dp[j] = 1; v2.i.dp[j] = 1;
				} else {
					n2.i.dp[j] = 0; v2.i.dp[j] = 0;
				}
				c2.r.dp[j] = c2.i.dp[j] = 0;
			}
			break;
		default:
			break;
	}

	scFlagEffect(p->InstType, z2, n2, v2, c2, mask);

	if(p->Index == iABS_C){
		for(int j = 0; j < NUMDP; j++) {
			if(mask.dp[j]){
				rAstatR.AS.dp[j] = isNeg12b(ct1.r.dp[j]);
				rAstatI.AS.dp[j] = isNeg12b(ct1.i.dp[j]);
				rAstatC.AS.dp[j] = (rAstatR.AS.dp[j] || rAstatI.AS.dp[j]);
			}
		}
	}

	if(isALUSatMode()){            	 		/* if saturation enabled  */
		for(int j = 0; j < NUMDP; j++) {
			ct3.r.dp[j] = satCheck12b(v2.r.dp[j], c2.r.dp[j]);    /* check if needed    */
			ct3.i.dp[j] = satCheck12b(v2.i.dp[j], c2.i.dp[j]);
		}

		if(p->InstType == t09h){
			for(int j = 0; j < NUMDP; j++) {
				ct3.r.dp[j] = ct3.i.dp[j] = 0;		/* type 09g & 09h: skip saturation logic */
			}
		}

		for(int j = 0; j < NUMDP; j++) {
			if(ct3.r.dp[j])               			/* if so, */
				ct5.r.dp[j] = ct3.r.dp[j];      			/* write saturated value  */
			if(ct3.i.dp[j])               			/* instead of addition result */
				ct5.i.dp[j] = ct3.i.dp[j];
		}
		scWrReg(Opr0, ct5.r, ct5.i, mask);
	} else
		scWrReg(Opr0, ct5.r, ct5.i, mask);     	/* write addition results */

	if(VerboseMode){
		printf("sProcessALU_CFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
	}
}


/** 
* @brief Handle core shift algorithm
* 
* @param p Pointer to current instruction
* @param temp1 Integer value of first operand
* @param temp2 Integer value of second operand
* @param Opr0 Pointer to destination operand string
* @param Opr3 Pointer to shifter option string
*/
//void processSHIFTFunc(sICode *p, int temp1, int temp2, char *Opr0, char *Opr3)
//{
//	int temp3, temp4, temp5;
//	char z, n, v, c;
//	unsigned int t4;
//
//	int isAccSrc;
//	int temp0;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use processSHIFTFunc() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	/* check if 1st source is an accumulator */
//	switch(p->InstType){
//		case t15c:
//		case t12m:
//		case t12o:
//		case t14k:
//		case t44d:
//		case t45d:
//	    	isAccSrc = TRUE;
//			break;
//		default:
//	    	isAccSrc = FALSE;
//			break;
//	}
//
//	switch(p->Index){
//		case iASHIFT:
//		case iASHIFTOR:
//			temp0 = temp1;
//
//			if(Opr3 && (!strcasecmp(Opr3, "HI") || !strcasecmp(Opr3, "HIRND"))) 
//				temp1 <<= 12;
//			if(isNeg12b(temp2)){	/* shift right */
//				if(temp2 == -32){
//					if(isAccSrc && isNeg24b(temp0)){
//						temp5 = 0xFFFFFFFF;
//					}else if(!isAccSrc && isNeg12b(temp0)){
//						temp5 = 0xFFFFFFFF;
//					}else {
//						temp5 = 0;
//					}
//				}else{
//					/* shift right here */
//					int numshift = (-temp2);	/* number of shift */
//					
//					/* shift (numshift -1) bits */
//					temp5 = (temp1 >> (numshift - 1));
//
//					/* if rounding should be performed, */
//					if(Opr3 && (!strcasecmp(Opr3, "LORND") || !strcasecmp(Opr3, "HIRND") || !strcasecmp(Opr3, "RND"))) {
//						/* add 1 to intermediate result */
//						temp5 += 1;
//					}
//
//					/* shift last 1 bit */
//					temp5 = (temp5 >> 1);
//				}
//			} else {				/* shift left */
//				if(temp2 == 32){
//					temp5 = 0;
//				}else{
//					temp5 = temp1 << temp2;
//				}
//			}
//			break;
//		case iLSHIFT:
//		case iLSHIFTOR:
//			/* check if 1st source is an accumulator */
//			if(isAccSrc){
//				;
//			}else{
//				temp1 &= 0xFFF;
//			}
//
//			t4 = temp1;
//			if(Opr3 && (!strcasecmp(Opr3, "HI") || !strcasecmp(Opr3, "HIRND"))) 
//				t4 <<= 12;
//			if(isNeg12b(temp2)){	/* shift right */
//				if(temp2 == -32){
//					temp5 = 0;
//				}else{
//					/* shift right here */
//					int numshift = (-temp2);	/* number of shift */
//					
//					/* shift (numshift -1) bits */
//					t4 = (t4 >> (numshift - 1));
//
//					/* if rounding should be performed, */
//					if(Opr3 && (!strcasecmp(Opr3, "LORND") || !strcasecmp(Opr3, "HIRND") || !strcasecmp(Opr3, "RND"))) {
//						/* add 1 to intermediate result */
//						t4 += 1;
//					}
//
//					/* shift last 1 bit */
//					temp5 = (t4 >> 1);
//				}
//			} else {				/* shift left */
//				if(temp2 == 32){
//					temp5 = 0;
//				}else{
//					temp5 = t4 << temp2;
//				}
//			}
//			break;
//		default:
//			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
//						"Invalid SHIFT operation called in processSHIFTFunc().\n");
//			return;
//			break;
//	}
//	
//	if((p->Index == iASHIFTOR) || (p->Index == iLSHIFTOR)){
//		temp5 |= RdReg(Opr0);   /* OR */
//	}
//	
//	v = OVCheck(p->InstType, temp5, 0);
//	flagEffect(p->InstType, 0, 0, v, 0);
//
//	WrReg(Opr0, temp5);
//
//	if(VerboseMode){
//		printf("processSHIFTFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
//	}
//}

/** 
* @brief Handle core shift algorithm
* 
* @param p Pointer to current instruction
* @param temp1 SIMD integer value of first operand
* @param temp2 SIMD integer value of second operand
* @param Opr0 Pointer to destination operand string
* @param Opr3 Pointer to shifter option string
* @param mask conditional execution mask
*/
void sProcessSHIFTFunc(sICode *p, sint temp1, sint temp2, char *Opr0, char *Opr3, sint mask)
{
	sint temp3, temp4, temp5;
	sint z, n, v, c;
	suint t4;

	int isAccSrc;
	sint temp0;
	sint sconst;

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

	switch(p->Index){
		case iASHIFT:
		case iASHIFTOR:
			temp0 = temp1;

			if(Opr3 && (!strcasecmp(Opr3, "HI") || !strcasecmp(Opr3, "HIRND"))) {
				for(int j = 0; j < NUMDP; j++) {
					temp1.dp[j] <<= 12;
				}
			}

			for(int j = 0; j < NUMDP; j++) {
				if(isNeg12b(temp2.dp[j])){	/* shift right */
					if(temp2.dp[j] == -32){
						if(isAccSrc && isNeg24b(temp0.dp[j])){
							temp5.dp[j] = 0xFFFFFFFF;
						}else if(!isAccSrc && isNeg12b(temp0.dp[j])){
							temp5.dp[j] = 0xFFFFFFFF;
						}else {
							temp5.dp[j] = 0;
						}
					}else{
						/* shift right here */
						int numshift = (-temp2.dp[j]);	/* number of shift */
					
						/* shift (numshift -1) bits */
						temp5.dp[j] = (temp1.dp[j] >> (numshift - 1));

						/* if rounding should be performed, */
						if(Opr3 && (!strcasecmp(Opr3, "LORND") || !strcasecmp(Opr3, "HIRND") || !strcasecmp(Opr3, "RND"))) {
							/* add 1 to intermediate result */
							temp5.dp[j] += 1;
						}

						/* shift last 1 bit */
						temp5.dp[j] = (temp5.dp[j] >> 1);
					}
				} else {				/* shift left */
					if(temp2.dp[j] == 32){
						temp5.dp[j] = 0;
					}else{
						temp5.dp[j] = temp1.dp[j] << temp2.dp[j];
					}
				}
			}
			break;
		case iLSHIFT:
		case iLSHIFTOR:
			/* check if 1st source is an accumulator */
			if(isAccSrc){
				;
			}else{
				for(int j = 0; j < NUMDP; j++) {
					temp1.dp[j] &= 0xFFF;
				}
			}

			for(int j = 0; j < NUMDP; j++) {
				t4.dp[j] = temp1.dp[j];
			}

			if(Opr3 && (!strcasecmp(Opr3, "HI") || !strcasecmp(Opr3, "HIRND"))) {
				for(int j = 0; j < NUMDP; j++) {
					t4.dp[j] <<= 12;
				}
			}

			for(int j = 0; j < NUMDP; j++) {
				if(isNeg12b(temp2.dp[j])){	/* shift right */
					if(temp2.dp[j] == -32){
						temp5.dp[j] = 0;
					}else{
						/* shift right here */
						int numshift = (-temp2.dp[j]);	/* number of shift */
					
						/* shift (numshift -1) bits */
						t4.dp[j] = (t4.dp[j] >> (numshift - 1));

						/* if rounding should be performed, */
						if(Opr3 && (!strcasecmp(Opr3, "LORND") || !strcasecmp(Opr3, "HIRND") || !strcasecmp(Opr3, "RND"))) {
							/* add 1 to intermediate result */
							t4.dp[j] += 1;
						}

						/* shift last 1 bit */
						temp5.dp[j] = (t4.dp[j] >> 1);
					}
				} else {				/* shift left */
					if(temp2.dp[j] == 32){
						temp5.dp[j] = 0;
					}else{
						temp5.dp[j] = t4.dp[j] << temp2.dp[j];
					}
				}
			}
			break;
		default:
			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid SHIFT operation called in sProcessSHIFTFunc().\n");
			return;
			break;
	}
	
	if((p->Index == iASHIFTOR) || (p->Index == iLSHIFTOR)){
		sint temp6 = sRdReg(Opr0);   
		for(int j = 0; j < NUMDP; j++) {
			temp5.dp[j] |= temp6.dp[j];   /* OR */
		}
	}
	
	/* init zero constant */
	for(int j = 0; j < NUMDP; j++) {
		sconst.dp[j] = 0;
	}

	v = sOVCheck(p->InstType, temp5, sconst);
	sFlagEffect(p->InstType, sconst, sconst, v, sconst, mask);

	sWrReg(Opr0, temp5, mask);

	if(VerboseMode){
		printf("sProcessSHIFTFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
	}
}

/** 
* @brief Handle core shift algorithm (complex version)
* 
* @param p Pointer to current instruction
* @param ct1 Complex value of first operand
* @param ct2 Complex value of second operand
* @param Opr0 Pointer to destination operand string
* @param Opr3 Pointer to shifter option string
*/
//void processSHIFT_CFunc(sICode *p, cplx ct1, cplx ct2, char *Opr0, char *Opr3)
//{
//	cplx ct3, ct4, ct5;
//	cplx z2, n2, v2, c2;
//	const cplx o2 = { 0, 0 };
//	unsigned int t4r, t4i;
//	int isAccSrc;
//	cplx ct0;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use processSHIFT_CFunc() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	/* check if 1st source is an accumulator */
//	switch(p->InstType){
//		case t15d:
//		case t12n:
//		case t12p:
//		case t14l:
//	    	isAccSrc = TRUE;
//			break;
//		default:
//	    	isAccSrc = FALSE;
//			break;
//	}
//
//	switch(p->Index){
//		case iASHIFT_C:
//		case iASHIFTOR_C:
//			ct0 = ct1;
//
//			if(Opr3 && (!strcasecmp(Opr3, "HI") || !strcasecmp(Opr3, "HIRND"))) {
//				ct1.r <<= 12;
//				ct1.i <<= 12;
//			}
//			if(isNeg12b(ct2.r)){	/* shift right */
//				if(ct2.r == -32){
//					if(isAccSrc && isNeg24b(ct0.r)){
//						ct5.r = 0xFFFFFFFF;
//					}else if(!isAccSrc && isNeg12b(ct0.r)){
//						ct5.r = 0xFFFFFFFF;
//					} else {
//						ct5.r = 0;
//					}
//				} else {
//					/* shift right here: real part */
//					int numshift = (-ct2.r);	/* number of shift */
//					
//					/* shift (numshift -1) bits */
//					ct5.r = (ct1.r >> (numshift - 1));
//
//					/* if rounding should be performed, */
//					if(Opr3 && (!strcasecmp(Opr3, "LORND") || !strcasecmp(Opr3, "HIRND") || !strcasecmp(Opr3, "RND"))) {
//						/* add 1 to intermediate result */
//						ct5.r += 1;
//					}
//
//					/* shift last 1 bit */
//					ct5.r = (ct5.r >> 1);
//				}
//			} else {				/* shift left */
//				if(ct2.r == 32){
//					ct5.r = 0;
//				}else{
//					ct5.r = ct1.r << ct2.r;
//				}
//			}
//			if(isNeg12b(ct2.i)){	/* shift right */
//				if(ct2.i == -32){
//					if(isAccSrc && isNeg24b(ct0.i)){
//						ct5.i = 0xFFFFFFFF;
//					}else if(!isAccSrc && isNeg12b(ct0.i)){
//						ct5.i = 0xFFFFFFFF;
//					} else {
//						ct5.i = 0;
//					}
//				} else {
//					/* shift right here: imaginary part */
//					int numshift = (-ct2.i);	/* number of shift */
//					
//					/* shift (numshift -1) bits */
//					ct5.i = (ct1.i >> (numshift - 1));
//
//					/* if rounding should be performed, */
//					if(Opr3 && (!strcasecmp(Opr3, "LORND") || !strcasecmp(Opr3, "HIRND") || !strcasecmp(Opr3, "RND"))) {
//						/* add 1 to intermediate result */
//						ct5.i += 1;
//					}
//
//					/* shift last 1 bit */
//					ct5.i = (ct5.i >> 1);
//				}
//			} else {				/* shift left */
//				if(ct2.i == 32){
//					ct5.i = 0;
//				}else{
//					ct5.i = ct1.i << ct2.i;
//				}
//			}
//			break;
//		case iLSHIFT_C:
//		case iLSHIFTOR_C:
//			/* check if 1st source is an accumulator */
//			if(isAccSrc){
//				;
//			}else{
//				ct1.r &= 0xFFF;
//				ct1.i &= 0xFFF;
//			}
//
//			t4r = ct1.r;
//			t4i = ct1.i;
//
//			if(Opr3 && (!strcasecmp(Opr3, "HI") || !strcasecmp(Opr3, "HIRND"))) {
//				t4r <<= 12;
//				t4i <<= 12;
//			}
//			if(isNeg12b(ct2.r)){	/* shift right */
//				if(ct2.r == -32){
//					ct5.r = 0;
//				} else {
//					/* shift right here: real part */
//					int numshift = (-ct2.r);	/* number of shift */
//					
//					/* shift (numshift -1) bits */
//					t4r = (t4r >> (numshift - 1));
//
//					/* if rounding should be performed, */
//					if(Opr3 && (!strcasecmp(Opr3, "LORND") || !strcasecmp(Opr3, "HIRND") || !strcasecmp(Opr3, "RND"))) {
//						/* add 1 to intermediate result */
//						t4r += 1;
//					}
//
//					/* shift last 1 bit */
//					ct5.r = (t4r >> 1);
//				}
//			} else {				/* shift left */
//				if(ct2.r == 32){
//					ct5.r = 0;
//				}else{
//					ct5.r = t4r << ct2.r;
//				}
//			}
//			if(isNeg12b(ct2.i)){	/* shift right */
//				if(ct2.i == -32){
//					ct5.i = 0;
//				} else {
//					/* shift right here: imaginary part */
//					int numshift = (-ct2.i);	/* number of shift */
//					
//					/* shift (numshift -1) bits */
//					t4i = (t4i >> (numshift - 1));
//
//					/* if rounding should be performed, */
//					if(Opr3 && (!strcasecmp(Opr3, "LORND") || !strcasecmp(Opr3, "HIRND") || !strcasecmp(Opr3, "RND"))) {
//						/* add 1 to intermediate result */
//						t4i += 1;
//					}
//
//					/* shift last 1 bit */
//					ct5.i = (t4i >> 1);
//				}
//			} else {				/* shift left */
//				if(ct2.i == 32){
//					ct5.i = 0;
//				}else{
//					ct5.i = t4i << ct2.i;
//				}
//			}
//			break;
//		default:
//			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
//						"Invalid SHIFT.C operation called in processSHIFT_CFunc().\n");
//			return;
//			break;
//	}
//	
//	if((p->Index == iASHIFTOR_C) || (p->Index == iLSHIFTOR_C)){	
//					ct3 = cRdReg(Opr0);
//					ct5.r |= ct3.r;     /* OR */
//					ct5.i |= ct3.i;     /* OR */
//	}
//	
//	v2.r = OVCheck(p->InstType, ct5.r, 0);
//	v2.i = OVCheck(p->InstType, ct5.i, 0);
//	cFlagEffect(p->InstType, o2, o2, v2, o2);
//
//	cWrReg(Opr0, ct5.r, ct5.i);
//
//	if(VerboseMode){
//		printf("processSHIFT_CFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
//	}
//}

/** 
* @brief Handle core shift algorithm (complex SIMD version)
* 
* @param p Pointer to current instruction
* @param ct1 Complex SIMD value of first operand
* @param ct2 Complex SIMD value of second operand
* @param Opr0 Pointer to destination operand string
* @param Opr3 Pointer to shifter option string
* @param mask conditional execution mask
*/
void sProcessSHIFT_CFunc(sICode *p, scplx ct1, scplx ct2, char *Opr0, char *Opr3, sint mask)
{
	scplx ct3, ct4, ct5;
	scplx z2, n2, v2, c2;
	scplx o2;
	suint t4r, t4i;
	int isAccSrc;
	scplx ct0;
	sint sconst;

	/* init o2 constant */
	for(int j = 0; j < NUMDP; j++) {
		o2.r.dp[j] = 0;
		o2.i.dp[j] = 0;
	}

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

			if(Opr3 && (!strcasecmp(Opr3, "HI") || !strcasecmp(Opr3, "HIRND"))) {
				for(int j = 0; j < NUMDP; j++) {
					ct1.r.dp[j] <<= 12;
					ct1.i.dp[j] <<= 12;
				}
			}

			for(int j = 0; j < NUMDP; j++) {
				if(isNeg12b(ct2.r.dp[j])){	/* shift right */
					if(ct2.r.dp[j] == -32){
						if(isAccSrc && isNeg24b(ct0.r.dp[j])){
							ct5.r.dp[j] = 0xFFFFFFFF;
						}else if(!isAccSrc && isNeg12b(ct0.r.dp[j])){
							ct5.r.dp[j] = 0xFFFFFFFF;
						} else {
							ct5.r.dp[j] = 0;
						}
					} else {
						/* shift right here: real part */
						int numshift = (-ct2.r.dp[j]);	/* number of shift */
					
						/* shift (numshift -1) bits */
						ct5.r.dp[j] = (ct1.r.dp[j] >> (numshift - 1));

						/* if rounding should be performed, */
						if(Opr3 && (!strcasecmp(Opr3, "LORND") || !strcasecmp(Opr3, "HIRND") || !strcasecmp(Opr3, "RND"))) {
							/* add 1 to intermediate result */
							ct5.r.dp[j] += 1;
						}

						/* shift last 1 bit */
						ct5.r.dp[j] = (ct5.r.dp[j] >> 1);
					}
				} else {				/* shift left */
					if(ct2.r.dp[j] == 32){
						ct5.r.dp[j] = 0;
					}else{
						ct5.r.dp[j] = ct1.r.dp[j] << ct2.r.dp[j];
					}
				}
				if(isNeg12b(ct2.i.dp[j])){	/* shift right */
					if(ct2.i.dp[j] == -32){
						if(isAccSrc && isNeg24b(ct0.i.dp[j])){
							ct5.i.dp[j] = 0xFFFFFFFF;
						}else if(!isAccSrc && isNeg12b(ct0.i.dp[j])){
							ct5.i.dp[j] = 0xFFFFFFFF;
						} else {
							ct5.i.dp[j] = 0;
						}
					} else {
						/* shift right here: imaginary part */
						int numshift = (-ct2.i.dp[j]);	/* number of shift */
					
						/* shift (numshift -1) bits */
						ct5.i.dp[j] = (ct1.i.dp[j] >> (numshift - 1));

						/* if rounding should be performed, */
						if(Opr3 && (!strcasecmp(Opr3, "LORND") || !strcasecmp(Opr3, "HIRND") || !strcasecmp(Opr3, "RND"))) {
							/* add 1 to intermediate result */
							ct5.i.dp[j] += 1;
						}

						/* shift last 1 bit */
						ct5.i.dp[j] = (ct5.i.dp[j] >> 1);
					}
				} else {				/* shift left */
					if(ct2.i.dp[j] == 32){
						ct5.i.dp[j] = 0;
					}else{
						ct5.i.dp[j] = ct1.i.dp[j] << ct2.i.dp[j];
					}
				}
			}
			break;
		case iLSHIFT_C:
		case iLSHIFTOR_C:
			/* check if 1st source is an accumulator */
			if(isAccSrc){
				;
			}else{
				for(int j = 0; j < NUMDP; j++) {
					ct1.r.dp[j] &= 0xFFF;
					ct1.i.dp[j] &= 0xFFF;
				}
			}

			for(int j = 0; j < NUMDP; j++) {
				t4r.dp[j] = ct1.r.dp[j];
				t4i.dp[j] = ct1.i.dp[j];
			}

			if(Opr3 && (!strcasecmp(Opr3, "HI") || !strcasecmp(Opr3, "HIRND"))) {
				for(int j = 0; j < NUMDP; j++) {
					t4r.dp[j] <<= 12;
					t4i.dp[j] <<= 12;
				}
			}

			for(int j = 0; j < NUMDP; j++) {
				if(isNeg12b(ct2.r.dp[j])){	/* shift right */
					if(ct2.r.dp[j] == -32){
						ct5.r.dp[j] = 0;
					} else {
						/* shift right here: real part */
						int numshift = (-ct2.r.dp[j]);	/* number of shift */
					
						/* shift (numshift -1) bits */
						t4r.dp[j] = (t4r.dp[j] >> (numshift - 1));

						/* if rounding should be performed, */
						if(Opr3 && (!strcasecmp(Opr3, "LORND") || !strcasecmp(Opr3, "HIRND") || !strcasecmp(Opr3, "RND"))) {
							/* add 1 to intermediate result */
							t4r.dp[j] += 1;
						}

						/* shift last 1 bit */
						ct5.r.dp[j] = (t4r.dp[j] >> 1);
					}
				} else {				/* shift left */
					if(ct2.r.dp[j] == 32){
						ct5.r.dp[j] = 0;
					}else{
						ct5.r.dp[j] = t4r.dp[j] << ct2.r.dp[j];
					}
				}
				if(isNeg12b(ct2.i.dp[j])){	/* shift right */
					if(ct2.i.dp[j] == -32){
						ct5.i.dp[j] = 0;
					} else {
						/* shift right here: imaginary part */
						int numshift = (-ct2.i.dp[j]);	/* number of shift */
					
						/* shift (numshift -1) bits */
						t4i.dp[j] = (t4i.dp[j] >> (numshift - 1));

						/* if rounding should be performed, */
						if(Opr3 && (!strcasecmp(Opr3, "LORND") || !strcasecmp(Opr3, "HIRND") || !strcasecmp(Opr3, "RND"))) {
							/* add 1 to intermediate result */
							t4i.dp[j] += 1;
						}

						/* shift last 1 bit */
						ct5.i.dp[j] = (t4i.dp[j] >> 1);
					}
				} else {				/* shift left */
					if(ct2.i.dp[j] == 32){
						ct5.i.dp[j] = 0;
					}else{
						ct5.i.dp[j] = t4i.dp[j] << ct2.i.dp[j];
					}
				}
			}
			break;
		default:
			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid SHIFT.C operation called in sProcessSHIFT_CFunc().\n");
			return;
			break;
	}
	
	if((p->Index == iASHIFTOR_C) || (p->Index == iLSHIFTOR_C)){	
		ct3 = scRdReg(Opr0);
		for(int j = 0; j < NUMDP; j++) {
			ct5.r.dp[j] |= ct3.r.dp[j];     /* OR */
			ct5.i.dp[j] |= ct3.i.dp[j];     /* OR */
		}
	}
	
	/* init zero constant */
	for(int j = 0; j < NUMDP; j++) {
		sconst.dp[j] = 0;
	}

	v2.r = sOVCheck(p->InstType, ct5.r, sconst);
	v2.i = sOVCheck(p->InstType, ct5.i, sconst);
	scFlagEffect(p->InstType, o2, o2, v2, o2, mask);

	scWrReg(Opr0, ct5.r, ct5.i, mask);

	if(VerboseMode){
		printf("sProcessSHIFT_CFunc() - Index: %s, Type: %s\n", sOp[p->Index], sType[p->InstType]);
	}
}

/** 
* @brief Update Ix register for post-modify ("+=") addressing mode.
* 
* @param s Name of register to write
* @param data Data to write (12-bit)
*/
void updateIReg(char *s, int data)
{
	int index = atoi(s+1);
	int loopSize;
	int base;

	if(s[0] == 'I' || s[0] == 'i') {		/* Ix register */

		/* read Lx register */
		if((::Cycles - LastAccessLx[index]) <= 1){				/* case: <= 1 - if too close, use old value */
			int diff_rbL0 = ::Cycles - LAbLx[index][0];
			int diff_rbL1 = ::Cycles - LAbLx[index][1];

			if(diff_rbL0 == 2){
				loopSize = rbL[index][1];
			}else{
				loopSize = rbL[index][0];
			}
		}else if((::Cycles - LastAccessLx[index]) == 2){		/* case: 2 - if too close, use old value */
			loopSize = rbL[index][0];
		}else{													/* case: >= 3 */
			loopSize = rL[index];
		}

		if(loopSize != 0){					/* if circular addressing */

			/* read Bx register */
			if((::Cycles - LastAccessBx[index]) <= 1){			/* case: <= 1 - if too close, use old value */
				int diff_rbB0 = ::Cycles - LAbBx[index][0];
				int diff_rbB1 = ::Cycles - LAbBx[index][1];

				if(diff_rbB0 == 2){
					base = rbB[index][1];
				}else{
					base = rbB[index][0];
				}
			}else if((::Cycles - LastAccessBx[index]) == 2){		/* case: 2 - if too close, use old value */
				base = rbB[index][0];
			}else{													/* case: >= 3 */
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
		int rn = atoi(s+1);

		/* back up values */
		/* no need!! */

		/* update */
		rbI[rn][1] = rbI[rn][0] = rI[rn] = data;	/* AGU update overrides LD/CP */
	}else{
		printRunTimeError(lineno, s, 
			"updateIReg() called with an non-Ix register argument.\n");
	}
}

/** 
* @brief Handle CORDIC-related algorithms (for iPOLAR_C and iRECT_C)
* 
* @param p Pointer to current instruction
* @param ct1 Complex value of first operand (r: magnitude in integer, i: angle in 0.14 format)
* @param Opr0 Pointer to destination operand string
* 
* @return cplx (r: x component, i: y component)
*/
//void processCordicFunc(sICode *p, cplx ct1, char *Opr0)
//{
//	cplx z2, n2;
//	const cplx o2 = { 0, 0 };
//	cplx ct5;
//
///*****************************************************************************/
//	printRunTimeError(lineno, s, 
//		"Cannot use processCordicFunc() in SIMD version of dspsim.\n");
///*****************************************************************************/
//
//	switch(p->Index){
//		case iRECT_C:
//			ct5 = runCordicRotationMode(ct1.r);
//			break;
//		case iPOLAR_C:
//			ct5.r = runCordicVectoringMode(ct1.r, ct1.i);
//			ct5.i = 0;
//			break;
//		default:
//			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
//						"Invalid CORDIC operation called in processCordicFunc().\n");
//			return;
//			break;
//	}
//
//	z2.r = (!ct5.r)?1:0;
//	z2.i = (!ct5.i)?1:0;
//	n2.r = isNeg12b(ct5.r);
//	n2.i = isNeg12b(ct5.i);
//
//	cWrReg(Opr0, ct5.r, ct5.i);
//
//	cFlagEffect(p->InstType, z2, n2, o2, o2);
//}

/** 
* @brief Handle CORDIC-related algorithms (for iPOLAR_C and iRECT_C) (SIMD version)
* 
* @param p Pointer to current instruction
* @param ct1 Complex SIMD value of first operand (r: magnitude in integer, i: angle in 0.14 format)
* @param Opr0 Pointer to destination operand string
* @param mask conditional execution mask
* 
* @return scplx (r: x component, i: y component)
*/
void sProcessCordicFunc(sICode *p, scplx ct1, char *Opr0, sint mask)
{
	scplx z2, n2;
	scplx o2;
	scplx ct5;
	cplx ct0;

	/* init o2 constant */
	for(int j = 0; j < NUMDP; j++) {
		o2.r.dp[j] = 0;
		o2.i.dp[j] = 0;
	}

	switch(p->Index){
		case iRECT_C:
			for(int j = 0; j < NUMDP; j++) {
				ct0 = runCordicRotationMode(ct1.r.dp[j]);
				ct5.r.dp[j] = ct0.r;
				ct5.i.dp[j] = ct0.i;
			}
			break;
		case iPOLAR_C:
			for(int j = 0; j < NUMDP; j++) {
				if(ct1.r.dp[j] == 0 && ct1.i.dp[j] == 0){	/* Exception: when (0, 0) is given */
					ct5.r.dp[j] = 0;						/* return angle 0 */
					ct5.i.dp[j] = 0;
				}else{
					ct5.r.dp[j] = runCordicVectoringMode(ct1.r.dp[j], ct1.i.dp[j]);
					ct5.i.dp[j] = 0;
				}
			}
			break;
		default:
			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid CORDIC operation called in processCordicFunc().\n");
			return;
			break;
	}

	for(int j = 0; j < NUMDP; j++) {
		z2.r.dp[j] = (!ct5.r.dp[j])?1:0;
		z2.i.dp[j] = (!ct5.i.dp[j])?1:0;
		n2.r.dp[j] = isNeg12b(ct5.r.dp[j]);
		n2.i.dp[j] = isNeg12b(ct5.i.dp[j]);
	}

	scWrReg(Opr0, ct5.r, ct5.i, mask);

	scFlagEffect(p->InstType, z2, n2, o2, o2, mask);
}

/** 
* @brief Handle complex arithmetic except CORDIC-related instructions (iPOLAR_C & iRECT_C)
* 
* @param p Pointer to current instruction
* @param ct1 Complex value of first operand 
* @param Opr0 Pointer to destination operand string
* 
* @return cplx (r: x component, i: y component)
*/
//void processComplexFunc(sICode *p, cplx ct1, char *Opr0)
//{
//	cplx z2, n2;
//	const cplx o2 = { 0, 0 };
//	cplx ct5;
//	int x, y, z;
//
//	switch(p->Index){
//		case iMAG_C:
//			x = ct1.r;
//			y = ct1.i;
//
//			/* compute |x|, |y| */
//			if(isNeg12b(x)) x = -x;
//			if(isNeg12b(y)) y = -y;
//
//			if (x > y){
//				z = x + (y >> 1);
//			}else{
//				z = (x >> 1) + y;
//			}
//			ct5.r = z;
//			ct5.i = 0;
//			break;
//		default:
//			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
//						"Invalid complex operation called in processComplexFunc().\n");
//			return;
//			break;
//	}
//
//	z2.r = (!ct5.r)?1:0;
//	z2.i = (!ct5.i)?1:0;
//	n2.r = isNeg12b(ct5.r);
//	n2.i = isNeg12b(ct5.i);
//
//	cWrReg(Opr0, ct5.r, ct5.i);
//
//	cFlagEffect(p->InstType, z2, n2, o2, o2);
//}

/** 
* @brief Handle complex SIMD arithmetic except CORDIC-related instructions (iPOLAR_C & iRECT_C)
* 
* @param p Pointer to current instruction
* @param ct1 Complex SIMD value of first operand 
* @param Opr0 Pointer to destination operand string
* @param mask conditional execution mask
* 
* @return scplx (r: x component, i: y component)
*/
void sProcessComplexFunc(sICode *p, scplx ct1, char *Opr0, sint mask)
{
	scplx z2, n2, v2, c2;
	scplx o2;
	scplx ct5, ct3;
	sint x, y, z;
	sint sconst;

	/* init o2 constant */
	for(int j = 0; j < NUMDP; j++) {
		o2.r.dp[j] = 0;
		o2.i.dp[j] = 0;
	}

	switch(p->Index){
		case iMAG_C:
			x = ct1.r;
			y = ct1.i;

			for(int j = 0; j < NUMDP; j++) {
				/* compute |x|, |y| */
				if(isNeg12b(x.dp[j])) x.dp[j] = -x.dp[j];
				if(isNeg12b(y.dp[j])) y.dp[j] = -y.dp[j];

				if (x.dp[j] > y.dp[j]){
					//z.dp[j] = x.dp[j] + (y.dp[j] >> 1);
					ct3.r.dp[j] = x.dp[j];
					ct3.i.dp[j] = (y.dp[j] >> 1);
					z.dp[j] = ct3.r.dp[j] + ct3.i.dp[j];
				}else{
					//z.dp[j] = (x.dp[j] >> 1) + y.dp[j];
					ct3.r.dp[j] = (x.dp[j] >> 1);
					ct3.i.dp[j] = y.dp[j];
					z.dp[j] = ct3.r.dp[j] + ct3.i.dp[j];
				}
				ct5.r.dp[j] = z.dp[j];
				ct5.i.dp[j] = 0;
			}
			break;
		default:
			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid complex operation called in sProcessComplexFunc().\n");
			return;
			break;
	}

	for(int j = 0; j < NUMDP; j++) {
		z2.r.dp[j] = (!ct5.r.dp[j])?1:0;
		z2.i.dp[j] = (!ct5.i.dp[j])?1:0;
		n2.r.dp[j] = isNeg12b(ct5.r.dp[j]);
		n2.i.dp[j] = isNeg12b(ct5.i.dp[j]);
	}


	for(int j = 0; j < NUMDP; j++) {
		sconst.dp[j] = 0;
	}
	v2.r = sOVCheck(p->InstType, ct3.r, ct3.i);
	v2.i = sconst;
	c2.r = sCarryCheck(p, ct3.r, ct3.i, sconst);
	c2.i = sconst;

	scFlagEffect(p->InstType, z2, n2, v2, c2, mask);

	scWrReg(Opr0, ct5.r, ct5.i, mask);
}

/** 
* @brief Reset program memory address to the given addr value.
* 
* @param addr New address
*/
void resetPMA(int addr)
{
	curaddr = addr;
}


/** 
* @brief Check if this register is read-only.
* 
* @param p Pointer to current instruction
* @param s Name of register to write
*
* @return TRUE or FALSE
*/
int isReadOnlyReg(sICode *p, char *s)
{
	int val = FALSE;

	if(!strcasecmp(s, "ASTAT.C")){		/* ASTAT.C */
		val = TRUE;
	}else if(!strcasecmp(s, "_SSTAT")){			/* _SSTAT */
		val = TRUE;
	}else if(!strcasecmp(s, "_DSTAT0")){			/* _DSTAT0 */
		val = TRUE;
	}else if(!strcasecmp(s, "_DSTAT1")){			/* _DSTAT1 */
		val = TRUE;
	}else if(!strcasecmp(s, "DID")){			/* _DID */
		val = TRUE;
	}

	if(val){
		printRunTimeWarning(p->LineCntr, s, 
			"This register is read-only - cannot be written.\n");
	}
	return val;
}



/** 
* @brief Handle unaligned double-word memory access cases. Called only for 24/32/64-bit accesses (not 12-bit).
* 
* @param p Pointer to current instruction
* @param addr Data memory address
* @param opr Pointer to operand string
*
* @return raddr If not aligned and UMA is allowed, return (addr-1), else (addr+1).
*/
int checkUnalignedMemoryAccess(sICode *p, int addr, char *opr)
{
	int raddr = addr+1;									//default: return address of (addr+1)

	if((addr & 0x01) && (!(p->MemoryRefExtern))){		//if not aligned

		for(int j = 0; j < NUMDP; j++) {
			rAstatC.UM.dp[j] = 1;
			rUMCOUNT.dp[j]++;
		}

		if(UnalignedMemoryAccessMode){					//if UMA is allowed, return address of (addr+1) which is even.
			raddr = addr+1;
			p->LatencyAdded = p->LatencyAdded +1;					//LatencyAdded++
		}else{											//if UMA is not aligned, make a runtime error and exit.
			printRunTimeError(p->LineCntr, opr,
			"24/32/64-bit memory read/write must be aligned to even-word boundaries.\n");
		}
	}else{												//if aligned, reset UM flag.
		for(int j = 0; j < NUMDP; j++) {
			rAstatC.UM.dp[j] = 0;
		}
	}
	return raddr;
}


