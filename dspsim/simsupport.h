/*
All Rights Reserved.
*/
/** 
* @file simsupport.h
* @brief Header for misc supporting routines for simulator main
* @date 2008-09-29
*/

#ifndef	_SIMSUPPORT_H
#define	_SIMSUPPORT_H

#include "dspsim.h"
#include "stack.h"
#include "cordic.h"

#ifndef	_SINT_
#define	_SINT_

#define NUMDP   4   /* number of SIMD data paths */

/** definition for SIMD int data structure */
typedef struct ssint {
	int dp[NUMDP];					/* integer value for each data path */
} sint;

#endif	/* _SINT_ */

/** definition for SIMD unsigned int data structure */
typedef struct ssuint {
	unsigned int dp[NUMDP];					/* integer value for each data path */
} suint;

/** definition for SIMD unsigned char data structure */
typedef struct ssuchar {
	unsigned char dp[NUMDP];		/* unsigned char value for each data path */
} suchar;

/** definition for SIMD complex data structure */
typedef struct sscplx {
	sint r;  /**< real part */
	sint i;  /**< imaginary part */
} scplx;

typedef struct sAcc {
	sint H;	/** 8b: acts as 12b only when used as an input operand */	
	sint M;	/** 12b */
	sint L;	/** 12b */
	sint value;	/** 32b */
} Acc;

typedef struct sAstat {		/* for ASTAT.x */
	suchar AZ;	/* bit 0 */
	suchar AN;	/* bit 1 */
	suchar AV;	/* bit 2 */
	suchar AC;	/* bit 3 */
	suchar AS;	/* bit 4 */
	suchar AQ;	/* bit 5 */
	suchar MV;	/* bit 6 */
	suchar SS;	/* bit 7 */
	suchar SV;	/* bit 8 */
	suchar UM;	/* bit 9 */
} Astat;

typedef struct sMstat {		/* for _MSTAT */
	unsigned char SR;	/* bit 0: SEC_REG or SR (TBD) */
	unsigned char BR;	/* bit 1: BIT_REV or BR */
	unsigned char OL;	/* bit 2: AV_LATCH or OL */
	unsigned char AS;	/* bit 3: AL_SAT or AS */
	unsigned char MM;	/* bit 4: M_MODE or MM */
	unsigned char TI;	/* bit 5: TIMER or TI (TBD) */
	unsigned char SD;	/* bit 6: SEC_DAG or SD (TBD) */
	unsigned char MB;	/* bit 7: M_BIAS or MB: different from ADSP-219x */
				/* same as BIASRND bit of ADSP-219x ICNTL */
} sMstat;

typedef struct sSstat {		/* for _SSTAT */
	unsigned char PCE;		/* bit 0: PCSTKEMPTY or PCE */
	unsigned char PCF;		/* bit 1: PCSTKFULL or PCF */
	unsigned char PCL;		/* bit 2: PCSTKLVL or PCL */
	unsigned char Reserved;	/* bit 3: Reserved */
	unsigned char LSE;		/* bit 4: LPSTKEMPTY or LSE */
	unsigned char LSF;		/* bit 5: LPSTKFULL or LSF */
	unsigned char SSE;		/* bit 6: STSSTKEMPY or SSE */
	unsigned char SOV;		/* bit 7: STKOVERFLOW or SOV */

} sSstat;

typedef struct sIcntl {
	unsigned char INE;		/* bit 4: Interrupt nesting enable: TBD */
	unsigned char GIE;		/* bit 5: Global interrupt enable */
} sIcntl;

typedef struct sIrptl {
    unsigned char EMU;      /* bit 0: Emulator interrupt mask - highest priority */
    unsigned char PWDN;     /* bit 1: Power-down interrupt mask */
    unsigned char SSTEP;    /* bit 2: Single-step interrupt mask */
    unsigned char STACK;    /* bit 3: Stack interrupt mask */
    unsigned char UserDef[12];  /* bit 4~15: User-defined - UserDef[0] = bit 4, UserDef[11] = bit 15 */
} sIrptl;

extern sint rR[32];		/**< Rx data registers: 12b */
extern sAcc rAcc[8];	/**< Accumulator registers: 32/36b */

extern int rI[8];		/**< Ix registers: 16b - not shared, no SIMD */
extern int rM[8];		/**< Mx registers: 16b - not shared, no SIMD */
extern int rL[8];		/**< Lx registers: 16b - not shared, no SIMD */
extern int rB[8];		/**< Bx registers: 16b - not shared, no SIMD */

/* backup copies of Ix/Mx/Lx/Bx (not real registers): 2009.07.08 */
extern int rbI[8][2];		/**< Ix registers: 16b - not shared, no SIMD */
extern int rbM[8][2];		/**< Mx registers: 16b - not shared, no SIMD */
extern int rbL[8][2];		/**< Lx registers: 16b - not shared, no SIMD */
extern int rbB[8][2];		/**< Bx registers: 16b - not shared, no SIMD */

/**< to record when Ix/Mx/Lx/Bx registers were written: 2009.07.08 */
extern int LastAccessIx[8];		/**< not shared, no SIMD */
extern int LastAccessMx[8];		/**< not shared, no SIMD */
extern int LastAccessLx[8];		/**< not shared, no SIMD */
extern int LastAccessBx[8];		/**< not shared, no SIMD */

/* backup copies of LastAccessIx/Mx/Lx/Bx (not real registers): 2010.07.21 */
extern int LAbIx[8][2];    /**< not shared, no SIMD */
extern int LAbMx[8][2];    /**< not shared, no SIMD */
extern int LAbLx[8][2];    /**< not shared, no SIMD */
extern int LAbBx[8][2];    /**< not shared, no SIMD */

extern int rLPSTACK;	/**< _LPSTACK */
extern int rPCSTACK;	/**< _PCSTACK */
extern sIcntl rICNTL;		/**< _ICNTL */
extern sIrptl rIMASK;		/**< _IMASK */
extern sIrptl rIRPTL;		/**< _IRPTL */

/**< for interrupt vectors: 2009.08 */
extern int rIVEC[4];			/**< _IVECx registers: 16b */

extern sAstat rAstatR;	/**< ASTAT.R: Note - Real instruction affects only ASTAT.R */
extern sAstat rAstatI;	/**< ASTAT.I */
extern sAstat rAstatC;	/**< ASTAT.C: Note - Complex instruction affects ASTAT.R, ASTAT.I, and ASTAT.C */
extern sMstat rMstat;	/**< _MSTAT */
extern sSstat rSstat;	/**< _SSTAT */
extern int rDstat0;		/**< _DSTAT0 */
extern int rDstat1;		/**< _DSTAT1 */
extern sint rDPENA;    /**< decoded version of _DSTAT0 - for simulator internal only */    
extern sint rDPMST;    /**< decoded version of _DSTAT1 - for simulator internal only */                 

extern int rCNTR;		/**< _CNTR */
extern int rLPEVER;    /**< _LPEVER: set if DO UNTIL FOREVER */

extern sint rUMCOUNT;    /**< UMCOUNT */

extern sint rDID;			/**< DID */

/* backup copy of MSTAT (not a real register): 2009.07.08 */
extern sMstat rbMstat; 

/**< to record when MSTAT register was written: 2009.07.08 */
extern int LastAccessMstat;

/* backup copy of CNTR (not a real register): 2010.07.20 */
extern int rbCNTR; 

/**< to record when CNTR register was written: 2010.07.20 */
extern int LastAccessCNTR;

extern sStack PCStack;             /**< PC Stack */
extern sStack LoopBeginStack;      /**< Loop Begin Stack */
extern sStack LoopEndStack;        /**< Loop End Stack */
extern sStack LoopCounterStack;    /**< Loop Counter Stack */
extern ssStack ASTATStack[3];       /**< ASTAT Stack */
extern sStack MSTATStack;      	/**< MSTAT Stack */
extern sStack LPEVERStack;         /**< LPEVER Stack */

extern int oldPC, PC;           

extern sint OVCount;

int isRReg16(sICode *p, char *s);
int isRReg(sICode *p, char *s);
int isCReg(sICode *p, char *s);
int isReg12(sICode *p, char *s);
int isXOP12(sICode *p, char *s);
int isReg12S(sICode *p, char *s);
int isReg24(sICode *p, char *s);
int isXOP24(sICode *p, char *s);
int isReg24S(sICode *p, char *s);
int isDReg12(sICode *p, char *s);
int isDReg12S(sICode *p, char *s);
int isDReg24(sICode *p, char *s);
int isDReg24S(sICode *p, char *s);
int isACC12(sICode *p, char *s);
int isACC12S(sICode *p, char *s);
int isACC24(sICode *p, char *s);
int isACC24S(sICode *p, char *s);
int isACC32(sICode *p, char *s);
int isACC32S(sICode *p, char *s);
int isACC64(sICode *p, char *s);
int isACC64S(sICode *p, char *s);
int isSysCtlReg(char *s);
int isInt(char *s);
int isIntSignedN(sICode *p, sTabList htable[], char *s, int n);
int isIntUnsignedN(sICode *p, sTabList htable[], char *s, int n);
int isIntNM(sICode *p, sTabList htable[], char *s, int n, int m);
int isIntNumNM(sICode *p, int x, int n, int m);
int isIReg(sICode *p, char *s);
int isIx(sICode *p, char *s);
int isIy(sICode *p, char *s);
int isMReg(sICode *p, char *s);
int isMx(sICode *p, char *s);
int isMy(sICode *p, char *s);
int isLReg(sICode *p, char *s);
int isNONE(sICode *p, char *s);
int isXReg12(sICode *p, char *s);
int isXReg24(sICode *p, char *s);
int isIDN(char *s);

int RdReg(char *reg);
sint sRdReg(char *reg);
int RdReg2(sICode *p, char *reg);
sint sRdXReg(char *reg, int id_offset, int active_dp);
void WrReg(char *reg, int data);
//void sWrReg(char *reg, sint data);
void sWrReg(char *reg, sint data, sint mask);
//void sWrXReg(char *reg, sint data, int id_offset, int active_dp);
void sWrXReg(char *reg, sint data, int id_offset, int active_dp, sint mask);
//cplx cRdReg(char *reg);
scplx scRdReg(char *reg);
scplx scRdXReg(char *reg, int id_offset, int active_dp);
//void cWrReg(char *reg, int data1, int data2);
//void scWrReg(char *reg, sint data1, sint data2);
void scWrReg(char *reg, sint data1, sint data2, sint mask);
//void scWrXReg(char *reg, sint data1, sint data2, int id_offset, int active_dp);
void scWrXReg(char *reg, sint data1, sint data2, int id_offset, int active_dp, sint mask);
//int	RdC1(void);
sint sRdC1(void);
//cplx RdC2(void);
scplx sRdC2(void);

//void flagEffect(int type, int z, int n, int v, int c);
//void sFlagEffect(int type, sint z, sint n, sint v, sint c);
void sFlagEffect(int type, sint z, sint n, sint v, sint c, sint mask);
//void cFlagEffect(int type, cplx z, cplx n, cplx v, cplx c);
//void scFlagEffect(int type, scplx z, scplx n, scplx v, scplx c);
void scFlagEffect(int type, scplx z, scplx n, scplx v, scplx c, sint mask);
void dumpRegister(sICode *p);

int getLabelAddr(sICode *p, sTabList htable[], char *s);
int getIntSymAddr(sICode *p, sTabList htable[], char *s);
int getIntImm(sICode *p, int fval, int immtype);
cplx getCplxImm(sICode *p, int fval, int immtype);
sICode *updatePC(sICode *p, sICode *n);
int isUnbiasedRounding(void);
int isFractionalMode(void);
int isAVLatchedMode(void);
int isALUSatMode(void);

//int OVCheck(int type, int x, int y);
sint sOVCheck(int type, sint x, sint y);
int isNeg8b(int x);
int isNeg12b(int x);
int isNeg16b(int x);
int isNeg24b(int x);
int isNeg32b(int x);
int isNeg(int x, int n);
//int CarryCheck(sICode *p, int x, int y, int c);
sint sCarryCheck(sICode *p, sint x, sint y, sint c);
int satCheck12b(int ov, int c);

//int RdDataMem(unsigned int addr);
sint sRdDataMem(unsigned int addr);
//int briefRdDataMem(unsigned int addr);
sint sBriefRdDataMem(unsigned int addr);
//void WrDataMem(int data, unsigned int addr);
//void sWrDataMem(sint data, unsigned int addr);
void sWrDataMem(sint data, unsigned int addr, sint mask);
unsigned int getBitReversedAddr(unsigned int addr);
//int RdProgMem(unsigned int addr);
//void WrProgMem(int data, unsigned int addr);

int isValidMemoryAddr(int addr);
int isScratchPadMemoryAddr(int addr);
int isBreakpoint(sICode *p);
int	ifCond(char *s);
int sIfCond(char *s, sint *mask);

int isMultiFunc(sICode *p);
int isALU(sICode *p);
int isALU_C(sICode *p);
int isMAC(sICode *p);
int isMAC_C(sICode *p);
int isAnyMAC(sICode *p);
int isMACMulti(sICode *p);
int isSHIFT(sICode *p);
int isSHIFT_C(sICode *p);
int isLD(sICode *p);
int isLD_C(sICode *p);
int isST(sICode *p);
int isST_C(sICode *p);
int isCP(sICode *p);
int isCP_C(sICode *p);
int isLDSTMulti(sICode *p);
int isLDMulti(sICode *p);
int isSTMulti(sICode *p);

void printRunTimeError(int ln, char *s, char *msg);
void printRunTimeWarning(int ln, char *s, char *msg);
void printRunTimeMessage(void);
int getCodeDReg12(char *ret, char *s);
int getCodeXOP12(char *ret, char *s);
int getCodeDReg12S(char *ret, char *s);
int getCodeDReg24(char *ret, char *s);
int getCodeDReg24S(char *ret, char *s);
int getCodeXOP24(char *ret, char *s);
int getCodeReg12(char *ret, char *s);
int getCodeReg12S(char *ret, char *s);
int getCodeReg24(char *ret, char *s);
int getCodeReg24S(char *ret, char *s);
int getCodeACC32(char *ret, char *s);
int getCodeACC32S(char *ret, char *s);
int getCodeACC64(char *ret, char *s);
int getCodeACC64S(char *ret, char *s);
int getCodeCOND(char *ret, char *s);
int getCodeRReg16(char *ret, char *s);
int getCodeRReg(sICode *p, char *ret, char *s);
int getCodeCReg(char *ret, char *s);
int getCodeXReg12(char *ret, char *s);
int getCodeXReg24(char *ret, char *s);
void getCodeAMF(char *ret, sICode *p);
void getCodeAF(char *ret, sICode *p);
void getCodeMF(char *ret, sICode *p);
void getCodeSF(char *ret, char *opr, unsigned int index);
void getCodeSF2(char *ret, char *opr, unsigned int index);
void getCodeLSF(char *ret, char *opr, unsigned int index);
int getCodeYZ(char *ret, char *opr);
int getCodeComplexYZ(char *ret, char *opr1, char *opr2);
int getCodeMS(char *ret, char *opr);
int getCodeU(char *ret, char *opr);
int getCodeIReg3b(char *ret, char *opr);
int getCodeIReg2b(char *ret, char *opr);
int getCodeMReg3b(char *ret, char *opr);
int getCodeMReg2b(char *ret, char *opr);
int getCodeTERM(char *ret, char *opr);
int getCodeLPP(char *ret, char *opr, unsigned int index);
int getCodePPP(char *ret, char *opr, unsigned int index);
int getCodeSPP(char *ret, char *opr0, char *opr1, unsigned int index);
int getCodeCONJ(char *ret, int conj);
int getCodeDC(sICode *p, char *ret, char *opr0, char *opr1);
void getCodeENA(char *ret, sICode *p);
int getCodeMN(char *ret, char *opr);
void getCodeIDE(char *ret, sICode *p);
void int2bin(char *ret, int val, int n);

//int calcRounding(int x);
sint sCalcRounding(sint x);
//void processMACFunc(sICode *p, int temp1, int temp2, char *Opr0, char *Opr3);
void sProcessMACFunc(sICode *p, sint temp1, sint temp2, char *Opr0, char *Opr3, sint mask);
//void processMAC_CFunc(sICode *p, cplx ct1, cplx ct2, char *Opr0, char *Opr3);
void sProcessMAC_CFunc(sICode *p, scplx ct1, scplx ct2, char *Opr0, char *Opr3, sint mask);
//void processMAC_RCFunc(sICode *p, int temp1, cplx ct2, char *Opr0, char *Opr3);
void sProcessMAC_RCFunc(sICode *p, sint temp1, scplx ct2, char *Opr0, char *Opr3, sint mask);
//void processALUFunc(sICode *p, int temp1, int temp2, char *Opr0);
void sProcessALUFunc(sICode *p, sint temp1, sint temp2, char *Opr0, sint mask);
//void processALU_CFunc(sICode *p, cplx ct1, cplx ct2, char *Opr0);
void sProcessALU_CFunc(sICode *p, scplx ct1, scplx ct2, char *Opr0, sint mask);
//void processSHIFTFunc(sICode *p, int temp1, int temp2, char *Opr0, char *Opr3);
void sProcessSHIFTFunc(sICode *p, sint temp1, sint temp2, char *Opr0, char *Opr3, sint mask);
//void processSHIFT_CFunc(sICode *p, cplx ct1, cplx ct2, char *Opr0, char *Opr3);
void sProcessSHIFT_CFunc(sICode *p, scplx ct1, scplx ct2, char *Opr0, char *Opr3, sint mask);
//void processCordicFunc(sICode *p, cplx ct1, char *Opr0);
void sProcessCordicFunc(sICode *p, scplx ct1, char *Opr0, sint mask);
//void processComplexFunc(sICode *p, cplx ct1, char *Opr0);
void sProcessComplexFunc(sICode *p, scplx ct1, char *Opr0, sint mask);

void updateIReg(char *s, int data);
void resetPMA(int addr);

int isReadOnlyReg(sICode *p, char *s);
int checkUnalignedMemoryAccess(sICode *p, int addr, char *opr);

#endif	/* _SIMSUPPORT_H */
