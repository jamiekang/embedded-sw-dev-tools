/*
All Rights Reserved.
*/
/** 
* @file dspdef.h
* Header for DSP architecture definition
* @date 2009-01-23
*/

#ifndef	_DSPDEF_H
#define	_DSPDEF_H

#include <stdio.h>			/* for NULL */

#define MAX_FIELD	15		/* max number of fields within a instruction format */

#define	REG_X_LO	24		/* lower bound of cross-path register window */
#define	REG_X_HI	31		/* upper bound of cross-path register window */
#define SCRPAD_LO	0xF000	/* lower bound of scratch pad memory address */
#define SCRPAD_HI	0xF07F	/* upper bound of scratch pad memory address */

/** 
* @brief Every opcode is represented as an eOp enum value after parsing.
*/
enum eOp
{
	iCOMMENT,	/**< for comment-only line */
	iLABEL,		/**< for label-only line */
	/** ALU instructions */
	iADD, 		/* ADD */
	iADD_C,		/* ADD.C */
	iADDC, 		/* ADDC */
	iADDC_C,	/* ADDC.C */
	iSUB, 		/* SUB */
	iSUB_C,		/* SUB.C */
	iSUBC, 		/* SUBC */
	iSUBC_C,	/* SUBC.C  */
	iSUBB, 		/* SUBB    */
	iSUBB_C,	/* SUBB.C  */
	iSUBBC, 	/* SUBBC   */
	iSUBBC_C,	/* SUBBC.C */
	iAND, iOR, iXOR,
	iTSTBIT, iSETBIT, iCLRBIT, iTGLBIT,
	iNOT, iNOT_C,
	iABS, iABS_C,
	iINC, iDEC,
	iDIVS, iDIVQ,
	iSCR, iSCR_C,
	iRCCW_C, iRCW_C,
	/* Note: |ALU| NONE is not regarded as an 'independent' opcode here */
	/** MAC instructions */
	iMPY, iMPY_C, iMPY_RC, 
	iMAC, iMAC_C, iMAC_RC,
	iMAS, iMAS_C, iMAS_RC, 
	iRNDACC, iRNDACC_C,
	iCLRACC, iCLRACC_C,
	iSATACC, iSATACC_C,
	/* Note: |MAC| NONE is not regarded as an 'independent' opcode here */
	/** Shifter instructions */
	iASHIFT, iASHIFT_C, iASHIFTOR, iASHIFTOR_C,
	iLSHIFT, iLSHIFT_C, iLSHIFTOR, iLSHIFTOR_C,
	iEXP, iEXP_C,
	iEXPADJ, iEXPADJ_C,
	/** Multifunction instructions */
	iMAC_LD_LD, iMAC_C_LD_C_LD_C,
	iALU_LD_LD, iALU_C_LD_C_LD_C,
	iLD_LD, iLD_C_LD_C,
	iMAC_LD, iMAC_C_LD_C,
	iALU_LD, iALU_C_LD_C,
	iSHIFT_LD, iSHIFT_C_LD_C,
	iMAC_ST, iMAC_C_ST_C,
	iALU_ST, iALU_C_ST_C,
	iSHIFT_ST, iSHIFT_C_ST_C,
	iMAC_CP, iMAC_C_CP_C,
	iALU_CP, iALU_C_CP_C,
	iSHIFT_CP, iSHIFT_C_CP_C,
	iALU_MAC, iALU_C_MAC_C,
	iALU_SHIFT, iALU_C_SHIFT_C,
	iMAC_SHIFT, iMAC_C_SHIFT_C,
	/** Data Move instructions */
	iCP, iCP_C,
	iLD, iLD_C,
	iST, iST_C,
	iCPXI, iCPXI_C,
	iCPXO, iCPXO_C,
	/** Program Flow instructions */
	iDO,
	iJUMP,
	iCALL,
	iRTI, iRTS,
	iPUSH, iPOP,
	iFLUSH, 
	iSETINT, iCLRINT,
	iNOP, iIDLE,
	iENA, iDIS,
	iENADP, iDPID,
	iRESET,
	/** Complex Arithmetic instructions */
	iPOLAR_C, iRECT_C, iCONJ_C,
	iMAG_C,
	/** Assembler pseudo instructions */
	i_GLOBAL, i_EXTERN, i_VAR, i_CODE, i_DATA, i_EQU,
	/** Unknown instructions */
	iUNKNOWN,
};

/** 
* Instruction type - Each instruction can have different type even 
* with the same opcode.
*/
enum eType {
	t01a = 1, t01b, t01c, 
	t03a, t03c, t03d, t03e, t03f, t03g, t03h, t03i,
	t04a, t04b, t04c, t04d, t04e, t04f, t04g, t04h,
	t06a, t06b, t06c, t06d,
	t08a, t08b, t08c, t08d,
	t09c, t09d, t09e, t09f, t09g, t09h, t09i,
	t10a, t10b, 
	t11a, 
	t12e, t12f, t12g, t12h, t12m, t12n, t12o, t12p,
	t14c, t14d, t14k, t14l, 
	t15a, t15b, t15c, t15d, t15e, t15f,
	t16a, t16b, t16c, t16d, t16e, t16f,
	t17a, t17b, t17c, t17d, t17e, t17f, t17g, t17h,
	t18a, t18b, t18c,
	t19a,
	t20a, 
	t22a, 
	t23a, 
	t25a, t25b, 
	t26a, 
	t29a, t29b, t29c, t29d,
	t30a, 
	t31a, 
	t32a, t32b, t32c, t32d,
	t37a, 
	t40e, t40f, t40g, 
	t41a, t41b, t41c, t41d, 
	t42a, t42b, 
	t43a, 
	t44b, t44d,
	t45b, t45d,
	t46a, t46b,
	t47a,
	t48a,
	t49a, t49b, t49c, t49d,
	t50a, 
	eTypeEnd,
};


/** 
* @brief Every Instruction Field is an eField enum value after parsing.
*/
enum eField {
	fOpcodeType, fAMF, fSF, fSF2, fAF, 
	fMF, fYZ, fDSTA, fSRCA1, fSRCA2, 
	fDSTM, fSRCM1, fSRCM2, fDSTS, fSRCS1,	
	fSRCS2,	fDSTLS,	fSRCLS,	fDSTC, fSRCC, 
	fDSTB, fDSTID, fTERM, fINTN, fU, 
	fU2, fS, fT, fLPP, fPPP, 
	fSPP, fC, fCONJ, fENA, fPOLAR, 
	fDD, fPD, 
	fDC, fLSF, fDF, fMN, 
	fIDE, fIDM, fIDR, fIDN, fCW,
	fReserved, 
	fCOND,
	fPMM, fPMI, fDMM, fDMI, 
};


/** 
* @brief Every register is represented as an eRegIndex enum value after parsing.
*/
enum eRegIndex
{
	eR0, eR1, eR2, eR3, 
	eR4, eR5, eR6, eR7,
	eR8, eR9, eR10, eR11, 
	eR12, eR13, eR14, eR15,
	eR16, eR17, eR18, eR19, 
	eR20, eR21, eR22, eR23,
	eR24, eR25, eR26, eR27, 
	eR28, eR29, eR30, eR31,
	eACC0_L, eACC0_M, eACC0_H, eACC0,
	eACC1_L, eACC1_M, eACC1_H, eACC1,
	eACC2_L, eACC2_M, eACC2_H, eACC2,
	eACC3_L, eACC3_M, eACC3_H, eACC3,
	eACC4_L, eACC4_M, eACC4_H, eACC4,
	eACC5_L, eACC5_M, eACC5_H, eACC5,
	eACC6_L, eACC6_M, eACC6_H, eACC6,
	eACC7_L, eACC7_M, eACC7_H, eACC7,
	eI0, eI1, eI2, eI3,
	eI4, eI5, eI6, eI7,
	eM0, eM1, eM2, eM3,
	eM4, eM5, eM6, eM7,
	eL0, eL1, eL2, eL3,
	eL4, eL5, eL6, eL7,
	eB0, eB1, eB2, eB3,
	eB4, eB5, eB6, eB7,
	eCNTR, eMSTAT, eSSTAT, eLPSTACK,
	ePCSTACK, eASTAT_R, eASTAT_I, eASTAT_C, 
	eICNTL, eIMASK, eIRPTL, eCACTL, 
	ePX, eLPEVER, eDummy1, eDummy2,
	eIVEC0, eIVEC1, eIVEC2, eIVEC3,
	eDSTAT0, eDSTAT1, eUMCOUNT, eDID,
	eNONE,
};

/** 
* @brief Register Type: Every register operand belongs to one of register type.
*/
enum eRegType {
	eREG12, eREG24, eDREG12, eDREG24, eACC12, eACC24, eACC32, eACC64, 
	eXOP12, eXOP24, eYOP12, eYOP24, eXOP12S, eXOP24S, eYOP12S, eYOP24S,
	eIREG, eMREG, eLREG, eIX, eMX, eLX, eIY, eMY, eLY, eRREG, eRREG16, eCREG,
	eREG12S, eACC12S, eDREG12S, eACC32S, eREG24S, eACC24S, eDREG24S, eACC64S,
	eXREG12, eXREG24,
};

/** 
* @brief Immediate Type: Imm. Category according to bit-width and sign bit (if any).
*/
enum eImmType {
	eIMM_INT4, eIMM_INT5, eIMM_INT6, eIMM_INT8,
	eIMM_INT12, eIMM_INT13, eIMM_INT16, eIMM_INT24,
	eIMM_COMPLEX8, eIMM_COMPLEX24, eIMM_COMPLEX32,
	eIMM_UINT2, eIMM_UINT4, eIMM_UINT12, eIMM_UINT16,
};

/** 
* Multiplication option for MAC (t40e/f/g) instructions.
*/
enum eMul {
	emRND = 1, emSS, emSU, emUS, emUU,
};

/** 
* Shift option for SHIFT (t15a/15b/16a/16b/16c/16d) instructions.
*/
enum eShift {
	esHI = 1, esLO, 
	esHIRND, esLORND,
	esNORND, esRND,
	esHIX, 
};

typedef struct sBinMapping {
	char *bits; /* pointer to binary opcode string */
	int width;  /* number of bits */
} sBinMapping;

typedef struct sInstFormat {
	int fields[MAX_FIELD];		/* fields (eField enum values) */
	int widths[MAX_FIELD];		/* field widths */
} sInstFormat;

extern const char *sOp[];						/**< Opcode mnemonics */
extern const char *sType[];						/**< Instruction type mnemonics */
extern const char *sField[];					/**< Instruction field mnemonics */
extern const char *sResSym[];					/**< Reserved keywords mnemonics */
extern const sInstFormat sInstFormatTable[];	/**< Instruction format table */
extern const sBinMapping sBinOp[];				/**< Binary opcode mapping table */

#endif	/* _DSPDEF_H */
