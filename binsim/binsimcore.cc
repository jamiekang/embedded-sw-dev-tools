/*
All Rights Reserved.
*/

/** 
* @file binsimcore.cc
* @brief Binary simulator main loop and supporting routines
* @date 2009-02-05
*/

#include <stdio.h>	/* perror() */
#include <stdlib.h>	/* strtol(), exit() */
#include <string.h>	/* strcpy() */
#include <ctype.h>	/* isdigit() */
#include "binsim.h"
#include "bcode.h"
//#include "symtab.h"
//#include "dmem.h"
#include "parse.h"
#include "binsimcore.h"
#include "binsimsupport.h"
#include "stack.h"
//#include "secinfo.h"

#ifdef VHPI
#include "dsp.h"	/* for vhpi interface */
#endif

/** 
* @brief Simulation main loop (processing simulation commands & running each instruction)
* 
* @param bcode User program converted to intermediate code format by lexer & parser
* 
* @return 0 if the simulation successfully ends.
*/
int simCore(sBCodeList bcode)
{
	int i, j;
	char ch;	
	char tstr[MAX_COMMAND_BUFFER+1];
	int tdata, tAddr = 0;
	int lastaddr = 0;
	char lastcommand = 0;
	int ret;
	int addrov = FALSE;

	sBCode *p = bcode.FirstNode;
	sBCode *oldp;

	p = updatePC(NULL, p);
	dumpRegister(p);

	/* reset warning/error message buffer */
	strcpy(msgbuf, "");

	while(p != NULL) {

		/* print current instruction here */
		printf("----\n");
		printf("Iteration:%d/%d NextPC:%04X Opcode:%s Type:%s\n",
			ItrCntr+1, ItrMax, p->PMA, (char *)sOp[p->Index], sType[p->InstType]);
		//if(p->Line){
		//	printf("Next>>\t%s\n", p->Line);
		//}

		if((SimMode == 'S') || ((SimMode == 'C') && (isBreakpoint(p)))){

			if(!QuietMode){
				/* display internal states */
				displayInternalStates();

				printf("\nCurrent simulation mode (<C>ontinue <S>tep into): %c\n", SimMode);
				if(BreakPoint != UNDEFINED)
					printf("Current Breakpoint PC: 0x%04X\n", BreakPoint);

get_command:
				printf("Choose command:\n");
				printf("\t<C>ontinue\n");
				printf("\t<S>tep into\n");
				printf("\tSet <B>reakpoint: <B> ADDR or just <B>\n");
				printf("\t<D>elete current breakpoint\n");
				printf("\tDump data <M>emory: <M> ADDR or just <M>\n");
				printf("\t<Enter> to repeat last <C> or <S> or <M>\n");
				printf("\t<Q>uit\n");
				printf("\t>> ");

				fgets(tstr, MAX_COMMAND_BUFFER, stdin);
				ch = tstr[0];

				if(ch == '\0')	;	/* no input, no change */
				else {
					if((ch == 'c') || (ch == 'C')){
						SimMode = 'C';
						printf("Continue\n\n");
						lastcommand = 'c';
					} else if((ch == 's') || (ch == 'S')){
						SimMode = 'S';
						printf("Step into\n\n");
						lastcommand = 's';
					} else if(ch == '\n'){
						if(lastcommand == 'c'){
							printf("Continue\n\n");
						} else if(lastcommand == 's'){
							printf("Step into\n\n");
						} else if(lastcommand == 'm'){
							tAddr = lastaddr;

							if(tAddr > 0xFFFF || tAddr < 0){
								tAddr &= 0xFFFF;
							}

							lastcommand = 'm';
							lastaddr = tAddr;

							printf("--------\n");
							addrov = FALSE;
							for(j = 0; j < 16; j++){
								if(tAddr > 0xFFFF) {
									addrov = TRUE;
									printf("Address out of range: ignored\n\n");
									break;
								}

								printf("%04x: ", tAddr+dataSegAddr);
								for(i = 0; i < 16; i++){
									if(tAddr > 0xFFFF) {
										addrov = TRUE;
										break;
									}

									tdata = briefRdDataMem(tAddr);
									tAddr++;
									printf("%03X ", tdata);
								}
								printf("\n");
							}
							printf("--------\n");
	
							if(!addrov) {
								if(!(tAddr > 0xFFFF))
									lastaddr = tAddr;
							}
							goto get_command;
						}
					} else if((ch == 'b') || (ch == 'B')){
						ret = sscanf(tstr+1, "%X", &tAddr);
						if(ret == EOF){
							if(dataSegAddr){
								printf("\tNote that current data segment base address is: 0x%04X\n", 
									dataSegAddr);
								printf("\tEnter start address (offset) in hexadecimal (ex. 01F0): ");
							}else{
								printf("\tEnter start address in hexadecimal (ex. 01F0): ");
							}

							fgets(tstr, MAX_COMMAND_BUFFER, stdin);
							sscanf(tstr, "%X", &tAddr);
						}
						if(tAddr > 0xFFFF || tAddr < 0){
							tAddr &= 0xFFFF;
						}

						BreakPoint = tAddr;
						printf("Breakpoint set at: 0x%04X\n\n", BreakPoint);
						goto get_command;
					} else if((ch == 'D') || (ch == 'd')){
						printf("Breakpoint deleted at: 0x%04X\n\n", BreakPoint);
						BreakPoint = UNDEFINED;
						goto get_command;
					} else if((ch == 'm') || (ch == 'M')){
						ret = sscanf(tstr+1, "%X", &tAddr);
						if(ret == EOF){
							if(dataSegAddr){
								printf("\tNote that current data segment base address is: 0x%04X\n", 
									dataSegAddr);
								printf("\tEnter start address (offset) in hexadecimal (ex. 01F0): ");
							}else{
								printf("\tEnter start address in hexadecimal (ex. 01F0): ");
							}

							fgets(tstr, MAX_COMMAND_BUFFER, stdin);
							sscanf(tstr, "%X", &tAddr);
						}
						if(tAddr > 0xFFFF || tAddr < 0){
							tAddr &= 0xFFFF;
						}

						lastcommand = 'm';
						lastaddr = tAddr;

						printf("--------\n");
						addrov = FALSE;
						for(j = 0; j < 16; j++){
							if(tAddr > 0xFFFF) {
								addrov = TRUE;
								printf("Address out of range: ignored\n\n");
								break;
							}

							printf("%04x: ", tAddr+dataSegAddr);
							for(i = 0; i < 16; i++){
								if(tAddr > 0xFFFF) {
									addrov = TRUE;
									break;
								}

								tdata = briefRdDataMem(tAddr);
								tAddr++;
								printf("%03X ", tdata);
							}
							printf("\n");
						}
						printf("--------\n");

						if(!addrov) {
							if(!(tAddr > 0xFFFF))
								lastaddr = tAddr;
						}
						goto get_command;
					} else if((ch == 'q') || (ch == 'Q')){
						SimMode = 'Q';
						return 1;	/* early exit */
					} else {	/* undefined input */
						printf("Undefined command %c - ignored.\n\n", ch);
						goto get_command;
					}
				}
			}	/* if(!QuietMode) */
		}
		////////////////////////////
		oldp = p;
		p = binSimOneStep(p);

		Cycles += oldp->Latency;

		dumpRegister(oldp);
		printRunTimeMessage();
		////////////////////////////
	}

	/* display internal states at the end */
	displayInternalStates();

	return 0;	/** Simulation successfully ended */
}

/** 
* @brief Initialize simulation variables and read data if needed.
*/
void initSim(void)
{
	int i;
	int data;

	/* initialize stacks */
	stackInit(&PCStack, iPC);
	stackInit(&LoopBeginStack, iLoopBegin);
	stackInit(&LoopEndStack, iLoopEnd);
	stackInit(&LoopCounterStack, iLoopCounter);
	for(i = 0; i < 4; i++)
		stackInit(&StatusStack[i], iStatus);
	stackInit(&LPEVERStack, iLPEVER);
	
	/* initialize registers */
	rSstat.PCE = 1;
	rSstat.PCL = 1;
	rSstat.LSE = 1;
	rSstat.SSE = 1;

	/* memory dump input */
	if(dumpInFP){
		//if -ad option set (dataSegAddr)
		dumpInStart -= dataSegAddr;

		for(i = 0; i < dumpInSize; i++){
			fscanf(dumpInFP, "%d\n", &data);	/* read decimal number */
			WrDataMem(NULL, data & 0xFFF, dumpInStart + i);
		}
	}
}


/** 
* @brief Close files and write data if needed.
*/
void closeSim(void)
{
	int i;
	int data;

	/* memory dump output */
	if(dumpOutFP){
		//if -ad option set (dataSegAddr)
		dumpOutStart -= dataSegAddr;

		for(i = 0; i < dumpOutSize; i++){
			data = briefRdDataMem(dumpOutStart + i);
			if(data >= 0x800){
				data -= 0x1000;
			}
			fprintf(dumpOutFP, "%d\n", data);
		}
	}

	/* close file */
	fclose(inFP);
	if(LdiFP) fclose(LdiFP);
	if(dumpDisFP) fclose(dumpDisFP);
	if(dumpInFP)  fclose(dumpInFP);
	if(dumpOutFP) fclose(dumpOutFP);
}


/** 
* @brief Simulate one binary instruction word
* 
* @param p Pointer to current instruction
* 
* @return Pointer to next instruction
*/
sBCode *binSimOneStep(sBCode *p)
{
	int loopCntr;
	sBCode *NextCode;
	int z, n, v, c;
	int isBranchTaken = FALSE;

	int fs0, fs1, fs2, fs3, fd;
	int ridx0, ridx1, ridx2, ridx3;
	int temp3, temp4, temp5;
	cplx z2, n2, v2, c2;
	const cplx  o2 = { 0, 0 };
	int tAddr;

	/* for iENA, iDIS */
	int code[9];
	int ena;

	/* for now, all instructions have 1 cycle latency */
	/* should be modified later */
	Latency = 1;

    switch(p->InstType){

		/* ALU Instructions */
		case t09c:
			{
				/* [IF COND] ADD/ADDC/SUB/SUBC/SUBB/SUBBC/AND/OR/XOR DREG12, XOP12, YOP12 */
				/* [IF COND] ADD/ADDC/SUB/SUBC/SUBB/SUBBC/AND/OR/XOR Opr0  , Opr1 , Opr2  */
				/* [IF COND] NOT/ABS/INC/DEC DREG12, XOP12 */
				/* [IF COND] NOT/ABS/INC/DEC Opr0  , Opr1  */

				if(ifCond(p->Cond)){
					/* read Opr1 data */
					fs1 = getIntField(p, fSRCA1);
					ridx1 = getRegIndex(p, fs1, eXOP12);
					int tData1 = RdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCA2);
					ridx2 = getRegIndex(p, fs2, eYOP12);
					int tData2 = RdReg(p, ridx2);

					/* get Opr3 addr */
					fd = getIntField(p, fDSTA);
					ridx3 = getRegIndex(p, fd, eDREG12);

					processALUFunc(p, tData1, tData2, ridx3);
				}
			}
			break;
		case t09d:
			{
				/* [IF COND] ADD.C/ADDC.C/SUB.C/SUBC.C/SUBB.C/SUBBC.C DREG24, XOP24, YOP24[*] */
				/* [IF COND] ADD.C/ADDC.C/SUB.C/SUBC.C/SUBB.C/SUBBC.C Opr0  , Opr1 , Opr2     */
				/* [IF COND] NOT.C/ABS.C DREG24, XOP24[*] */
				/* [IF COND] NOT.C/ABS.C Opr0  , Opr1     */

				if(ifCond(p->Cond)){
					/* read Opr1 data */
					fs1 = getIntField(p, fSRCA1);
					ridx1 = getRegIndex(p, fs1, eXOP24);
					cplx ct1 = cRdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCA2);
					ridx2 = getRegIndex(p, fs2, eYOP24);
					cplx ct2 = cRdReg(p, ridx2);

					/* get Opr3 addr */
					fd = getIntField(p, fDSTA);
					ridx3 = getRegIndex(p, fd, eDREG24);

					/* CONJ(*) modifier */
					if(p->Conj) ct2.i = -ct2.i; 

					processALU_CFunc(p, ct1, ct2, ridx3);
				}
			}
			break;
		case t09e:
			{
				/* [IF COND] ADD/ADDC/SUB/SUBC/SUBB/SUBBC DREG12, XOP12, IMM_INT4 */
				/* [IF COND] ADD/ADDC/SUB/SUBC/SUBB/SUBBC Opr0  , Opr1 , Opr2     */

				if(ifCond(p->Cond)){
					/* read Opr1 data */
					fs1 = getIntField(p, fSRCA1);
					ridx1 = getRegIndex(p, fs1, eXOP12);
					int tData1 = RdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCA2);
					int tData2 = getIntImm(p, fs2, eIMM_INT4);

					/* get Opr3 addr */
					fd = getIntField(p, fDSTA);
					ridx3 = getRegIndex(p, fd, eDREG12);

					processALUFunc(p, tData1, tData2, ridx3);
				}
			}
			break;
		case t09f:
			{
				/* [IF COND] ADD.C/ADDC.C/SUB.C/SUBC.C/SUBB.C/SUBBC.C DREG24, XOP24[*], IMM_COMPLEX8 */
				/* [IF COND] ADD.C/ADDC.C/SUB.C/SUBC.C/SUBB.C/SUBBC.C Opr0  , Opr1    , Opr2         */

				if(ifCond(p->Cond)){
					/* read Opr1 data */
					fs1 = getIntField(p, fSRCA1);
					ridx1 = getRegIndex(p, fs1, eXOP24);
					cplx ct1 = cRdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCA2);
					cplx ct2 = getCplxImm(p, fs2, eIMM_COMPLEX8);

					/* get Opr3 addr */
					fd = getIntField(p, fDSTA);
					ridx3 = getRegIndex(p, fd, eDREG24);

					/* CONJ(*) modifier */
					if(p->Conj) ct1.i = -ct1.i; 

					processALU_CFunc(p, ct1, ct2, ridx3);
				}
			}
			break;
		case t09i:
			{
				/* [IF COND] AND/OR/XOR/TSTBIT/SETBIT/CLRBIT/TGLBIT DREG12, XOP12, IMM_UINT4 */
				/* [IF COND] AND/OR/XOR/TSTBIT/SETBIT/CLRBIT/TGLBIT Opr0  , Opr1 , Opr2     */

				if(ifCond(p->Cond)){
					/* read Opr1 data */
					fs1 = getIntField(p, fSRCA1);
					ridx1 = getRegIndex(p, fs1, eXOP12);
					int tData1 = RdReg(p, ridx1);

					/* read YZ field */
					int fyz = getIntField(p, fYZ);
					/* YZ = 1 if Opr2 is zero or not existent */

					if(!fyz || (p->Index == iCLRBIT)){	/* YZ=0 OR CLRBIT */
						/* read Opr2 data */
						fs2 = getIntField(p, fSRCA2);
						int tData2 = getIntImm(p, fs2, eIMM_UINT4);

						/* get Opr3 addr */
						fd = getIntField(p, fDSTA);
						ridx3 = getRegIndex(p, fd, eDREG12);

						processALUFunc(p, tData1, tData2, ridx3);
					}else{		/* YZ: 1 */
						/* get Opr3 addr */
						fd = getIntField(p, fDSTA);
						ridx3 = getRegIndex(p, fd, eDREG12);

						processALUFunc(p, tData1, 0, ridx3);
					}
				}
			}
			break;
		case t23a:
			{
				/* DIVS/DIVQ REG12, XOP12, YOP12 */
				/* DIVS/DIVQ Opr0 , Opr1 , Opr2  */
				/* Input:  (Op1|Op0)(Dividend 24b), Op2(Divisor 12b) */
                /* Output: Op0(Quotient 12b), Op1(Remainder 12b)     */
                /* Before DIVS/DIVQ, lower 12b of dividend must be loaded into Op0 */

                /* Note: In ADSP 219x,                              */
                /* Input:  (AF|AY0)(Dividend 24b), AX0(Divisor 12b) */
                /* Output: AY0(Quotient 12b), AF(Remainder 12b)     */
                /* for details, refer to 2-25~2-29 of ADSP-219x     */
                /* DSP Hardware Reference                           */

				/* read DF field */
				int fdf = getIntField(p, fDF);
				/* DF: 0 if DIVQ, 1 if DIVS */

				/* read Opr0 data */
				fs0 = getIntField(p, fDSTA);
				ridx0 = getRegIndex(p, fs0, eREG12);
				int temp0 = RdReg(p, ridx0);	/* Lower 12b of dividend */

				/* read Opr1 data */
				fs1 = getIntField(p, fSRCA1);
				ridx1 = getRegIndex(p, fs1, eXOP12);
				int temp1 = RdReg(p, ridx1);	/* Upper 12b of dividend */

				/* read Opr2 data */
				fs2 = getIntField(p, fSRCA2);
				ridx2 = getRegIndex(p, fs2, eYOP12);
				int temp2 = RdReg(p, ridx2);	/* Divisor 12b */

				if(fdf){	/* if iDIVS */
					int msb0 = isNeg(temp1, 12) ^ isNeg(temp2, 12);
					rAstatR.AQ = msb0;          /* set AQ */

					int msb1 = isNeg(temp0, 12); /* get msb of AY0 */
					temp1 = ((temp1 << 1) | msb1);  /* AF  = (AF  << 1) | (msb of AY0) */
					temp0 = ((temp0 << 1) | msb0);  /* AY0 = (AY0 << 1) | (new AQ)     */
				}else{		/* if iDIVQ */
					if(rAstatR.AQ){
						temp5 = temp1 + temp2;  /* R = Y+X if AQ = 1 */
					}else{
						temp5 = temp1 - temp2;  /* R = Y-X if AQ = 0 */
					}

					int msb0 = isNeg(temp5, 12) ^ isNeg(temp2, 12);
					rAstatR.AQ = msb0;          /* set AQ */

					int msb1 = isNeg(temp0, 12); /* get msb of AY0 */
					temp1 = (((0x7FF & temp5) << 1) | msb1);    /* AF  = (R << 1) | (msb of AY0) */
					temp0 = ((temp0 << 1) | (!msb0));  			/* AY0 = (AY0 << 1) | !(new AQ)  */
				}

                WrReg(p, ridx1, temp1);         /* update AF  */
                WrReg(p, ridx0, temp0);         /* update AY0 */
			}
			break;
		case t46a:
			{
				/* [IF COND] SCR DREG12, XOP12, YOP12 */
				/* [IF COND] SCR Opr0  , Opr1 , Opr2  */

				if(ifCond(p->Cond)){
					/* read Opr1 data */
					fs1 = getIntField(p, fSRCA1);
					ridx1 = getRegIndex(p, fs1, eXOP12);
					int tData1 = RdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCA2);
					ridx2 = getRegIndex(p, fs2, eYOP12);
					int tData2 = RdReg(p, ridx2);

					/* get Opr3 addr */
					fd = getIntField(p, fDSTA);
					ridx3 = getRegIndex(p, fd, eDREG12);

					processALUFunc(p, tData1, tData2, ridx3);
				}
			}
			break;
		case t46b:
			{
				/* [IF COND] SCR.C DREG24, XOP24, YOP24[*] */
				/* [IF COND] SCR.C Opr0  , Opr1 , Opr2     */

				if(ifCond(p->Cond)){
					/* read Opr1 data */
					fs1 = getIntField(p, fSRCA1);
					ridx1 = getRegIndex(p, fs1, eXOP24);
					cplx ct1 = cRdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCA2);
					ridx2 = getRegIndex(p, fs2, eYOP24);
					cplx ct2 = cRdReg(p, ridx2);

					/* get Opr3 addr */
					fd = getIntField(p, fDSTA);
					ridx3 = getRegIndex(p, fd, eDREG24);

					/* CONJ(*) modifier */
					if(p->Conj) ct2.i = -ct2.i; 

					processALU_CFunc(p, ct1, ct2, ridx3);
				}
			}
			break;
		/* MAC Instructions */
		case t40e:
			{
				/* [IF COND] MPY/MAC/MAS ACC32, XOP12, YOP12 (|RND,SS,SU,US,UU|) */
				/* [IF COND] MPY/MAC/MAS Opr0 , Opr1 , Opr2  (|RND,SS,SU,US,UU|) */

				if(ifCond(p->Cond)){
					/* read MN field */
					int fmn = getIntField(p, fMN);
					/* MN = 1 if destination is NONE */

					/* read multiplication option: RND, SS, SU, US, UU */
					int opt = getOptionFromAMF(p);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCM1);
					ridx1 = getRegIndex(p, fs1, eXOP12);
					int tData1 = RdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCM2);
					ridx2 = getRegIndex(p, fs2, eYOP12);
					int tData2 = RdReg(p, ridx2);

					/* get Opr3 addr */
					fd = getIntField(p, fDSTM);
					ridx3 = getRegIndex(p, fd, eACC32);

					processMACFunc(p, tData1, tData2, ridx3, opt, fmn);
				}
			}
			break;
		case t40f:
			{
				/* [IF COND] MPY.C/MAC.C/MAS.C ACC64, XOP24, YOP24[*] (|RND,SS,SU,US,UU|) */
				/* [IF COND] MPY.C/MAC.C/MAS.C Opr0 , Opr1 , Opr2     (|RND,SS,SU,US,UU|) */

				if(ifCond(p->Cond)){
					/* read MN field */
					int fmn = getIntField(p, fMN);
					/* MN = 1 if destination is NONE */

					/* read multiplication option: RND, SS, SU, US, UU */
					int opt = getOptionFromAMF(p);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCM1);
					ridx1 = getRegIndex(p, fs1, eXOP24);
					cplx ct1 = cRdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCM2);
					ridx2 = getRegIndex(p, fs2, eYOP24);
					cplx ct2 = cRdReg(p, ridx2);

					/* get Opr3 addr */
					fd = getIntField(p, fDSTM);
					ridx3 = getRegIndex(p, fd, eACC64);

					/* CONJ(*) modifier */
					if(p->Conj) ct2.i = -ct2.i; 

					processMAC_CFunc(p, ct1, ct2, ridx3, opt, fmn);
				}
			}
			break;
		case t40g:
			{
				/* [IF COND] MPY.RC/MAC.RC/MAS.RC ACC64, XOP12, YOP24[*] (|RND,SS,SU,US,UU|) */
				/* [IF COND] MPY.RC/MAC.RC/MAS.RC Opr0 , Opr1 , Opr2     (|RND,SS,SU,US,UU|) */

				if(ifCond(p->Cond)){
					/* read MN field */
					int fmn = getIntField(p, fMN);
					/* MN = 1 if destination is NONE */

					/* read multiplication option: RND, SS, SU, US, UU */
					int opt = getOptionFromAMF(p);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCM1);
					ridx1 = getRegIndex(p, fs1, eXOP12);
					int tData1 = RdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCM2);
					ridx2 = getRegIndex(p, fs2, eYOP24);
					cplx ct2 = cRdReg(p, ridx2);

					/* get Opr3 addr */
					fd = getIntField(p, fDSTM);
					ridx3 = getRegIndex(p, fd, eACC64);

					/* CONJ(*) modifier */
					if(p->Conj) ct2.i = -ct2.i; 

					processMAC_RCFunc(p, tData1, ct2, ridx3, opt, fmn);
				}
			}
			break;
		case t41a:
			{
				/* [IF COND] RNDACC ACC32 */
				/* [IF COND] RNDACC Opr0  */
				if(ifCond(p->Cond)){
					/* read Opr0 data */
					fs0 = getIntField(p, fDSTM);
					ridx0 = getRegIndex(p, fs0, eACC32);
					int temp5 = RdReg(p, ridx0);

					/* Rounding operation */
					/* when result is 0x800, add 1 to bit 11 */
					/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
					/* Note that after rounding, the content of ACCx.L is INVALID. */
					temp5 = calcRounding(temp5);

					v = OVCheck(p->InstType, temp5, 0);
					WrReg(p, ridx0, temp5);
					
					flagEffect(p->InstType, 0, 0, v, 0);
				}
			}
			break;
		case t41b:
			{
				/* [IF COND] RNDACC.C ACC64[*] */
				/* [IF COND] RNDACC.C Opr0[*]  */
				if(ifCond(p->Cond)){
					/* read Opr0 data */
					fs0 = getIntField(p, fDSTM);
					ridx0 = getRegIndex(p, fs0, eACC64);
					cplx ct5 = cRdReg(p, ridx0);

					/* CONJ(*) modifier */
					if(p->Conj) ct5.i = -ct5.i; 

					/* Rounding operation */
					/* when result is 0x800, add 1 to bit 11 */
					/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
					/* Note that after rounding, the content of ACCx.L is INVALID. */
					ct5.r = calcRounding(ct5.r);
					ct5.i = calcRounding(ct5.i);

					v2.r = OVCheck(p->InstType, ct5.r, 0);
					v2.i = OVCheck(p->InstType, ct5.i, 0);
					cWrReg(p, ridx0, ct5.r, ct5.i);

					cFlagEffect(p->InstType, o2, o2, v2, o2);
				}
			}
			break;
		case t41c:
			{
				/* [IF COND] CLRACC ACC32 */
				/* [IF COND] CLRACC Opr0  */
				if(ifCond(p->Cond)){
					/* read Opr0 data */
					fs0 = getIntField(p, fDSTM);
					ridx0 = getRegIndex(p, fs0, eACC32);

					WrReg(p, ridx0, 0);
					flagEffect(p->InstType, 0, 0, 0, 0);
				}
			}
			break;
		case t41d:
			{
				/* [IF COND] CLRACC.C ACC64[*] */
				/* [IF COND] CLRACC.C Opr0[*]  */
				if(ifCond(p->Cond)){
					/* read Opr0 data */
					fs0 = getIntField(p, fDSTM);
					ridx0 = getRegIndex(p, fs0, eACC64);

					cWrReg(p, ridx0, 0, 0);
					cFlagEffect(p->InstType, o2, o2, o2, o2);
				}
			}
			break;
		case t25a:
			{
				/* [IF COND] SATACC ACC32 */
				/* [IF COND] SATACC Opr0  */
				if(ifCond(p->Cond)){
					/* read Opr0 data */
					fs0 = getIntField(p, fDSTM);
					ridx0 = getRegIndex(p, fs0, eACC32);
					int temp5 = RdReg(p, ridx0);

					int mul_ov;
					if(((temp5 & 0x0FF800000) == 0x0FF800000) ||((temp5 & 0xFF800000) == 0)){
						mul_ov = FALSE;
					}else{
						mul_ov = TRUE;
					}

					if(mul_ov){			/* if overflow */
						if(temp5 >= 0){		/* if MSB of ACCx.H is 0 */
							temp5 = 0x007FFFFF;		/* set to max positive number */
						} else {			/* if MSB of ACCx.H is 1 */
							temp5 = 0xFF800000;		/* set to min negative number */
						}
					}
					WrReg(p, ridx0, temp5);
				}
			}
			break;
		case t25b:
			{
				/* [IF COND] SATACC.C ACC64[*] */
				/* [IF COND] SATACC.C Opr0[*]  */
				if(ifCond(p->Cond)){
					/* read Opr0 data */
					fs0 = getIntField(p, fDSTM);
					ridx0 = getRegIndex(p, fs0, eACC64);
					cplx ct5 = cRdReg(p, ridx0);

					/* CONJ(*) modifier */
					if(p->Conj) ct5.i = -ct5.i; 

					cplx mul_ov;
					if(((ct5.r & 0x0FF800000) == 0x0FF800000) ||((ct5.r & 0xFF800000) == 0)){
						mul_ov.r = FALSE;
					}else{
						mul_ov.r = TRUE;
					}
					if(((ct5.i & 0x0FF800000) == 0x0FF800000) ||((ct5.i & 0xFF800000) == 0)){
						mul_ov.i = FALSE;
					}else{
						mul_ov.i = TRUE;
					}

					if(mul_ov.r){			/* if overflow */
						if(ct5.r >= 0){     /* if MSB of ACCx.H is 0 */
							ct5.r = 0x007FFFFF;
						} else {            /* if MSB of ACCx.H is 1 */
							ct5.r = 0xFF800000;
						}
					}
					if(mul_ov.i){			/* if overflow */
						if(ct5.i >= 0){     /* if MSB of ACCx.H is 0 */
							ct5.i = 0x007FFFFF;
						} else {            /* if MSB of ACCx.H is 1 */
							ct5.i = 0xFF800000;
						}
					}
					cWrReg(p, ridx0, ct5.r, ct5.i);
				}
			}
			break;
		case t09g:
			{
				/* [IF COND] ADD/SUB/SUBB ACC32, ACC32, ACC32 */
				/* [IF COND] ADD/SUB/SUBB Opr0 , Opr1 , Opr2  */

				if(ifCond(p->Cond)){
					/* read Opr1 data */
					fs1 = getIntField(p, fSRCM1);
					ridx1 = getRegIndex(p, fs1, eACC32);
					int tData1 = RdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCM2);
					ridx2 = getRegIndex(p, fs2, eACC32);
					int tData2 = RdReg(p, ridx2);

					/* get Opr0 addr */
					fd = getIntField(p, fDSTM);
					ridx0 = getRegIndex(p, fd, eACC32);

					processALUFunc(p, tData1, tData2, ridx0);
				}
			}
			break;
		case t09h:
			{
				/* [IF COND] ADD.C/SUB.C/SUBB.C ACC64, ACC64, ACC64[*] */
				/* [IF COND] ADD.C/SUB.C/SUBB.C Opr0 , Opr1 , Opr2[*]  */

				if(ifCond(p->Cond)){
					/* read Opr1 data */
					fs1 = getIntField(p, fSRCM1);
					ridx1 = getRegIndex(p, fs1, eACC64);
					cplx ct1 = cRdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCM2);
					ridx2 = getRegIndex(p, fs2, eACC64);
					cplx ct2 = cRdReg(p, ridx2);

					/* CONJ(*) modifier */
					if(p->Conj) ct2.i = -ct2.i; 

					/* get Opr0 addr */
					fd = getIntField(p, fDSTM);
					ridx0 = getRegIndex(p, fd, eACC64);

					processALU_CFunc(p, ct1, ct2, ridx0);
				}
			}
			break;

		/* Shifter Instructions */
		case t16a:
			{
				/* [IF COND] ASHIFT/ASHIFTOR/LSHIFT/LSHIFTOR ACC32, XOP12, YOP12 (|HI,LO,HIRND,LORND|) */
				/* [IF COND] ASHIFT/ASHIFTOR/LSHIFT/LSHIFTOR Opr0 , Opr1 , Opr2  (|HI,LO,HIRND,LORND|) */

				if(ifCond(p->Cond)){
					/* read shift option: Hi,LO,HIRND,LORND */
					int opt = getOptionFromSF(p);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCS1);
					ridx1 = getRegIndex(p, fs1, eXOP12);
					int tData1 = RdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCS2);
					ridx2 = getRegIndex(p, fs2, eYOP12);
					int tData2 = RdReg(p, ridx2);

					if(tData2 > 32) tData2 = 32;
					else if(tData2 < -32) tData2 = -32;

					/* get Opr3 addr */
					fd = getIntField(p, fDSTS);
					ridx3 = getRegIndex(p, fd, eACC32);

					processSHIFTFunc(p, tData1, tData2, ridx3, opt);
				}
			}
			break;
		case t16b:
			{
				/* [IF COND] ASHIFT.C/ASHIFTOR.C/LSHIFT.C/LSHIFTOR.C ACC64, XOP24, YOP12 (|HI,LO,HIRND,LORND|) */
				/* [IF COND] ASHIFT.C/ASHIFTOR.C/LSHIFT.C/LSHIFTOR.C Opr0 , Opr1 , Opr2  (|HI,LO,HIRND,LORND|) */

				if(ifCond(p->Cond)){
					/* read shift option: Hi,LO,HIRND,LORND */
					int opt = getOptionFromSF(p);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCS1);
					ridx1 = getRegIndex(p, fs1, eXOP24);
					cplx ct1 = cRdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCS2);
					ridx2 = getRegIndex(p, fs2, eYOP12);
					cplx ct2;
					ct2.r = ct2.i = RdReg(p, ridx2);

					if(ct2.r > 32) ct2.r = ct2.i = 32;
					else if(ct2.r < -32) ct2.r = ct2.i = -32;

					/* get Opr3 addr */
					fd = getIntField(p, fDSTS);
					ridx3 = getRegIndex(p, fd, eACC64);

					processSHIFT_CFunc(p, ct1, ct2, ridx3, opt);
				}
			}
			break;
		case t16c:
			{
				if(ifCond(p->Cond)){
					if(p->Index == iEXP){
						/* [IF COND] EXP DREG12, XOP12 (|HIX,HI,LO|) */
						/* [IF COND] EXP Opr0  , Opr1  (|HIX,HI,LO|) */

						/* read shift option: Hi, LO */
						int opt = getOptionFromSF(p);

						/* read Opr1 data */
						fs1 = getIntField(p, fSRCS1);
						ridx1 = getRegIndex(p, fs1, eXOP12);
						int temp1 = RdReg(p, ridx1);

                   		temp5 = 0;

						/* get Opr0 addr */
						fd = getIntField(p, fDSTS);
						ridx0 = getRegIndex(p, fd, eDREG12);

						if((opt == esHI)||(opt == esHIX)){
							int msb = 0x800 & temp1;
							int mask = 0x400;

							for(int i = 0; i < 11; i++){
								if(temp1 & mask){   /* if current bit is 1 */
									if(msb) temp5++;
									else
										break;
								}else{              /* if current bit is 0 */
									if(!msb) temp5++;
									else
										break;
								}
								mask >>= 1;
							}

							/* if HIX, check AV */
							if(opt == esHIX){
								if(rAstatR.AV){
									temp5 = -1;     /* indicates overflow happened
									                   just before this instruction */
								}
							}

							/* set SS flag */
							if(!rAstatR.AV){            /* if AV == 0 */
								rAstatR.SS = isNeg(temp1, 12);
							}else{                  /* if AV == 1 */
								if(rAstatR.AV && (opt == esHIX))
									rAstatR.SS = (isNeg(temp1, 12)? 0: 1);
							}

							WrReg(p, ridx0, temp5);
						}else if(opt == esLO){
							temp5 = RdReg(p, ridx0);

							if(temp5 == 11){    /* if upper word is all sign bits */
								int sb = rAstatR.SS;        /* sign bit of previous word */
								int mask = 0x800;

								for(int i = 0; i < 12; i++){
									if(temp1 & mask){   /* if current bit is 1 */
										if(sb) temp5++;
										else
											break;
									}else{              /* if current bit is 0 */
										if(!sb) temp5++;
										else
											break;
									}
									mask >>= 1;
								}
							}else{              /* else return previous (HI/HIX) result */
								;
							}
							WrReg(p, ridx0, temp5);
						}
					}else if(p->Index == iEXPADJ){
						/* [IF COND] EXPADJ DREG12, XOP12 */
						/* [IF COND] EXPADJ Opr0  , Opr1  */

						/* get Opr0 addr */
						fd = getIntField(p, fDSTS);
						ridx0 = getRegIndex(p, fd, eDREG12);

						int temp0 = RdReg(p, ridx0);    /* DST Opr0 must be initialized to 12 */

						/* read Opr1 data */
						fs1 = getIntField(p, fSRCS1);
						ridx1 = getRegIndex(p, fs1, eXOP12);
						int temp1 = RdReg(p, ridx1);

						temp5 = 0;

						int msb = 0x800 & temp1;
						int mask = 0x400;

						for(int i = 0; i < 11; i++){
							if(temp1 & mask){   /* if current bit is 1 */
								if(msb) temp5++;
								else
									break;
							}else{              /* if current bit is 0 */
								if(!msb) temp5++;
								else
									break;
							}
							mask >>= 1;
						}

						if(temp5 < temp0) {     /* if current result is smaller than Opr0 */
							temp0 = temp5;      /* update Opr0 */
						}
						WrReg(p, ridx0, temp0);
					}else{
						printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Invalid inst. index found while processing t16c in binSimOneStep().\n");
					}
                }
			}
			break;
		case t16d:
			{
				if(ifCond(p->Cond)){
					if(p->Index == iEXP_C){
						/* [IF COND] EXP.C DREG24, XOP24 (|HIX,HI,LO|) */
						/* [IF COND] EXP.C Opr0  , Opr1  (|HIX,HI,LO|) */

						/* read shift option: Hi, LO */
						int opt = getOptionFromSF(p);

						/* read Opr1 data */
						fs1 = getIntField(p, fSRCS1);
						ridx1 = getRegIndex(p, fs1, eXOP24);
						cplx ct1 = cRdReg(p, ridx1);

						cplx ct5;
						ct5.r = ct5.i = 0;

						/* get Opr0 addr */
						fd = getIntField(p, fDSTS);
						ridx0 = getRegIndex(p, fd, eDREG24);

						if((opt == esHI)||(opt == esHIX)){
							/* real part */
							int msb = 0x800 & ct1.r;
							int mask = 0x400;

							for(int i = 0; i < 11; i++){
								if(ct1.r & mask){   /* if current bit is 1 */
									if(msb) ct5.r++;
									else
										break;
								}else{              /* if current bit is 0 */
									if(!msb) ct5.r++;
									else
										break;
								}
								mask >>= 1;
							}

							/* imaginary part */
							msb = 0x800 & ct1.i;
							mask = 0x400;

							for(int i = 0; i < 11; i++){
								if(ct1.i & mask){   /* if current bit is 1 */
									if(msb) ct5.i++;
									else
										break;
								}else{              /* if current bit is 0 */
									if(!msb) ct5.i++;
									else
										break;
								}
								mask >>= 1;
							}

							/* if HIX, check AV */
							if(opt == esHIX){
								if(rAstatR.AV){
									ct5.r = -1;     /* indicates overflow happened
													   just before this instruction */
								}

								if(rAstatI.AV){
									ct5.i = -1;     /* indicates overflow happened
													   just before this instruction */
								}
							}

							/* set SS flag */
							if(!rAstatR.AV){            /* if AV == 0 */
								rAstatR.SS = isNeg(ct1.r, 12);
							}else{                  /* if AV == 1 */
								if(rAstatR.AV && (opt == esHIX))
									rAstatR.SS = (isNeg(ct1.r, 12)? 0: 1);
							}
							if(!rAstatI.AV){            /* if AV == 0 */
								rAstatI.SS = isNeg(ct1.i, 12);
							}else{                  /* if AV == 1 */
								if(rAstatI.AV && (opt == esHIX))
									rAstatI.SS = (isNeg(ct1.i, 12)? 0: 1);
							}

							cWrReg(p, ridx0, ct5.r, ct5.i);
						}else if(opt == esLO){
							cplx ct5 = cRdReg(p, ridx0);

							if(ct5.r == 11){    /* if upper word is all sign bits */
								int sb = rAstatR.SS;        /* sign bit of previous word */
								int mask = 0x800;

								for(int i = 0; i < 12; i++){
									if(ct1.r & mask){   /* if current bit is 1 */
										if(sb) ct5.r++;
										else
											break;
									}else{              /* if current bit is 0 */
										if(!sb) ct5.r++;
										else
											break;
									}
									mask >>= 1;
								}
							}else{              /* else return previous (HI/HIX) result */
								;
							}

							if(ct5.i == 11){    /* if upper word is all sign bits */
								int sb = rAstatI.SS;        /* sign bit of previous word */
								int mask = 0x800;

								for(int i = 0; i < 12; i++){
									if(ct1.i & mask){   /* if current bit is 1 */
										if(sb) ct5.i++;
										else
											break;
									}else{              /* if current bit is 0 */
										if(!sb) ct5.i++;
										else
											break;
									}
									mask >>= 1;
								}
							}else{              /* else return previous (HI/HIX) result */
								;
							}
							cWrReg(p, ridx0, ct5.r, ct5.i);
						}
					}else if(p->Index == iEXPADJ_C){
						/* [IF COND] EXPADJ.C DREG24, XOP24 */
						/* [IF COND] EXPADJ.C Opr0  , Opr1  */

						/* get Opr0 addr */
						fd = getIntField(p, fDSTS);
						ridx0 = getRegIndex(p, fd, eDREG24);

						cplx ct0 = cRdReg(p, ridx0);    /* DST Opr0 must be initialized to (12,12) */

						/* read Opr1 data */
						fs1 = getIntField(p, fSRCS1);
						ridx1 = getRegIndex(p, fs1, eXOP24);
						cplx ct1 = cRdReg(p, ridx1);

						cplx ct5;
						ct5.r = ct5.i = 0;

						int msb = 0x800 & ct1.r;
						int mask = 0x400;

						for(int i = 0; i < 11; i++){
							if(ct1.r & mask){   /* if current bit is 1 */
								if(msb) ct5.r++;
								else
									break;
							}else{              /* if current bit is 0 */
								if(!msb) ct5.r++;
								else
									break;
							}
							mask >>= 1;
						}

						msb = 0x800 & ct1.i;
						mask = 0x400;

						for(int i = 0; i < 11; i++){
							if(ct1.i & mask){   /* if current bit is 1 */
								if(msb) ct5.i++;
								else
									break;
							}else{              /* if current bit is 0 */
								if(!msb) ct5.i++;
								else
									break;
							}
							mask >>= 1;
						}

						if(ct5.r < ct0.r) {     /* if current result is smaller than Opr0 */
							ct0.r = ct5.r;
						}
						if(ct5.i < ct0.i) {     /* if current result is smaller than Opr0 */
							ct0.i = ct5.i;
						}
						cWrReg(p, ridx0, ct0.r, ct0.i); /* update Opr0 */
					}else{
						printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Invalid inst. index found while processing t16d in binSimOneStep().\n");
					}
				}
			}
			break;
		case t16e:
			{
				/* [IF COND] ASHIFT/ASHIFTOR/LSHIFT/LSHIFTOR DREG12, XOP12, YOP12 (|NORND,RND|) */
				/* [IF COND] ASHIFT/ASHIFTOR/LSHIFT/LSHIFTOR Opr0  , Opr1 , Opr2  (|NORND,RND|) */

				if(ifCond(p->Cond)){
					/* read shift option: NORND,RND */
					int opt = getOptionFromSF(p);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCS1);
					ridx1 = getRegIndex(p, fs1, eXOP12);
					int tData1 = RdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCS2);
					ridx2 = getRegIndex(p, fs2, eYOP12);
					int tData2 = RdReg(p, ridx2);

					if(tData2 > 32) tData2 = 32;
					else if(tData2 < -32) tData2 = -32;

					/* get Opr3 addr */
					fd = getIntField(p, fDSTS);
					ridx3 = getRegIndex(p, fd, eDREG12);

					processSHIFTFunc(p, tData1, tData2, ridx3, opt);
				}
			}
			break;
		case t16f:
			{
				/* [IF COND] ASHIFT.C/ASHIFTOR.C/LSHIFT.C/LSHIFTOR.C DREG24, XOP24, YOP12 (|NORND,RND|) */
				/* [IF COND] ASHIFT.C/ASHIFTOR.C/LSHIFT.C/LSHIFTOR.C Opr0  , Opr1 , Opr2  (|NORND,RND|) */

				if(ifCond(p->Cond)){
					/* read shift option: NORND,RND */
					int opt = getOptionFromSF(p);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCS1);
					ridx1 = getRegIndex(p, fs1, eXOP24);
					cplx ct1 = cRdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCS2);
					ridx2 = getRegIndex(p, fs2, eYOP12);
					cplx ct2;
					ct2.r = ct2.i = RdReg(p, ridx2);

					if(ct2.r > 32) ct2.r = ct2.i = 32;
					else if(ct2.r < -32) ct2.r = ct2.i = -32;

					/* get Opr3 addr */
					fd = getIntField(p, fDSTS);
					ridx3 = getRegIndex(p, fd, eDREG24);

					processSHIFT_CFunc(p, ct1, ct2, ridx3, opt);
				}
			}
			break;
		case t15a:
			{
				/* ASHIFT/ASHIFTOR/LSHIFT/LSHIFTOR ACC32, XOP12, IMM_INT5 (|HI,LO,HIRND,LORND|) */
				/* ASHIFT/ASHIFTOR/LSHIFT/LSHIFTOR Opr0 , Opr1 , Opr2     (|HI,LO,HIRND,LORND|) */

				{
					/* read shift option: Hi, LO,HIRND,LORND */
					int opt = getOptionFromSF(p);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCS1);
					ridx1 = getRegIndex(p, fs1, eXOP12);
					int tData1 = RdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCS2);
					int tData2 = getIntImm(p, fs2, eIMM_INT5);

					/* get Opr3 addr */
					fd = getIntField(p, fDSTS);
					ridx3 = getRegIndex(p, fd, eACC32);

					processSHIFTFunc(p, tData1, tData2, ridx3, opt);
				}
			}
			break;
		case t15b:
			{
				/* ASHIFT.C/ASHIFTOR.C/LSHIFT.C/LSHIFTOR.C ACC64, XOP24, IMM_INT5 (|HI,LO,HIRND,LORND|) */
				/* ASHIFT.C/ASHIFTOR.C/LSHIFT.C/LSHIFTOR.C Opr0 , Opr1 , Opr2     (|HI,LO,HIRND,LORND|) */

				if(ifCond(p->Cond)){
					/* read shift option: Hi, LO,HIRND,LORND */
					int opt = getOptionFromSF(p);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCS1);
					ridx1 = getRegIndex(p, fs1, eXOP24);
					cplx ct1 = cRdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCS2);
					cplx ct2;
					ct2.r = ct2.i = getIntImm(p, fs2, eIMM_INT5);

					/* get Opr3 addr */
					fd = getIntField(p, fDSTS);
					ridx3 = getRegIndex(p, fd, eACC64);

					processSHIFT_CFunc(p, ct1, ct2, ridx3, opt);
				}
			}
			break;
		case t15c:
			{
				/* ASHIFT/LSHIFT ACC32, ACC32, IMM_INT6 (|NORND,RND|) */
				/* ASHIFT/LSHIFT Opr0 , Opr1 , Opr2     (|NORND,RND|) */

				{
					/* read shift option: NORND,RND */
					int opt = getOptionFromSF(p);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCS1);
					ridx1 = getRegIndex(p, fs1, eACC32);
					int tData1 = RdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCS2);
					int tData2 = getIntImm(p, fs2, eIMM_INT6);

					/* get Opr3 addr */
					fd = getIntField(p, fDSTS);
					ridx3 = getRegIndex(p, fd, eACC32);

					processSHIFTFunc(p, tData1, tData2, ridx3, opt);
				}
			}
			break;
		case t15d:
			{
				/* ASHIFT.C/LSHIFT.C ACC64, ACC64, IMM_INT6 (|NORND,RND|) */
				/* ASHIFT.C/LSHIFT.C Opr0 , Opr1 , Opr2     (|NORND,RND|) */

				{
					/* read shift option: NORND,RND */
					int opt = getOptionFromSF(p);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCS1);
					ridx1 = getRegIndex(p, fs1, eACC64);
					cplx ct1 = cRdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCS2);
					cplx ct2;
					ct2.r = ct2.i = getIntImm(p, fs2, eIMM_INT6);

					/* get Opr3 addr */
					fd = getIntField(p, fDSTS);
					ridx3 = getRegIndex(p, fd, eACC64);

					processSHIFT_CFunc(p, ct1, ct2, ridx3, opt);
				}
			}
			break;
		case t15e:
			{
				/* [IF COND] ASHIFT/ASHIFTOR/LSHIFT/LSHIFTOR DREG12, XOP12, IMM_INT5 (|NORND,RND|) */
				/* [IF COND] ASHIFT/ASHIFTOR/LSHIFT/LSHIFTOR Opr0  , Opr1 , Opr2     (|NORND,RND|) */

				if(ifCond(p->Cond)){
					/* read shift option: NORND,RND */
					int opt = getOptionFromSF(p);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCS1);
					ridx1 = getRegIndex(p, fs1, eXOP12);
					int tData1 = RdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCS2);
					int tData2 = getIntImm(p, fs2, eIMM_INT5);

					if(tData2 > 32) tData2 = 32;
					else if(tData2 < -32) tData2 = -32;

					/* get Opr3 addr */
					fd = getIntField(p, fDSTS);
					ridx3 = getRegIndex(p, fd, eDREG12);

					processSHIFTFunc(p, tData1, tData2, ridx3, opt);
				}
			}
			break;
		case t15f:
			{
				/* [IF COND] ASHIFT.C/ASHIFTOR.C/LSHIFT.C/LSHIFTOR.C DREG24, XOP24, IMM_INT5 (|NORND,RND|) */
				/* [IF COND] ASHIFT.C/ASHIFTOR.C/LSHIFT.C/LSHIFTOR.C Opr0  , Opr1 , Opr2     (|NORND,RND|) */

				if(ifCond(p->Cond)){
					/* read shift option: NORND,RND */
					int opt = getOptionFromSF(p);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCS1);
					ridx1 = getRegIndex(p, fs1, eXOP24);
					cplx ct1 = cRdReg(p, ridx1);

					/* read Opr2 data */
					fs2 = getIntField(p, fSRCS2);
					cplx ct2;
					ct2.r = ct2.i = getIntImm(p, fs2, eIMM_INT5);

					if(ct2.r > 32) ct2.r = ct2.i = 32;
					else if(ct2.r < -32) ct2.r = ct2.i = -32;

					/* get Opr3 addr */
					fd = getIntField(p, fDSTS);
					ridx3 = getRegIndex(p, fd, eDREG24);

					processSHIFT_CFunc(p, ct1, ct2, ridx3, opt);
				}
			}
			break;

		/* Multifunction Instructions */
		case t01a:		/* MAC || LD || LD or LD || LD */
			/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) 
				|| LD XOP12S, DM(IX+/+=MX) || LD YOP12S, DM(IY+/+=MY) */
			/* LD XOP12S, DM(IX+/+=MX) || LD YOP12S, DM(IY+/+=MY) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t01b:		/* MAC.C || LD.C || LD.C or LD.C || LD.C */
			/* MAC.C ACC64S, XOP24S, YOP24S (|RND,SS,SU,US,UU|)
				|| LD.C XOP24S, DM(IX+/+=MX) || LD.C YOP24S, DM(IY+/+=MY) */
			/* LD.C XOP24S, DM(IX+/+=MX) || LD.C YOP24S, DM(IY+/+=MY) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t01c:		/* ALU || LD || LD */
			/* ALU DREG12S, XOP12S, YOP12S 
				|| LD XOP12S, DM(IX+/+=MX) || LD YOP12S, DM(IY+/+=MY) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t04a:		/* MAC || LD */
			/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) 
				|| LD DREG12, DM(IREG+/+=MREG) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t04c:		/* ALU || LD */
			/* ALU DREG12S, XOP12S, YOP12S 
				|| LD DREG12, DM(IREG+/+=MREG) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t04b:		/* MAC.C || LD.C */
			/* MAC.C ACC64S, XOP24S, YOP24S (|RND,SS,SU,US,UU|) 
				|| LD.C DREG24, DM(IREG+/+=MREG) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t04d:		/* ALU.C || LD.C */
			/* ALU.C DREG24S, XOP24S, YOP24S 
				|| LD.C DREG24, DM(IREG+/+=MREG) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t12e:		/* SHIFT || LD */
			/* SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) 
				|| LD DREG12, DM(IREG+=MREG) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t12f:		/* SHIFT.C || LD.C */
			/* SHIFT.C ACC64S, XOP24S, IMM_INT5 (|HI,LO,HIRND,LORND|) 
				|| LD.C DREG24, DM(IREG+=MREG) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t12m:		/* SHIFT || LD */
			/* SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) 
				|| LD DREG12, DM(IREG+=MREG) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t12n:		/* SHIFT.C || LD.C */
			/* SHIFT.C ACC64S, ACC64S, IMM_INT5 (|NORND,RND|) 
				|| LD.C DREG24, DM(IREG+=MREG) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t04e:		/* MAC || ST */
			/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) 
				|| ST DM(IREG+/+=MREG), DREG12 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t04g:		/* ALU || ST */
			/* ALU DREG12S, XOP12S, YOP12S 
				|| ST DM(IREG+/+=MREG), DREG12 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t04f:		/* MAC.C || ST.C */
			/* MAC.C ACC64S, XOP24S, YOP24S (|RND,SS,SU,US,UU|) 
				|| ST.C DM(IREG+/+=MREG), DREG24 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t04h:		/* ALU.C || ST.C */
			/* ALU.C DREG24S, XOP24S, YOP24S 
				|| ST.C DM(IREG+/+=MREG), DREG24 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t12g:		/* SHIFT || ST */
			/* SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) 
				|| ST DM(IREG+=MREG), DREG12 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t12h:		/* SHIFT.C || ST.C */
			/* SHIFT.C ACC64S, XOP24S, IMM_INT5 (|HI,LO,HIRND,LORND|) 
				|| ST.C DM(IREG+=MREG), DREG24 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t12o:		/* SHIFT || ST */
			/* SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) 
				|| ST DM(IREG+=MREG), DREG12 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t12p:		/* SHIFT.C || ST.C */
			/* SHIFT.C ACC64S, ACC64S, IMM_INT5 (|NORND,RND|) 
				|| ST.C DM(IREG+=MREG), DREG24 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t08a:		/* MAC || CP */
			/* MAC ACC32S, XOP12S, YOP12S [( |RND, SS, SU, US, UU| )] 
				|| CP DREG12, DREG12 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t08c:		/* ALU || CP */
			/* ALU DREG12S, XOP12S, YOP12S 
				|| CP DREG12, DREG12 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t08b:		/* MAC.C || CP.C */
			/* MAC.C ACC64S, XOP24S, YOP24S [( |RND, SS, SU, US, UU| )] 
				|| CP.C DREG24, DREG24 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t08d:		/* ALU.C || CP.C */
			/* ALU.C DREG24S, XOP24S, YOP24S 
				|| CP.C DREG24, DREG24 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t14c:		/* SHIFT || CP */
			/* SHIFT ACC32S, XOP12S, IMM_INT5 ( |HI,LO,HIRND,LORND| ) 
				|| CP DREG12, DREG12 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t14d:		/* SHIFT.C || CP.C */
			/* SHIFT.C ACC64S, XOP24S, IMM_INT5 ( |HI,LO,HIRND,LORND| ) 
				|| CP.C DREG24, DREG24 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t14k:		/* SHIFT || CP */
			/* SHIFT ACC32S, ACC32S, IMM_INT5 ( |NORND,RND| ) 
				|| CP DREG12, DREG12 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t14l:		/* SHIFT.C || CP.C */
			/* SHIFT.C ACC64S, ACC64S, IMM_INT5 ( |NORND,RND| ) 
				|| CP.C DREG24, DREG24 */
			p = binSimOneStepMultiFunc(p);
			break;
		case t43a:		/* ALU || MAC */
			/* ALU DREG12S, XOP12S, YOP12S 
				|| MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t44b:		/* ALU || SHIFT */
			/* ALU DREG12S, XOP12S, YOP12S 
				|| SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t44d:		/* ALU || SHIFT */
			/* ALU DREG12S, XOP12S, YOP12S 
				|| SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t45b:		/* MAC || SHIFT */
			/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) 
				|| SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) */
			p = binSimOneStepMultiFunc(p);
			break;
		case t45d:		/* MAC || SHIFT */
			/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) 
				|| SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) */
			p = binSimOneStepMultiFunc(p);
			break;

		/* Data Move Instructions */
		case t17a:
			{
				/* CP DREG12, DREG12 */
				/* CP Opr0  , Opr1   */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eDREG12);

				/* read Opr1 data */
				fs1 = getIntField(p, fSRCLS);
				ridx1 = getRegIndex(p, fs1, eDREG12);
				int tData1 = RdReg(p, ridx1);

				/* write to Opr0 */
				WrReg(p, ridx3, tData1);
			}
			break;
		case t17b:
			{
				/* CP DREG12, RREG16 */	/* fDC = fs2 = 0 */
				/* CP RREG16, DREG12 */	/* fDC = fs2 = 1 */
				/* CP Opr0  , Opr1   */

				/* read DC field */
				fs2 = getIntField(p, fDC);	
				/* fs2: 0 if DREG12 <- RREG16, 1 if RREG16 <- DREG12 */

				if(fs2){		/* if RREG16 <- DREG12 */
					/* get Opr0 addr */
					fd = getIntField(p, fSRCLS);
					ridx3 = getRegIndex(p, fd, eRREG16);

					/* read Opr1 data */
					fs1 = getIntField(p, fDSTLS);
					ridx1 = getRegIndex(p, fs1, eDREG12);
					int tData1 = RdReg(p, ridx1);

					/* write to Opr0 */
					if(!isReadOnlyReg(p, ridx3))
						WrReg(p, ridx3, tData1);
				}else{			/* if DREG12 <- RREG16 */
					/* get Opr0 addr */
					fd = getIntField(p, fDSTLS);
					ridx3 = getRegIndex(p, fd, eDREG12);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCLS);
					ridx1 = getRegIndex(p, fs1, eRREG16);
					int tData1 = RdReg(p, ridx1);

					/* write to Opr0 */
					WrReg(p, ridx3, tData1 & 0xFFF);
				}
			}
			break;
		case t17g:
			{
				/* CP RREG16, RREG16 */
				/* CP Opr0  , Opr1   */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eRREG16);

				/* read Opr1 data */
				fs1 = getIntField(p, fSRCLS);
				ridx1 = getRegIndex(p, fs1, eRREG16);
				int tData1 = RdReg(p, ridx1);

				/* write to Opr0 */
				if(!isReadOnlyReg(p, ridx3))
					WrReg(p, ridx3, tData1);
			}
			break;
		case t17c:
			{
				/* CP ACC32, ACC32 */
				/* CP Opr0 , Opr1  */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eACC32);

				/* read Opr1 data */
				fs1 = getIntField(p, fSRCLS);
				ridx1 = getRegIndex(p, fs1, eACC32);
				int temp1 = RdReg(p, ridx1);

				/* write to Opr0 */
				WrReg(p, ridx3, temp1);
			}
			break;
		case t17h:
			{
				/* CP ACC32, DREG12 */
				/* CP Opr0 , Opr1  */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eACC32);

				/* read Opr1 data */
				fs1 = getIntField(p, fSRCLS);
				ridx1 = getRegIndex(p, fs1, eDREG12);
				int temp1 = RdReg(p, ridx1);

				/* write to Opr0 */
				WrReg(p, ridx3, temp1);
			}
			break;
		case t17d:
			{
				/* CP.C DREG24, DREG24 */
				/* CP.C Opr0  , Opr1   */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eDREG24);

				/* read Opr1 data */
				fs1 = getIntField(p, fSRCLS);
				ridx1 = getRegIndex(p, fs1, eDREG24);
				cplx cData = cRdReg(p, ridx1);

				/* write to Opr0 */
				cWrReg(p, ridx3, cData.r, cData.i);
			}
			break;
		case t17e:
			{
				/* CP.C DREG24, RREG16 */ /* fDC = fs2 = 0 */
				/* CP.C RREG16, DREG24 */ /* fDC = fs2 = 1 */
				/* CP.C Opr0  , Opr1   */

				/* read DC field */
				fs2 = getIntField(p, fDC);	
				/* fs2: 0 if DREG24 <- RREG16, 1 if RREG16 <- DREG24 */

				if(fs2){		/* if RREG16 <- DREG24 */
					/* get Opr0 addr */
					fd = getIntField(p, fSRCLS);
					ridx3 = getRegIndex(p, fd, eRREG16);

					/* read Opr1 data */
					fs1 = getIntField(p, fDSTLS);
					ridx1 = getRegIndex(p, fs1, eDREG24);
					cplx ct1 = cRdReg(p, ridx1);                /* read 24-bit source */

					int lsbVal = 0x0FFF & ct1.r;            /* get LSB 12-bit */
					int msbVal = 0x0F & ct1.i;              /* get MSB 4-bit */
					int val = ((msbVal << 12) | lsbVal);

					/* write to Opr0 */
					if(!isReadOnlyReg(p, ridx3))
						WrReg(p, ridx3, val);           /* write to 16-bit register */
				}else{			/* if DREG24 <- RREG16 */
					/* get Opr0 addr */
					fd = getIntField(p, fDSTLS);
					ridx3 = getRegIndex(p, fd, eDREG24);

					/* read Opr1 data */
					fs1 = getIntField(p, fSRCLS);
					ridx1 = getRegIndex(p, fs1, eRREG16);
					int val = RdReg(p, ridx1);		/* read 16-bit source */
					int lsbVal = 0x0FFF & val;              /* get LSB 12-bit */
					int msbVal = ((0x0F000 & val) >> 12);   /* get MSB 4-bit */
					if(isNeg(val, 16)) msbVal |= 0xFF0;      /* sign-extension */

					/* write to Opr0 */
					cWrReg(p, ridx3, lsbVal, msbVal);           /* write to complex pairs */
				}
			}
			break;
		case t17f:
			{
				/* CP.C ACC64, ACC64 */
				/* CP.C Opr0 , Opr1  */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eACC64);

				/* read Opr1 data */
				fs1 = getIntField(p, fSRCLS);
				ridx1 = getRegIndex(p, fs1, eACC64);

				cplx ct1 = cRdReg(p, ridx1);

				/* write to Opr0 */
				cWrReg(p, ridx3, ct1.r, ct1.i);
			}
			break;
		case t03a:
			{
				/* LD DREG12, DM(IMM_UINT16) */
				/* LD Opr0  , DM(Opr1)       */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eDREG12);

				/* read Opr1 addr */
				fs1 = getIntField(p, fSRCLS);
				int	tAddr = getIntImm(p, fs1, eIMM_UINT16);

				/* read Opr1 data */
				int tData1 = RdDataMem(p, tAddr);

#ifdef VHPI
				if(VhpiMode){   /* rh: read 12b */
					dsp_rh(tAddr, (short int *)&tData1, 1);
					dsp_wait(1);
				}
#endif

				WrReg(p, ridx3, tData1);
			}
			break;
		case t03c:
			{
				/* LD.C DREG24, DM(IMM_UINT16) */
				/* LD.C Opr0  , DM(Opr1)       */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eDREG24);

				/* read Opr1 addr */
				fs1 = getIntField(p, fSRCLS);
				int tAddr = getIntImm(p, fs1, eIMM_UINT16);

				if(tAddr & 0x01){       //if not aligned
					printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Complex memory read/write must be aligned to even-word boundaries.\n");
					break;
				}

				/* read Opr1 data */
				int tData1 = RdDataMem(p, tAddr);
				int tData2 = RdDataMem(p, tAddr+1);

#ifdef VHPI
				if(VhpiMode){   /* rw: read 24b */
					int tData;

					dsp_rw(tAddr, &tData, 1);
					dsp_wait(1);

					/* Note: tData = (((0x0FFF & tData1) << 12) | (0x0FFF & tData2)); */
					tData1 = (0x0FFF & (tData >> 12));
					tData2 = 0xFFF & tData;
				}
#endif
				cWrReg(p, ridx3, tData1, tData2);
			}
			break;
		case t03d:
			{
				/* LD ACC32, DM(IMM_UINT16) (|HI,LO|) */
				/* LD Opr0 , DM(Opr1)       */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eACC32);

				/* read Opr1 addr */
				fs1 = getIntField(p, fSRCLS);
				int tAddr = getIntImm(p, fs1, eIMM_UINT16);

				/* read HI, LO option */
				fs2 = getIntField(p, fLSF);		/* fLSF: 1 if HI */

				/* read Opr1 data */
				int tData1 = RdDataMem(p, tAddr);
				int tData2 = RdDataMem(p, tAddr+1);

#ifdef VHPI
				if(VhpiMode){   /* rw: read 24b */
					int tData;
	
					dsp_rw(tAddr, &tData, 1);
					dsp_wait(1);

					/* Note: tData = (((0x0FFF & tData1) << 12) | (0x0FFF & tData2)); */
					tData1 = (0x0FFF & (tData >> 12));
					tData2 = 0xFFF & tData;
				}
#endif

				int temp1 = (((0x0FFF & tData2) << 12) | (0x0FFF & tData1));
				if(isNeg(temp1, 24)) temp1 |= 0x0FF000000;

				/* write to Opr0 */
				if(fs2) temp1 <<= 8;
				WrReg(p, ridx3, temp1);
			}
			break;
		case t03e:
			{
				/* LD.C ACC64, DM(IMM_UINT16) (|HI,LO|) */
				/* LD.C Opr0 , DM(Opr1)       */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eACC64);

				/* read Opr1 addr */
				fs1 = getIntField(p, fSRCLS);
				int tAddr = getIntImm(p, fs1, eIMM_UINT16);

				if(tAddr & 0x01){       //if not aligned
					printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Complex memory read/write must be aligned to even-word boundaries.\n");
					break;
				}

				/* read HI, LO option */
				fs2 = getIntField(p, fLSF);		/* fLSF: 1 if HI */

				/* read Opr1 data */
				cplx cData1, cData2;
				/* read real part 24 bits */
				cData1.r = RdDataMem(p, tAddr);       /* read low word */
				cData1.i = RdDataMem(p, tAddr+1);     /* read high word */
				/* read imaginary part 24 bits */
				cData2.r = RdDataMem(p, tAddr+2);     /* read low word */
				cData2.i = RdDataMem(p, tAddr+3);     /* read high word */

#ifdef VHPI
				if(VhpiMode){   /* rw+rw: read 24b+24b */
					int tData1, tData2;

					dsp_rw_rw(tAddr, &tData1, tAddr+2, &tData2, 1);
					dsp_wait(1);

					/* Note:
					tData1 = (((0x0FFF & cData1.r) << 12) | (0x0FFF & cData1.i));
					tData2 = (((0x0FFF & cData2.r) << 12) | (0x0FFF & cData2.i));
					*/
					cData1.r = (0x0FFF & (tData1 >> 12));
					cData1.i = 0xFFF & tData1;
					cData2.r = (0x0FFF & (tData2 >> 12));
					cData2.i = 0xFFF & tData2;
				}
#endif
				int temp1 = (((0x0FFF & cData1.i) << 12) | (0x0FFF & cData1.r));
				int temp2 = (((0x0FFF & cData2.i) << 12) | (0x0FFF & cData2.r));

				if(isNeg(temp1, 24)) temp1 |= 0x0FF000000;   /* sign extension */
				if(isNeg(temp2, 24)) temp2 |= 0x0FF000000;

				/* write to Opr0 */
				if(fs2) {
					temp1 <<= 8;
					temp2 <<= 8;
				}
				cWrReg(p, ridx3, temp1, temp2);
			}
			break;
		case t03f:
			{
				/* ST DM(IMM_UINT16), DREG12 */
				/* ST DM(Opr0)      , Opr1   */

				/* get Opr0 addr */
				fd = getIntField(p, fSRCLS);
				int tAddr = getIntImm(p, fd, eIMM_UINT16);

				/* read Opr1 data */
				fs1 = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fs1, eDREG12);
				int tData1 = RdReg(p, ridx3);

				WrDataMem(p, tData1, tAddr);

#ifdef VHPI
				if(VhpiMode){   /* wh: write 12b */
					dsp_wh(tAddr, (short int *)&tData1, 1);
				}
#endif
			}
			break;
		case t03g:
			{	
				/* ST.C DM(IMM_UINT16), DREG24 */
				/* ST.C DM(Opr0)      , Opr1   */

				/* get Opr0 addr */
				fd = getIntField(p, fSRCLS);
				int tAddr = getIntImm(p, fd, eIMM_UINT16);

				if(tAddr & 0x01){       //if not aligned
					printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Complex memory read/write must be aligned to even-word boundaries.\n");
					break;
				}

				/* read Opr1 data */
				fs1 = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fs1, eDREG24);
				cplx cData = cRdReg(p, ridx3);

				WrDataMem(p, cData.r, tAddr);
				WrDataMem(p, cData.i, tAddr+1);

#ifdef VHPI
                if(VhpiMode){   /* ww: write 24b */
                    int tData = (((0x0FFF & cData.r) << 12) | (0x0FFF & cData.i));
                    dsp_ww(tAddr, &tData, 1);
                }
#endif
			}
			break;
		case t03h:
			{
				/* ST DM(IMM_UINT16), ACC32 (|HI,LO|) */
				/* ST DM(Opr0)      , Opr1   */

				/* read HI, LO option */
				fs2 = getIntField(p, fLSF);		/* fLSF: 1 if HI */

				/* read Opr1 data */
				fs1 = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fs1, eACC32);
				int temp1 = RdReg(p, ridx3);

				if(fs2){
					temp1 >>= 8;	
				}

				int tData1 = 0x0FFF & temp1;                /* get low word of acc  */
				int tData2 = (0x0FFF000 & temp1) >> 12;     /* get high word of acc */

				/* get Opr0 addr */
				fd = getIntField(p, fSRCLS);
				int tAddr = getIntImm(p, fd, eIMM_UINT16);

				WrDataMem(p, tData1, tAddr);                   /* write low word  */
                WrDataMem(p, tData2, tAddr+1);                 /* write high word */

#ifdef VHPI
                if(VhpiMode){   /* ww: write 24b */
                    int tData = (((0x0FFF & tData1) << 12) | (0x0FFF & tData2));
                    dsp_ww(tAddr, &tData, 1);
                }
#endif
			}
			break;
		case t03i:
			{
				/* ST.C DM(IMM_UINT16), ACC64 (|HI,LO|) */
				/* ST.C DM(Opr0)      , Opr2   */

				/* read HI, LO option */
				fs2 = getIntField(p, fLSF);		/* fLSF: 1 if HI */

				/* read Opr1 data */
				fs1 = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fs1, eACC64);
				cplx ct1 = cRdReg(p, ridx3);

				if(fs2){
					ct1.r >>= 8;	/* real part */
					ct1.i >>= 8;	/* imaginary part */
				}

				/* get Opr0 addr */
				fd = getIntField(p, fSRCLS);
				int tAddr = getIntImm(p, fd, eIMM_UINT16);

				if(tAddr & 0x01){       //if not aligned
					printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Complex memory read/write must be aligned to even-word boundaries.\n");
					break;
				}

				cplx cData1, cData2;

				/* read real part 24 bits */
				cData1.r = 0x0FFF & ct1.r;                  /* get low word of acc  */
				cData1.i = (0x0FFF000 & ct1.r) >> 12;       /* get high word of acc */
				/* read real part 24 bits */
				cData2.r = 0x0FFF & ct1.i;                  /* get low word of acc  */
				cData2.i = (0x0FFF000 & ct1.i) >> 12;       /* get high word of acc */

				/* write real part 24 bits */
				WrDataMem(p, cData1.r, tAddr);                 /* write low word  */
				WrDataMem(p, cData1.i, tAddr+1);               /* write high word */
				/* write imaginary part 24 bits */
				WrDataMem(p, cData2.r, tAddr+2);               /* write low word  */
				WrDataMem(p, cData2.i, tAddr+3);               /* write high word */

#ifdef VHPI
				if(VhpiMode){   /* ww+ww: write 24b+24b */
					int tData1 = (((0x0FFF & cData1.r) << 12) | (0x0FFF & cData1.i));
					int tData2 = (((0x0FFF & cData2.r) << 12) | (0x0FFF & cData2.i));
					dsp_ww_ww(tAddr, &tData1, tAddr+2, &tData2, 1);
				}
#endif
			}
			break;
		case t06a:
			{
				/* LD RREG16, IMM_INT16 */
				/* LD Opr0  , Opr1      */
			
				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eRREG16);

				/* read Opr1 data */
				fs1 = getIntField(p, fSRCLS);
				int tData1 = getIntImm(p, fs1, eIMM_INT16);

				if(!isReadOnlyReg(p, ridx3))
					WrReg(p, ridx3, tData1);
			}
			break;
		case t06b:
			{
				/* LD DREG12, IMM_INT12 */
				/* LD Opr0  , Opr1      */
			
				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eDREG12);

				/* read Opr1 data */
				fs1 = getIntField(p, fSRCLS);
				int tData1 = getIntImm(p, fs1, eIMM_INT12);

				WrReg(p, ridx3, tData1);
			}
			break;
		case t06c:
			{
				/* LD.C DREG24, IMM_COMPLEX24 */
				/* LD.C Opr0  , Opr1          */
			
				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eDREG24);

				/* read Opr1 data */
				fs1 = getIntField(p, fSRCLS);
				cplx ct1 = getCplxImm(p, fs1, eIMM_COMPLEX24);

				cWrReg(p, ridx3, ct1.r, ct1.i);
			}
			break;
		case t06d:
			{
				/* LD ACC32, IMM_INT24 */
				/* LD Opr0 , Opr1      */
				
				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fd, eACC32);

				/* read Opr1 data */
				fs1 = getIntField(p, fSRCLS);
				int tData1 = getIntImm(p, fs1, eIMM_INT24);

				WrReg(p, ridx3, tData1);
			}
			break;
		case t32a:
			{
				/* LD DREG12, DM(IREG +=/+ MREG) */
				/* LD Opr0  , DM(Opr2 Opr1 Opr3) */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx0 = getRegIndex(p, fd, eDREG12);

				/* read Opr1 */
				fs1 = getIntField(p, fU);	/* 1 if "+=" */

				/* read Opr2 addr */
				fs2 = getIntField(p, fDMI);
				ridx2 = getRegIndex(p, fs2, eIREG);
				int tAddr2 = RdReg2(p, ridx2);

				/* read Opr3 addr */
				fs3 = getIntField(p, fDMM);
				ridx3 = getRegIndex(p, fs3, eMREG);
				int tAddr3 = RdReg2(p, ridx3);

				/* if premodify: don't update IREG */
				if(!fs1) tAddr2 += tAddr3;

				/* read Opr2 data */
				int tData2 = RdDataMem(p, tAddr2);

#ifdef VHPI
				if(VhpiMode){   /* rh: read 12b */
					dsp_rh(tAddr2, (short int *)&tData2, 1);
					dsp_wait(1);
				}
#endif
				WrReg(p, ridx0, tData2);

				/* if postmodify: update IREG */
				if(fs1){
					tAddr2 += tAddr3;
					updateIReg(p, ridx2, tAddr2);
				}
			}
			break;
		case t32b:
			{
				/* LD.C DREG24, DM(IREG +=/+ MREG) */
				/* LD.C Opr0  , DM(Opr2 Opr1 Opr3) */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx0 = getRegIndex(p, fd, eDREG24);

				/* read Opr1 */
				fs1 = getIntField(p, fU);	/* 1 if "+=" */

				/* read Opr2 addr */
				fs2 = getIntField(p, fDMI);
				ridx2 = getRegIndex(p, fs2, eIREG);
				int tAddr2 = RdReg2(p, ridx2);

				/* read Opr3 addr */
				fs3 = getIntField(p, fDMM);
				ridx3 = getRegIndex(p, fs3, eMREG);
				int tAddr3 = RdReg2(p, ridx3);

				/* if premodify: don't update IREG */
				if(!fs1) tAddr2 += tAddr3;

				if(tAddr2 & 0x01){       //if not aligned
					printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Complex memory read/write must be aligned to even-word boundaries.\n");
					break;
				}

				/* read Opr2 data */
				int tData21 = RdDataMem(p, tAddr2);
				int tData22 = RdDataMem(p, tAddr2+1);

#ifdef VHPI
				if(VhpiMode){   /* rw: read 24b */
					int tData;

					dsp_rw(tAddr2, &tData, 1);
					dsp_wait(1);

					/* Note: int tData = (((0x0FFF & tData21) << 12) | (0x0FFF & tData22)); */
					tData21 = (0x0FFF & (tData >> 12));
					tData22 = 0xFFF & tData;
				}
#endif
				cWrReg(p, ridx0, tData21, tData22);

				/* if postmodify: update IREG */
				if(fs1){
					tAddr2 += tAddr3;
					updateIReg(p, ridx2, tAddr2);
				}
			}
			break;
		case t32c:
			{
				/* ST DM(IREG +=/+ MREG), DREG12 */
				/* ST DM(Opr1 Opr0 Opr2), Opr3   */

				/* read Opr0 */
				fs0 = getIntField(p, fU);	/* 1 if "+=" */

				/* read Opr1 addr */
				fs1 = getIntField(p, fDMI);
				ridx1 = getRegIndex(p, fs1, eIREG);
				int tAddr1 = RdReg2(p, ridx1);

				/* read Opr2 addr */
				fs2 = getIntField(p, fDMM);
				ridx2 = getRegIndex(p, fs2, eMREG);
				int tAddr2 = RdReg2(p, ridx2);

				/* if premodify: don't update IREG */
				if(!fs0) tAddr1 += tAddr2;

				/* get Opr3 addr */
				fs3 = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fs3, eDREG12);
				int tData3 = RdReg(p, ridx3);

				WrDataMem(p, tData3, tAddr1);

#ifdef VHPI
				if(VhpiMode){   /* wh: write 12b */
					dsp_wh(tAddr1, (short int *)&tData3, 1);
				}
#endif

				/* if postmodify: update IREG */
				if(fs0){
					tAddr1 += tAddr2;
					updateIReg(p, ridx1, tAddr1);
				}
			}
			break;
		case t32d:
			{
				/* ST.C DM(IREG +=/+ MREG), DREG24 */
				/* ST.C DM(Opr1 Opr0 Opr2), Opr3   */

				/* read Opr0 */
				fs0 = getIntField(p, fU);	/* 1 if "+=" */

				/* read Opr1 addr */
				fs1 = getIntField(p, fDMI);
				ridx1 = getRegIndex(p, fs1, eIREG);
				int tAddr1 = RdReg2(p, ridx1);

				/* read Opr2 addr */
				fs2 = getIntField(p, fDMM);
				ridx2 = getRegIndex(p, fs2, eMREG);
				int tAddr2 = RdReg2(p, ridx2);

				/* if premodify: don't update IREG */
				if(!fs0) tAddr1 += tAddr2;

				if(tAddr1 & 0x01){       //if not aligned
					printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Complex memory read/write must be aligned to even-word boundaries.\n");
					break;
				}

				/* get Opr3 addr */
				fs3 = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fs3, eDREG24);

				cplx cData3 = cRdReg(p, ridx3);

				WrDataMem(p, cData3.r, tAddr1);
				WrDataMem(p, cData3.i, tAddr1+1);

#ifdef VHPI
				if(VhpiMode){   /* ww: write 24b */
					int tData = (((0x0FFF & cData3.r) << 12) | (0x0FFF & cData3.i));
					dsp_ww(tAddr1, &tData, 1);
				}
#endif

				/* if postmodify: update IREG */
				if(fs0){
					tAddr1 += tAddr2;
					updateIReg(p, ridx1, tAddr1);
				}
			}
			break;
		case t29a:
			{
				/* LD DREG12, DM(IREG +=/+ IMM_INT8) */
				/* LD Opr0  , DM(Opr2 Opr1 Opr3)     */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx0 = getRegIndex(p, fd, eDREG12);

				/* read Opr1 */
				fs1 = getIntField(p, fU);	/* 1 if "+=" */

				/* read Opr2 addr */
				fs2 = getIntField(p, fDMI);
				ridx2 = getRegIndex(p, fs2, eIREG);
				int tAddr2 = RdReg2(p, ridx2);

				/* read Opr3 addr */
				fs3 = getIntField(p, fSRCLS);
				int tAddr3 = getIntImm(p, fs3, eIMM_INT8);

				/* if premodify: don't update IREG */
				if(!fs1) tAddr2 += tAddr3;

				/* read Opr2 data */
				int tData2 = RdDataMem(p, tAddr2);

#ifdef VHPI
				if(VhpiMode){   /* rh: read 12b */
					dsp_rh(tAddr2, (short int *)&tData2, 1);
					dsp_wait(1);
				}
#endif
				WrReg(p, ridx0, tData2);

				/* if postmodify: update IREG */
				if(fs1){
					tAddr2 += tAddr3;
					updateIReg(p, ridx2, tAddr2);
				}
			}
			break;
		case t29b:
			{
				/* LD.C DREG24, DM(IREG +=/+ IMM_INT8) */
				/* LD.C Opr0  , DM(Opr2 Opr1 Opr3) */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTLS);
				ridx0 = getRegIndex(p, fd, eDREG24);

				/* read Opr1 */
				fs1 = getIntField(p, fU);	/* 1 if "+=" */

				/* read Opr2 addr */
				fs2 = getIntField(p, fDMI);
				ridx2 = getRegIndex(p, fs2, eIREG);
				int tAddr2 = RdReg2(p, ridx2);

				/* read Opr3 addr */
				fs3 = getIntField(p, fSRCLS);
				int tAddr3 = getIntImm(p, fs3, eIMM_INT8);

				/* if premodify: don't update IREG */
				if(!fs1) tAddr2 += tAddr3;

				if(tAddr2 & 0x01){       //if not aligned
					printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Complex memory read/write must be aligned to even-word boundaries.\n");
					break;
				}

				/* read Opr2 data */
				int tData21 = RdDataMem(p, tAddr2);
				int tData22 = RdDataMem(p, tAddr2+1);

#ifdef VHPI
				if(VhpiMode){   /* rw: read 24b */
					int tData;

					dsp_rw(tAddr2, &tData, 1);
					dsp_wait(1);

					/* Note: int tData = (((0x0FFF & tData21) << 12) | (0x0FFF & tData22)); */
					tData21 = (0x0FFF & (tData >> 12));
					tData22 = 0xFFF & tData;
				}
#endif
				cWrReg(p, ridx0, tData21, tData22);

				/* if postmodify: update IREG */
				if(fs1){
					tAddr2 += tAddr3;
					updateIReg(p, ridx2, tAddr2);
				}
			}
			break;
		case t29c:
			{
				/* ST DM(IREG +=/+ IMM_INT8), DREG12 */
				/* ST DM(Opr1 Opr0 Opr2)    , Opr3   */

				/* read Opr0 */
				fs0 = getIntField(p, fU);	/* 1 if "+=" */

				/* read Opr1 addr */
				fs1 = getIntField(p, fDMI);
				ridx1 = getRegIndex(p, fs1, eIREG);
				int tAddr1 = RdReg2(p, ridx1);

				/* read Opr2 addr */
				fs2 = getIntField(p, fSRCLS);
				int tAddr2 = getIntImm(p, fs2, eIMM_INT8);

				/* if premodify: don't update IREG */
				if(!fs0) tAddr1 += tAddr2;

				/* get Opr3 addr */
				fs3 = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fs3, eDREG12);
				int tData3 = RdReg(p, ridx3);

				WrDataMem(p, tData3, tAddr1);

#ifdef VHPI
				if(VhpiMode){   /* wh: write 12b */
					dsp_wh(tAddr1, (short int *)&tData3, 1);
				}
#endif

				/* if postmodify: update IREG */
				if(fs0){
					tAddr1 += tAddr2;
					updateIReg(p, ridx1, tAddr1);
				}
			}
			break;
		case t29d:
			{
				/* ST.C DM(IREG +=/+ IMM_INT8), DREG24 */
				/* ST.C DM(Opr1 Opr0 Opr2)    , Opr3   */

				/* read Opr0 */
				fs0 = getIntField(p, fU);	/* 1 if "+=" */

				/* read Opr1 addr */
				fs1 = getIntField(p, fDMI);
				ridx1 = getRegIndex(p, fs1, eIREG);
				int tAddr1 = RdReg2(p, ridx1);

				/* read Opr2 addr */
				fs2 = getIntField(p, fSRCLS);
				int tAddr2 = getIntImm(p, fs2, eIMM_INT8);

				/* if premodify: don't update IREG */
				if(!fs0) tAddr1 += tAddr2;

				if(tAddr1 & 0x01){       //if not aligned
					printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"Complex memory read/write must be aligned to even-word boundaries.\n");
					break;
				}

				/* get Opr3 addr */
				fs3 = getIntField(p, fDSTLS);
				ridx3 = getRegIndex(p, fs3, eDREG24);

				cplx cData3 = cRdReg(p, ridx3);

				WrDataMem(p, cData3.r, tAddr1);
				WrDataMem(p, cData3.i, tAddr1+1);

#ifdef VHPI
				if(VhpiMode){   /* ww: write 24b */
					int tData = (((0x0FFF & cData3.r) << 12) | (0x0FFF & cData3.i));
					dsp_ww(tAddr1, &tData, 1);
				}
#endif

				/* if postmodify: update IREG */
				if(fs0){
					tAddr1 += tAddr2;
					updateIReg(p, ridx1, tAddr1);
				}
			}
			break;
		case t22a:
			{
				/* ST DM(IREG += MREG), IMM_INT12 */
				/* ST DM(Opr0 += Opr1), Opr2      */

				/* read Opr0 addr */
				fs0 = getIntField(p, fDMI);
				ridx0 = getRegIndex(p, fs0, eIREG);
				int tAddr0 = RdReg2(p, ridx0);

				/* read Opr1 addr */
				fs1 = getIntField(p, fDMM);
				ridx1 = getRegIndex(p, fs1, eMREG);
				int tAddr1 = RdReg2(p, ridx1);

				/* read Opr2 data */
				fs2 = getIntField(p, fDSTLS);
				int tData2 = getIntImm(p, fs2, eIMM_INT12);

				WrDataMem(p, tData2, tAddr0);

#ifdef VHPI
				if(VhpiMode){   /* wh: write 12b */
					dsp_wh(tAddr0, (short int *)&tData2, 1);
				}
#endif

				/* if postmodify: update IREG */
				tAddr0 += tAddr1;
				updateIReg(p, ridx0, tAddr0);
			}
			break;

		/* Program Flow Instructions */
		case t11a:
			{
				/* DO IMM_UINT12 UNTIL TERM */
				/* DO Opr0      UNTIL Opr1 */

				/* read Opr0 data */
				fd = getIntField(p, fDSTB);
				int tData0 = getIntImm(p, fd, eIMM_UINT12);

				stackPush(&LoopBeginStack, p->PMA +1);  /* loop begin addr */
                stackPush(&LoopEndStack, p->PMA + tData0);  /* loop end addr */

				/* read TERM field */
				int fterm = getIntField(p, fTERM);
				/* TERM = 0xE if CE, 0xF if FOREVER */

				if(fterm == 0xF){ 			/* if FOREVER */
                    loopCntr = RdReg(p, eCNTR);
                    stackPush(&LoopCounterStack, loopCntr);

                    WrReg(p, eLPEVER, 1);
					stackPush(&LPEVERStack, 1);		//added 2009.05.22
                } else if(fterm == 0xE){ 	/* if CE */
                    loopCntr = RdReg(p, eCNTR);
                    stackPush(&LoopCounterStack, loopCntr);

                    WrReg(p, eLPEVER, 0);
					stackPush(&LPEVERStack, 0);		//added 2009.05.22
                }
                flagEffect(p->InstType, 0, 0, 0, 0);
			}
			break;
		case t10a:
			{
				/* [IF COND] CALL/JUMP IMM_INT13 */
				/* [IF COND] CALL/JUMP Opr0      */
				if(ifCond(p->Cond)){
					/* read Opr0 data */
					fd = getIntField(p, fDSTB);
					int tData0 = getIntImm(p, fd, eIMM_INT13);

					if(p->Index == iCALL){		/* if CALL */
						if(!DelaySlotMode){		/* if delay slot not enabled */
							stackPush(&PCStack, p->PMA +1);	/* return addr */
						}else{					/* if delay slot enabled */
							stackPush(&PCStack, p->PMA +2);	/* return addr */
						}
					}
					NextCode = sBCodeListSearch(&bCode, p->PMA + tData0);
					isBranchTaken = TRUE;

					if(DelaySlotMode){
						sBCode *pn = getNextCode(p);
						if(pn->isDelaySlot){
							pn->BrTarget = NextCode;	/* save branch target address in delay slot */
						}else{		/* error */
							printRunTimeError(p->PMA, (char *)sOp[p->Index], 
								"Delay slot processing error.\n");
						}
					}

					if(p->Index == iCALL){		/* if CALL */
                		flagEffect(p->InstType, 0, 0, 0, 0);
					}
				}
			}
			break;
		case t10b:
			{
				/* CALL/JUMP IMM_INT16 */
				/* CALL/JUMP Opr0      */
				/* read Opr0 data */
				fd = getIntField(p, fDSTB);
				int tData0 = getIntImm(p, fd, eIMM_INT16);

				if(p->Index == iCALL){		/* if CALL */
					if(!DelaySlotMode){		/* if delay slot not enabled */
						stackPush(&PCStack, p->PMA +1);	/* return addr */
					}else{					/* if delay slot enabled */
						stackPush(&PCStack, p->PMA +2);	/* return addr */
					}
				}
				NextCode = sBCodeListSearch(&bCode, p->PMA + tData0);
				isBranchTaken = TRUE;

				if(DelaySlotMode){
					sBCode *pn = getNextCode(p);
					if(pn->isDelaySlot){
						pn->BrTarget = NextCode;	/* save branch target address in delay slot */
					}else{		/* error */
						printRunTimeError(p->PMA, (char *)sOp[p->Index], 
							"Delay slot processing error.\n");
					}
				}

				if(p->Index == iCALL){		/* if CALL */
               		flagEffect(p->InstType, 0, 0, 0, 0);
				}
			}
			break;
		case t19a:
			{
				/* [IF COND] CALL/JUMP (IREG) */
				/* [IF COND] CALL/JUMP (Opr0) */
				if(ifCond(p->Cond)){
					/* read Opr0 addr */
					fs0 = getIntField(p, fDMI);
					ridx0 = getRegIndex(p, fs0, eIREG);
					int tAddr0 = RdReg(p, ridx0);

					if(p->Index == iCALL){		/* if CALL */
						if(!DelaySlotMode){		/* if delay slot not enabled */
							stackPush(&PCStack, p->PMA +1);	/* return addr */
						}else{					/* if delay slot enabled */
							stackPush(&PCStack, p->PMA +2);	/* return addr */
						}
					}
					NextCode = sBCodeListSearch(&bCode, tAddr0);
					isBranchTaken = TRUE;

					if(DelaySlotMode){
						sBCode *pn = getNextCode(p);
						if(pn->isDelaySlot){
							pn->BrTarget = NextCode;	/* save branch target address in delay slot */
						}else{		/* error */
							printRunTimeError(p->PMA, (char *)sOp[p->Index], 
								"Delay slot processing error.\n");
						}
					}

					if(p->Index == iCALL){		/* if CALL */
                		flagEffect(p->InstType, 0, 0, 0, 0);
					}
				}
			}
			break;
		case t20a:
			{
				/* [IF COND] RTI/RTS */
				if(ifCond(p->Cond)){
					if(p->Index == iRTS){	/* RTS */
						NextCode = sBCodeListSearch(&bCode, stackTop(&PCStack));
						stackPop(&PCStack);
						isBranchTaken = TRUE;

						if(DelaySlotMode){
							sBCode *pn = getNextCode(p);
							if(pn->isDelaySlot){
								pn->BrTarget = NextCode;	/* save branch target address in delay slot */
							}else{		/* error */
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Delay slot processing error.\n");
							}
						}

                		flagEffect(p->InstType, 0, 0, 0, 0);
					} else if(p->Index == iRTI){	/* RTI */
						NextCode = sBCodeListSearch(&bCode, stackTop(&PCStack));
						stackPop(&PCStack);
						isBranchTaken = TRUE;

						if(DelaySlotMode){
							sBCode *pn = getNextCode(p);
							if(pn->isDelaySlot){
								pn->BrTarget = NextCode;	/* save branch target address in delay slot */
							}else{		/* error */
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Delay slot processing error.\n");
							}
						}

                		flagEffect(p->InstType, 0, 0, 0, 0);
					}	
				}
			}
			break;
		case t26a:
			{
				/* PUSH/POP |PC, LOOP| [, STS] */

				int fpp[3];
				/* process Loop Stack push/pop */
				fpp[0] = 0x3 & getIntField(p, fLPP);
				if(fpp[0] == 0x1){			/* push enabled */
					stackPush(&LoopBeginStack, RdReg(p, ePCSTACK));
                    stackPush(&LoopEndStack, RdReg(p, eLPSTACK));
                    stackPush(&LoopCounterStack, RdReg(p, eCNTR));
                    stackPush(&LPEVERStack, RdReg(p, eLPEVER));
				}else if(fpp[0] == 0x3){	/* pop enabled */
					WrReg(p, ePCSTACK, stackTop(&LoopBeginStack));
                    stackPop(&LoopBeginStack);
                    WrReg(p, eLPSTACK, stackTop(&LoopEndStack));
                    stackPop(&LoopEndStack);
                    WrReg(p, eCNTR, stackTop(&LoopCounterStack));
                    stackPop(&LoopCounterStack);
                    WrReg(p, eLPEVER, stackTop(&LPEVERStack));
                    stackPop(&LPEVERStack);
				}

				/* process PC Stack push/pop */
				fpp[1] = 0x3 & getIntField(p, fPPP);
				if(fpp[1] == 0x1){			/* push enabled */
					stackPush(&PCStack, RdReg(p, ePCSTACK));
				}else if(fpp[1] == 0x3){	/* pop enabled */
					WrReg(p, ePCSTACK, stackTop(&PCStack));
                    stackPop(&PCStack);
				}

				/* process Status Stack push/pop */
				fpp[2] = 0x3 & getIntField(p, fSPP);
				if(fpp[2] == 0x1){			/* push enabled */
					stackPush(&StatusStack[0], RdReg(p, eASTAT_R));
                    stackPush(&StatusStack[1], RdReg(p, eASTAT_I));
                    stackPush(&StatusStack[2], RdReg(p, eASTAT_C));
                    stackPush(&StatusStack[3], RdReg(p, eMSTAT));
				}else if(fpp[2] == 0x3){	/* pop enabled */
					WrReg(p, eASTAT_R, stackTop(&StatusStack[0]));
                    stackPop(&StatusStack[0]);
                    WrReg(p, eASTAT_I, stackTop(&StatusStack[1]));
                    stackPop(&StatusStack[1]);
                    WrReg(p, eASTAT_C, stackTop(&StatusStack[2]));
                    stackPop(&StatusStack[2]);
                    WrReg(p, eMSTAT,   stackTop(&StatusStack[3]));
                    stackPop(&StatusStack[3]);
				}
               	flagEffect(p->InstType, 0, 0, 0, 0);
			}
			break;
		case t37a:
			{
				/* SETINT/CLRINT IMM_UINT4 */
				/* SETINT/CLRINT Opr0      */

				/* read Opr0 data */
				fs0 = getIntField(p, fINTN);
				int temp1 = getIntImm(p, fs0, eIMM_UINT4);

				if(p->Index == iSETINT){
					switch(temp1){
						case 0:
							rIRPTL.EMU = TRUE;
							break;
						case 1:
							rIRPTL.PWDN = TRUE;
							break;
						case 2:
							rIRPTL.SSTEP = TRUE;
							break;
						case 3:
							rIRPTL.STACK = TRUE;
							break;
						case 4:
						case 5:
						case 6:
						case 7:
						case 8:
						case 9:
						case 10:
						case 11:
						case 12:
						case 13:
						case 14:
						case 15:
							rIRPTL.UserDef[temp1-4] = TRUE;
							break;
						default:
							break;
					}
           			flagEffect(p->InstType, 0, 0, 0, 0);
				}else if(p->Index == iCLRINT){
	                switch(temp1){
						case 0:
							rIRPTL.EMU = FALSE;
							break;
						case 1:
							rIRPTL.PWDN = FALSE;
							break;
						case 2:
							rIRPTL.SSTEP = FALSE;
							break;
						case 3:
							rIRPTL.STACK = FALSE;
							break;
						case 4:
						case 5:
						case 6:
						case 7:
						case 8:
						case 9:
						case 10:
						case 11:
						case 12:
						case 13:
						case 14:
						case 15:
							rIRPTL.UserDef[temp1-4] = FALSE;
							break;
						default:
							break;
					}
				}
			}
			break;
		case t30a:
			{
				/* NOP */
				;
			}
			break;
		case t31a:
			{
				/* IDLE */
				;
			}
			break;
		case t18a:
			{
				/* ENA/DIS |SEC_REG, BIT_REV, AV_LATCH, AL_SAT, M_MODE, 
					TIMER, SEC_DAG, M_BIAS, INT|, 
					(… up to 9 in parallel) */
				ena = getIntField(p, fENA);
				for(int k = 0; k < 9; k++){
					code[k] = ena & 0x3;
					ena >>= 2;
				}

				if(code[8] == 0x3){			/* SR enable */
					rMstat.SR = TRUE;
				}else if(code[8] == 0x1){	/* SR disable */
					rMstat.SR = FALSE;
				}

				if(code[7] == 0x3){			/* BR enable */
					rMstat.BR = TRUE;
				}else if(code[7] == 0x1){	/* BR disable */
					rMstat.BR = FALSE;
				}

				if(code[6] == 0x3){			/* OL enable */
					rMstat.OL = TRUE;
				}else if(code[6] == 0x1){	/* OL disable */
					rMstat.OL = FALSE;
				}

				if(code[5] == 0x3){			/* AS enable */
					rMstat.AS = TRUE;
				}else if(code[5] == 0x1){	/* AS disable */
					rMstat.AS = FALSE;
				}

				if(code[4] == 0x3){			/* MM enable */
					rMstat.MM = TRUE;
				}else if(code[4] == 0x1){	/* MM disable */
					rMstat.MM = FALSE;
				}

				if(code[3] == 0x3){			/* TI enable */
					rMstat.TI = TRUE;
				}else if(code[3] == 0x1){	/* TI disable */
					rMstat.TI = FALSE;
				}

				if(code[2] == 0x3){			/* SD enable */
					rMstat.SD = TRUE;
				}else if(code[2] == 0x1){	/* SD disable */
					rMstat.SD = FALSE;
				}

				if(code[1] == 0x3){			/* MB enable */
					rMstat.MB = TRUE;
				}else if(code[1] == 0x1){	/* MB disable */
					rMstat.MB = FALSE;
				}

				if(code[0] == 0x3){			/* INT enable */
					rICNTL.GIE = TRUE;
				}else if(code[0] == 0x1){	/* INT disable */
					rICNTL.GIE = FALSE;
				}
			}
			break;

		/* Complex Arithmetic Instructions */
		case t42a:
			{
				/* POLAR.C/RECT.C DREG24, XOP24[*] */
				/* POLAR.C/RECT.C Opr0  , Opr1 [*] */

				/* read Opr1 data */
				fs1 = getIntField(p, fSRCC);
				ridx1 = getRegIndex(p, fs1, eXOP24);
				cplx ct1 = cRdReg(p, ridx1);

				if(p->Conj) ct1.i = -ct1.i; /* CONJ(*) modifier */

				/* get Opr0 addr */
				fd = getIntField(p, fDSTC);
				ridx0 = getRegIndex(p, fd, eDREG24);

				processCordicFunc(p, ct1, ridx0);
			}
			break;
		case t42b:
			{
				/* CONJ.C DREG24, XOP24 */
				/* CONJ.C Opr0  , Opr1  */

				/* read Opr1 data */
				fs1 = getIntField(p, fSRCC);
				ridx1 = getRegIndex(p, fs1, eXOP24);
				cplx ct1 = cRdReg(p, ridx1);

				/* get Opr0 addr */
				fd = getIntField(p, fDSTC);
				ridx0 = getRegIndex(p, fd, eDREG24);

				ct1.i = -ct1.i; 

				cWrReg(p, ridx0, ct1.r, ct1.i);
			}
			break;

		/* Unknown Instructions */
		default:
				printRunTimeError(p->PMA, (char *)sOp[p->Index], 
					"Invalid inst. index found while processing in binSimOneStep().\n");
			break;
    }

	if(stackEmpty(&LoopBeginStack)){        /* no loop running */
		if(!DelaySlotMode){			/* if delay slot disabled */
			if(isBranchTaken){			/* if branch taken */
				isBranchTaken = FALSE;	/* reset flag      */
			}else{
				NextCode = p->Next;
			}
		}else{						/* if delay slot enabled */
			if(isBranchTaken){			/* if branch taken */
				isBranchTaken = FALSE;	/* reset flag      */
				NextCode = p->Next;
			}else if(p->isDelaySlot){	/* if delay slot */
				NextCode = p->BrTarget;
			}else{						/* neither branch nor delay slot */
				NextCode = p->Next;
			}
		}
	}else{
		if(!DelaySlotMode){			/* if delay slot disabled */
			if(p->PMA == stackTop(&LoopEndStack)){   /* if end of the loop */
				if(isBranchTaken){          /* if branch taken */
					isBranchTaken = FALSE;  /* reset flag      */
				} else if(RdReg(p, eLPEVER))  {   /* DO UNTIL FOREVER */
					loopCntr = stackTop(&LoopCounterStack);
					stackPop(&LoopCounterStack);
					stackPush(&LoopCounterStack, loopCntr-1);
					NextCode = sBCodeListSearch(&bCode, (unsigned int)stackTop(&LoopBeginStack));
				}else{          /* DO UNTIL CE */
					loopCntr = stackTop(&LoopCounterStack);
					if(loopCntr > 1){ /* decrement counter */
						stackPop(&LoopCounterStack);
						stackPush(&LoopCounterStack, loopCntr-1);
						NextCode = sBCodeListSearch(&bCode, (unsigned int)stackTop(&LoopBeginStack));
					}else{  /* exit loop */
						stackPop(&LoopBeginStack);
						WrReg(p, ePCSTACK, stackTop(&LoopBeginStack));
						stackPop(&LoopEndStack);
						WrReg(p, eLPSTACK, stackTop(&LoopEndStack));
						stackPop(&LoopCounterStack);
						WrReg(p, eCNTR, stackTop(&LoopCounterStack));
						stackPop(&LPEVERStack);
						WrReg(p, eLPEVER, stackTop(&LPEVERStack));

           		     	flagEffect(t11a, 0, 0, 0, 0);
						NextCode = p->Next;
					}
				}
			} else {    /* within the loop */
				if(isBranchTaken){          /* if branch taken */
					isBranchTaken = FALSE;  /* reset flag      */
				}else{
					NextCode = p->Next;
				}
			}
		}else{						/* if delay slot enabled */
			if(p->PMA == stackTop(&LoopEndStack)){   /* if end of the loop */
				if(isBranchTaken){          /* if branch taken */
					isBranchTaken = FALSE;  /* reset flag      */

					/* error */
					printRunTimeError(p->PMA, (char *)sOp[p->Index], 
						"This case should not happen - Branch at the end of do-until block.\n");

				} else if(RdReg(p, eLPEVER))  {   /* DO UNTIL FOREVER */
					loopCntr = stackTop(&LoopCounterStack);
					stackPop(&LoopCounterStack);
					stackPush(&LoopCounterStack, loopCntr-1);
					NextCode = sBCodeListSearch(&bCode, (unsigned int)stackTop(&LoopBeginStack));
				}else{          /* DO UNTIL CE */
					loopCntr = stackTop(&LoopCounterStack);
					if(loopCntr > 1){ /* decrement counter */
						stackPop(&LoopCounterStack);
						stackPush(&LoopCounterStack, loopCntr-1);
						NextCode = sBCodeListSearch(&bCode, (unsigned int)stackTop(&LoopBeginStack));
					}else{  /* exit loop */
						stackPop(&LoopBeginStack);
						WrReg(p, ePCSTACK, stackTop(&LoopBeginStack));
						stackPop(&LoopEndStack);
						WrReg(p, eLPSTACK, stackTop(&LoopEndStack));
						stackPop(&LoopCounterStack);
						WrReg(p, eCNTR, stackTop(&LoopCounterStack));
						stackPop(&LPEVERStack);
						WrReg(p, eLPEVER, stackTop(&LPEVERStack));

           		     	flagEffect(t11a, 0, 0, 0, 0);
						NextCode = p->Next;
					}
				}
			} else {    /* within the loop */
				if(isBranchTaken){          /* if branch taken */
					isBranchTaken = FALSE;  /* reset flag      */
					NextCode = p->Next;
				}else if(p->isDelaySlot){	/* if delay slot */
					NextCode = p->BrTarget;
				}else{						/* neither branch nor delay slot */
					NextCode = p->Next;
				}
			}
		}
	}

	NextCode = updatePC(p, NextCode);
	return (NextCode);
}


/** 
* @brief Simulate one binary multifunction instruction word
* 
* @param p Pointer to current instruction
* 
* @return Pointer to current instruction
*/
sBCode *binSimOneStepMultiFunc(sBCode *p)
{
	switch(p->MultiCounter){
		case	1:				/* two-opcode multifunction */
			{
				switch(p->InstType){	
					case t01a:
						{
							/* LD || LD */
							/* LD XOP12S, DM(IX    +/+= MX   ) || LD YOP12S, DM(IY    +/+= MY   ) */
							/* LD Opr10 , DM(Opr11 +/+= Opr12) || LD Opr20 , DM(Opr21 +/+= Opr22) */

							/* read first LD operands */
							/* LD XOP12S, DM(IX    +/+= MX   ) */
							/* LD Opr10 , DM(Opr11 +/+= Opr12) */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDD);
							int ridx10 = getRegIndex(p, fd10, eXOP12S);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMI);
							int ridx11 = getRegIndex(p, fs11, eIX);
							int tAddr11 = RdReg2(p, ridx11);

							/* read Opr12 addr */
							int fs12 = getIntField(p, fDMM);
							int ridx12 = getRegIndex(p, fs12, eMX);
							int tAddr12 = RdReg2(p, ridx12);

							/* check if postmodify */
							int isPost1 = FALSE;
							if(getIntField(p, fU)) isPost1 = TRUE;

							/* if premodify */
							if(!isPost1) tAddr11 += tAddr12;

							/* read Opr11 data */
							int tData11 = RdDataMem(p, tAddr11);

							/* read second LD operands */
							/* LD YOP12S, DM(IY    +/+= MY   ) */
							/* LD Opr20 , DM(Opr21 +/+= Opr22) */

							/* get Opr20 addr */
							int fd20 = getIntField(p, fPD);
							int ridx20 = getRegIndex(p, fd20, eYOP12S);

							/* read Opr21 addr */
							int fs21 = getIntField(p, fPMI);
							int ridx21 = getRegIndex(p, fs21, eIY);
							int tAddr21 = RdReg2(p, ridx21);

							/* read Opr22 addr */
							int fs22 = getIntField(p, fPMM);
							int ridx22 = getRegIndex(p, fs22, eMY);
							int tAddr22 = RdReg2(p, ridx22);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU2)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr21 += tAddr22;

							/* read Opr21 data */
							int tData21 = RdDataMem(p, tAddr21);

#ifdef VHPI
							if(VhpiMode){	/* rh+rh: read 12b+12b */
								dsp_rh_rh(tAddr11, (short int *)&tData11, tAddr21, (short int *)&tData21, 1);
								dsp_wait(1);
							}
#endif

							/* write first LD result */
							WrReg(p, ridx10, tData11);

							/* write second LD result */
							WrReg(p, ridx20, tData21);

							/* postmodify: update Ix */
							if(isPost1){
								tAddr11 += tAddr12;
								updateIReg(p, ridx11, tAddr11);
							}

							if(isPost2){
								tAddr21 += tAddr22;
								updateIReg(p, ridx21, tAddr21);
							}
						}	/* end of t01a */
						break;
					case t01b:
						{
							/* type 1b */
							p->InstType = t01b;
							/* LD.C || LD.C */
							/* LD.C XOP24S, DM(IX+/+= MX) || LD.C YOP24S, DM(IY+/+=MY) */
							/* LD.C Opr10, DM(Opr11+/+=Opr12) || LD.C Opr20, DM(Opr21+/+=Opr22) */

							/* read first LD.C operands */
							/* LD.C XOP24S, DM(IX    +/+= MX   ) */
							/* LD.C Opr10 , DM(Opr11 +/+= Opr12) */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDD);
							int ridx10 = getRegIndex(p, fd10, eXOP24S);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMI);
							int ridx11 = getRegIndex(p, fs11, eIX);
							int tAddr11 = RdReg2(p, ridx11);

							if(tAddr11 & 0x01){       //if not aligned
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Complex memory read/write must be aligned to even-word boundaries.\n");
								break;
							}

							/* read Opr12 addr */
							int fs12 = getIntField(p, fDMM);
							int ridx12 = getRegIndex(p, fs12, eMX);
							int tAddr12 = RdReg2(p, ridx12);

							/* check if postmodify */
							int isPost1 = FALSE;
							if(getIntField(p, fU)) isPost1 = TRUE;

							/* if premodify */
							if(!isPost1) tAddr11 += tAddr12;

							/* read Opr11 data */
							int tData111 = RdDataMem(p, tAddr11);
							int tData112 = RdDataMem(p, tAddr11+1);

							/* read second LD.C operands */
							/* LD.C YOP24S, DM(IY    +/+= MY   ) */
							/* LD.C Opr20 , DM(Opr21 +/+= Opr22) */

							/* get Opr20 addr */
							int fd20 = getIntField(p, fPD);
							int ridx20 = getRegIndex(p, fd20, eYOP24S);

							/* read Opr21 addr */
							int fs21 = getIntField(p, fPMI);
							int ridx21 = getRegIndex(p, fs21, eIY);
							int tAddr21 = RdReg2(p, ridx21);

							if(tAddr21 & 0x01){       //if not aligned
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Complex memory read/write must be aligned to even-word boundaries.\n");
								break;
							}

							/* read Opr22 addr */
							int fs22 = getIntField(p, fPMM);
							int ridx22 = getRegIndex(p, fs22, eMY);
							int tAddr22 = RdReg2(p, ridx22);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU2)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr21 += tAddr22;

							/* read Opr21 data */
							int tData211 = RdDataMem(p, tAddr21);
							int tData212 = RdDataMem(p, tAddr21+1);

#ifdef VHPI
							if(VhpiMode){	/* rw+rw: read 24b+24b */
								int tData1, tData2;

								dsp_rw_rw(tAddr11, &tData1, tAddr21, &tData2, 1);
								dsp_wait(1);

								/* Note:
								tData1 = (((0x0FFF & tData111) << 12) | (0x0FFF & tData112));	
								tData2 = (((0x0FFF & tData211) << 12) | (0x0FFF & tData212));	
								*/
								tData111 = (0x0FFF & (tData1 >> 12));
								tData112 = 0xFFF & tData1;
								tData211 = (0x0FFF & (tData2 >> 12));
								tData212 = 0xFFF & tData2;
							}
#endif

							/* write first LD.C result */
							cWrReg(p, ridx10, tData111, tData112);

							/* write second LD.C result */
							cWrReg(p, ridx20, tData211, tData212);

							/* postmodify: update Ix */
							if(isPost1){
								tAddr11 += tAddr12;
								updateIReg(p, ridx11, tAddr11);
							}

							if(isPost2){
								tAddr21 += tAddr22;
								updateIReg(p, ridx21, tAddr21);
							}
						}	/* end of t01b */
						break;
					case t04a:
						{
							/* type 4a */
							p->InstType = t04a;
							/* MAC || LD */
							/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) || LD DREG12, DM(IREG  +/+= MREG ) */
							/* MAC Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) || LD Opr10 , DM(Opr11 +/+= Opr12) */

							/* MAC operation */
							/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) */
							/* MAC Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) */

							/* read multiplication option: RND, SS, SU, US, UU */
							int opt = getOptionFromMF(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCM1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCM2);
							int ridx02 = getRegIndex(p, fs02, eYOP12S);
							int tData02 = RdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTM);
							int ridx00 = getRegIndex(p, fd, eACC32S);

							/* read LD operands */
							/* LD DREG12, DM(IREG  +/+= MREG ) */
							/* LD Opr10 , DM(Opr11 +/+= Opr12) */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG12);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMI);
							int ridx11 = getRegIndex(p, fs11, eIREG);
							int tAddr11 = RdReg2(p, ridx11);

							/* read Opr12 addr */
							int fs12 = getIntField(p, fDMM);
							int ridx12 = getRegIndex(p, fs12, eMREG);
							int tAddr12 = RdReg2(p, ridx12);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr11 += tAddr12;

							/* read Opr11 data */
							int tData11 = RdDataMem(p, tAddr11);

#ifdef VHPI
							if(VhpiMode){	/* rh: read 12b */
								dsp_rh(tAddr11, (short int *)&tData11, 1);
								dsp_wait(1);
							}
#endif

							/* compute MAC operation */
							processMACFunc(p, tData01, tData02, ridx00, opt, 0);

							/* write LD result */
							WrReg(p, ridx10, tData11);

							/* postmodify: update Ix */
							if(isPost2){
								tAddr11 += tAddr12;
								updateIReg(p, ridx11, tAddr11);
							}
						}	/* end of t04a */
						break;
					case t04c:
						{
							/* type 4c */
							p->InstType = t04c;
							/* ALU || LD */
							/* ALU DREG12S, XOP12S, YOP12S || LD DREG12, DM(IREG  +/+= MREG ) */
							/* ALU Opr00  , Opr01 , Opr02  || LD Opr10 , DM(Opr11 +/+= Opr12) */

							/* ALU operation */
							/* ALU DREG12S, XOP12S, YOP12S */
							/* ALU Opr00  , Opr01 , Opr02  */

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCA1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);
							int tData02;

							int ridx00;

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCA2);
							int ridx02 = getRegIndex(p, fs02, eYOP12S);
							tData02 = RdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTA);
							ridx00 = getRegIndex(p, fd, eDREG12S);

							/* read first LD operands */
							/* LD DREG12, DM(IREG  +/+= MREG ) */
							/* LD Opr10 , DM(Opr11 +/+= Opr12) */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG12);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMI);
							int ridx11 = getRegIndex(p, fs11, eIREG);
							int tAddr11 = RdReg2(p, ridx11);

							/* read Opr12 addr */
							int fs12 = getIntField(p, fDMM);
							int ridx12 = getRegIndex(p, fs12, eMREG);
							int tAddr12 = RdReg2(p, ridx12);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr11 += tAddr12;

							/* read Opr11 data */
							int tData11 = RdDataMem(p, tAddr11);

#ifdef VHPI
							if(VhpiMode){	/* rh: read 12b */
								dsp_rh(tAddr11, (short int *)&tData11, 1);
								dsp_wait(1);
							}
#endif

							/* compute ALU operation */
							processALUFunc(p, tData01, tData02, ridx00);

							/* write LD result */
							WrReg(p, ridx10, tData11);

							/* postmodify: update Ix */
							if(isPost2){
								tAddr11 += tAddr12;
								updateIReg(p, ridx11, tAddr11);
							}
						}	/* end of t04c */
						break;
					case t04b:
						{
							/* type 4b */
							p->InstType = t04b;
							/* MAC.C || LD.C */
							/* MAC.C ACC64S, XOP24S, YOP24S (|RND,SS,SU,US,UU|) || LD.C DREG24, DM(IREG  +/+= MREG ) */
							/* MAC.C Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) || LD.C Opr10 , DM(Opr11 +/+= Opr12) */

							/* MAC.C operation */
							/* MAC.C ACC64S, XOP24S, YOP24S (|RND,SS,SU,US,UU|) */
							/* MAC.C Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) */

							/* read multiplication option: RND, SS, SU, US, UU */
							int opt = getOptionFromMF(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCM1);
							int ridx01 = getRegIndex(p, fs01, eXOP24S);
							cplx ct1 = cRdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCM2);
							int ridx02 = getRegIndex(p, fs02, eYOP24S);
							cplx ct2 = cRdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTM);
							int ridx00 = getRegIndex(p, fd, eACC64S);

							/* read LD.C operands */
							/* LD.C DREG24, DM(IREG  +/+= MREG ) */
							/* LD.C Opr10 , DM(Opr11 +/+= Opr12) */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG24);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMI);
							int ridx11 = getRegIndex(p, fs11, eIREG);
							int tAddr11 = RdReg2(p, ridx11);

							if(tAddr11 & 0x01){       //if not aligned
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Complex memory read/write must be aligned to even-word boundaries.\n");
								break;
							}

							/* read Opr12 addr */
							int fs12 = getIntField(p, fDMM);
							int ridx12 = getRegIndex(p, fs12, eMREG);
							int tAddr12 = RdReg2(p, ridx12);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr11 += tAddr12;

							/* read Opr11 data */
							int tData111 = RdDataMem(p, tAddr11);
							int tData112 = RdDataMem(p, tAddr11+1);

#ifdef VHPI
							if(VhpiMode){	/* rw: read 24b */
								int tData1;

								dsp_rw(tAddr11, &tData1, 1);
								dsp_wait(1);

								/* Note: tData1 = (((0x0FFF & tData111) << 12) | (0x0FFF & tData112)); */
								tData111 = (0x0FFF & (tData1 >> 12));
								tData112 = 0xFFF & tData1;
							}
#endif

							/* write MAC result */
							processMAC_CFunc(p, ct1, ct2, ridx00, opt, 0);

							/* write LD result */
							cWrReg(p, ridx10, tData111, tData112);

							/* postmodify: update Ix */
							if(isPost2){
								tAddr11 += tAddr12;
								updateIReg(p, ridx11, tAddr11);
							}
						}	/* end of t04b */
						break;
					case t04d:
						{
							/* type 4d */
							p->InstType = t04d;
							/* ALU.C || LD.C */
							/* ALU.C DREG24S, XOP24S, YOP24S || LD.C DREG24, DM(IREG  +/+= MREG ) */
							/* ALU.C Opr00  , Opr01 , Opr02  || LD.C Opr10 , DM(Opr11 +/+= Opr12) */

							/* ALU.C operation */
							/* ALU.C DREG24S, XOP24S, YOP24S */
							/* ALU.C Opr00  , Opr01 , Opr02  */

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCA1);
							int ridx01 = getRegIndex(p, fs01, eXOP24S);
							cplx ct1 = cRdReg(p, ridx01);
							cplx ct2;

							int ridx00;

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCA2);
							int ridx02 = getRegIndex(p, fs02, eYOP24S);
							ct2 = cRdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTA);
							ridx00 = getRegIndex(p, fd, eDREG24S);

							/* read LD.C operands */
							/* LD.C DREG24, DM(IREG  +/+= MREG ) */
							/* LD.C Opr10 , DM(Opr11 +/+= Opr12) */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG24);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMI);
							int ridx11 = getRegIndex(p, fs11, eIREG);
							int tAddr11 = RdReg2(p, ridx11);

							if(tAddr11 & 0x01){       //if not aligned
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Complex memory read/write must be aligned to even-word boundaries.\n");
								break;
							}

							/* read Opr12 addr */
							int fs12 = getIntField(p, fDMM);
							int ridx12 = getRegIndex(p, fs12, eMREG);
							int tAddr12 = RdReg2(p, ridx12);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr11 += tAddr12;

							/* read Opr11 data */
							int tData111 = RdDataMem(p, tAddr11);
							int tData112 = RdDataMem(p, tAddr11+1);

#ifdef VHPI
							if(VhpiMode){	/* rw: read 24b */
								int tData1;

								dsp_rw(tAddr11, &tData1, 1);
								dsp_wait(1);

								/* Note: tData1 = (((0x0FFF & tData111) << 12) | (0x0FFF & tData112)); */
								tData111 = (0x0FFF & (tData1 >> 12));
								tData112 = 0xFFF & tData1;
							}
#endif

							/* compute ALU.C operation */
							processALU_CFunc(p, ct1, ct2, ridx00);

							/* write LD result */
							cWrReg(p, ridx10, tData111, tData112);

							/* postmodify: update Ix */
							if(isPost2){
								tAddr11 += tAddr12;
								updateIReg(p, ridx11, tAddr11);
							}
						}	/* end of t04d */
						break;
					case t04e:
						{
							/* type 4e */
							p->InstType = t04e;
							/* MAC || ST */
							/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) || ST DM(IREG  +/+= MREG ), DREG12 */
							/* MAC Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) || ST DM(Opr10 +/+= Opr11), Opr12  */

							/* MAC operation */
							/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) */
							/* MAC Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) */

							/* read multiplication option: RND, SS, SU, US, UU */
							int opt = getOptionFromMF(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCM1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCM2);
							int ridx02 = getRegIndex(p, fs02, eYOP12S);
							int tData02 = RdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTM);
							int ridx00 = getRegIndex(p, fd, eACC32S);

							/* ST DM(IREG  +/+= MREG ), DREG12 */
							/* ST DM(Opr10 +/+= Opr11), Opr12  */
	
							/* read Opr10 addr */
							int fs10 = getIntField(p, fDMI);
							int ridx10 = getRegIndex(p, fs10, eIREG);
							int tAddr10 = RdReg2(p, ridx10);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMM);
							int ridx11 = getRegIndex(p, fs11, eMREG);
							int tAddr11 = RdReg2(p, ridx11);

							/* get Opr12 addr */
							int fs12 = getIntField(p, fSRCLS);
							int ridx12 = getRegIndex(p, fs12, eDREG12);
							int tData12 = RdReg(p, ridx12);

							/* compute MAC operation */
							processMACFunc(p, tData01, tData02, ridx00, opt, 0);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr10 += tAddr11;

							/* write ST result */
							WrDataMem(p, tData12, tAddr10);

#ifdef VHPI
							if(VhpiMode){   /* wh: write 12b */
								dsp_wh(tAddr10, (short int *)&tData12, 1);
							}
#endif

							/* postmodify: update Ix */
							if(isPost2){
								tAddr10 += tAddr11;
								updateIReg(p, ridx10, tAddr10);
							}
						}	/* end of t04e */
						break;
					case t04g:
						{
							/* type 4g */
							p->InstType = t04g;
							/* ALU || ST */
							/* ALU DREG12S, XOP12S, YOP12S || ST DM(IREG  +/+= MREG),  DREG12 */
							/* ALU Opr00  , Opr01 , Opr02  || ST DM(Opr10 +/+= Opr11), Opr12  */

							/* ALU operation */
							/* ALU DREG12S, XOP12S, YOP12S */
							/* ALU Opr00  , Opr01 , Opr02  */

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCA1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);
							int tData02;

							int ridx00;

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCA2);
							int ridx02 = getRegIndex(p, fs02, eYOP12S);
							tData02 = RdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTA);
							ridx00 = getRegIndex(p, fd, eDREG12S);

							/* ST DM(IREG  +/+= MREG ), DREG12 */
							/* ST DM(Opr10 +/+= Opr11), Opr12  */
	
							/* read Opr10 addr */
							int fs10 = getIntField(p, fDMI);
							int ridx10 = getRegIndex(p, fs10, eIREG);
							int tAddr10 = RdReg2(p, ridx10);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMM);
							int ridx11 = getRegIndex(p, fs11, eMREG);
							int tAddr11 = RdReg2(p, ridx11);

							/* get Opr12 addr */
							int fs12 = getIntField(p, fSRCLS);
							int ridx12 = getRegIndex(p, fs12, eDREG12);
							int tData12 = RdReg(p, ridx12);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr10 += tAddr11;

							/* write ST result */
							WrDataMem(p, tData12, tAddr10);

#ifdef VHPI
							if(VhpiMode){	/* wh: write 12b */
								dsp_wh(tAddr10, (short int *)&tData12, 1);
							}
#endif
							/* compute ALU operation */
							processALUFunc(p, tData01, tData02, ridx00);

							/* postmodify: update Ix */
							if(isPost2){
								tAddr10 += tAddr11;
								updateIReg(p, ridx10, tAddr10);
							}
						}	/* end of t04g */
						break;
					case t04f:
						{
							/* type 4f */
							p->InstType = t04f;
							/* MAC.C || ST.C */
							/* MAC.C ACC64S, XOP24S, YOP24S (|RND,SS,SU,US,UU|) || ST.C DM(IREG  +/+= MREG ), DREG24 */
							/* MAC.C Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) || ST.C DM(Opr10 +/+= Opr11), Opr12  */

							/* MAC.C operation */
							/* MAC.C ACC64S, XOP24S, YOP24S (|RND,SS,SU,US,UU|) */
							/* MAC.C Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) */

							/* read multiplication option: RND, SS, SU, US, UU */
							int opt = getOptionFromMF(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCM1);
							int ridx01 = getRegIndex(p, fs01, eXOP24S);
							cplx ct1 = cRdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCM2);
							int ridx02 = getRegIndex(p, fs02, eYOP24S);
							cplx ct2 = cRdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTM);
							int ridx00 = getRegIndex(p, fd, eACC64S);

							/* ST.C DM(IREG  +/+= MREG ), DREG24 */
							/* ST.C DM(Opr10 +/+= Opr11), Opr12  */

							/* read Opr10 addr */
							int fs10 = getIntField(p, fDMI);
							int ridx10 = getRegIndex(p, fs10, eIREG);
							int tAddr10 = RdReg2(p, ridx10);

							if(tAddr10 & 0x01){       //if not aligned
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Complex memory read/write must be aligned to even-word boundaries.\n");
								break;
							}

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMM);
							int ridx11 = getRegIndex(p, fs11, eMREG);
							int tAddr11 = RdReg2(p, ridx11);

							/* get Opr12 addr */
							int fs12 = getIntField(p, fSRCLS);
							int ridx12 = getRegIndex(p, fs12, eDREG24);
							cplx cData1 = cRdReg(p, ridx12);

							/* write MAC result */
							processMAC_CFunc(p, ct1, ct2, ridx00, opt, 0);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr10 += tAddr11;

							/* write ST result */
							WrDataMem(p, cData1.r, tAddr10);
							WrDataMem(p, cData1.i, tAddr10+1);

#ifdef VHPI
							if(VhpiMode){	/* ww: write 24b */
								int tData1 = (((0x0FFF & cData1.r) << 12) | (0x0FFF & cData1.i));	
								dsp_ww(tAddr10, &tData1, 1);
							}
#endif

							/* postmodify: update Ix */
							if(isPost2){
								tAddr10 += tAddr11;
								updateIReg(p, ridx10, tAddr10);
							}
						}	/* end of t04f */
						break;
					case t04h:
						{
							/* type 4h */
							p->InstType = t04h;
							/* ALU.C || ST.C */
							/* ALU.C DREG24S, XOP24S, YOP24S || ST.C DM(IREG  +/+= MREG ), DREG24 */
							/* ALU.C Opr00  , Opr01 , Opr02  || ST.C DM(Opr10 +/+= Opr11), Opr12  */

							/* ALU.C operation */
							/* ALU.C DREG24S, XOP24S, YOP24S */
							/* ALU.C Opr00  , Opr01 , Opr02  */

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCA1);
							int ridx01 = getRegIndex(p, fs01, eXOP24S);
							cplx ct1 = cRdReg(p, ridx01);
							cplx ct2;

							int ridx00;

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCA2);
							int ridx02 = getRegIndex(p, fs02, eYOP24S);
							ct2 = cRdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTA);
							ridx00 = getRegIndex(p, fd, eDREG24S);

							/* ST.C DM(IREG  +/+= MREG ), DREG24 */
							/* ST.C DM(Opr10 +/+= Opr11), Opr12  */

							/* read Opr10 addr */
							int fs10 = getIntField(p, fDMI);
							int ridx10 = getRegIndex(p, fs10, eIREG);
							int tAddr10 = RdReg2(p, ridx10);

							if(tAddr10 & 0x01){       //if not aligned
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Complex memory read/write must be aligned to even-word boundaries.\n");
								break;
							}

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMM);
							int ridx11 = getRegIndex(p, fs11, eMREG);
							int tAddr11 = RdReg2(p, ridx11);

							/* get Opr12 addr */
							int fs12 = getIntField(p, fSRCLS);
							int ridx12 = getRegIndex(p, fs12, eDREG24);
							cplx cData1 = cRdReg(p, ridx12);

							/* compute ALU operation */
							processALU_CFunc(p, ct1, ct2, ridx00);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr10 += tAddr11;

							/* write ST result */
							WrDataMem(p, cData1.r, tAddr10);
							WrDataMem(p, cData1.i, tAddr10+1);

#ifdef VHPI
							if(VhpiMode){	/* ww: write 24b */
								int tData1 = (((0x0FFF & cData1.r) << 12) | (0x0FFF & cData1.i));	
								dsp_ww(tAddr10, &tData1, 1);
							}
#endif

							/* postmodify: update Ix */
							if(isPost2){
								tAddr10 += tAddr11;
								updateIReg(p, ridx10, tAddr10);
							}
						}	/* end of t04h */
						break;
					case t08a:
						{
							/* type 8a */
							p->InstType = t08a;
							/* MAC || CP */
							/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) || CP DREG12, DREG12 */
							/* MAC Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) || CP Opr10 , Opr11  */

							/* MAC operation */
							/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) */
							/* MAC Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) */

							/* read multiplication option: RND, SS, SU, US, UU */
							int opt = getOptionFromMF(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCM1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCM2);
							int ridx02 = getRegIndex(p, fs02, eYOP12S);
							int tData02 = RdReg(p, ridx02);

							/* get Opr3 addr */
							int fd00 = getIntField(p, fDSTM);
							int ridx00 = getRegIndex(p, fd00, eACC32S);

							/* CP DREG12, DREG12 */
							/* CP Opr10 , Opr11  */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG12);

							/* read Opr11 data */
							int fs11 = getIntField(p, fSRCLS);
							int ridx11 = getRegIndex(p, fs11, eDREG12);
							int tData11 = RdReg(p, ridx11);

							/* compute MAC operation */
							processMACFunc(p, tData01, tData02, ridx00, opt, 0);

							/* write CP result */
							/* write to Opr10 */
							WrReg(p, ridx10, tData11);
						}	/* end of t08a */
						break;
					case t08c:
						{
							/* type 8c */
							p->InstType = t08c;
							/* ALU || CP */
							/* ALU DREG12S, XOP12S, YOP12S || CP DREG12, DREG12 */
							/* ALU Opr00  , Opr01 , Opr02  || CP Opr10 , Opr11  */

							/* ALU operation */
							/* ALU DREG12S, XOP12S, YOP12S */
							/* ALU Opr00  , Opr01 , Opr02  */

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCA1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);
							int tData02;

							int ridx00;

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCA2);
							int ridx02 = getRegIndex(p, fs02, eYOP12S);
							tData02 = RdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTA);
							ridx00 = getRegIndex(p, fd, eDREG12S);

							/* CP DREG12, DREG12 */
							/* CP Opr10 , Opr11  */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG12);

							/* read Opr11 data */
							int fs11 = getIntField(p, fSRCLS);
							int ridx11 = getRegIndex(p, fs11, eDREG12);
							int tData11 = RdReg(p, ridx11);

							/* compute ALU operation */
							processALUFunc(p, tData01, tData02, ridx00);

							/* write CP result */
							/* write to Opr10 */
							WrReg(p, ridx10, tData11);
						}	/* end of t08c */
						break;
					case t08b:
						{
							/* type 8b */
							p->InstType = t08b;
							/* MAC.C || CP.C */
							/* MAC.C ACC64S, XOP24S, YOP24S (|RND,SS,SU,US,UU|) || CP.C DREG24, DREG24 */
							/* MAC.C Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) || CP.C Opr10 , Opr11  */

							/* MAC.C operation */
							/* MAC.C ACC64S, XOP24S, YOP24S (|RND,SS,SU,US,UU|) */
							/* MAC.C Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) */

							/* read multiplication option: RND, SS, SU, US, UU */
							int opt = getOptionFromMF(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCM1);
							int ridx01 = getRegIndex(p, fs01, eXOP24S);
							cplx ct1 = cRdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCM2);
							int ridx02 = getRegIndex(p, fs02, eYOP24S);
							cplx ct2 = cRdReg(p, ridx02);

							/* get Opr3 addr */
							int fd00 = getIntField(p, fDSTM);
							int ridx00 = getRegIndex(p, fd00, eACC64S);

							/* CP.C DREG24, DREG24 */
							/* CP.C Opr10  , Opr11   */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG24);
			
							/* read Opr11 data */
							int fs11 = getIntField(p, fSRCLS);
							int ridx11 = getRegIndex(p, fs11, eDREG24);
							cplx cData1 = cRdReg(p, ridx11);

							/* write MAC result */
							processMAC_CFunc(p, ct1, ct2, ridx00, opt, 0);

							/* write CP.C result */
							/* write to Opr10 */
							cWrReg(p, ridx10, cData1.r, cData1.i);
						}	/* end of t08b */
						break;
					case t08d:
						{
							/* type 8d */
							p->InstType = t08d;
							/* ALU.C || CP.C */
							/* ALU.C DREG24S, XOP24S, YOP24S || CP.C DREG24, DREG24 */
							/* ALU.C Opr00  , Opr01 , Opr02  || CP.C Opr10 , Opr11  */

							/* ALU.C operation */
							/* ALU.C DREG24S, XOP24S, YOP24S */
							/* ALU.C Opr00  , Opr01 , Opr02  */

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCA1);
							int ridx01 = getRegIndex(p, fs01, eXOP24S);
							cplx ct1 = cRdReg(p, ridx01);
							cplx ct2;

							int ridx00;

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCA2);
							int ridx02 = getRegIndex(p, fs02, eYOP24S);
							ct2 = cRdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTA);
							ridx00 = getRegIndex(p, fd, eDREG24S);

							/* CP.C DREG24, DREG24 */
							/* CP.C Opr10  , Opr11   */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG24);
			
							/* read Opr11 data */
							int fs11 = getIntField(p, fSRCLS);
							int ridx11 = getRegIndex(p, fs11, eDREG24);
							cplx cData1 = cRdReg(p, ridx11);

							/* compute ALU operation */
							processALU_CFunc(p, ct1, ct2, ridx00);

							/* write CP.C result */
							/* write to Opr10 */
							cWrReg(p, ridx10, cData1.r, cData1.i);
						}	/* end of t08d */
						break;
					case t12e:
						{
							/* type 12e */
							p->InstType = t12e;
							/* SHIFT || LD */
							/* SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) || LD DREG12, DM(IREG  +/+= MREG ) */
							/* SHIFT Opr00 , Opr01 , Opr02    (|HI,LO,HIRND,LORND|) || LD Opr10 , DM(Opr11 +/+= Opr12) */

							/* SHIFT operation */
							/* SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) */
							/* SHIFT Opr00 , Opr01 , Opr02    (|HI,LO,HIRND,LORND|) */

							/* read shift option: Hi,LO,HIRND,LORND */
							int opt = getOptionFromSF2(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCS1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCS2);
							int tData02 = getIntImm(p, fs02, eIMM_INT5);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTS);
							int ridx00 = getRegIndex(p, fd, eACC32S);

							/* read LD operands */
							/* LD DREG12, DM(IREG  +/+= MREG ) */
							/* LD Opr10 , DM(Opr11 +/+= Opr12) */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG12);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMI);
							int ridx11 = getRegIndex(p, fs11, eIREG);
							int tAddr11 = RdReg2(p, ridx11);

							/* read Opr12 addr */
							int fs12 = getIntField(p, fDMM);
							int ridx12 = getRegIndex(p, fs12, eMREG);
							int tAddr12 = RdReg2(p, ridx12);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr11 += tAddr12;

							/* read Opr11 data */
							int tData11 = RdDataMem(p, tAddr11);

#ifdef VHPI
							if(VhpiMode){	/* rh: read 12b */
								dsp_rh(tAddr11, (short int *)&tData11, 1);
								dsp_wait(1);
							}
#endif

							/* write SHIFT result */
							processSHIFTFunc(p, tData01, tData02, ridx00, opt);

							/* write LD result */
							WrReg(p, ridx10, tData11);

							/* postmodify: update Ix */
							if(isPost2){
								tAddr11 += tAddr12;
								updateIReg(p, ridx11, tAddr11);
							}
						}	/* end of t12e */
						break;
					case t12f:
						{
							/* type 12f */
							p->InstType = t12f;
							/* SHIFT.C || LD.C */
							/* SHIFT.C ACC64S, XOP24S, IMM_INT5 (|HI,LO,HIRND,LORND|) || LD.C DREG24, DM(IREG  +/+= MREG ) */
							/* SHIFT.C Opr00 , Opr01 , Opr02    (|HI,LO,HIRND,LORND|) || LD.C Opr10 , DM(Opr11 +/+= Opr12) */

							/* SHIFT.C operation */
							/* SHIFT.C ACC64S, XOP24S, IMM_INT5 (|HI,LO,HIRND,LORND|) */
							/* SHIFT.C Opr00 , Opr01 , Opr02    (|HI,LO,HIRND,LORND|) */

							/* read shift option: Hi,LO,HIRND,LORND */
							int opt = getOptionFromSF2(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCS1);
							int ridx01 = getRegIndex(p, fs01, eXOP24S);
							cplx ct1 = cRdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCS2);
							cplx ct2;
							ct2.r = ct2.i = getIntImm(p, fs02, eIMM_INT5);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTS);
							int ridx00 = getRegIndex(p, fd, eACC64S);

							/* read LD.C operands */
							/* LD.C DREG24, DM(IREG  +/+= MREG ) */
							/* LD.C Opr10 , DM(Opr11 +/+= Opr12) */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG24);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMI);
							int ridx11 = getRegIndex(p, fs11, eIREG);
							int tAddr11 = RdReg2(p, ridx11);

							if(tAddr11 & 0x01){       //if not aligned
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Complex memory read/write must be aligned to even-word boundaries.\n");
								break;
							}

							/* read Opr12 addr */
							int fs12 = getIntField(p, fDMM);
							int ridx12 = getRegIndex(p, fs12, eMREG);
							int tAddr12 = RdReg2(p, ridx12);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr11 += tAddr12;

							/* read Opr11 data */
							int tData111 = RdDataMem(p, tAddr11);
							int tData112 = RdDataMem(p, tAddr11+1);

#ifdef VHPI
							if(VhpiMode){	/* rw: read 24b */
								int tData1;

								dsp_rw(tAddr11, &tData1, 1);
								dsp_wait(1);

								/* Note: tData1 = (((0x0FFF & tData111) << 12) | (0x0FFF & tData112)); */
								tData111 = (0x0FFF & (tData1 >> 12));
								tData112 = 0xFFF & tData1;
							}
#endif

							/* write SHIFT.C result */
							processSHIFT_CFunc(p, ct1, ct2, ridx00, opt);

							/* write LD result */
							cWrReg(p, ridx10, tData111, tData112);

							/* postmodify: update Ix */
							if(isPost2){
								tAddr11 += tAddr12;
								updateIReg(p, ridx11, tAddr11);
							}
						}	/* end of t12f */
						break;
					case t12m:
						{
							/* type 12m */
							p->InstType = t12m;
							/* SHIFT || LD */
							/* SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) || LD DREG12, DM(IREG  +/+= MREG ) */
							/* SHIFT Opr00 , Opr01 , Opr02    (|NORND,RND|) || LD Opr10 , DM(Opr11 +/+= Opr12) */

							/* SHIFT operation */
							/* SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) */
							/* SHIFT Opr00 , Opr01 , Opr02    (|NORND,RND|) */

							/* read shift option: NORND,RND */
							int opt = getOptionFromSF2(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCS1);
							int ridx01 = getRegIndex(p, fs01, eACC32S);
							int tData01 = RdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCS2);
							int tData02 = getIntImm(p, fs02, eIMM_INT5);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTS);
							int ridx00 = getRegIndex(p, fd, eACC32S);

							/* read LD operands */
							/* LD DREG12, DM(IREG  +/+= MREG ) */
							/* LD Opr10 , DM(Opr11 +/+= Opr12) */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG12);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMI);
							int ridx11 = getRegIndex(p, fs11, eIREG);
							int tAddr11 = RdReg2(p, ridx11);

							/* read Opr12 addr */
							int fs12 = getIntField(p, fDMM);
							int ridx12 = getRegIndex(p, fs12, eMREG);
							int tAddr12 = RdReg2(p, ridx12);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr11 += tAddr12;

							/* read Opr11 data */
							int tData11 = RdDataMem(p, tAddr11);

#ifdef VHPI
							if(VhpiMode){	/* rh: read 12b */
								dsp_rh(tAddr11, (short int *)&tData11, 1);
								dsp_wait(1);
							}
#endif

							/* write SHIFT result */
							processSHIFTFunc(p, tData01, tData02, ridx00, opt);

							/* write LD result */
							WrReg(p, ridx10, tData11);

							/* postmodify: update Ix */
							if(isPost2){
								tAddr11 += tAddr12;
								updateIReg(p, ridx11, tAddr11);
							}
						}	/* end of t12m */
						break;
					case t12n:
						{
							/* type 12n */
							p->InstType = t12n;
							/* SHIFT.C || LD.C */
							/* SHIFT.C ACC64S, ACC64S, IMM_INT5 (|NORND,RND|) || LD.C DREG24, DM(IREG  +/+= MREG ) */
							/* SHIFT.C Opr00 , Opr01 , Opr02    (|NORND,RND|) || LD.C Opr10 , DM(Opr11 +/+= Opr12) */

							/* SHIFT.C operation */
							/* SHIFT.C ACC64S, ACC64S, IMM_INT5 (|NORND,RND|) */
							/* SHIFT.C Opr00 , Opr01 , Opr02    (|NORND,RND|) */

							/* read shift option: NORND,RND */
							int opt = getOptionFromSF2(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCS1);
							int ridx01 = getRegIndex(p, fs01, eACC64S);
							cplx ct1 = cRdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCS2);
							cplx ct2;
							ct2.r = ct2.i = getIntImm(p, fs02, eIMM_INT5);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTS);
							int ridx00 = getRegIndex(p, fd, eACC64S);

							/* read LD.C operands */
							/* LD.C DREG24, DM(IREG  +/+= MREG ) */
							/* LD.C Opr10 , DM(Opr11 +/+= Opr12) */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG24);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMI);
							int ridx11 = getRegIndex(p, fs11, eIREG);
							int tAddr11 = RdReg2(p, ridx11);

							if(tAddr11 & 0x01){       //if not aligned
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Complex memory read/write must be aligned to even-word boundaries.\n");
								break;
							}

							/* read Opr12 addr */
							int fs12 = getIntField(p, fDMM);
							int ridx12 = getRegIndex(p, fs12, eMREG);
							int tAddr12 = RdReg2(p, ridx12);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr11 += tAddr12;

							/* read Opr11 data */
							int tData111 = RdDataMem(p, tAddr11);
							int tData112 = RdDataMem(p, tAddr11+1);

#ifdef VHPI
							if(VhpiMode){	/* rw: read 24b */
								int tData1;

								dsp_rw(tAddr11, &tData1, 1);
								dsp_wait(1);

								/* Note: tData1 = (((0x0FFF & tData111) << 12) | (0x0FFF & tData112)); */
								tData111 = (0x0FFF & (tData1 >> 12));
								tData112 = 0xFFF & tData1;
							}
#endif

							/* write SHIFT.C result */
							processSHIFT_CFunc(p, ct1, ct2, ridx00, opt);

							/* write LD result */
							cWrReg(p, ridx10, tData111, tData112);

							/* postmodify: update Ix */
							if(isPost2){
								tAddr11 += tAddr12;
								updateIReg(p, ridx11, tAddr11);
							}
						}	/* end of t12n */
						break;
					case t12g:
						{
							/* type 12g */
							p->InstType = t12g;
							/* SHIFT || ST */
							/* SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) || ST DM(IREG  +/+= MREG ), DREG12 */
							/* SHIFT Opr00 , Opr01 , Opr02    (|HI,LO,HIRND,LORND|) || ST DM(Opr10 +/+= Opr11), Opr12  */

							/* SHIFT operation */
							/* SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) */
							/* SHIFT Opr00 , Opr01 , Opr02    (|HI,LO,HIRND,LORND|) */

							/* read shift option: Hi,LO,HIRND,LORND */
							int opt = getOptionFromSF2(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCS1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCS2);
							int tData02 = getIntImm(p, fs02, eIMM_INT5);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTS);
							int ridx00 = getRegIndex(p, fd, eACC32S);

							/* ST DM(IREG  +/+= MREG ), DREG12 */
							/* ST DM(Opr10 +/+= Opr11), Opr12  */
	
							/* read Opr10 addr */
							int fs10 = getIntField(p, fDMI);
							int ridx10 = getRegIndex(p, fs10, eIREG);
							int tAddr10 = RdReg2(p, ridx10);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMM);
							int ridx11 = getRegIndex(p, fs11, eMREG);
							int tAddr11 = RdReg2(p, ridx11);

							/* get Opr12 addr */
							int fs12 = getIntField(p, fSRCLS);
							int ridx12 = getRegIndex(p, fs12, eDREG12);
							int tData12 = RdReg(p, ridx12);

							/* write SHIFT result */
							processSHIFTFunc(p, tData01, tData02, ridx00, opt);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr10 += tAddr11;

							/* write ST result */
							WrDataMem(p, tData12, tAddr10);

#ifdef VHPI
							if(VhpiMode){	/* wh: write 12b */
								dsp_wh(tAddr10, (short int *)&tData12, 1);
							}
#endif

							/* postmodify: update Ix */
							if(isPost2){
								tAddr10 += tAddr11;
								updateIReg(p, ridx10, tAddr10);
							}
						}	/* end of t12g */
						break;
					case t12h:
						{
							/* type 12h */
							p->InstType = t12h;
							/* SHIFT.C || ST.C */
							/* SHIFT.C ACC64S, XOP24S, IMM_INT5 (|HI,LO,HIRND,LORND|) || ST.C DM(IREG  +/+= MREG ), DREG24 */
							/* SHIFT.C Opr00 , Opr01 , Opr02    (|HI,LO,HIRND,LORND|) || ST.C DM(Opr10 +/+= Opr11), Opr12  */

							/* SHIFT.C operation */
							/* SHIFT.C ACC64S, XOP24S, IMM_INT5 (|HI,LO,HIRND,LORND|) */
							/* SHIFT.C Opr00 , Opr01 , Opr02    (|HI,LO,HIRND,LORND|) */

							/* read shift option: HI,LO,HIRND,LORND */
							int opt = getOptionFromSF2(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCS1);
							int ridx01 = getRegIndex(p, fs01, eXOP24S);
							cplx ct1 = cRdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCS2);
							cplx ct2;
							ct2.r = ct2.i = getIntImm(p, fs02, eIMM_INT5);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTS);
							int ridx00 = getRegIndex(p, fd, eACC64S);

							/* ST.C DM(IREG  +/+= MREG ), DREG24 */
							/* ST.C DM(Opr10 +/+= Opr11), Opr12  */

							/* read Opr10 addr */
							int fs10 = getIntField(p, fDMI);
							int ridx10 = getRegIndex(p, fs10, eIREG);
							int tAddr10 = RdReg2(p, ridx10);

							if(tAddr10 & 0x01){       //if not aligned
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Complex memory read/write must be aligned to even-word boundaries.\n");
								break;
							}

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMM);
							int ridx11 = getRegIndex(p, fs11, eMREG);
							int tAddr11 = RdReg2(p, ridx11);

							/* get Opr12 addr */
							int fs12 = getIntField(p, fSRCLS);
							int ridx12 = getRegIndex(p, fs12, eDREG24);
							cplx cData1 = cRdReg(p, ridx12);

							/* write SHIFT.C result */
							processSHIFT_CFunc(p, ct1, ct2, ridx00, opt);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr10 += tAddr11;

							/* write ST result */
							WrDataMem(p, cData1.r, tAddr10);
							WrDataMem(p, cData1.i, tAddr10+1);

#ifdef VHPI
							if(VhpiMode){	/* ww: write 24b */
								int tData1 = (((0x0FFF & cData1.r) << 12) | (0x0FFF & cData1.i));	
								dsp_ww(tAddr10, &tData1, 1);
							}
#endif

							/* postmodify: update Ix */
							if(isPost2){
								tAddr10 += tAddr11;
								updateIReg(p, ridx10, tAddr10);
							}
						}	/* end of t12h */
						break;
					case t12o:
						{
							/* type 12o */
							p->InstType = t12o;
							/* SHIFT || ST */
							/* SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) || ST DM(IREG  +/+= MREG ), DREG12 */
							/* SHIFT Opr00 , Opr01 , Opr02    (|NORND,RND|) || ST DM(Opr10 +/+= Opr11), Opr12  */

							/* SHIFT operation */
							/* SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) */
							/* SHIFT Opr00 , Opr01 , Opr02    (|NORND,RND|) */

							/* read shift option: Hi,LO,HIRND,LORND */
							int opt = getOptionFromSF2(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCS1);
							int ridx01 = getRegIndex(p, fs01, eACC32S);
							int tData01 = RdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCS2);
							int tData02 = getIntImm(p, fs02, eIMM_INT5);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTS);
							int ridx00 = getRegIndex(p, fd, eACC32S);

							/* ST DM(IREG  +/+= MREG ), DREG12 */
							/* ST DM(Opr10 +/+= Opr11), Opr12  */
	
							/* read Opr10 addr */
							int fs10 = getIntField(p, fDMI);
							int ridx10 = getRegIndex(p, fs10, eIREG);
							int tAddr10 = RdReg2(p, ridx10);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMM);
							int ridx11 = getRegIndex(p, fs11, eMREG);
							int tAddr11 = RdReg2(p, ridx11);

							/* get Opr12 addr */
							int fs12 = getIntField(p, fSRCLS);
							int ridx12 = getRegIndex(p, fs12, eDREG12);
							int tData12 = RdReg(p, ridx12);

							/* write SHIFT result */
							processSHIFTFunc(p, tData01, tData02, ridx00, opt);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr10 += tAddr11;

							/* write ST result */
							WrDataMem(p, tData12, tAddr10);

#ifdef VHPI
							if(VhpiMode){	/* wh: write 12b */
								dsp_wh(tAddr10, (short int *)&tData12, 1);
							}
#endif

							/* postmodify: update Ix */
							if(isPost2){
								tAddr10 += tAddr11;
								updateIReg(p, ridx10, tAddr10);
							}
						}	/* end of t12o */
						break;
					case t12p:
						{
							/* type 12p */
							p->InstType = t12p;
							/* SHIFT.C || ST.C */
							/* SHIFT.C ACC64S, ACC64S, IMM_INT5 (|NORND,RND|) || ST.C DM(IREG  +/+= MREG ), DREG24 */
							/* SHIFT.C Opr00 , Opr01 , Opr02    (|NORND,RND|) || ST.C DM(Opr10 +/+= Opr11), Opr12  */

							/* SHIFT.C operation */
							/* SHIFT.C ACC64S, ACC64S, IMM_INT5 (|NORND,RND|) */
							/* SHIFT.C Opr00 , Opr01 , Opr02    (|NORND,RND|) */

							/* read shift option: NORND,RND */
							int opt = getOptionFromSF2(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCS1);
							int ridx01 = getRegIndex(p, fs01, eACC64S);
							cplx ct1 = cRdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCS2);
							cplx ct2;
							ct2.r = ct2.i = getIntImm(p, fs02, eIMM_INT5);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTS);
							int ridx00 = getRegIndex(p, fd, eACC64S);

							/* ST.C DM(IREG  +/+= MREG ), DREG24 */
							/* ST.C DM(Opr10 +/+= Opr11), Opr12  */

							/* read Opr10 addr */
							int fs10 = getIntField(p, fDMI);
							int ridx10 = getRegIndex(p, fs10, eIREG);
							int tAddr10 = RdReg2(p, ridx10);

							if(tAddr10 & 0x01){       //if not aligned
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Complex memory read/write must be aligned to even-word boundaries.\n");
								break;
							}

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMM);
							int ridx11 = getRegIndex(p, fs11, eMREG);
							int tAddr11 = RdReg2(p, ridx11);

							/* get Opr12 addr */
							int fs12 = getIntField(p, fSRCLS);
							int ridx12 = getRegIndex(p, fs12, eDREG24);
							cplx cData1 = cRdReg(p, ridx12);

							/* write SHIFT.C result */
							processSHIFT_CFunc(p, ct1, ct2, ridx00, opt);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr10 += tAddr11;

							/* write ST result */
							WrDataMem(p, cData1.r, tAddr10);
							WrDataMem(p, cData1.i, tAddr10+1);

#ifdef VHPI
							if(VhpiMode){	/* ww: write 24b */
								int tData1 = (((0x0FFF & cData1.r) << 12) | (0x0FFF & cData1.i));	
								dsp_ww(tAddr10, &tData1, 1);
							}
#endif

							/* postmodify: update Ix */
							if(isPost2){
								tAddr10 += tAddr11;
								updateIReg(p, ridx10, tAddr10);
							}
						}	/* end of t12p */
						break;
					case t14c:
						{
							/* type 14c */
							p->InstType = t14c;
							/* SHIFT || CP */
							/* SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) || CP DREG12, DREG12 */
							/* SHIFT Opr00 , Opr01 , Opr02    (|HI,LO,HIRND,LORND|) || CP Opr10 , Opr11  */

							/* SHIFT operation */
							/* SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) */
							/* SHIFT Opr00 , Opr01 , Opr02    (|HI,LO,HIRND,LORND|) */

							/* read shift option: HI,LO,HIRND,LORND */
							int opt = getOptionFromSF2(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCS1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCS2);
							int tData02 = getIntImm(p, fs02, eIMM_INT5);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTS);
							int ridx00 = getRegIndex(p, fd, eACC32S);

							/* CP DREG12, DREG12 */
							/* CP Opr10 , Opr11  */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG12);

							/* read Opr11 data */
							int fs11 = getIntField(p, fSRCLS);
							int ridx11 = getRegIndex(p, fs11, eDREG12);
							int tData11 = RdReg(p, ridx11);

							/* write SHIFT result */
							processSHIFTFunc(p, tData01, tData02, ridx00, opt);

							/* write CP result */
							/* write to Opr10 */
							WrReg(p, ridx10, tData11);
						}	/* end of t14c */
						break;
					case t14d:
						{
							/* type 14d */
							p->InstType = t14d;
							/* SHIFT.C || CP.C */
							/* SHIFT.C ACC64S, XOP24S, IMM_INT5 (|HI,LO,HIRND,LORND|) || CP.C DREG24, DREG24 */
							/* SHIFT.C Opr00 , Opr01 , Opr02    (|HI,LO,HIRND,LORND|) || CP.C Opr10 , Opr11  */

							/* SHIFT.C operation */
							/* SHIFT.C ACC64S, XOP24S, IMM_INT5 (|HI,LO,HIRND,LORND|) */
							/* SHIFT.C Opr00 , Opr01 , Opr02    (|HI,LO,HIRND,LORND|) */

							/* read shift option: HI,LO,HIRND,LORND */
							int opt = getOptionFromSF2(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCS1);
							int ridx01 = getRegIndex(p, fs01, eXOP24S);
							cplx ct1 = cRdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCS2);
							cplx ct2;
							ct2.r = ct2.i = getIntImm(p, fs02, eIMM_INT5);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTS);
							int ridx00 = getRegIndex(p, fd, eACC64S);

							/* CP.C DREG24, DREG24 */
							/* CP.C Opr10  , Opr11   */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG24);
			
							/* read Opr11 data */
							int fs11 = getIntField(p, fSRCLS);
							int ridx11 = getRegIndex(p, fs11, eDREG24);
							cplx cData1 = cRdReg(p, ridx11);

							/* write SHIFT.C result */
							processSHIFT_CFunc(p, ct1, ct2, ridx00, opt);

							/* write CP.C result */
							/* write to Opr10 */
							cWrReg(p, ridx10, cData1.r, cData1.i);
						}	/* end of t14d */
						break;
					case t14k:
						{
							/* type 14k */
							p->InstType = t14k;
							/* SHIFT || CP */
							/* SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) || CP DREG12, DREG12 */
							/* SHIFT Opr00 , Opr01 , Opr02    (|NORND,RND|) || CP Opr10 , Opr11  */

							/* SHIFT operation */
							/* SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) */
							/* SHIFT Opr00 , Opr01 , Opr02    (|NORND,RND|) */

							/* read shift option: NORND,RND */
							int opt = getOptionFromSF2(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCS1);
							int ridx01 = getRegIndex(p, fs01, eACC32S);
							int tData01 = RdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCS2);
							int tData02 = getIntImm(p, fs02, eIMM_INT5);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTS);
							int ridx00 = getRegIndex(p, fd, eACC32S);

							/* CP DREG12, DREG12 */
							/* CP Opr10 , Opr11  */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG12);

							/* read Opr11 data */
							int fs11 = getIntField(p, fSRCLS);
							int ridx11 = getRegIndex(p, fs11, eDREG12);
							int tData11 = RdReg(p, ridx11);

							/* write SHIFT result */
							processSHIFTFunc(p, tData01, tData02, ridx00, opt);

							/* write CP result */
							/* write to Opr10 */
							WrReg(p, ridx10, tData11);
						}	/* end of t14k */
						break;
					case t14l:
						{
							/* type 14l */
							p->InstType = t14l;
							/* SHIFT.C || CP.C */
							/* SHIFT.C ACC64S, ACC64S, IMM_INT5 (|NORND,RND|) || CP.C DREG24, DREG24 */
							/* SHIFT.C Opr00 , Opr01 , Opr02    (|NORND,RND|) || CP.C Opr10 , Opr11  */

							/* SHIFT.C operation */
							/* SHIFT.C ACC64S, ACC64S, IMM_INT5 (|NORND,RND|) */
							/* SHIFT.C Opr00 , Opr01 , Opr02    (|NORND,RND|) */

							/* read shift option: NORND,RND */
							int opt = getOptionFromSF2(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCS1);
							int ridx01 = getRegIndex(p, fs01, eACC64S);
							cplx ct1 = cRdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCS2);
							cplx ct2;
							ct2.r = ct2.i = getIntImm(p, fs02, eIMM_INT5);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTS);
							int ridx00 = getRegIndex(p, fd, eACC64S);

							/* CP.C DREG24, DREG24 */
							/* CP.C Opr10  , Opr11   */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDSTLS);
							int ridx10 = getRegIndex(p, fd10, eDREG24);
			
							/* read Opr11 data */
							int fs11 = getIntField(p, fSRCLS);
							int ridx11 = getRegIndex(p, fs11, eDREG24);
							cplx cData1 = cRdReg(p, ridx11);

							/* write SHIFT.C result */
							processSHIFT_CFunc(p, ct1, ct2, ridx00, opt);

							/* write CP.C result */
							/* write to Opr10 */
							cWrReg(p, ridx10, cData1.r, cData1.i);
						}	/* end of t14l */
						break;
					case t43a:
						{
							/* type 43a */
							p->InstType = t43a;
							/* ALU || MAC */
							/* ALU DREG12S, XOP12S, YOP12S || MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) */
							/* ALU Opr00  , Opr01 , Opr02  || MAC Opr10 , Opr11 , Opr12  (|RND,SS,SU,US,UU|) */

							/* ALU operation */
							/* ALU DREG12S, XOP12S, YOP12S */
							/* ALU Opr00  , Opr01 , Opr02  */

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCA1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);
							int tData02;

							int ridx00;

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCA2);
							int ridx02 = getRegIndex(p, fs02, eYOP12S);
							tData02 = RdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTA);
							ridx00 = getRegIndex(p, fd, eDREG12S);

							/* MAC operation */
							/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) */
							/* MAC Opr10 , Opr11 , Opr12  (|RND,SS,SU,US,UU|) */

							/* read multiplication option: RND, SS, SU, US, UU */
							int opt = getOptionFromMF(p);

							/* read Opr11 data */
							int fs11 = getIntField(p, fSRCM1);
							int ridx11 = getRegIndex(p, fs11, eXOP12S);
							int tData11 = RdReg(p, ridx11);

							/* read Opr12 data */
							int fs12 = getIntField(p, fSRCM2);
							int ridx12 = getRegIndex(p, fs12, eYOP12S);
							int tData12 = RdReg(p, ridx12);

							/* get Opr3 addr */
							int fd10 = getIntField(p, fDSTM);
							int ridx10 = getRegIndex(p, fd10, eACC32S);

							/* compute ALU operation */
							processALUFunc(p, tData01, tData02, ridx00);

							/* compute MAC operation */
							processMACFunc(p, tData11, tData12, ridx10, opt, 0);
						}	/* end of t43a */
						break;
					case t44b:
						{
							/* type 44b */
							p->InstType = t44b;
							/* ALU || SHIFT */
							/* ALU DREG12S, XOP12S, YOP12S || SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) */
							/* ALU Opr00  , Opr01 , Opr02  || SHIFT Opr10 , Opr11 , Opr12    (|HI,LO,HIRND,LORND|) */

							/* ALU operation */
							/* ALU DREG12S, XOP12S, YOP12S */
							/* ALU Opr00  , Opr01 , Opr02  */

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCA1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);
							int tData02;

							int ridx00;

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCA2);
							int ridx02 = getRegIndex(p, fs02, eYOP12S);
							tData02 = RdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTA);
							ridx00 = getRegIndex(p, fd, eDREG12S);

							/* SHIFT operation */
							/* SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) */
							/* SHIFT Opr10 , Opr11 , Opr12    (|HI,LO,HIRND,LORND|) */

							/* read shift option: HI,LO,HIRND,LORND */
							int opt = getOptionFromSF2(p);

							/* read Opr11 data */
							int fs11 = getIntField(p, fSRCS1);
							int ridx11 = getRegIndex(p, fs11, eXOP12S);
							int tData11 = RdReg(p, ridx11);

							/* read Opr12 data */
							int fs12 = getIntField(p, fSRCS2);
							int tData12 = getIntImm(p, fs12, eIMM_INT5);

							/* get Opr3 addr */
							int fd10 = getIntField(p, fDSTS);
							int ridx10 = getRegIndex(p, fd10, eACC32S);

							/* compute ALU operation */
							processALUFunc(p, tData01, tData02, ridx00);

							/* write SHIFT result */
							processSHIFTFunc(p, tData11, tData12, ridx10, opt);
						}	/* end of t44b */
						break;
					case t44d:
						{
							/* type 44d */
							p->InstType = t44d;
							/* ALU || SHIFT */
							/* ALU DREG12S, XOP12S, YOP12S || SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) */
							/* ALU Opr00  , Opr01 , Opr02  || SHIFT Opr10 , Opr11 , Opr12    (|NORND,RND|) */

							/* ALU operation */
							/* ALU DREG12S, XOP12S, YOP12S */
							/* ALU Opr00  , Opr01 , Opr02  */

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCA1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);
							int tData02;

							int ridx00;

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCA2);
							int ridx02 = getRegIndex(p, fs02, eYOP12S);
							tData02 = RdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTA);
							ridx00 = getRegIndex(p, fd, eDREG12S);

							/* SHIFT operation */
							/* SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) */
							/* SHIFT Opr10 , Opr11 , Opr12    (|NORND,RND|) */

							/* read shift option: NORND,RND */
							int opt = getOptionFromSF2(p);

							/* read Opr11 data */
							int fs11 = getIntField(p, fSRCS1);
							int ridx11 = getRegIndex(p, fs11, eACC32S);
							int tData11 = RdReg(p, ridx11);

							/* read Opr12 data */
							int fs12 = getIntField(p, fSRCS2);
							int tData12 = getIntImm(p, fs12, eIMM_INT5);

							/* get Opr3 addr */
							int fd10 = getIntField(p, fDSTS);
							int ridx10 = getRegIndex(p, fd10, eACC32S);

							/* compute ALU operation */
							processALUFunc(p, tData01, tData02, ridx00);

							/* write SHIFT result */
							processSHIFTFunc(p, tData11, tData12, ridx10, opt);
						}	/* end of t44d */
						break;
					case t45b:
						{
							/* type 45b */
							p->InstType = t45b;
							/* MAC || SHIFT */
							/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) || SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) */
							/* MAC Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) || SHIFT Opr10 , Opr11 , Opr12    (|HI,LO,HIRND,LORND|) */

							/* MAC operation */
							/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) */
							/* MAC Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) */

							/* read multiplication option: RND, SS, SU, US, UU */
							int opt0 = getOptionFromMF(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCM1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCM2);
							int ridx02 = getRegIndex(p, fs02, eYOP12S);
							int tData02 = RdReg(p, ridx02);

							/* get Opr3 addr */
							int fd00 = getIntField(p, fDSTM);
							int ridx00 = getRegIndex(p, fd00, eACC32S);

							/* SHIFT operation */
							/* SHIFT ACC32S, XOP12S, IMM_INT5 (|HI,LO,HIRND,LORND|) */
							/* SHIFT Opr10 , Opr11 , Opr12    (|HI,LO,HIRND,LORND|) */

							/* read shift option: HI,LO,HIRND,LORND */
							int opt1 = getOptionFromSF2(p);

							/* read Opr11 data */
							int fs11 = getIntField(p, fSRCS1);
							int ridx11 = getRegIndex(p, fs11, eXOP12S);
							int tData11 = RdReg(p, ridx11);

							/* read Opr12 data */
							int fs12 = getIntField(p, fSRCS2);
							int tData12 = getIntImm(p, fs12, eIMM_INT5);

							/* get Opr3 addr */
							int fd10 = getIntField(p, fDSTS);
							int ridx10 = getRegIndex(p, fd10, eACC32S);

							/* compute MAC operation */
							processMACFunc(p, tData01, tData02, ridx00, opt0, 0);

							/* write SHIFT result */
							processSHIFTFunc(p, tData11, tData12, ridx10, opt1);
						}	/* end of t45b */
						break;
					case t45d:
						{
							/* type 45d */
							p->InstType = t45d;
							/* MAC || SHIFT */
							/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) || SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) */
							/* MAC Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) || SHIFT Opr10 , Opr11 , Opr12    (|NORND,RND|) */

							/* MAC operation */
							/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) */
							/* MAC Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) */

							/* read multiplication option: RND, SS, SU, US, UU */
							int opt0 = getOptionFromMF(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCM1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCM2);
							int ridx02 = getRegIndex(p, fs02, eYOP12S);
							int tData02 = RdReg(p, ridx02);

							/* get Opr3 addr */
							int fd00 = getIntField(p, fDSTM);
							int ridx00 = getRegIndex(p, fd00, eACC32S);

							/* SHIFT operation */
							/* SHIFT ACC32S, ACC32S, IMM_INT5 (|NORND,RND|) */
							/* SHIFT Opr10 , Opr11 , Opr12    (|NORND,RND|) */

							/* read shift option: NORND,RND */
							int opt1 = getOptionFromSF2(p);

							/* read Opr11 data */
							int fs11 = getIntField(p, fSRCS1);
							int ridx11 = getRegIndex(p, fs11, eACC32S);
							int tData11 = RdReg(p, ridx11);

							/* read Opr12 data */
							int fs12 = getIntField(p, fSRCS2);
							int tData12 = getIntImm(p, fs12, eIMM_INT5);

							/* get Opr3 addr */
							int fd10 = getIntField(p, fDSTS);
							int ridx10 = getRegIndex(p, fd10, eACC32S);

							/* compute MAC operation */
							processMACFunc(p, tData01, tData02, ridx00, opt0, 0);

							/* write SHIFT result */
							processSHIFTFunc(p, tData11, tData12, ridx10, opt1);
						}	/* end of t45d */
						break;
					default:
						break;
				}	/* end of switch(p->InstType) */
			}
			break;	/* end of case 1 */
		case	2:			/* three-opcode multifunction */
			{
				//m1 = p->Multi[0];
				//m2 = p->Multi[1];

				switch(p->InstType){
					case t01a:
						{
							/* type 1a */
							p->InstType = t01a;
							/* MAC || LD || LD */
							/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) 
								|| LD XOP12S, DM(IX+/+= MX) || LD YOP12S, DM(IY+/+=MY) */
							/* MAC Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) 
								|| LD Opr10, DM(Opr11+/+=Opr12) || LD Opr20, DM(Opr21+/+=Opr22) */

							/* MAC operation */
							/* MAC ACC32S, XOP12S, YOP12S (|RND,SS,SU,US,UU|) */
							/* MAC Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) */

							/* read multiplication option: RND, SS, SU, US, UU */
							int opt = getOptionFromMF(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCM1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCM2);
							int ridx02 = getRegIndex(p, fs02, eYOP12S);
							int tData02 = RdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTM);
							int ridx00 = getRegIndex(p, fd, eACC32S);

							/* read first LD operands */
							/* LD XOP12S, DM(IX    +/+= MX   ) */
							/* LD Opr10 , DM(Opr11 +/+= Opr12) */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDD);
							int ridx10 = getRegIndex(p, fd10, eXOP12S);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMI);
							int ridx11 = getRegIndex(p, fs11, eIX);
							int tAddr11 = RdReg2(p, ridx11);

							/* read Opr12 addr */
							int fs12 = getIntField(p, fDMM);
							int ridx12 = getRegIndex(p, fs12, eMX);
							int tAddr12 = RdReg2(p, ridx12);

							/* check if postmodify */
							int isPost1 = FALSE;
							if(getIntField(p, fU)) isPost1 = TRUE;

							/* if premodify */
							if(!isPost1) tAddr11 += tAddr12;

							/* read Opr11 data */
							int tData11 = RdDataMem(p, tAddr11);

							/* read second LD operands */
							/* LD YOP12S, DM(IY    +/+= MY   ) */
							/* LD Opr20 , DM(Opr21 +/+= Opr22) */

							/* get Opr20 addr */
							int fd20 = getIntField(p, fPD);
							int ridx20 = getRegIndex(p, fd20, eYOP12S);

							/* read Opr21 addr */
							int fs21 = getIntField(p, fPMI);
							int ridx21 = getRegIndex(p, fs21, eIY);
							int tAddr21 = RdReg2(p, ridx21);

							/* read Opr22 addr */
							int fs22 = getIntField(p, fPMM);
							int ridx22 = getRegIndex(p, fs22, eMY);
							int tAddr22 = RdReg2(p, ridx22);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU2)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr21 += tAddr22;

							/* read Opr21 data */
							int tData21 = RdDataMem(p, tAddr21);

#ifdef VHPI
							if(VhpiMode){	/* rh+rh: read 12b+12b */
								dsp_rh_rh(tAddr11, (short int *)&tData11, tAddr21, (short int *)&tData21, 1);
								dsp_wait(1);
							}
#endif

							/* compute MAC operation */
							processMACFunc(p, tData01, tData02, ridx00, opt, 0);

							/* write first LD result */
							WrReg(p, ridx10, tData11);

							/* write second LD result */
							WrReg(p, ridx20, tData21);

							/* postmodify: update Ix */
							if(isPost1){
								tAddr11 += tAddr12;
								updateIReg(p, ridx11, tAddr11);
							}

							if(isPost2){
								tAddr21 += tAddr22;
								updateIReg(p, ridx21, tAddr21);
							}
						}	/* end of t01a */
						break;
					case t01b:
						{
							/* type 1b */
							p->InstType = t01b;
							/* MAC.C || LD.C || LD.C */
							/* MAC.C ACC64S, XOP24S, YOP24S (|RND,SS,SU,US,UU|) 
								|| LD.C XOP24S, DM(IX+/+= MX) || LD.C YOP24S, DM(IY+/+=MY) */
							/* MAC.C Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) 
								|| LD.C Opr10, DM(Opr11+/+=Opr12) || LD.C Opr20, DM(Opr21+/+=Opr22) */

							/* MAC.C operation */
							/* MAC.C ACC64S, XOP24S, YOP24S (|RND,SS,SU,US,UU|) */
							/* MAC.C Opr00 , Opr01 , Opr02  (|RND,SS,SU,US,UU|) */

							/* read multiplication option: RND, SS, SU, US, UU */
							int opt = getOptionFromMF(p);

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCM1);
							int ridx01 = getRegIndex(p, fs01, eXOP24S);
							cplx ct1 = cRdReg(p, ridx01);

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCM2);
							int ridx02 = getRegIndex(p, fs02, eYOP24S);
							cplx ct2 = cRdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTM);
							int ridx00 = getRegIndex(p, fd, eACC64S);

							/* read first LD.C operands */
							/* LD.C XOP24S, DM(IX    +/+= MX   ) */
							/* LD.C Opr10 , DM(Opr11 +/+= Opr12) */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDD);
							int ridx10 = getRegIndex(p, fd10, eXOP24S);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMI);
							int ridx11 = getRegIndex(p, fs11, eIX);
							int tAddr11 = RdReg2(p, ridx11);

							if(tAddr11 & 0x01){       //if not aligned
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Complex memory read/write must be aligned to even-word boundaries.\n");
								break;
							}

							/* read Opr12 addr */
							int fs12 = getIntField(p, fDMM);
							int ridx12 = getRegIndex(p, fs12, eMX);
							int tAddr12 = RdReg2(p, ridx12);

							/* check if postmodify */
							int isPost1 = FALSE;
							if(getIntField(p, fU)) isPost1 = TRUE;

							/* if premodify */
							if(!isPost1) tAddr11 += tAddr12;

							/* read Opr11 data */
							int tData111 = RdDataMem(p, tAddr11);
							int tData112 = RdDataMem(p, tAddr11+1);

							/* read second LD.C operands */
							/* LD.C YOP24S, DM(IY    +/+= MY   ) */
							/* LD.C Opr20 , DM(Opr21 +/+= Opr22) */

							/* get Opr20 addr */
							int fd20 = getIntField(p, fPD);
							int ridx20 = getRegIndex(p, fd20, eYOP24S);

							/* read Opr21 addr */
							int fs21 = getIntField(p, fPMI);
							int ridx21 = getRegIndex(p, fs21, eIY);
							int tAddr21 = RdReg2(p, ridx21);

							if(tAddr21 & 0x01){       //if not aligned
								printRunTimeError(p->PMA, (char *)sOp[p->Index], 
									"Complex memory read/write must be aligned to even-word boundaries.\n");
								break;
							}

							/* read Opr22 addr */
							int fs22 = getIntField(p, fPMM);
							int ridx22 = getRegIndex(p, fs22, eMY);
							int tAddr22 = RdReg2(p, ridx22);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU2)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr21 += tAddr22;

							/* read Opr21 data */
							int tData211 = RdDataMem(p, tAddr21);
							int tData212 = RdDataMem(p, tAddr21+1);

#ifdef VHPI
							if(VhpiMode){	/* rw+rw: read 24b+24b */
								int tData1, tData2;

								dsp_rw_rw(tAddr11, &tData1, tAddr21, &tData2, 1);
								dsp_wait(1);

								/* Note:
								tData1 = (((0x0FFF & tData111) << 12) | (0x0FFF & tData112));	
								tData2 = (((0x0FFF & tData211) << 12) | (0x0FFF & tData212));	
								*/
								tData111 = (0x0FFF & (tData1 >> 12));
								tData112 = 0xFFF & tData1;
								tData211 = (0x0FFF & (tData2 >> 12));
								tData212 = 0xFFF & tData2;
							}
#endif

							/* compute MAC operation */
							processMAC_CFunc(p, ct1, ct2, ridx00, opt, 0);

							/* write first LD.C result */
							cWrReg(p, ridx10, tData111, tData112);

							/* write second LD.C result */
							cWrReg(p, ridx20, tData211, tData212);

							/* postmodify: update Ix */
							if(isPost1){
								tAddr11 += tAddr12;
								updateIReg(p, ridx11, tAddr11);
							}

							if(isPost2){
								tAddr21 += tAddr22;
								updateIReg(p, ridx21, tAddr21);
							}
						}	/* end of t01b */
						break;
					case t01c:
						{
							/* type 1c */
							p->InstType = t01c;
							/* ALU || LD || LD */
							/* ALU DREG12S, XOP12S, YOP12S 
								|| LD XOP12S, DM(IX    +/+= MX   ) || LD YOP12S, DM(IY    +/+= MY   ) */
							/* ALU Opr00  , Opr01 , Opr02  
								|| LD Opr10 , DM(Opr11 +/+= Opr12) || LD Opr20 , DM(Opr21 +/+= Opr22) */

							/* ALU operation */
							/* ALU DREG12S, XOP12S, YOP12S */
							/* ALU Opr00  , Opr01 , Opr02  */

							/* read Opr01 data */
							int fs01 = getIntField(p, fSRCA1);
							int ridx01 = getRegIndex(p, fs01, eXOP12S);
							int tData01 = RdReg(p, ridx01);
							int tData02;

							int ridx00;

							/* read Opr02 data */
							int fs02 = getIntField(p, fSRCA2);
							int ridx02 = getRegIndex(p, fs02, eYOP12S);
							tData02 = RdReg(p, ridx02);

							/* get Opr3 addr */
							int fd = getIntField(p, fDSTA);
							ridx00 = getRegIndex(p, fd, eDREG12S);

							/* read first LD operands */
							/* LD XOP12S, DM(IX    +/+= MX   ) */
							/* LD Opr10 , DM(Opr11 +/+= Opr12) */

							/* get Opr10 addr */
							int fd10 = getIntField(p, fDD);
							int ridx10 = getRegIndex(p, fd10, eXOP12S);

							/* read Opr11 addr */
							int fs11 = getIntField(p, fDMI);
							int ridx11 = getRegIndex(p, fs11, eIX);
							int tAddr11 = RdReg2(p, ridx11);

							/* read Opr12 addr */
							int fs12 = getIntField(p, fDMM);
							int ridx12 = getRegIndex(p, fs12, eMX);
							int tAddr12 = RdReg2(p, ridx12);

							/* check if postmodify */
							int isPost1 = FALSE;
							if(getIntField(p, fU)) isPost1 = TRUE;

							/* if premodify */
							if(!isPost1) tAddr11 += tAddr12;

							/* read Opr11 data */
							int tData11 = RdDataMem(p, tAddr11);

							/* read second LD operands */
							/* LD YOP12S, DM(IY    +/+= MY   ) */
							/* LD Opr20 , DM(Opr21 +/+= Opr22) */

							/* get Opr20 addr */
							int fd20 = getIntField(p, fPD);
							int ridx20 = getRegIndex(p, fd20, eYOP12S);

							/* read Opr21 addr */
							int fs21 = getIntField(p, fPMI);
							int ridx21 = getRegIndex(p, fs21, eIY);
							int tAddr21 = RdReg2(p, ridx21);

							/* read Opr22 addr */
							int fs22 = getIntField(p, fPMM);
							int ridx22 = getRegIndex(p, fs22, eMY);
							int tAddr22 = RdReg2(p, ridx22);

							/* check if postmodify */
							int isPost2 = FALSE;
							if(getIntField(p, fU2)) isPost2 = TRUE;

							/* if premodify */
							if(!isPost2) tAddr21 += tAddr22;

							/* read Opr21 data */
							int tData21 = RdDataMem(p, tAddr21);

#ifdef VHPI
							if(VhpiMode){	/* rh+rh: read 12b+12b */
								dsp_rh_rh(tAddr11, (short int *)&tData11, tAddr21, (short int *)&tData21, 1);
								dsp_wait(1);
							}
#endif

							/* compute ALU operation */
							processALUFunc(p, tData01, tData02, ridx00);

							/* write first LD result */
							WrReg(p, ridx10, tData11);

							/* write second LD result */
							WrReg(p, ridx20, tData21);

							/* postmodify: update Ix */
							if(isPost1){
								tAddr11 += tAddr12;
								updateIReg(p, ridx11, tAddr11);
							}

							if(isPost2){
								tAddr21 += tAddr22;
								updateIReg(p, ridx21, tAddr21);
							}
						}	/* end of t01c */
						break;
					default:	/* default of inner switch */
						break;
				}	/* end of switch(p->InstType) */
			} /* end of case 2 */
			break;	
		default:
			break;	/* default of outer switch */
	} /* end of switch(p->MultiCounter) */
	return p;
}


/**
* @brief Scan intermediate code one-by-one to determine the opcode index of each instruction.
*
* @param bcode Pointer to user binary converted to binary code format by parser
*
* @return 0 if the simulation successfully ends.
*/
int codeScan(sBCodeList *bcode)
{
    sBCode *p = bcode->FirstNode;

    //curaddr = 0;

    while(p != NULL) {
        p = codeScanOneInst(p);
    }

    return 0;   /* successfully ended */
}


/**
* Scan one instruction word for opcode Index (eOp) resolution.
* If multifunction, resolve MultiCounter & IndexMulti[] too.
*
* @param *p Pointer to current instruction
*
* @return Pointer to next instruction
*/
sBCode *codeScanOneInst(sBCode *p)
{
    sBCode *NextCode;

	/* for iENA, iDIS */
	int code[8];
	int ena;

	/* default: 1 cycle */
	p->Latency = 1;

    switch(p->InstType){
		/* ALU Instructions */
		case t09c:
		case t09e:
		case t09i:
			p->Index = getIndexFromAF(p, 0);
			break;
		case t09d:
		case t09f:
			p->Index = getIndexFromAF(p, 1);
			break;
		case t23a:
			p->Index = getIndexFromDF(p);
			break;
		case t46a:
			p->Index = iSCR;
			break;
		case t46b:
			p->Index = iSCR_C;
			break;
		/* MAC Instructions */
		case t40e:
			p->Index = getIndexFromAMF(p, 0);
			break;
		case t40f:
			p->Index = getIndexFromAMF(p, 1);
			break;
		case t40g:
			p->Index = getIndexFromAMF(p, 2);
			break;
		case t41a:
			p->Index = iRNDACC;
			break;
		case t41b:
			p->Index = iRNDACC_C;
			break;
		case t41c:
			p->Index = iCLRACC;
			break;
		case t41d:
			p->Index = iCLRACC_C;
			break;
		case t25a:
			p->Index = iSATACC;
			break;
		case t25b:
			p->Index = iSATACC_C;
			break;
		case t09g:
			p->Index = getIndexFromAMF(p, 0);
			break;
		case t09h:
			p->Index = getIndexFromAMF(p, 1);
			break;
		/* Shifter Instructions */
		case t16a:
		case t16c:
		case t16e:
		case t15a:
		case t15c:
		case t15e:
			p->Index = getIndexFromSF(p, 0);
			break;
		case t16b:
		case t16d:
		case t16f:
		case t15b:
		case t15d:
		case t15f:
			p->Index = getIndexFromSF(p, 1);
			break;
		/* Multifunction Instructions */
		case t01a:		/* MAC || LD || LD */
			p->Index = getIndexFromMF(p, 0);
			if(!p->Index){		/* NONE: LD || LD */
			 	p->Index = iLD;
				p->MultiCounter = 1;
				p->IndexMulti[0] = iLD;
			}else{				/* MAC || LD || LD */
				p->MultiCounter = 2;
				p->IndexMulti[0] = p->IndexMulti[1] = iLD;
			}
			break;
		case t01b:		/* MAC.C || LD.C || LD.C */
			p->Index = getIndexFromMF(p, 1);
			if(!p->Index){		/* NONE: LD.C || LD.C */
			 	p->Index = iLD_C;
				p->MultiCounter = 1;
				p->IndexMulti[0] = iLD_C;
			}else{				/* MAC.C || LD.C || LD.C */
				p->MultiCounter = 2;
				p->IndexMulti[0] = p->IndexMulti[1] = iLD_C;
			}
			break;
		case t01c:		/* ALU || LD || LD */
			p->Index = getIndexFromAF(p, 0);
			p->MultiCounter = 2;
			p->IndexMulti[0] = p->IndexMulti[1] = iLD;
			break;
		case t04a:		/* MAC || LD */
			p->Index = getIndexFromMF(p, 0);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iLD;
			break;
		case t04c:		/* ALU || LD */
			p->Index = getIndexFromAF(p, 0);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iLD;
			break;
		case t04b:		/* MAC.C || LD.C */
			p->Index = getIndexFromMF(p, 1);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iLD_C;
			break;
		case t04d:		/* ALU.C || LD.C */
			p->Index = getIndexFromAF(p, 1);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iLD_C;
			break;
		case t12e:		/* SHIFT || LD */
		case t12m:
			p->Index = getIndexFromSF2(p, 0);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iLD;
			break;
		case t12f:		/* SHIFT.C || LD.C */
		case t12n:
			p->Index = getIndexFromSF2(p, 1);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iLD_C;
			break;
		case t04e:		/* MAC || ST */
			p->Index = getIndexFromMF(p, 0);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iST;
			break;
		case t04g:		/* ALU || ST */
			p->Index = getIndexFromAF(p, 0);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iST;
			break;
		case t04f:		/* MAC.C || ST.C */
			p->Index = getIndexFromMF(p, 1);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iST_C;
			break;
		case t04h:		/* ALU.C || ST.C */
			p->Index = getIndexFromAF(p, 1);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iST_C;
			break;
		case t12g:		/* SHIFT || ST */
		case t12o:
			p->Index = getIndexFromSF2(p, 0);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iST;
			break;
		case t12h:		/* SHIFT.C || ST.C */
		case t12p:
			p->Index = getIndexFromSF2(p, 1);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iST_C;
			break;
		case t08a:		/* MAC || CP */
			p->Index = getIndexFromMF(p, 0);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iCP;
			break;
		case t08c:		/* ALU || CP */
			p->Index = getIndexFromAF(p, 0);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iCP;
			break;
		case t08b:		/* MAC.C || CP.C */
			p->Index = getIndexFromMF(p, 1);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iCP_C;
			break;
		case t08d:		/* ALU.C || CP.C */
			p->Index = getIndexFromAF(p, 1);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iCP_C;
			break;
		case t14c:		/* SHIFT || CP */
		case t14k:
			p->Index = getIndexFromSF2(p, 0);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iCP;
			break;
		case t14d:		/* SHIFT.C || CP.C */
		case t14l:
			p->Index = getIndexFromSF2(p, 1);
			p->MultiCounter = 1;
			p->IndexMulti[0] = iCP_C;
			break;
		case t43a:		/* ALU || MAC */
			p->Index = getIndexFromAF(p, 0);
			p->MultiCounter = 1;
			p->IndexMulti[0] = getIndexFromMF(p, 0);
			break;
		case t44b:		/* ALU || SHIFT */
		case t44d:
			p->Index = getIndexFromAF(p, 0);
			p->MultiCounter = 1;
			p->IndexMulti[0] = getIndexFromSF2(p, 0);
			break;
		case t45b:		/* MAC || SHIFT */
		case t45d:
			p->Index = getIndexFromMF(p, 0);
			p->MultiCounter = 1;
			p->IndexMulti[0] = getIndexFromSF2(p, 0);
			break;
		/* Data Move Instructions */
		case t17a:
		case t17b:
		case t17c:
		case t17g:
		case t17h:
			p->Index = iCP;
			break;
		case t17d:
		case t17e:
		case t17f:
			p->Index = iCP_C;
			break;
		case t03a:
		case t03d:
		case t06a:
		case t06b:
		case t06d:
		case t32a:
		case t29a:
			p->Index = iLD;
			break;
		case t03c:
		case t03e:
		case t06c:
		case t32b:
		case t29b:
			p->Index = iLD_C;
			break;
		case t03f:
		case t03h:
		case t32c:
		case t29c:
		case t22a:
			p->Index = iST;
			break;
		case t03g:
		case t03i:
		case t32d:
		case t29d:
			p->Index = iST_C;

			if(p->InstType == t03i){
				/* latency restriction: if LD/ST follows t03i, t03i becomes 2-cycle instruction */
				sBCode *pn = getNextCode(p);
				if(isLD(pn) || isLD_C(pn) || isST(pn) || isST_C (pn) || isLDSTMulti(pn)){
					p->Latency = 2;
				}
			}
			break;
		/* Program Flow Instructions */
		case t11a:
			p->Index = iDO;
			break;
		case t10a:
		case t10b:
		case t19a:
			switch(getIntField(p, fS)){
				case 0:		/* S = 0: JUMP */
					p->Index = iJUMP;

					if(DelaySlotMode){
						sBCode *pn = getNextCode(p);
						pn->isDelaySlot = TRUE;			/* identify delay slot */
					}else{		/* if delay slot diasbled, 1-cycle stall */
						p->Latency = 2;
					}
					break;
				case 1:		/* S = 1: CALL */
					p->Index = iCALL;

					if(DelaySlotMode){
						sBCode *pn = getNextCode(p);
						pn->isDelaySlot = TRUE;			/* identify delay slot */
					}else{		/* if delay slot diasbled, 1-cycle stall */
						p->Latency = 2;
					}
					break;
				default:
					p->Index = iUNKNOWN;
					break;
			}
			break;
		case t20a:
			switch(getIntField(p, fT)){
				case 0:		/* T = 0: RTS */
					p->Index = iRTS;

					if(DelaySlotMode){
						sBCode *pn = getNextCode(p);
						pn->isDelaySlot = TRUE;			/* identify delay slot */
					}else{		/* if delay slot diasbled, 1-cycle stall */
						p->Latency = 2;
					}
					break;
				case 1:		/* T = 1: RTI */
					p->Index = iRTI;

					if(DelaySlotMode){
						sBCode *pn = getNextCode(p);
						pn->isDelaySlot = TRUE;			/* identify delay slot */
					}else{		/* if delay slot diasbled, 1-cycle stall */
						p->Latency = 2;
					}
					break;
				default:
					p->Index = iUNKNOWN;
					break;
			}
			break;
		case t26a:
			code[0] = 0x3 & getIntField(p, fLPP);
			code[1] = 0x3 & getIntField(p, fPPP);
			code[2] = 0x3 & getIntField(p, fSPP);

			for(int i = 0; i < 3; i++){
				if(code[i] == 0x3){
					p->Index = iPOP;
					break;
				}else if(code[i] == 0x1){
					p->Index = iPUSH;
					break;
				}
			}
			if(!(p->Index == iPOP || p->Index == iPUSH)){
				p->Index = iUNKNOWN;
			}
			break;
		case t37a:
			switch(getIntField(p, fC)){
				case 0:		/* C = 0: CLRINT */
					p->Index = iCLRINT;
					break;
				case 1:		/* C = 1: SETINT */
					p->Index = iSETINT;
					break;
				default:
					p->Index = iUNKNOWN;
					break;
			}
			break;
		case t30a:
			p->Index = iNOP;
			break;
		case t31a:
			p->Index = iIDLE;
			break;
		case t18a:
			ena = getIntField(p, fENA);
			for(int i = 0; i < 9; i++){
				code[i] = ena & 0x3;
				ena >>= 2;
			}
			for(int i = 0; i < 9; i++){
				if(code[i] == 0x3){
					p->Index = iENA;
					break;
				}else if(code[i] == 0x1){
					p->Index = iDIS;
					break;
				}
			}
			if(!(p->Index == iENA || p->Index == iDIS)){
				p->Index = iUNKNOWN;
			}
			break;
		/* Complex Arithmetic Instructions */
		case t42a:
			switch(getIntField(p, fPOLAR)){
				case 0:		/* POLAR = 0: POLAR.C */
					p->Index = iPOLAR_C;
					break;
				case 1:		/* POLAR = 1: RECT.C */
					p->Index = iRECT_C;
					break;
				default:
					p->Index = iUNKNOWN;
					break;
			}
			break;
		case t42b:
			p->Index = iCONJ_C;
			break;
		/* Unknown Instructions */
		default:
			break;
    }

    NextCode = p->Next;

    //curaddr++;

    return (NextCode);
}


