/*
All Rights Reserved.
*/
/** 
* @file dspdef.cc
* DSP architecture definition
*
* @date 2009-01-23
*/

#ifdef BINSIM
	#include "binsim.h"
#else 
	#ifdef DSPSIM
		#include "dspsim.h"
	#else
		#ifdef DSPLNK
			#include "dsplnk.h"
		#endif
	#endif
#endif

#include "dspdef.h"

/** 
* @brief Opcode mnemonics; use eOp for array indexing
*/
const char *sOp[] = 
{
	"(COMMENT)",	/**< for comment-only line */
	"(LABEL)",		/**< for label-only line */
	/** ALU instructions */
	"ADD", "ADD.C", "ADDC", "ADDC.C",
	"SUB", "SUB.C", "SUBC", "SUBC.C",
	"SUBB", "SUBB.C", "SUBBC", "SUBBC.C",
	"AND", "OR", "XOR",
	"TSTBIT", "SETBIT", "CLRBIT", "TGLBIT",
	"NOT", "NOT.C",
	"ABS", "ABS.C",
	"INC", "DEC",
	"DIVS", "DIVQ",
	"SCR", "SCR.C",
	/* Note: |ALU| NONE is not regarded as an 'independent' opcode here */
	/** MAC instructions */
	"MPY", "MPY.C", "MPY.RC", 
	"MAC", "MAC.C", "MAC.RC",
	"MAS", "MAS.C", "MAS.RC", 
	"RNDACC", "RNDACC.C",
	"CLRACC", "CLRACC.C",
	"SATACC", "SATACC.C",
	/* Note: |MAC| NONE is not regarded as an 'independent' opcode here */
	/** Shifter instructions */
	"ASHIFT", "ASHIFT.C", "ASHIFTOR", "ASHIFTOR.C",
	"LSHIFT", "LSHIFT.C", "LSHIFTOR", "LSHIFTOR.C",
	"EXP", "EXP.C",
	"EXPADJ", "EXPADJ.C",
	/** Multifunction instructions */
	"MAC||LD||LD", "MAC.C||LD.C||LD.C",
	"ALU||LD||LD", "ALU.C||LD.C||LD.C",
	"LD||LD", "LD.C||LD.C",
	"MAC||LD", "MAC.C||LD.C",
	"ALU||LD", "ALU.C||LD.C",
	"SHIFT||LD", "SHIFT.C||LD.C",
	"MAC||ST", "MAC.C||ST.C",
	"ALU||ST", "ALU.C||ST.C",
	"SHIFT||ST", "SHIFT.C||ST.C",
	"MAC||CP", "MAC.C||CP.C",
	"ALU||CP", "ALU.C||CP.C",
	"SHIFT||CP", "SHIFT.C||CP.C",
	"ALU||MAC", "ALU.C||MAC.C",
	"ALU||SHIFT", "ALU.C||SHIFT.C",
	"MAC||SHIFT", "MAC.C||SHIFT.C",
	/** Data Move instructions */
	"CP", "CP.C",
	"LD", "LD.C",
	"ST", "ST.C",
	"MODIFY",
	/** Program Flow instructions */
	"DO",
	"JUMP",
	"CALL",
	"RTI", "RTS",
	"PUSH", "POP",
	"FLUSH", 
	"SETINT", "CLRINT",
	"NOP", "IDLE",
	"ENA", "DIS",
	/** Complex Arithmetic instructions */
	"POLAR.C", "RECT.C", "CONJ.C",
	/** Assembler pseudo instructions */
	".GLOBAL", ".EXTERN", ".VAR", ".CODE", ".DATA",
	"(Unknown)",
	NULL
};

/** 
* @brief Instruction type mnemonics; use eType for array indexing
*/
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
		"12e", "12f", "12g", "12h", "12m", "12n", "12o", "12p",
		"14c", "14d", "14k", "14l", 
		"15a", "15b", "15c", "15d", "15e", "15f",
		"16a", "16b", "16c", "16d", "16e", "16f",
		"17a", "17b", "17c", "17d", "17e", "17f", "17g", "17h",
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
		"44b", "44d",
		"45b", "45d",
		"46a", "46b",
		NULL
};

/** 
* @brief Instruction Field mnemonics; use eField for array indexing
*/
const char *sField[] = 
{
	"OpcodeType", "AMF", "SF", "SF2", "AF", 
	"MF", "YZ", "DSTA", "SRCA1", "SRCA2", 
	"DSTM", "SRCM1", "SRCM2", "DSTS", "SRCS1", 
	"SRCS2", "DSTLS", "SRCLS", "DSTC", "SRCC", 
	"DSTB", "TERM", "INTN", "U", "U2", 
	"S", "T", "LPP", "PPP", "SPP", 
	"C", "CONJ", "ENA", "POLAR", "DD", 
	"PD", "DMI", "DMM", "PMI", "PMM", 
	"DC", "LSF", "DF", "MN", "Reserved", 
	"COND",
	NULL
};

/** 
* Instruction format: use eType for array indexing
*/
const sInstFormat sInstFormatTable[] = 
{
	{ {}, {} },		/* NULL */
	{ 	
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fU, fU2, fDD, fPD, fDMI, fDMM, fPMI, fPMM, 0, },
	  	{ sBinOp[t01a].width,   4,     2,      3,      3,  1,   1,   3,   3,    2,    2,    2,    2, 0, } 
	}, /* 01a */
	{ 	
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fU, fU2, fDD, fPD, fDMI, fDMM, fPMI, fPMM, 0, }, 
	  	{ sBinOp[t01b].width,   4,     2,      2,      2,  1,   1,   2,   2,    2,    2,    2,    2, 0, } 
	}, /* 01b */
	{ 	
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fU, fU2, fDD, fPD, fDMI, fDMM, fPMI, fPMM, 0, },
	  	{ sBinOp[t01c].width,   4,     4,      3,      3,  1,   1,   3,    3,    2,    2,    2,   2, 0, } 
	}, /* 01c */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t03a].width,      6,     16, 0, } 
	}, /* 03a */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t03c].width,      5,     16, 0, } 
	}, /* 03c */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fLSF, 0, },
	  	{ sBinOp[t03d].width,      3,     16,    1, 0, } 
	}, /* 03d */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fLSF, 0, },
	  	{ sBinOp[t03e].width,      2,     16,    1, 0, } 
	}, /* 03e */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t03f].width,      6,     16, 0, } 
	}, /* 03f */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t03g].width,      5,     16, 0, } 
	}, /* 03g */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fLSF, 0, },
	  	{ sBinOp[t03h].width,      3,     16,    1, 0, } 
	}, /* 03h */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fLSF, 0, },
	  	{ sBinOp[t03i].width,      2,     16,    1, 0, } 
	}, /* 03i */
	{ 
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fDSTLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t04a].width,   4,     2,      3,      3,      6,   1,   3,    3, 0, } 
	}, /* 04a */
	{ 
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fDSTLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t04b].width,   4,     2,      2,      2,      5,  1,    3,    3, 0, } 
	}, /* 04b */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fDSTLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t04c].width,   4,     4,      3,      3,      6,  1,    3,    3, 0, } 
	}, /* 04c */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fDSTLS, fU,  fDMI, fDMM, 0, },
	  	{ sBinOp[t04d].width,   4,     3,      2,      2,      5,  1,     3,    3, 0, } 
	}, /* 04d */
	{ 
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fSRCLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t04e].width,   4,     2,      3,      3,      6,  1,    3,    3, 0, } 
	}, /* 04e */
	{ 
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fSRCLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t04f].width,   4,     2,      2,      2,      5,  1,    3,    3, 0, } 
	}, /* 04f */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fSRCLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t04g].width,   4,     4,      3,      3,      6,  1,    3,    3, 0, } 
	}, /* 04g */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fSRCLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t04h].width,   4,     3,      2,      2,      5,  1,    3,    3, 0, } 
	}, /* 04h */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t06a].width,      6,     16, 0, } 
	}, /* 06a */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t06b].width,      6,     12, 0, } 
	}, /* 06b */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t06c].width,      5,     24, 0, } 
	}, /* 06c */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t06d].width,      3,     24, 0, } 
	}, /* 06d */
	{ 
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t08a].width,   4,     2,      3,      3,      6,      6, 0, } 
	}, /* 08a */
	{ 
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t08b].width,   4,     2,      2,      2,      5,      5, 0, } 
	}, /* 08b */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t08c].width,   4,     4,      3,      3,      6,      6, 0, } 
	}, /* 08c */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t08d].width,   4,     3,      2,      2,      5,      5, 0, } 
	}, /* 08d */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fCOND, 0, },
	  	{ sBinOp[t09c].width,   4,     6,      5,      5,     5, 0, } 
	}, /* 09c */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fCONJ, fCOND, 0, },
	  	{ sBinOp[t09d].width,   4,     5,      4,      4,     1,     5, 0, } 
	}, /* 09d */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fCOND, 0, },
	  	{ sBinOp[t09e].width,   4,     6,      5,      4,     5, 0, } 
	}, /* 09e */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fCONJ, fCOND, 0, },
	  	{ sBinOp[t09f].width,   4,     5,      4,      8,     1,     5, 0, } 
	}, /* 09f */
	{ 
		{ fOpcodeType,        fAMF, fDSTM, fSRCM1, fSRCM2, fCOND, 0, },
	  	{ sBinOp[t09g].width,    5,     3,      3,      3,     5, 0, } 
	}, /* 09g */
	{ 
		{ fOpcodeType,        fAMF, fDSTM, fSRCM1, fSRCM2, fCONJ, fCOND, 0, },
	  	{ sBinOp[t09h].width,    5,     2,      2,      2,     1,     5, 0, } 
	}, /* 09h */
	{ 
		{ fOpcodeType,        fAF, fYZ, fDSTA, fSRCA1, fSRCA2, fCOND, 0, },
	  	{ sBinOp[t09i].width,   4,   1,     6,      5,      4,     5, 0, } 
	}, /* 09i */
	{ 
		{ fOpcodeType,        fDSTB, fS, fCOND, 0, },
	  	{ sBinOp[t10a].width,    13,  1,     5, 0, } 
	}, /* 10a */
	{ 
		{ fOpcodeType,        fDSTB, fS, 0, },
	  	{ sBinOp[t10b].width,    16,  1, 0, } 
	}, /* 10b */
	{ 
		{ fOpcodeType,        fDSTB, fTERM, 0, },
	  	{ sBinOp[t11a].width,    12,     4, 0, } 
	}, /* 11a */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fDSTLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t12e].width,    4,     2,      3,      5,      6,  1,    3,    3, 0, } 
	}, /* 12e */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fDSTLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t12f].width,    4,     2,      2,      5,      5,  1,    3,    3, 0, } 
	}, /* 12f */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fSRCLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t12g].width,    4,     2,      3,      5,      6,  1,    3,    3, 0, } 
	}, /* 12g */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fSRCLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t12h].width,    4,     2,      2,      5,      5,  1,    3,    3, 0, } 
	}, /* 12h */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fDSTLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t12m].width,    4,     2,      2,      5,      6,  1,    3,    3, 0, } 
	}, /* 12m */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fDSTLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t12n].width,    4,     2,      2,      5,      5,  1,    3,    3, 0, } 
	}, /* 12n */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fSRCLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t12o].width,    4,     2,      2,      5,      6,  1,    3,    3, 0, } 
	}, /* 12o */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fSRCLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t12p].width,    4,     2,      2,      5,      5,  1,    3,    3, 0, } 
	}, /* 12p */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t14c].width,    4,     2,      3,      5,      6,      6, 0, } 
	}, /* 14c */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t14d].width,    4,     2,      2,      5,      5,      5, 0, } 
	}, /* 14d */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t14k].width,    4,     2,      2,      5,      6,      6, 0, } 
	}, /* 14k */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t14l].width,    4,     2,      2,      5,      5,      5, 0, } 
	}, /* 14l */
	{ 
		{ fOpcodeType,        fSF, fDSTS, fSRCS1, fSRCS2, fCOND, 0, },
	  	{ sBinOp[t15a].width,   5,     3,      5,      5,     5, 0, } 
	}, /* 15a */
	{ 
		{ fOpcodeType,        fSF, fDSTS, fSRCS1, fSRCS2, fCOND, 0, },
	  	{ sBinOp[t15b].width,   5,     2,      4,      5,     5, 0, } 
	}, /* 15b */
	{ 
		{ fOpcodeType,        fSF, fDSTS, fSRCS1, fSRCS2, fCOND, 0, },
	  	{ sBinOp[t15c].width,   5,     3,      3,      6,     5, 0, } 
	}, /* 15c */
	{ 
		{ fOpcodeType,        fSF, fDSTS, fSRCS1, fSRCS2, fCOND, 0, },
	  	{ sBinOp[t15d].width,   5,     2,      2,      6,     5, 0, } 
	}, /* 15d */
	{ 
		{ fOpcodeType,        fSF, fDSTS, fSRCS1, fSRCS2, fCOND, 0, },
	  	{ sBinOp[t15e].width,   5,     6,      5,      5,     5, 0, } 
	}, /* 15e */
	{ 
		{ fOpcodeType,        fSF, fDSTS, fSRCS1, fSRCS2, fCOND, 0, },
	  	{ sBinOp[t15f].width,   5,     5,      4,      5,     5, 0, } 
	}, /* 15f */
	{ 
		{ fOpcodeType,        fSF, fDSTS, fSRCS1, fSRCS2, fCOND, 0, },
	  	{ sBinOp[t16a].width,   5,     3,      5,      5,     5, 0, } 
	}, /* 16a */
	{ 
		{ fOpcodeType,        fSF, fDSTS, fSRCS1, fSRCS2, fCOND, 0, },
	  	{ sBinOp[t16b].width,   5,     2,      4,      5,     5, 0, } 
	}, /* 16b */
	{ 
		{ fOpcodeType,        fSF, fDSTS, fSRCS1, fCOND, 0, },
	  	{ sBinOp[t16c].width,   5,     6,      5,     5, 0, } 
	}, /* 16c */
	{ 
		{ fOpcodeType,        fSF, fDSTS, fSRCS1, fCOND, 0, },
	  	{ sBinOp[t16d].width,   5,     5,      4,     5, 0, } 
	}, /* 16d */
	{ 
		{ fOpcodeType,        fSF, fDSTS, fSRCS1, fSRCS2, fCOND, 0, },
	  	{ sBinOp[t16e].width,   5,     6,      5,      5,     5, 0, } 
	}, /* 16e */
	{ 
		{ fOpcodeType,        fSF, fDSTS, fSRCS1, fSRCS2, fCOND, 0, },
	  	{ sBinOp[t16f].width,   5,     5,      4,      5,     5, 0, } 
	}, /* 16f */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t17a].width,      6,      6, 0, } 
	}, /* 17a */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fDC, 0, },
	  	{ sBinOp[t17b].width,      6,      6,   1, 0, } 
	}, /* 17b */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t17c].width,      3,      3, 0, } 
	}, /* 17c */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t17d].width,      5,      5, 0, } 
	}, /* 17d */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fDC, 0, },
	  	{ sBinOp[t17e].width,      5,      6,   1, 0, } 
	}, /* 17e */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t17f].width,      2,      2, 0, } 
	}, /* 17f */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t17g].width,      6,      6, 0, } 
	}, /* 17g */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, 0, },
	  	{ sBinOp[t17h].width,      3,      6, 0, } 
	}, /* 17h */
	{ 
		{ fOpcodeType,        fENA, 0, },
	  	{ sBinOp[t18a].width,   18, 0, } 
	}, /* 18a */
	{ 
		{ fOpcodeType,        fS, fDMI, fCOND, 0, },
	  	{ sBinOp[t19a].width,  1,    3,     5, 0, } 
	}, /* 19a */
	{ 
		{ fOpcodeType,        fT, fCOND, 0, },
	  	{ sBinOp[t20a].width,  1,     5, 0, } 
	}, /* 20a */
	{ 
		{ fOpcodeType,        fDSTLS, fDMI, fDMM, 0, },
	  	{ sBinOp[t22a].width,     12,    3,    3, 0, } 
	}, /* 22a */
	{ 
		{ fOpcodeType,        fDSTA, fSRCA1, fSRCA2, fDF, 0, },
	  	{ sBinOp[t23a].width,     5,      5,      5,   1, 0, } 
	}, /* 23a */
	{ 
		{ fOpcodeType,        fDSTM, fCOND, 0, },
	  	{ sBinOp[t25a].width,     3,     5, 0, } 
	}, /* 25a */
	{ 
		{ fOpcodeType,        fDSTM, fCONJ, fCOND, 0, },
	  	{ sBinOp[t25b].width,     2,     1,     5, 0, } 
	}, /* 25b */
	{ 
		{ fOpcodeType,        fLPP, fPPP, fSPP, 0, },
	  	{ sBinOp[t26a].width,    2,    2,    2, 0, } 
	}, /* 26a */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fU, fDMI, 0, },
	  	{ sBinOp[t29a].width,      6,      8,  1,    3, 0, } 
	}, /* 29a */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fU, fDMI, 0, },
	  	{ sBinOp[t29b].width,      5,      8,  1,    3, 0, } 
	}, /* 29b */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fU, fDMI, 0, },
	  	{ sBinOp[t29c].width,      6,      8,  1,    3, 0, } 
	}, /* 29c */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fU, fDMI, 0, },
	  	{ sBinOp[t29d].width,      5,      8,  1,    3, 0, } 
	}, /* 29d */
	{ 
		{ fOpcodeType,        0, },
	  	{ sBinOp[t30a].width, 0, } 
	}, /* 30a */
	{ 
		{ fOpcodeType,        0, },
	  	{ sBinOp[t31a].width, 0, } 
	}, /* 31a */
	{ 
		{ fOpcodeType,        fDSTLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t32a].width,      6,  1,    3,    3, 0, } 
	}, /* 32a */
	{ 
		{ fOpcodeType,        fDSTLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t32b].width,      5,  1,    3,    3, 0, } 
	}, /* 32b */
	{ 
		{ fOpcodeType,        fDSTLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t32c].width,      6,  1,    3,    3, 0, } 
	}, /* 32c */
	{ 
		{ fOpcodeType,        fDSTLS, fU, fDMI, fDMM, 0, },
	  	{ sBinOp[t32d].width,      5,  1,    3,    3, 0, } 
	}, /* 32d */
	{ 
		{ fOpcodeType,        fINTN, fC, 0, },
	  	{ sBinOp[t37a].width,     4,  1, 0, } 
	}, /* 37a */
	{ 
		{ fOpcodeType,        fAMF, fDSTM, fSRCM1, fSRCM2, fMN, fCOND, 0, },
	  	{ sBinOp[t40e].width,    5,     3,      5,      5,   1,     5, 0, } 
	}, /* 40e */
	{ 
		{ fOpcodeType,        fAMF, fDSTM, fSRCM1, fSRCM2, fCONJ, fMN, fCOND, 0, },
	  	{ sBinOp[t40f].width,    5,     2,      4,      4,     1,   1,     5, 0, } 
	}, /* 40f */
	{ 
		{ fOpcodeType,        fAMF, fDSTM, fSRCM1, fSRCM2, fCONJ, fMN, fCOND, 0, },
	  	{ sBinOp[t40g].width,    5,     2,      5,      4,     1,   1,     5, 0, } 
	}, /* 40g */
	{ 
		{ fOpcodeType,        fDSTM, fCOND, 0, },
	  	{ sBinOp[t41a].width,     3,     5, 0, } 
	}, /* 41a */
	{ 
		{ fOpcodeType,        fDSTM, fCONJ, fCOND, 0, },
	  	{ sBinOp[t41b].width,     2,     1,     5, 0, } 
	}, /* 41b */
	{ 
		{ fOpcodeType,        fDSTM, fCOND, 0, },
	  	{ sBinOp[t41c].width,     3,     5, 0, } 
	}, /* 41c */
	{ 
		{ fOpcodeType,        fDSTM, fCOND, 0, },
	  	{ sBinOp[t41d].width,     2,     5, 0, } 
	}, /* 41d */
	{ 
		{ fOpcodeType,        fDSTC, fSRCC, fCONJ, fPOLAR, 0, },
	  	{ sBinOp[t42a].width,     5,     4,     1,      1, 0, } 
	}, /* 42a */
	{ 
		{ fOpcodeType,        fDSTC, fSRCC, 0, },
	  	{ sBinOp[t42b].width,     5,     4, 0, } 
	}, /* 42b */
	{ 
		{ fOpcodeType,        fAF, fMF, fDSTA, fSRCA1, fSRCA2, fDSTM, fSRCM1, fSRCM2, 0, },
	  	{ sBinOp[t43a].width,   4,   4,     4,      3,      3,     2,      3,      3, 0, } 
	}, /* 43a */
	{ 
		{ fOpcodeType,        fSF2, fAF, fDSTA, fSRCA1, fSRCA2, fDSTS, fSRCS1, fSRCS2, 0, },
	  	{ sBinOp[t44b].width,    4,   4,     4,      3,      3,     2,      3,      5, 0, } 
	}, /* 44b */
	{ 
		{ fOpcodeType,        fSF2, fAF, fDSTA, fSRCA1, fSRCA2, fDSTS, fSRCS1, fSRCS2, 0, },
	  	{ sBinOp[t44d].width,    4,   4,     4,      3,      3,     2,      2,      5, 0, } 
	}, /* 44d */
	{ 
		{ fOpcodeType,        fSF2, fMF, fDSTM, fSRCM1, fSRCM2, fDSTS, fSRCS1, fSRCS2, 0, },
	  	{ sBinOp[t45b].width,    4,   4,     2,      3,      3,     2,      3,      5, 0, } 
	}, /* 45b */
	{ 
		{ fOpcodeType,        fSF2, fMF, fDSTM, fSRCM1, fSRCM2, fDSTS, fSRCS1, fSRCS2, 0, },
	  	{ sBinOp[t45d].width,    4,   4,     2,      3,      3,     2,      2,      5, 0, } 
	}, /* 45d */
	{ 
		{ fOpcodeType,        fDSTA, fSRCA1, fSRCA2, fCOND, 0, },
	  	{ sBinOp[t46a].width,     6,      5,      5,     5, 0, } 
	}, /* 46a */
	{ 
		{ fOpcodeType,        fDSTA, fSRCA1, fSRCA2, fCONJ, fCOND, 0, },
	  	{ sBinOp[t46b].width,     5,      4,      4,     1,     5, 0, } 
	}, /* 46b */
};

/** 
* Binary Opcode mapping; use eType for array indexing
* Last update: 2009-02-02
* Total 88 unique binary opcodes, each 1-to-1 mapped to instruction type.
*/
const sBinMapping sBinOp[] = 
{
	{ NULL                              ,  0 },                              
	{ "0110                            ",  4 }, /* 01a */
	{ "11101100                        ",  8 }, /* 01b */
	{ "00                              ",  2 }, /* 01c */
	{ "1111011000                      ", 10 }, /* 03a */
	{ "11110111110                     ", 11 }, /* 03c */
	{ "111110001010                    ", 12 }, /* 03d */
	{ "1111100011100                   ", 13 }, /* 03e */
	{ "1111011001                      ", 10 }, /* 03f */
	{ "11110111111                     ", 11 }, /* 03g */
	{ "111110001011                    ", 12 }, /* 03h */
	{ "1111100011101                   ", 13 }, /* 03i */
	{ "1101100                         ",  7 }, /* 04a */
	{ "1111011010                      ", 10 }, /* 04b */
	{ "10000                           ",  5 }, /* 04c */
	{ "111100100                       ",  9 }, /* 04d */
	{ "1101101                         ",  7 }, /* 04e */
	{ "1111011011                      ", 10 }, /* 04f */
	{ "10001                           ",  5 }, /* 04g */
	{ "111100101                       ",  9 }, /* 04h */
	{ "1111011100                      ", 10 }, /* 06a */
	{ "11111001000100                  ", 14 }, /* 06b */
	{ "010                             ",  3 }, /* 06c */
	{ "10010                           ",  5 }, /* 06d */
	{ "11101101                        ",  8 }, /* 08a */
	{ "111110001100                    ", 12 }, /* 08b */
	{ "101110                          ",  6 }, /* 08c */
	{ "11111000000                     ", 11 }, /* 08d */
	{ "1101110                         ",  7 }, /* 09c */
	{ "111100110                       ",  9 }, /* 09d */
	{ "11101110                        ",  8 }, /* 09e */
	{ "10011                           ",  5 }, /* 09f */
	{ "1111100011110                   ", 13 }, /* 09g */
	{ "111110010010010                 ", 15 }, /* 09h */
	{ "1101111                         ",  7 }, /* 09i */
	{ "1111100011111                   ", 13 }, /* 10a */
	{ "111110010010011                 ", 15 }, /* 10b */
	{ "1111100100101100                ", 16 }, /* 11a */
	{ "10100                           ",  5 }, /* 12e */
	{ "1110000                         ",  7 }, /* 12f */
	{ "10101                           ",  5 }, /* 12g */
	{ "1110001                         ",  7 }, /* 12h */
	{ "101111                          ",  6 }, /* 12m */
	{ "1110010                         ",  7 }, /* 12n */
	{ "110000                          ",  6 }, /* 12o */
	{ "1110011                         ",  7 }, /* 12p */
	{ "110001                          ",  6 }, /* 14c */
	{ "111100111                       ",  9 }, /* 14d */
	{ "1110100                         ",  7 }, /* 14k */
	{ "111101000                       ",  9 }, /* 14l */
	{ "111101001                       ",  9 }, /* 15a */
	{ "11111000001                     ", 11 }, /* 15b */
	{ "1111011101                      ", 10 }, /* 15c */
	{ "111110001101                    ", 12 }, /* 15d */
	{ "110010                          ",  6 }, /* 15e */
	{ "11101111                        ",  8 }, /* 15f */
	{ "111101010                       ",  9 }, /* 16a */
	{ "11111000010                     ", 11 }, /* 16b */
	{ "11111000011                     ", 11 }, /* 16c */
	{ "1111100100000                   ", 13 }, /* 16d */
	{ "110011                          ",  6 }, /* 16e */
	{ "11110000                        ",  8 }, /* 16f */
	{ "11111001001011100110            ", 20 }, /* 17a */
	{ "1111100100101110000             ", 19 }, /* 17b */
	{ "11111001001011101100011110      ", 26 }, /* 17c */
	{ "1111100100101110101110          ", 22 }, /* 17d */
	{ "11111001001011100111            ", 20 }, /* 17e */
	{ "1111100100101110110010000110    ", 28 }, /* 17f */
	{ "11111001001011101000            ", 20 }, /* 17g */
	{ "11111001001011101011110         ", 23 }, /* 17h */
	{ "11111001000101                  ", 14 }, /* 18a */
	{ "11111001001011101011111         ", 23 }, /* 19a */
	{ "11111001001011101100011111      ", 26 }, /* 20a */
	{ "11111001000110                  ", 14 }, /* 22a */
	{ "1111100100101101                ", 16 }, /* 23a */
	{ "111110010010111011000010        ", 24 }, /* 25a */
	{ "111110010010111011000011        ", 24 }, /* 25b */
	{ "11111001001011101100100000      ", 26 }, /* 26a */
	{ "11111001000111                  ", 14 }, /* 29a */
	{ "111110010010100                 ", 15 }, /* 29b */
	{ "11111001001000                  ", 14 }, /* 29c */
	{ "111110010010101                 ", 15 }, /* 29d */
	{ "11111001001011101100100001110000", 32 }, /* 30a */
	{ "11111001001011101100100001110001", 32 }, /* 31a */
	{ "1111100100101110001             ", 19 }, /* 32a */
	{ "11111001001011101001            ", 20 }, /* 32b */
	{ "1111100100101110010             ", 19 }, /* 32c */
	{ "11111001001011101010            ", 20 }, /* 32d */
	{ "111110010010111011001000010     ", 27 }, /* 37a */
	{ "11110001                        ",  8 }, /* 40e */
	{ "1111011110                      ", 10 }, /* 40f */
	{ "111101011                       ",  9 }, /* 40g */
	{ "111110010010111011000100        ", 24 }, /* 41a */
	{ "111110010010111011000101        ", 24 }, /* 41b */
	{ "111110010010111011000110        ", 24 }, /* 41c */
	{ "1111100100101110110001110       ", 25 }, /* 41d */
	{ "111110010010111010110           ", 21 }, /* 42a */
	{ "11111001001011101100000         ", 23 }, /* 42b */
	{ "110100                          ",  6 }, /* 43a */
	{ "0111                            ",  4 }, /* 44b */
	{ "10110                           ",  5 }, /* 44d */
	{ "110101                          ",  6 }, /* 45b */
	{ "1110101                         ",  7 }, /* 45d */
	{ "11111000100                     ", 11 }, /* 46a */
	{ "1111100100001                   ", 13 }, /* 46b */
};



