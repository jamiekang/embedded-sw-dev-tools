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
	"RCCW.C", "RCW.C",
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
	"CPXI", "CPXI.C",
	"CPXO", "CPXO.C",
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
	"ENADP", "DPID",
	"RESET",
	/** Complex Arithmetic instructions */
	"POLAR.C", "RECT.C", "CONJ.C",
	"MAG.C",
	/** Assembler pseudo instructions */
	".GLOBAL", ".EXTERN", ".VAR", ".CODE", ".DATA", ".EQU",
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
		"18a", "18b", "18c",
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
		"47a",
		"48a",
		"49a", "49b", "49c", "49d",
		"50a", 
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
	"DSTB", "DSTID", "TERM", "INTN", "U", 
	"U2", "S", "T", "LPP", "PPP", 
	"SPP", "C", "CONJ", "ENA", "POLAR", 
	"DD", "PD", 
	"DC", "LSF", "DF", "MN", 
	"IDE", "IDM", "IDR", "IDN", "CW",
	"Reserved", 
	"COND",
	"PMM", "PMI", "DMM", "DMI", 
	NULL
};

/** 
* @brief reserved keywords
*/
const char *sResSym[] = 
{
	/** keywords for system registers - these are reserved to force using their "_" prefix version. */
	"CNTR", "MSTAT", "SSTAT",
	"LPSTACK", "PCSTACK",
	"ICNTL", "IMASK", "IRPTL",
	"LPEVER",
	"DSTAT0", "DSTAT1",
	"IVEC0", "IVEC1", "IVEC2", "IVEC3",
	/** other keywords */
	NULL
};

/** 
* Instruction format: use eType for array indexing
*/
const sInstFormat sInstFormatTable[] = 
{
	{ {}, {} },		/* NULL */
	{ 	
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fU, fU2, fDD, fPD, fPMM, fPMI, fDMM, fDMI, 0, },
	  	{ sBinOp[t01a].width,   4,     2,      3,      3,  1,   1,   3,   3,    2,    2,    2,    2, 0, } 
	}, /* 01a */
	{ 	
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fU, fU2, fDD, fPD, fPMM, fPMI, fDMM, fDMI, 0, }, 
	  	{ sBinOp[t01b].width,   4,     2,      2,      2,  1,   1,   2,   2,    2,    2,    2,    2, 0, } 
	}, /* 01b */
	{ 	
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fDD, fPD, fPMM, fPMI, fDMM, fDMI, 0, },
	  	{ sBinOp[t01c].width,   4,     4,      3,      3,   3,   3,    2,    2,    2,    2, 0, } 
	}, /* 01c */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fCOND, 0, },
	  	{ sBinOp[t03a].width,      6,     16,     5, 0, } 
	}, /* 03a */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fCOND, 0, },
	  	{ sBinOp[t03c].width,      5,     16,     5, 0, } 
	}, /* 03c */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fLSF, fCOND, 0, },
	  	{ sBinOp[t03d].width,      3,     16,    1,     5, 0, } 
	}, /* 03d */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fLSF, fCOND, 0, },
	  	{ sBinOp[t03e].width,      2,     16,    1,     5, 0, } 
	}, /* 03e */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fCOND, 0, },
	  	{ sBinOp[t03f].width,      6,     16,     5, 0, } 
	}, /* 03f */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fCOND, 0, },
	  	{ sBinOp[t03g].width,      5,     16,     5, 0, } 
	}, /* 03g */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fLSF, fCOND, 0, },
	  	{ sBinOp[t03h].width,      3,     16,    1,     5, 0, } 
	}, /* 03h */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fLSF, fCOND, 0, },
	  	{ sBinOp[t03i].width,      2,     16,    1,     5, 0, } 
	}, /* 03i */
	{ 
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fDSTLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t04a].width,   4,     2,      3,      3,      6,   1,   3,    3, 0, } 
	}, /* 04a */
	{ 
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fDSTLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t04b].width,   4,     2,      2,      2,      5,  1,    3,    3, 0, } 
	}, /* 04b */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fDSTLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t04c].width,   4,     4,      3,      3,      6,  1,    3,    3, 0, } 
	}, /* 04c */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fDSTLS, fU,  fDMM, fDMI, 0, },
	  	{ sBinOp[t04d].width,   4,     3,      2,      2,      5,  1,     3,    3, 0, } 
	}, /* 04d */
	{ 
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fSRCLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t04e].width,   4,     2,      3,      3,      6,  1,    3,    3, 0, } 
	}, /* 04e */
	{ 
		{ fOpcodeType,        fMF, fDSTM, fSRCM1, fSRCM2, fSRCLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t04f].width,   4,     2,      2,      2,      5,  1,    3,    3, 0, } 
	}, /* 04f */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fSRCLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t04g].width,   4,     4,      3,      3,      6,  1,    3,    3, 0, } 
	}, /* 04g */
	{ 
		{ fOpcodeType,        fAF, fDSTA, fSRCA1, fSRCA2, fSRCLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t04h].width,   4,     3,      2,      2,      5,  1,    3,    3, 0, } 
	}, /* 04h */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fCOND, 0, },
	  	{ sBinOp[t06a].width,      6,     16,     5, 0, } 
	}, /* 06a */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fCOND, 0, },
	  	{ sBinOp[t06b].width,      6,     12,     5, 0, } 
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
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fDSTLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t12e].width,    4,     2,      3,      5,      6,  1,    3,    3, 0, } 
	}, /* 12e */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fDSTLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t12f].width,    4,     2,      2,      5,      5,  1,    3,    3, 0, } 
	}, /* 12f */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fSRCLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t12g].width,    4,     2,      3,      5,      6,  1,    3,    3, 0, } 
	}, /* 12g */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fSRCLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t12h].width,    4,     2,      2,      5,      5,  1,    3,    3, 0, } 
	}, /* 12h */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fDSTLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t12m].width,    4,     2,      2,      5,      6,  1,    3,    3, 0, } 
	}, /* 12m */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fDSTLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t12n].width,    4,     2,      2,      5,      5,  1,    3,    3, 0, } 
	}, /* 12n */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fSRCLS, fU, fDMM, fDMI, 0, },
	  	{ sBinOp[t12o].width,    4,     2,      2,      5,      6,  1,    3,    3, 0, } 
	}, /* 12o */
	{ 
		{ fOpcodeType,        fSF2, fDSTS, fSRCS1, fSRCS2, fSRCLS, fU, fDMM, fDMI, 0, },
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
		{ fOpcodeType,        fDSTLS, fSRCLS, fCOND, 0, },
	  	{ sBinOp[t17a].width,      6,      6,     5, 0, } 
	}, /* 17a */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fDC, fCOND, 0, },
	  	{ sBinOp[t17b].width,      6,      6,   1,     5, 0, } 
	}, /* 17b */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fCOND, 0, },
	  	{ sBinOp[t17c].width,      3,      3,     5, 0, } 
	}, /* 17c */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fCOND, 0, },
	  	{ sBinOp[t17d].width,      5,      5,     5, 0, } 
	}, /* 17d */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fDC, fCOND, 0, },
	  	{ sBinOp[t17e].width,      5,      6,   1,     5, 0, } 
	}, /* 17e */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fCOND, 0, },
	  	{ sBinOp[t17f].width,      2,      2,     5, 0, } 
	}, /* 17f */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fCOND, 0, },
	  	{ sBinOp[t17g].width,      6,      6,     5, 0, } 
	}, /* 17g */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fCOND, 0, },
	  	{ sBinOp[t17h].width,      3,      6,     5, 0, } 
	}, /* 17h */
	{ 
		{ fOpcodeType,        fENA, fCOND, 0, },
	  	{ sBinOp[t18a].width,   18,     5, 0, } 
	}, /* 18a */
	{ 
		{ fOpcodeType,        fIDE, fIDM, fCOND, 0, },
	  	{ sBinOp[t18b].width,    4,    2,     5, 0, } 
	}, /* 18b */
	{ 
		{ fOpcodeType,        fDSTID, fCOND, 0, },
	  	{ sBinOp[t18c].width,      6,     5, 0, } 
	}, /* 18c */
	{ 
		{ fOpcodeType,        fS, fCOND, fDMI, 0, },
	  	{ sBinOp[t19a].width,  1,     5,    3, 0, } 
	}, /* 19a */
	{ 
		{ fOpcodeType,        fT, fCOND, 0, },
	  	{ sBinOp[t20a].width,  1,     5, 0, } 
	}, /* 20a */
	{ 
		{ fOpcodeType,        fDSTLS, fU, fCOND, fDMM, fDMI, 0, },
	  	{ sBinOp[t22a].width,     12,  1,     5,    3,    3, 0, } 
	}, /* 22a */
	{ 
		{ fOpcodeType,        fDSTA, fSRCA1, fSRCA2, fDF, fCOND, 0, },
	  	{ sBinOp[t23a].width,     5,      5,      5,   1,     5, 0, } 
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
		{ fOpcodeType,        fLPP, fPPP, fSPP, fCOND, 0, },
	  	{ sBinOp[t26a].width,    2,    2,    2,     5, 0, } 
	}, /* 26a */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fU, fCOND, fDMI, 0, },
	  	{ sBinOp[t29a].width,      6,      8,  1,     5,    3, 0, } 
	}, /* 29a */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fU, fCOND, fDMI, 0, },
	  	{ sBinOp[t29b].width,      5,      8,  1,     5,    3, 0, } 
	}, /* 29b */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fU, fCOND, fDMI, 0, },
	  	{ sBinOp[t29c].width,      6,      8,  1,     5,    3, 0, } 
	}, /* 29c */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fU, fCOND, fDMI, 0, },
	  	{ sBinOp[t29d].width,      5,      8,  1,     5,    3, 0, } 
	}, /* 29d */
	{ 
		{ fOpcodeType,        fCOND, 0, },
	  	{ sBinOp[t30a].width,     5, 0, } 
	}, /* 30a */
	{ 
		{ fOpcodeType,        fCOND, 0, },
	  	{ sBinOp[t31a].width,     5, 0, } 
	}, /* 31a */
	{ 
		{ fOpcodeType,        fDSTLS, fU, fCOND, fDMM, fDMI, 0, },
	  	{ sBinOp[t32a].width,      6,  1,     5,    3,    3, 0, } 
	}, /* 32a */
	{ 
		{ fOpcodeType,        fDSTLS, fU, fCOND, fDMM, fDMI, 0, },
	  	{ sBinOp[t32b].width,      5,  1,     5,    3,    3, 0, } 
	}, /* 32b */
	{ 
		{ fOpcodeType,        fDSTLS, fU, fCOND, fDMM, fDMI, 0, },
	  	{ sBinOp[t32c].width,      6,  1,     5,    3,    3, 0, } 
	}, /* 32c */
	{ 
		{ fOpcodeType,        fDSTLS, fU, fCOND, fDMM, fDMI, 0, },
	  	{ sBinOp[t32d].width,      5,  1,     5,    3,    3, 0, } 
	}, /* 32d */
	{ 
		{ fOpcodeType,        fINTN, fC, fCOND, 0, },
	  	{ sBinOp[t37a].width,     4,  1,     5, 0, } 
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
		{ fOpcodeType,        fDSTC, fSRCC, fCONJ, fPOLAR, fCOND, 0, },
	  	{ sBinOp[t42a].width,     5,     4,     1,      1,     5, 0, } 
	}, /* 42a */
	{ 
		{ fOpcodeType,        fDSTC, fSRCC, fCOND, 0, },
	  	{ sBinOp[t42b].width,     5,     4,     5, 0, } 
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
	{ 
		{ fOpcodeType,        fDSTC, fSRCC, fCONJ, fCOND, 0, },
	  	{ sBinOp[t47a].width,     5,     4,     1,     5, 0, } 
	}, /* 47a */
	{ 
		{ fOpcodeType,        fCOND, 0, },
	  	{ sBinOp[t48a].width,     5, 0, } 
	}, /* 48a */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fIDR, fIDN, fCOND, 0, },
	  	{ sBinOp[t49a].width,      6,      3,    2,    4,     5, 0, } 
	}, /* 49a */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fIDR, fIDN, fCOND, 0, },
	  	{ sBinOp[t49b].width,      5,      2,    2,    4,     5, 0, } 
	}, /* 49b */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fIDR, fIDN, fCOND, 0, },
	  	{ sBinOp[t49c].width,      3,      6,    2,    4,     5, 0, } 
	}, /* 49c */
	{ 
		{ fOpcodeType,        fDSTLS, fSRCLS, fIDR, fIDN, fCOND, 0, },
	  	{ sBinOp[t49d].width,      2,      5,    2,    4,     5, 0, } 
	}, /* 49d */
	{ 
		{ fOpcodeType,        fDSTA, fSRCA1, fSRCA2, fCW, fCOND, 0, },
	  	{ sBinOp[t50a].width,     5,      4,      5,   1,     5, 0, } 
	}, /* 50a */
};

/** 
* Binary Opcode mapping; use eType for array indexing
* Last update: 2010-02-04
* Total 112 unique binary opcodes, each 1-to-1 mapped to instruction type.
*/
const sBinMapping sBinOp[] = 
{
	{ NULL                              ,  0 },                              
	{ "0010                            ",  4 }, /* 01a */
	{ "11100000                        ",  8 }, /* 01b */
	{ "0011                            ",  4 }, /* 01c */
	{ "01010                           ",  5 }, /* 03a */
	{ "101000                          ",  6 }, /* 03c */
	{ "1100100                         ",  7 }, /* 03d */
	{ "11100001                        ",  8 }, /* 03e */
	{ "01011                           ",  5 }, /* 03f */
	{ "101001                          ",  6 }, /* 03g */
	{ "1100101                         ",  7 }, /* 03h */
	{ "11100010                        ",  8 }, /* 03i */
	{ "1100110                         ",  7 }, /* 04a */
	{ "1110111100                      ", 10 }, /* 04b */
	{ "01100                           ",  5 }, /* 04c */
	{ "111010010                       ",  9 }, /* 04d */
	{ "1100111                         ",  7 }, /* 04e */
	{ "1110111101                      ", 10 }, /* 04f */
	{ "01101                           ",  5 }, /* 04g */
	{ "111010011                       ",  9 }, /* 04h */
	{ "01110                           ",  5 }, /* 06a */
	{ "111010100                       ",  9 }, /* 06b */
	{ "000                             ",  3 }, /* 06c */
	{ "01111                           ",  5 }, /* 06d */
	{ "11100011                        ",  8 }, /* 08a */
	{ "111100010100                    ", 12 }, /* 08b */
	{ "101010                          ",  6 }, /* 08c */
	{ "11110000100                     ", 11 }, /* 08d */
	{ "1101000                         ",  7 }, /* 09c */
	{ "111010101                       ",  9 }, /* 09d */
	{ "11100100                        ",  8 }, /* 09e */
	{ "10000                           ",  5 }, /* 09f */
	{ "1111000110010                   ", 13 }, /* 09g */
	{ "111100011100010                 ", 15 }, /* 09h */
	{ "1101001                         ",  7 }, /* 09i */
	{ "1111000110011                   ", 13 }, /* 10a */
	{ "111100011100011                 ", 15 }, /* 10b */
	{ "1111000111010010                ", 16 }, /* 11a */
	{ "10001                           ",  5 }, /* 12e */
	{ "1101010                         ",  7 }, /* 12f */
	{ "10010                           ",  5 }, /* 12g */
	{ "1101011                         ",  7 }, /* 12h */
	{ "101011                          ",  6 }, /* 12m */
	{ "1101100                         ",  7 }, /* 12n */
	{ "101100                          ",  6 }, /* 12o */
	{ "1101101                         ",  7 }, /* 12p */
	{ "101101                          ",  6 }, /* 14c */
	{ "111010110                       ",  9 }, /* 14d */
	{ "1101110                         ",  7 }, /* 14k */
	{ "111010111                       ",  9 }, /* 14l */
	{ "111011000                       ",  9 }, /* 15a */
	{ "11110000101                     ", 11 }, /* 15b */
	{ "1110111110                      ", 10 }, /* 15c */
	{ "111100010101                    ", 12 }, /* 15d */
	{ "101110                          ",  6 }, /* 15e */
	{ "11100101                        ",  8 }, /* 15f */
	{ "111011001                       ",  9 }, /* 16a */
	{ "11110000110                     ", 11 }, /* 16b */
	{ "11110000111                     ", 11 }, /* 16c */
	{ "1111000110100                   ", 13 }, /* 16d */
	{ "101111                          ",  6 }, /* 16e */
	{ "11100110                        ",  8 }, /* 16f */
	{ "111100011100100                 ", 15 }, /* 17a */
	{ "11110001101100                  ", 14 }, /* 17b */
	{ "111100011101010110000           ", 21 }, /* 17c */
	{ "11110001110101000               ", 17 }, /* 17d */
	{ "111100011100101                 ", 15 }, /* 17e */
	{ "11110001110101011010010         ", 23 }, /* 17f */
	{ "111100011100110                 ", 15 }, /* 17g */
	{ "111100011101010100              ", 18 }, /* 17h */
	{ "111011010                       ",  9 }, /* 18a */
	{ "111100011101010110001           ", 21 }, /* 18b */
	{ "111100011101010110010           ", 21 }, /* 18c */
	{ "11110001110101011010011         ", 23 }, /* 19a */
	{ "11110001110101011010110110      ", 26 }, /* 20a */
	{ "11100111                        ",  8 }, /* 22a */
	{ "11110001000                     ", 11 }, /* 23a */
	{ "111100011101010110101000        ", 24 }, /* 25a */
	{ "111100011101010110101001        ", 24 }, /* 25b */
	{ "111100011101010110011           ", 21 }, /* 26a */
	{ "111011011                       ",  9 }, /* 29a */
	{ "1110111111                      ", 10 }, /* 29b */
	{ "111011100                       ",  9 }, /* 29c */
	{ "1111000000                      ", 10 }, /* 29d */
	{ "111100011101010110101101110     ", 27 }, /* 30a */
	{ "111100011101010110101101111     ", 27 }, /* 31a */
	{ "11110001101101                  ", 14 }, /* 32a */
	{ "111100011100111                 ", 15 }, /* 32b */
	{ "11110001101110                  ", 14 }, /* 32c */
	{ "111100011101000                 ", 15 }, /* 32d */
	{ "1111000111010101101000          ", 22 }, /* 37a */
	{ "11101000                        ",  8 }, /* 40e */
	{ "1111000001                      ", 10 }, /* 40f */
	{ "111011101                       ",  9 }, /* 40g */
	{ "111100011101010110101010        ", 24 }, /* 41a */
	{ "111100011101010110101011        ", 24 }, /* 41b */
	{ "111100011101010110101100        ", 24 }, /* 41c */
	{ "1111000111010101101011010       ", 25 }, /* 41d */
	{ "1111000111010011                ", 16 }, /* 42a */
	{ "111100011101010101              ", 18 }, /* 42b */
	{ "110000                          ",  6 }, /* 43a */
	{ "0100                            ",  4 }, /* 44b */
	{ "10011                           ",  5 }, /* 44d */
	{ "110001                          ",  6 }, /* 45b */
	{ "1101111                         ",  7 }, /* 45d */
	{ "11110001001                     ", 11 }, /* 46a */
	{ "1111000110101                   ", 13 }, /* 46b */
	{ "11110001110101001               ", 17 }, /* 47a */
	{ "111100011101010110101110000     ", 27 }, /* 48a */
	{ "111100010110                    ", 12 }, /* 49a */
	{ "11110001101111                  ", 14 }, /* 49b */
	{ "111100010111                    ", 12 }, /* 49c */
	{ "11110001110000                  ", 14 }, /* 49d */
	{ "111100011000                    ", 12 }, /* 50a */
};



