/*
All Rights Reserved.
*/
/** 
* @file binsimsupport.h
* @brief Header for misc supporting routines for binary simulator main
* @date 2009-02-05
*/

#ifndef	_BINSIMSUPPORT_H
#define	_BINSIMSUPPORT_H

#define	UNDEF_FIELD		(-1)		/* for getIntField() */
#define	UNDEF_REG		(-1)		/* for getRegIndex() */

#include "stack.h"
//#include "cordic.h"

typedef struct sAcc {
	int H;	/** 8b: acts as 12b only when used as an input operand */	
	int M;	/** 12b */
	int L;	/** 12b */
	int value;	/** 32b */
} Acc;

typedef struct sAstat {
	unsigned char AZ;	/* bit 0 */
	unsigned char AN;	/* bit 1 */
	unsigned char AV;	/* bit 2 */
	unsigned char AC;	/* bit 3 */
	unsigned char AS;	/* bit 4 */
	unsigned char AQ;	/* bit 5 */
	unsigned char MV;	/* bit 6 */
	unsigned char SS;	/* bit 7 */
	unsigned char SV;	/* bit 8 */
} Astat;

typedef struct sMstat {
	unsigned char SR;	/* bit 0: SEC_REG or SR (TBD) */
	unsigned char BR;	/* bit 1: BIT_REV or BR */
	unsigned char OL;	/* bit 2: AV_LATCH or OL */
	unsigned char AS;	/* bit 3: AL_SAT or AS */
	unsigned char MM;	/* bit 4: M_MODE or MM */
	unsigned char TI;	/* bit 5: TIMER or TI (TBD) */
	unsigned char SD;	/* bit 6: SEC_DAG or SD (TBD) */
	unsigned char MB;	/* bit 7: M_BIAS or MB: different from ADSP-219x */
						/*        same as BIASRND bit of ADSP-219x ICNTL */
} sMstat;

typedef struct sSstat {
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

extern int rR[32];		/**< Rx data registers: 12b */
extern sAcc rAcc[8];	/**< Accumulator registers: 32/36b */
extern int rI[8];		/**< Ix registers: 16b */
extern int rM[8];		/**< Mx registers: 16b */
extern int rL[8];		/**< Lx registers: 16b */
extern int rB[8];		/**< Bx registers: 16b */

/* backup copies of Ix/Mx/Lx/Bx (not real registers): 2009.07.08 */
extern int rbI[8];		/**< Ix registers: 16b */
extern int rbM[8];		/**< Mx registers: 16b */
extern int rbL[8];		/**< Lx registers: 16b */
extern int rbB[8];		/**< Bx registers: 16b */

/**< to record when Ix/Mx/Lx/Bx registers were written: 2009.07.08 */
extern int LastAccessIx[8];	
extern int LastAccessMx[8];	
extern int LastAccessLx[8];	
extern int LastAccessBx[8];	

extern int rLPSTACK;	/**< LPSTACK */
extern int rPCSTACK;	/**< PCSTACK */
extern sIcntl rICNTL;		/**< ICNTL */
extern sIrptl rIMASK;		/**< IMASK */
extern sIrptl rIRPTL;		/**< IRPTL */
extern int rCNTR;		/**< CNTR */
//extern int rCACTL;        /**< CACTL */
//extern int rPX;           /**< PX */

/**< for interrupt vectors: 2009.08 */
extern int rIVEC[4];            /**< IVECx registers: 16b */

extern sAstat rAstatR;	/**< ASTAT.R: Note - Real instruction affects only ASTAT.R */
extern sAstat rAstatI;	/**< ASTAT.I */
extern sAstat rAstatC;	/**< ASTAT.C: Note - Complex instruction affects ASTAT.R, ASTAT.I, and ASTAT.C */
extern sMstat rMstat;	/**< MSTAT */
extern sSstat rSstat;	/**< SSTAT */

/* backup copy of MSTAT (not a real register): 2009.07.08 */
extern sMstat rbMstat; 

/**< to record when MSTAT register was written: 2009.07.08 */
extern int LastAccessMstat;

extern sStack PCStack;             /**< PC Stack */
extern sStack LoopBeginStack;      /**< Loop Begin Stack */
extern sStack LoopEndStack;        /**< Loop End Stack */
extern sStack LoopCounterStack;    /**< Loop Counter Stack */
extern sStack LPEVERStack;         /**< LPEVER Stack */
extern sStack StatusStack[4];      /**< Status Stack */

//int isRReg16(char *s);
//int isRReg(char *s);
//int isCReg(char *s);
//int isReg12(char *s);
//int isXOP12(char *s);
//int isReg12S(char *s);
//int isReg24(char *s);
//int isXOP24(char *s);
//int isReg24S(char *s);
//int isDReg12(char *s);
//int isDReg12S(char *s);
//int isDReg24(char *s);
//int isDReg24S(char *s);
//int isACC12(char *s);
//int isACC12S(char *s);
//int isACC24(char *s);
//int isACC24S(char *s);
//int isACC32(char *s);
//int isACC32S(char *s);
//int isACC64(char *s);
//int isACC64S(char *s);
//int isSysCtlReg(char *s);
//int isInt(char *s);
//int isIntSignedN(sBCode *p, sTabList htable[], char *s, int n);
//int isIntUnsignedN(sBCode *p, sTabList htable[], char *s, int n);
//int isIntNM(sBCode *p, sTabList htable[], char *s, int n, int m);
//int isIReg(char *s);
//int isIx(char *s);
//int isIy(char *s);
//int isMReg(char *s);
//int isMx(char *s);
//int isMy(char *s);
//int isLReg(char *s);

int getRegIndex(sBCode *p, int fval, int regtype);
int RdReg(sBCode *p, int ridx);
int RdReg2(sBCode *p, int ridx);
void WrReg(sBCode *p, int ridx, int data);
cplx cRdReg(sBCode *p, int ridx);
void cWrReg(sBCode *p, int ridx, int data1, int data2);
int	RdC1(void);
cplx RdC2(void);

void flagEffect(int type, int z, int n, int v, int c);
void cFlagEffect(int type, cplx z, cplx n, cplx v, cplx c);
void dumpRegister(sBCode *p);

//int getLabelAddr(sBCode *p, sTabList htable[], char *s);
//int getIntSymAddr(sBCode *p, sTabList htable[], char *s);
int getIntImm(sBCode *p, int fval, int immtype);
cplx getCplxImm(sBCode *p, int fval, int immtype);
sBCode *updatePC(sBCode *p, sBCode *n);
int isUnbiasedRounding(void);
int isFractionalMode(void);
int isAVLatchedMode(void);
int isALUSatMode(void);

int OVCheck(int type, int x, int y);
int isNeg(int x, int n);
int CarryCheck(sBCode *p, int x, int y, int c);
int satCheck12b(int ov, int c);

int RdDataMem(sBCode *p, unsigned int addr);
int briefRdDataMem(unsigned int addr);
void WrDataMem(sBCode *p, int data, unsigned int addr);
//unsigned int getBitReversedAddr(unsigned int addr);

int isValidMemoryAddr(sBCode *p, int addr);
int isBreakpoint(sBCode *p);
int	ifCond(int cond);

int isMultiFunc(sBCode *p);
//int isALU(sBCode *p);
//int isALU_C(sBCode *p);
//int isMAC(sBCode *p);
//int isMAC_C(sBCode *p);
//int isSHIFT(sBCode *p);
//int isSHIFT_C(sBCode *p);
int isLD(sBCode *p);
int isLD_C(sBCode *p);
int isST(sBCode *p);
int isST_C(sBCode *p);
//int isCP(sBCode *p);
//int isCP_C(sBCode *p);
int isLDSTMulti(sBCode *p);

void printRunTimeError(int ln, char *s, char *msg);
void printRunTimeWarning(int ln, char *s, char *msg);
void printRunTimeMessage(void);
//int getCodeDReg12(char *ret, char *s);
//int getCodeXOP12(char *ret, char *s);
//int getCodeDReg12S(char *ret, char *s);
//int getCodeDReg24(char *ret, char *s);
//int getCodeDReg24S(char *ret, char *s);
//int getCodeXOP24(char *ret, char *s);
//int getCodeReg12(char *ret, char *s);
//int getCodeReg12S(char *ret, char *s);
//int getCodeReg24(char *ret, char *s);
//int getCodeReg24S(char *ret, char *s);
//int getCodeACC32(char *ret, char *s);
//int getCodeACC32S(char *ret, char *s);
//int getCodeACC64(char *ret, char *s);
//int getCodeACC64S(char *ret, char *s);
//int getCodeCOND(char *ret, char *s);
//int getCodeRReg16(char *ret, char *s);
//int getCodeRReg(char *ret, char *s);
//int getCodeCReg(char *ret, char *s);
//void getCodeAMF(char *ret, sBCode *p);
//void getCodeAF(char *ret, sBCode *p);
//void getCodeMF(char *ret, sBCode *p);
//void getCodeSF(char *ret, char *opr, unsigned int index);
//void getCodeLSF(char *ret, char *opr, unsigned int index);
//int getCodeYZ(char *ret, char *opr);
//int getCodeComplexYZ(char *ret, char *opr1, char *opr2);
//int getCodeMS(char *ret, char *opr);
//int getCodeU(char *ret, char *opr);
//int getCodeG(char *ret, char *opr);
//int getCodeIReg(char *ret, char *opr);
//int getCodeMReg(char *ret, char *opr);
//int getCodeTERM(char *ret, char *opr);
//int getCodeLPP(char *ret, char *opr, unsigned int index);
//int getCodePPP(char *ret, char *opr, unsigned int index);
//int getCodeSPP(char *ret, char *opr, unsigned int index);
//int getCodeCONJ(char *ret, int conj);
//int getCodeDC(char *ret, char *opr0, char *opr1);
//void getCodeENA(char *ret, sBCode *p);
//int getCodeMN(char *ret, char *opr);
//void int2bin(char *ret, int val, int n);

int calcRounding(int x);
void processMACFunc(sBCode *p, int temp1, int temp2, int rdst, int opt, int fmn);
void processMAC_CFunc(sBCode *p, cplx ct1, cplx ct2, int rdst, int opt, int fmn);
void processMAC_RCFunc(sBCode *p, int temp1, cplx ct2, int rdst, int opt, int fmn);
void processALUFunc(sBCode *p, int vs1, int vs2, int rdst);
void processALU_CFunc(sBCode *p, cplx cvs1, cplx cvs2, int rdst);
void processSHIFTFunc(sBCode *p, int temp1, int temp2, int rdst, int opt);
void processSHIFT_CFunc(sBCode *p, cplx ct1, cplx ct2, int rdst, int opt);
void processCordicFunc(sBCode *p, cplx ct1, int rdst);

void updateIReg(sBCode *p, int ridx, int data);
//void resetPMA(int addr);

int getIntField(sBCode *p, int fval);
//int getWidthOfField(sBCode *p, int fval);
int getIndexFromAF(sBCode *p, int cflag);
int getIndexFromAMF(sBCode *p, int cflag);
int getIndexFromSF(sBCode *p, int cflag);
int getIndexFromSF2(sBCode *p, int cflag);
int getIndexFromMF(sBCode *p, int cflag);
int getIndexFromDF(sBCode *p);
int getOptionFromAMF(sBCode *p);
int getOptionFromMF(sBCode *p);
int getOptionFromSF(sBCode *p);
int getOptionFromSF2(sBCode *p);

int isReadOnlyReg(sBCode *p, int ridx);

#endif	/* _BINSIMSUPPORT_H */
