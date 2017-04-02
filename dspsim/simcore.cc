/*
All Rights Reserved.
*/

/** 
* @file simcore.cc
* @brief Simulator main loop and supporting routines
* @date 2008-09-24
*/

#include <stdio.h>	/* perror() */
#include <stdlib.h>	/* strtol(), exit() */
#include <string.h>	/* strcpy() */
#include <ctype.h>	/* isdigit() */
#include "dspsim.h"
#include "symtab.h"
#include "icode.h"
#include "dmem.h"
#include "optab.h"
#include "simcore.h"
#include "simsupport.h"
#include "stack.h"
#include "secinfo.h"
#include "dspdef.h"

#ifdef VHPI
#include "dsp.h"	/* for vhpi interface */
#endif

/** 
* @brief Simulation main loop (processing simulation commands & running each instruction)
* 
* @param icode User program converted to intermediate code format by lexer & parser
* 
* @return 0 if the simulation successfully ends.
*/
int simCore(sICodeList icode)
{
	int i, k;
	char ch;	
	char tstr[MAX_COMMAND_BUFFER+1];
	sint tdata[8*16];
	int tAddr;
    int lastaddr = 0;
    char lastcommand = 0;
    int ret;
    int addrov = FALSE;
	int master_id;

	sICode *p = icode.FirstNode;
	sICode *oldp;

	p = updatePC(NULL, p);
	dumpRegister(p);

	/* reset warning/error message buffer */
	strcpy(msgbuf, "");

	while(p != NULL) {

		if(VerboseMode || (SimMode == 'S') || ((SimMode == 'C') && (isBreakpoint(p)))){
			/* checking simulation mode */
			if(!isNotRealInst(p->Index)){
				printf("----\n");
				printf("Iteration:%d/%d NextPC:%04X Line:%d Opcode:%s Type:%s\n", 
					ItrCntr+1, ItrMax, p->PMA, p->LineCntr, (char *)sOp[p->Index], 
					sType[p->InstType]);
				if(p->Line){
					printf("Next>>\t%s\n", p->Line);
				}
			}
		}

		if((SimMode == 'S') || ((SimMode == 'C') && (isBreakpoint(p)))){

			if(!QuietMode && !isNotRealInst(p->Index)){
				/* display internal states */
				displayInternalStates();

				printf("\nCurrent simulation mode (<C>ontinue <S>tep into): %c\n", SimMode);
				if(BreakPoint != UNDEFINED)
					printf("Current Breakpoint PC: 0x%04X\n", BreakPoint);

get_command:
				printf("Choose command:\n");
				printf("\t<C>ontinue\n");
				printf("\t<S>tep into\n");
				printf("\t<Enter> to repeat last <C> or <S>\n");
				if(!isCommentLabelInst(p->Index)){
					printf("\tSet <B>reakpoint: <B> ADDR or just <B>\n");
					printf("\t<D>elete current breakpoint\n");
				}
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

							/* find master DP ID */
							for(int j = 0; j < NUMDP; j++) {
								if(rDPMST.dp[j]){
									master_id = j;
									break;
								}
							}

                            lastcommand = 'm';
                            lastaddr = tAddr;

                            printf("--------\n");
                            addrov = FALSE;

							int btAddr = tAddr;		/* backup of tAddr */
							for(k = 0; k < 16; k++){
                                if(tAddr > 0xFFFF) {
                                    addrov = TRUE;
                                    //printf("Address out of range: ignored\n\n");
                                    break;
                                }

                            	for(i = 0; i < 8; i++){
                               		if(tAddr > 0xFFFF) {
                               	    	addrov = TRUE;
                                    	break;
                                	}

                                	tdata[i+k*8] = sBriefRdDataMem(tAddr);
                                	tAddr++;
                            	}
							}

							tAddr = btAddr;
							for(k = 0; k < 16; k++){
								if(tAddr > 0xFFFF) {
                           	    	addrov = TRUE;
                           	    	printf("Address out of range: ignored\n\n");
                                	break;
                            	}

                            	printf("%04x:\t", tAddr);

								for(int j = 0; j < NUMDP; j++) {
									if(rDPENA.dp[j] || SIMD4ForceMode){
                               			if(tAddr > 0xFFFF) {
                                    		addrov = TRUE;
                                    		break;
                                		}

										if(j == master_id){         /* if master */
											printf("[*] ");
										}else if(rDPENA.dp[j]){     /* else, if enabled */
											printf("[%d] ",j);
										}else if(SIMD4ForceMode){	/* if disabled */
											printf("[.] ");
										}

                            			for(i = 0; i < 8; i++){
                                   			printf("%03X ", tdata[i+k*8].dp[j]);
										}
										printf("\t");
									}
                            	}
                            	printf("\n");
                            	tAddr+=8;
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
                            printf("\tEnter breakpoint address in hexadecimal (ex. 01F0): ");

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
                            printf("\tEnter start address in hexadecimal (ex. 01F0): ");

                            fgets(tstr, MAX_COMMAND_BUFFER, stdin);
                            sscanf(tstr, "%X", &tAddr);
                        }
                        if(tAddr > 0xFFFF || tAddr < 0){
                            tAddr &= 0xFFFF;
                        }

						/* find master DP ID */
						for(int j = 0; j < NUMDP; j++) {
							if(rDPMST.dp[j]){
								master_id = j;
								break;
							}
						}

                        lastcommand = 'm';
                        lastaddr = tAddr;

						printf("--------\n");
						addrov = FALSE;

						int btAddr = tAddr;		/* backup of tAddr */
						for(k = 0; k < 16; k++){
							if(tAddr > 0xFFFF) {
                                addrov = TRUE;
                                //printf("Address out of range: ignored\n\n");
                                break;
                            }

                            for(i = 0; i < 8; i++){
                                if(tAddr > 0xFFFF) {
                                    addrov = TRUE;
                                    break;
                                }

                                tdata[i+k*8] = sBriefRdDataMem(tAddr);
                                tAddr++;
                            }
						}

						tAddr = btAddr;
						for(k = 0; k < 16; k++){
							if(tAddr > 0xFFFF) {
                                addrov = TRUE;
                                printf("Address out of range: ignored\n\n");
                                break;
                            }

                            printf("%04x:\t", tAddr);

							for(int j = 0; j < NUMDP; j++) {
								if(rDPENA.dp[j] || SIMD4ForceMode){
                               		if(tAddr > 0xFFFF) {
                                   		addrov = TRUE;
                                    	break;
                                	}

									if(j == master_id){         /* if master */
										printf("[*] ");
									}else if(rDPENA.dp[j]){     /* else, if enabled */
										printf("[%d] ",j);
										}else if(SIMD4ForceMode){	/* if disabled */
											printf("[.] ");
									}

                            		for(i = 0; i < 8; i++){
                                   		printf("%03X ", tdata[i+k*8].dp[j]);
									}
									printf("\t");
                            	}
                            }
                            printf("\n");
                            tAddr+=8;
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
		p = asmSimOneStep(p, icode);
		if(p) p->LastExecuted = oldp;

		if(!isNotRealInst(oldp->Index)) {
			Cycles += oldp->Latency;			/* nominal latency (determined at compile time */
			if(oldp->LatencyAdded){
				Cycles += oldp->LatencyAdded;	/* dynamic latency (added at run time) */
				oldp->LatencyAdded = 0;
			}

			if(VerboseMode || (SimMode == 'S') || ((SimMode == 'C') && (p) && (isBreakpoint(p)))){
				dumpRegister(oldp);
				printRunTimeMessage();
			}
		}
		////////////////////////////
	}

	/* display internal states at the end */
	displayInternalStates();

	return 0;	/** Simulation successfully ended */
}

/** 
* @brief Simulate one assembly source line
* 
* @param *p Pointer to current instruction 
* @param icode User program converted to intermediate code format by lexer & parser
* 
* @return Pointer to next instruction
*/
sICode *asmSimOneStep(sICode *p, sICodeList icode)
{
	int loopCntr;
	sICode *NextCode;
	int z, n, v, c;
	int	isBranchTaken = FALSE;
	int isResetHappened = FALSE;

//	int temp1, temp2;	
//	int temp3, temp4, temp5;
//	cplx	cData;
	scplx   z2, n2, v2, c2;
	sint	so2;
	scplx	sco2;
//	cplx    ct1, ct2, ct3, ct4, ct5;
	
	sint	condMask;
	sint	trueMask = { 1, 1, 1, 1 };

	/* init o2 constant */
	for(int j = 0; j < NUMDP; j++) {
		so2.dp[j] = 0;
		sco2.r.dp[j] = 0;
		sco2.i.dp[j] = 0;
	}

	char *Opr0 = p->Operand[0]; 
	char *Opr1 = p->Operand[1];			
	char *Opr2 = p->Operand[2];			
	char *Opr3 = p->Operand[3];			
	char *Opr4 = p->Operand[4];			
	char *Opr5 = p->Operand[5];			

	switch(p->Index){
		case	iABS:
		case	iNOT:
		case	iINC:
		case	iDEC:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isXOP12(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9c */
					/* [IF COND] ABS/NOT/INC/DEC DREG12, XOP12 */
					/* [IF COND] ABS/NOT/INC/DEC Op0,    Op1   */
					p->InstType = t09c;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2;	/* dummy */
					sProcessALUFunc(p, stemp1, stemp2, Opr0, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iABS_C:
		case	iNOT_C:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isDReg24(p, Opr0) && isXOP24(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9d */
					/* [IF COND] ABS.C/NOT.C DREG24, XOP24[*] */
					/* [IF COND] ABS.C/NOT.C Op0,    Op1[*]   */
					p->InstType = t09d;

					scplx sct1 = scRdReg(Opr1);
					scplx sct2;		/* dummy */

					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct1.i.dp[j] = -sct1.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					sProcessALU_CFunc(p, sct1, sct2, Opr0, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iADD:
		case	iSUB:
		case	iSUBB:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9c */
					/* [IF COND] ADD/SUB/SUBB DREG12, XOP12, YOP12 */
					/* [IF COND] ADD/SUB/SUBB Op0,    Op1,   Op2   */
					p->InstType = t09c;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2 = sRdReg(Opr2);
					sProcessALUFunc(p, stemp1, stemp2, Opr0, condMask);
				} 
			} else if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isInt(Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9e */
					/* [IF COND] ADD/SUB/SUBB DREG12, XOP12, IMM_INT4 */
					/* [IF COND] ADD/SUB/SUBB Op0,    Op1,   Op2   */
					p->InstType = t09e;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2;
					int imm4 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT4);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm4;
					}

					sProcessALUFunc(p, stemp1, stemp2, Opr0, condMask);
				} 
			} else if(isACC32(p, Opr0) && isACC32(p, Opr1) && isACC32(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9g */
					/* [IF COND] ADD/SUB/SUBB ACC32, ACC32, ACC32 */
					/* [IF COND] ADD/SUB/SUBB Op0,   Op1,   Op2   */
					p->InstType = t09g;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2 = sRdReg(Opr2);
					sProcessALUFunc(p, stemp1, stemp2, Opr0, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iADD_C:
		case	iSUB_C:
		case	iSUBB_C:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isXOP24(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9d */
					/* [IF COND] ADD.C/SUB.C/SUBB.C DREG24, XOP24, YOP24[*] */
					/* [IF COND] ADD.C/SUB.C/SUBB.C Op0,    Op1,   Op2[*]   */
					p->InstType = t09d;

					scplx sct1 = scRdReg(Opr1);
					scplx sct2 = scRdReg(Opr2);
					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct2.i.dp[j] = -sct2.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					sProcessALU_CFunc(p, sct1, sct2, Opr0, condMask);
				}
			} else if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isInt(Opr2)){
				if(!isInt(Opr3)){
					printRunTimeError(p->LineCntr, Opr2, 
						"Complex constant should be in a pair, e.g. (3, 4)\n");
					break;
				}
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9f */
					/* [IF COND] ADD.C/SUB.C/SUBB.C DREG24, XOP24[*], IMM_COMPLEX8 */
					/* [IF COND] ADD.C/SUB.C/SUBB.C Op0,    Op1[*],    (Op2, Op3)     */
					p->InstType = t09f;

					scplx sct1 = scRdReg(Opr1);
					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct1.i.dp[j] = -sct1.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					int imm41 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT4);
					int imm42 = getIntImm(p, getIntSymAddr(p, symTable, Opr3), eIMM_INT4);

					scplx sct2;
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = imm41;
						sct2.i.dp[j] = imm42;
					}

					sProcessALU_CFunc(p, sct1, sct2, Opr0, condMask);
				}
			} else if(isACC64(p, Opr0) && isACC64(p, Opr1) && isACC64(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9h */
					/* [IF COND] ADD.C/SUB.C/SUBB.C ACC64, ACC64, ACC64[*] */
					/* [IF COND] ADD.C/SUB.C/SUBB.C Op0,   Op1,   Op2      */
					p->InstType = t09h;

					scplx sct1 = scRdReg(Opr1);
					scplx sct2 = scRdReg(Opr2);

					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct2.i.dp[j] = -sct2.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					sProcessALU_CFunc(p, sct1, sct2, Opr0, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			} 
			break;
		case	iADDC:
		case	iSUBC:
		case	iSUBBC:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9c */
					/* [IF COND] ADDC/SUBC/SUBBC DREG12, XOP12, YOP12 */
					/* [IF COND] ADDC/SUBC/SUBBC Op0,    Op1,   Op2   */
					p->InstType = t09c;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2 = sRdReg(Opr2);
					sProcessALUFunc(p, stemp1, stemp2, Opr0, condMask);
				}
			} else if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isInt(Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9e */
					/* [IF COND] ADDC/SUBC/SUBBC DREG12, XOP12, IMM_INT4 */
					/* [IF COND] ADDC/SUBC/SUBBC Op0,    Op1,    Op2        */
					p->InstType = t09e;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2;
					int imm4 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT4);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm4;
					}
					sProcessALUFunc(p, stemp1, stemp2, Opr0, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			} 
			break;
		case	iADDC_C:
		case	iSUBC_C:
		case	iSUBBC_C:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isXOP24(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9d */
					/* [IF COND] ADDC.C DREG24, XOP24, YOP24[*] */
					/* [IF COND] ADDC.C Op0,    Op1,   Op2[*]   */
					p->InstType = t09d;

					scplx sct1 = scRdReg(Opr1);
					scplx sct2 = scRdReg(Opr2);
					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct2.i.dp[j] = -sct2.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					sProcessALU_CFunc(p, sct1, sct2, Opr0, condMask);
				} 
			} else if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isInt(Opr2)){
				if(!isInt(Opr3)){
					printRunTimeError(p->LineCntr, Opr2, 
						"Complex constant should be in a pair, e.g. (3, 4)\n");
					break;
				}

				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9f */
					/* [IF COND] ADDC.C DREG24, XOP24[*], IMM_COMPLEX8 */
					/* [IF COND] ADDC.C Op0,    Op1[*],    (Op2, Op3)     */
					p->InstType = t09f;

					scplx sct1 = scRdReg(Opr1);
					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct1.i.dp[j] = -sct1.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					int imm41 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT4);
					int imm42 = getIntImm(p, getIntSymAddr(p, symTable, Opr3), eIMM_INT4);

					scplx sct2;
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = imm41;
						sct2.i.dp[j] = imm42;
					}

					sProcessALU_CFunc(p, sct1, sct2, Opr0, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			} 
			break;
		case	iAND:
		case	iOR:
		case	iXOR:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9c */
					/* [IF COND] AND DREG12, XOP12, YOP12 */
					/* [IF COND] AND Op0,    Op1,   Op2   */
					p->InstType = t09c;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2 = sRdReg(Opr2);
					sProcessALUFunc(p, stemp1, stemp2, Opr0, condMask);
				} 
			} else if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isInt(Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9i */
					/* [IF COND] AND DREG12, XOP12, IMM_UINT4 */
					/* [IF COND] AND Op0,    Op1,   Op2        */
					p->InstType = t09i;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2;
					int imm4 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_UINT4);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm4;
					}

					sProcessALUFunc(p, stemp1, stemp2, Opr0, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iASHIFT:
		case	iASHIFTOR:
		case	iLSHIFT:
		case	iLSHIFTOR:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isACC32(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2) && Opr3){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 16a */
					/* [IF COND] ASHIFT ACC32, XOP12, YOP12 (|HI, LO, HIRND, LORND|) */
					/* [IF COND] ASHIFT Op0,   Op1,   Op2     Op3                    */
					p->InstType = t16a;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2 = sRdReg(Opr2);

					for(int j = 0; j < NUMDP; j++) {
						if(stemp2.dp[j] > 32) stemp2.dp[j] = 32;
						else if(stemp2.dp[j] < -32) stemp2.dp[j] = -32;
					}

					sProcessSHIFTFunc(p, stemp1, stemp2, Opr0, Opr3, condMask);
				} 
			} else if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2) && Opr3){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 16e */
					/* [IF COND] ASHIFT DREG12, XOP12, YOP12 (|NORND, RND|) */
					/* [IF COND] ASHIFT Op0,   Op1,   Op2      Op3          */
					p->InstType = t16e;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2 = sRdReg(Opr2);

					for(int j = 0; j < NUMDP; j++) {
						if(stemp2.dp[j] > 32) stemp2.dp[j] = 32;
						else if(stemp2.dp[j] < -32) stemp2.dp[j] = -32;
					}

					sProcessSHIFTFunc(p, stemp1, stemp2, Opr0, Opr3, condMask);
				} 
			} else if(isACC32(p, Opr0) && isXOP12(p, Opr1) && isInt(Opr2) && Opr3){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 15a */
					/* [IF COND] ASHIFT ACC32, XOP12, IMM_INT5 (|HI, LO, HIRND, LORND|) */
					/* [IF COND] ASHIFT Op0,   Op1,   Op2        Op3                    */
					p->InstType = t15a;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2;
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm5;
					}

					sProcessSHIFTFunc(p, stemp1, stemp2, Opr0, Opr3, condMask);
				}
			} else if(isACC32(p, Opr0) && isACC32(p, Opr1) && isInt(Opr2) && Opr3){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 15c */
					/* [IF COND] ASHIFT ACC32, ACC32, IMM_INT6 (|NORND, RND|) */
					/* [IF COND] ASHIFT Op0,   Op1,   Op2        Op3          */
					p->InstType = t15c;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2;
					int imm6 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT6);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm6;
					}

					sProcessSHIFTFunc(p, stemp1, stemp2, Opr0, Opr3, condMask);
				}
			} else if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isInt(Opr2) && Opr3){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 15e */
					/* [IF COND] ASHIFT DREG12, XOP12, IMM_INT5 (|NORND, RND|) */
					/* [IF COND] ASHIFT Op0,   Op1,   Op2        Op3           */
					p->InstType = t15e;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2;
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm5;
					}

					sProcessSHIFTFunc(p, stemp1, stemp2, Opr0, Opr3, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iASHIFT_C:
		case	iASHIFTOR_C:
		case	iLSHIFT_C:
		case	iLSHIFTOR_C:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isACC64(p, Opr0) && isXOP24(p, Opr1) && isXOP12(p, Opr2) && Opr3){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 16b */
					/* [IF COND] ASHIFT.C ACC64, XOP24, YOP12 (|HI, LO, HIRND, LORND|) */
					/* [IF COND] ASHIFT.C Op0,   Op1,   Op2     Op3                    */
					p->InstType = t16b;

					scplx sct1, sct2;
					sct1 = scRdReg(Opr1);
					sct2.r = sct2.i = sRdReg(Opr2);

					for(int j = 0; j < NUMDP; j++) {
						if(sct2.r.dp[j] > 32) sct2.r.dp[j] = sct2.i.dp[j] = 32;
						else if(sct2.r.dp[j] < -32) sct2.r.dp[j] = sct2.i.dp[j] = -32;
					}

					sProcessSHIFT_CFunc(p, sct1, sct2, Opr0, Opr3, condMask);
				}
			} else if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isXOP12(p, Opr2) && Opr3){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 16f */
					/* [IF COND] ASHIFT.C DREG24, XOP24, YOP12 (|NORND, RND|) */
					/* [IF COND] ASHIFT.C Op0,    Op1,   Op2     Op3                    */
					p->InstType = t16f;

					scplx sct1, sct2;
					sct1 = scRdReg(Opr1);
					sct2.r = sct2.i = sRdReg(Opr2);

					for(int j = 0; j < NUMDP; j++) {
						if(sct2.r.dp[j] > 32) sct2.r.dp[j] = sct2.i.dp[j] = 32;
						else if(sct2.r.dp[j] < -32) sct2.r.dp[j] = sct2.i.dp[j] = -32;
					}

					sProcessSHIFT_CFunc(p, sct1, sct2, Opr0, Opr3, condMask);
				}
			} else if(isACC64(p, Opr0) && isXOP24(p, Opr1) && isInt(Opr2) && Opr3){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 15b */
					/* [IF COND] ASHIFT.C ACC64, XOP24, IMM_INT5 (|HI, LO, HIRND, LORND|) */
					/* [IF COND] ASHIFT.C Op0,   Op1,   Op2        Op3      */
					p->InstType = t15b;

					scplx sct1, sct2;
					sct1 = scRdReg(Opr1);
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = sct2.i.dp[j] = imm5;
					}

					sProcessSHIFT_CFunc(p, sct1, sct2, Opr0, Opr3, condMask);
				}
			} else if(isACC64(p, Opr0) && isACC64(p, Opr1) && isInt(Opr2) && Opr3){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 15d */
					/* [IF COND] ASHIFT.C ACC64, ACC64, IMM_INT6 (|NORND, RND|) */
					/* [IF COND] ASHIFT.C Op0,   Op1,   Op2        Op3          */
					p->InstType = t15d;

					scplx sct1, sct2;
					sct1 = scRdReg(Opr1);
					int imm6 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT6);
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = sct2.i.dp[j] = imm6;
					}

					sProcessSHIFT_CFunc(p, sct1, sct2, Opr0, Opr3, condMask);
				}
			} else if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isInt(Opr2) && Opr3){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 15f */
					/* [IF COND] ASHIFT.C DREG24, XOP24, IMM_INT5 (|NORRND, RND|) */
					/* [IF COND] ASHIFT.C Op0,    Op1,   Op2        Op3           */
					p->InstType = t15f;

					scplx sct1, sct2;
					sct1 = scRdReg(Opr1);
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = sct2.i.dp[j] = imm5;
					}

					sProcessSHIFT_CFunc(p, sct1, sct2, Opr0, Opr3, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iCALL:
			if(!p->Cond){
				if(isIReg(p, Opr0)){
					/* type 19a */
					/* [IF TRUE] CALL (<IREG>) */
					/* [IF TRUE] CALL (Op0   ) */
					p->InstType = t19a;

					int tAddr = 0xFFFF & RdReg(Opr0);
					
					if(!DelaySlotMode){		/* if delay slot not enabled */
						stackPush(&PCStack, p->PMA +1);	/* return addr */
						p->LatencyAdded = 3; 	/* 4 cycles if taken */
					}else{					/* if delay slot enabled */
						stackPush(&PCStack, p->PMA +2);	/* return addr */
					}
					NextCode = sICodeListSearch(&iCode, tAddr);
					isBranchTaken = TRUE;

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						if(pn->isDelaySlot){
							pn->BrTarget = NextCode;	/* save branch target address in delay slot */
						}else{		/* error */
							printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
								"Delay slot processing error. Please report.\n");
						}
					}

					sFlagEffect(p->InstType, so2, so2, so2, so2, trueMask);
				}else {
					/* type 10b */
					/* CALL <IMM_INT16> */
					/* CALL  Opr0       */
					p->InstType = t10b;

					if(!DelaySlotMode){		/* if delay slot not enabled */
						stackPush(&PCStack, p->PMA +1);	/* return addr */
						p->LatencyAdded = 3; 	/* 4 cycles if taken */
					}else{					/* if delay slot enabled */
						stackPush(&PCStack, p->PMA +2);	/* return addr */
					}
					NextCode = sICodeListSearch(&iCode, getLabelAddr(p, symTable, Opr0));
					isBranchTaken = TRUE;
			
					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						if(pn->isDelaySlot){
							pn->BrTarget = NextCode;	/* save branch target address in delay slot */
						}else{		/* error */
							printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
								"Delay slot processing error. Please report.\n");
						}
					}

					sFlagEffect(p->InstType, so2, so2, so2, so2, trueMask);
				}
			}else if(ifCond(p->Cond)){
				if(isIReg(p, Opr0)){
					/* type 19a */
					/* [IF COND] CALL (<IREG>) */
					/* [IF COND] CALL (Op0   ) */
					p->InstType = t19a;

					int tAddr = 0xFFFF & RdReg(Opr0);
					
					if(!DelaySlotMode){		/* if delay slot not enabled */
						stackPush(&PCStack, p->PMA +1);	/* return addr */
						p->LatencyAdded = 3; 	/* 4 cycles if taken */
					}else{					/* if delay slot enabled */
						stackPush(&PCStack, p->PMA +2);	/* return addr */
					}
					NextCode = sICodeListSearch(&iCode, tAddr);
					isBranchTaken = TRUE;

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						if(pn->isDelaySlot){
							pn->BrTarget = NextCode;	/* save branch target address in delay slot */
						}else{		/* error */
							printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
								"Delay slot processing error. Please report.\n");
						}
					}

					sFlagEffect(p->InstType, so2, so2, so2, so2, trueMask);
				} else {
					/* type 10a */
					/* [IF COND] CALL <IMM_INT13> */
					/* [IF COND] CALL  Opr0       */
					p->InstType = t10a;

					if(!DelaySlotMode){		/* if delay slot not enabled */
						stackPush(&PCStack, p->PMA +1);	/* return addr */
						p->LatencyAdded = 3; 	/* 4 cycles if taken */
					}else{					/* if delay slot enabled */
						stackPush(&PCStack, p->PMA +2);	/* return addr */
					}
					NextCode = sICodeListSearch(&iCode, getLabelAddr(p, symTable, Opr0));
					isBranchTaken = TRUE;

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						if(pn->isDelaySlot){
							pn->BrTarget = NextCode;	/* save branch target address in delay slot */
						}else{		/* error */
							printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
								"Delay slot processing error. Please report.\n");
						}
					}

					sFlagEffect(p->InstType, so2, so2, so2, so2, trueMask);
				}
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iCLRACC:
			if(isACC32(p, Opr0)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 41c */
					/* [IF COND] CLRACC ACC32 */
					/* [IF COND] CLRACC Op0   */
					p->InstType = t41c;
					sWrReg(Opr0, so2, condMask);
					sFlagEffect(p->InstType, so2, so2, so2, so2, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iCLRACC_C:
			if(isACC64(p, Opr0)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 41d */
					/* [IF COND] CLRACC.C ACC64 */
					/* [IF COND] CLRACC.C Op0   */
					p->InstType = t41d;
					scWrReg(Opr0, so2, so2, condMask);
					scFlagEffect(p->InstType, sco2, sco2, sco2, sco2, condMask);
				} 
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iCLRBIT:
		case	iSETBIT:
		case	iTGLBIT:
		case	iTSTBIT:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isInt(Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 9i */
					/* [IF COND] CLRBIT/SETBIT DREG12, XOP12, IMM_UINT4   */
					/* [IF COND] CLRBIT/SETBIT Op0,    Op1,   Op2 */
					p->InstType = t09i;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2;

					int imm4 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_UINT4);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm4;
					}

					sProcessALUFunc(p, stemp1, stemp2, Opr0, condMask);
				} 
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
        case    iCLRINT:
            if(isInt(Opr0)) {
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
                	/* type 37a */
               		/* [IF COND] CLRINT IMM_UINT4   */
               		/* [IF COND] CLRINT Op0 */
                	p->InstType = t37a;

                	//temp1 = 0x0F & getIntSymAddr(p, symTable, Opr0);
					int temp1 = getIntImm(p, getIntSymAddr(p, symTable, Opr0), eIMM_UINT4);

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
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
            }
            break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iCONJ_C:
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 42b */
					/* [IF COND] CONJ.C DREG24, XOP24 */
					/* [IF COND] CONJ.C Op0,    Op1    */
					p->InstType = t42b;

					scplx sct1 = scRdReg(Opr1);
					for(int j = 0; j < NUMDP; j++) {
						sct1.i.dp[j] = -sct1.i.dp[j];	/* CONJ(*) modifier */
					}
					scWrReg(Opr0, sct1.r, sct1.i, condMask);	
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iCP:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isDReg12(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 17a */
					/* [IF COND] CP DREG12, DREG12 */
					/* [IF COND] CP Opr0, Opr1 */
					p->InstType = t17a;

					//WrReg(Opr0, RdReg(Opr1));
					sint tData = sRdReg(Opr1);
					sWrReg(Opr0, tData, condMask);
				}
			} else if(isDReg12(p, Opr0) && isRReg16(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 17b */
					/* [IF COND] CP DREG12, RREG16 */
					/* [IF COND] CP Opr0, Opr1 */
					p->InstType = t17b;

					/* MSBs of 16-bit source should be truncated for 12-bit destination. */
					//WrReg(Opr0, 0x0FFF & RdReg(Opr1));
					sint tData = sRdReg(Opr1);
					for(int j = 0; j < NUMDP; j++) {
						tData.dp[j] &= 0x0FFF;
					}
					sWrReg(Opr0, tData, condMask);
				}
			} else if(isRReg16(p, Opr0) && isDReg12(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 17b */
					/* [IF COND] CP RREG16, DREG12 */
					/* [IF COND] CP Opr0, Opr1 */
					p->InstType = t17b;

					/* MSBs of 16-bit destination is automatically sign-extended. */
					if(!isReadOnlyReg(p, Opr0)){
						//WrReg(Opr0, RdReg(Opr1));
						sint tData = sRdReg(Opr1);
						sWrReg(Opr0, tData, condMask);
					}
				}
			} else if(isRReg16(p, Opr0) && isRReg16(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 17g */
					/* [IF COND] CP RREG16, RREG16 */
					/* [IF COND] CP Opr0, Opr1 */
					p->InstType = t17g;

					if(!isReadOnlyReg(p, Opr0)){
						//WrReg(Opr0, RdReg(Opr1));
						sint tData = sRdReg(Opr1);
						sWrReg(Opr0, tData, condMask);
					}
				}
			} else if(isACC32(p, Opr0) && isACC32(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 17c */
					/* [IF COND] CP ACC32, ACC32 */
					/* [IF COND] CP Opr0,  Opr1 */
					p->InstType = t17c;

					//WrReg(Opr0, RdReg(Opr1));
					sint tData = sRdReg(Opr1);
					sWrReg(Opr0, tData, condMask);
				}
			} else if(isACC32(p, Opr0) && isDReg12(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 17h */
					/* [IF COND] CP ACC32, DReg12 */
					/* [IF COND] CP Opr0,  Opr1 */
					p->InstType = t17h;

					//WrReg(Opr0, RdReg(Opr1));
					sint tData = sRdReg(Opr1);
					sWrReg(Opr0, tData, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iCP_C:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isDReg24(p, Opr0) && isDReg24(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 17d */
					/* [IF COND] CP.C DREG24, DREG24 */
					/* [IF COND] CP.C Opr0, Opr1 */
					p->InstType = t17d;

					scplx sct1 = scRdReg(Opr1);
					scWrReg(Opr0, sct1.r, sct1.i, condMask);
				}
			} else if(isDReg24(p, Opr0) && isRReg16(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 17e */
					/* [IF COND] CP.C DREG24, RREG16 */
					/* [IF COND] CP.C Opr0, Opr1 */
					p->InstType = t17e;

					sint val = sRdReg(Opr1);				/* read 16-bit source */
					sint lsbVal, msbVal;
					for(int j = 0; j < NUMDP; j++) {
						lsbVal.dp[j] = 0x0FFF & val.dp[j];				/* get LSB 12-bit */
						msbVal.dp[j] = ((0x0F000 & val.dp[j]) >> 12);	/* get MSB 4-bit */
						if(isNeg16b(val.dp[j])) msbVal.dp[j] |= 0xFF0;		/* sign-extension */
					}

					scWrReg(Opr0, lsbVal, msbVal, condMask);			/* write to complex pairs */
				}
			} else if(isRReg16(p, Opr0) && isDReg24(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 17e */
					/* [IF COND] CP.C RREG16, DREG24 */
					/* [IF COND] CP.C Opr0, Opr1 */
					p->InstType = t17e;

					scplx sct1 = scRdReg(Opr1);				/* read 24-bit source */
					sint lsbVal, msbVal, val;
					for(int j = 0; j < NUMDP; j++) {
						lsbVal.dp[j] = 0x0FFF & sct1.r.dp[j];		/* get LSB 12-bit */
						msbVal.dp[j] = 0x0F   & sct1.i.dp[j];		/* get MSB 4-bit */
						val.dp[j] = ((msbVal.dp[j] << 12) | lsbVal.dp[j]);
					}

					if(!isReadOnlyReg(p, Opr0))
						sWrReg(Opr0, val, condMask);			/* write to 16-bit register */
				}
			} else if(isACC64(p, Opr0) && isACC64(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 17f */
					/* [IF COND] CP.C ACC64, ACC64 */
					/* [IF COND] CP.C Opr0,  Opr1 */
					p->InstType = t17f;

					scplx sct1 = scRdReg(Opr1);
					scWrReg(Opr0, sct1.r, sct1.i, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iCPXI:
			if(isDReg12(p, Opr0) && isXReg12(p, Opr1) && isInt(Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 49a */
					/* [IF COND] CPXI DREG12, XREG12, IMM_UINT2, IMM_UINT4 */
					/* [IF COND] CPXI Opr0,   Opr1,   Opr2,      Opr3      */
					p->InstType = t49a;

					//WrReg(Opr0, RdReg(Opr1));
					int imm2 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_UINT2);
					int imm4 = getIntImm(p, getIntSymAddr(p, symTable, Opr3), eIMM_UINT4);
					sint tData = sRdXReg(Opr1, imm2, imm4);	/* cross-path register read */
					sWrReg(Opr0, tData, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iCPXI_C:
			if(isDReg24(p, Opr0) && isXReg24(p, Opr1) && isInt(Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 49b */
					/* [IF COND] CPXI.C DREG24, XREG24, IMM_UINT2, IMM_UINT4 */
					/* [IF COND] CPXI.C Opr0,   Opr1,   Opr2,      Opr3      */
					p->InstType = t49b;

					int imm2 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_UINT2);
					int imm4 = getIntImm(p, getIntSymAddr(p, symTable, Opr3), eIMM_UINT4);
					scplx tData = scRdXReg(Opr1, imm2, imm4);	/* cross-path register read */
					scWrReg(Opr0, tData.r, tData.i, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iCPXO:
			if(isXReg12(p, Opr0) && isDReg12(p, Opr1) && isInt(Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 49c	*/
					/* [IF COND] CPXO XREG12, DREG12, IMM_UINT2, IMM_UINT4 */
					/* [IF COND] CPXO Opr0,   Opr1,   Opr2,      Opr3      */
					p->InstType = t49c;

					int imm2 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_UINT2);
					int imm4 = getIntImm(p, getIntSymAddr(p, symTable, Opr3), eIMM_UINT4);
					sint tData = sRdReg(Opr1);	
					sWrXReg(Opr0, tData, imm2, imm4, condMask);	/* cross-path register write */
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iCPXO_C:
			if(isXReg24(p, Opr0) && isDReg24(p, Opr1) && isInt(Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 49d */
					/* [IF COND] CPXO.C XREG24, DREG24, IMM_UINT2, IMM_UINT4 */
					/* [IF COND] CPXO.C Opr0,   Opr1,   Opr2,      Opr3      */
					p->InstType = t49d;

					int imm2 = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_UINT2);
					int imm4 = getIntImm(p, getIntSymAddr(p, symTable, Opr3), eIMM_UINT4);
					scplx tData = scRdReg(Opr1);	
					scWrXReg(Opr0, tData.r, tData.i, imm2, imm4, condMask);	/* cross-path register write */
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iDIS:
			if(p->OperandCounter){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				if(ifCond(p->Cond)){
					/* type 18a */
					/* [IF COND] DIS |SR, BR, OL, AS, MM, TI, SD, MB, INT|, ... */
					/* [IF COND] DIS Opr0 [, Opr1][, Opr2], ... */
					p->InstType = t18a;

					for(int i = 0; i < p->OperandCounter; i++){
						if(!strcasecmp(p->Operand[i], "SR")){
							rMstat.SR = FALSE;
						} else if(!strcasecmp(p->Operand[i], "BR")){
							rMstat.BR = FALSE;
						} else if(!strcasecmp(p->Operand[i], "OL")){
							rMstat.OL = FALSE;
						} else if(!strcasecmp(p->Operand[i], "AS")){
							rMstat.AS = FALSE;
						} else if(!strcasecmp(p->Operand[i], "MM")){
							rMstat.MM = FALSE;
						} else if(!strcasecmp(p->Operand[i], "TI")){
							rMstat.TI = FALSE;
						} else if(!strcasecmp(p->Operand[i], "SD")){
							rMstat.SD = FALSE;
						} else if(!strcasecmp(p->Operand[i], "MB")){
							rMstat.MB = FALSE;
						} else if(!strcasecmp(p->Operand[i], "INT")){
							rICNTL.GIE = FALSE;
						} 
					}
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iDIVS:
			if(isReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 23a */
					/* [IF COND] DIVS REG12, XOP12, YOP12 */
					/* [IF COND] DIVS Op0,   Op1,   Op2   */
					/* Input:  (Op1|Op0)(Dividend 24b), Op2(Divisor 12b) */
					/* Output: Op0(Quotient 12b), Op1(Remainder 12b)     */
					/* Before DIVS, lower 12b of dividend must be loaded innto Op0 */

					/* Note: In ADSP 219x,                              */
					/* Input:  (AF|AY0)(Dividend 24b), AX0(Divisor 12b) */
					/* Output: AY0(Quotient 12b), AF(Remainder 12b)     */
					/* for details, refer to 2-25~2-29 of ADSP-219x     */
					/* DSP Hardware Reference                           */

					p->InstType = t23a;
	
					sint stemp0 = sRdReg(Opr0);	/* Lower 12b of dividend */
					sint stemp1 = sRdReg(Opr1);		/* Upper 12b of dividend */
					sint stemp2 = sRdReg(Opr2);		/* Divisor 12b */

					for(int j = 0; j < NUMDP; j++) {
						sint msb0, msb1;

						msb0.dp[j] = isNeg12b(stemp1.dp[j]) ^ isNeg12b(stemp2.dp[j]);
						rAstatR.AQ.dp[j] = msb0.dp[j];			/* set AQ */

						msb1.dp[j] = isNeg12b(stemp0.dp[j]);	/* get msb of AY0 */
						stemp1.dp[j] = ((stemp1.dp[j] << 1) | msb1.dp[j]);	/* AF  = (AF << 1) | (msb of AY0) */
						stemp0.dp[j] = ((stemp0.dp[j] << 1) | msb0.dp[j]);  /* AY0 = (AY0 << 1) | (new AQ)    */
					}

					sWrReg(Opr1, stemp1, condMask);			/* update AF  */
					sWrReg(Opr0, stemp0, condMask);			/* update AY0 */
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iDIVQ:
			if(isReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 23a */
					/* [IF COND] DIVQ REG12, XOP12, YOP12 */
					/* [IF COND] DIVQ Op0,    Op1,   Op2   */
					/* Input:  (Op1|Op0)(Dividend 24b), Op2(Divisor 12b) */
					/* Output: Op0(Quotient 12b), Op1(Remainder 12b)     */
					/* Before DIVQ, lower 12b of dividend must be loaded innto Op0 */

					/* Note: In ADSP 219x,                              */
					/* Input:  (AF|AY0)(Dividend 24b), AX0(Divisor 12b) */
					/* Output: AY0(Quotient 12b), AF(Remainder 12b)     */
					/* for details, refer to 2-25~2-29 of ADSP-219x     */
					/* DSP Hardware Reference                           */

					p->InstType = t23a;

					sint stemp0 = sRdReg(Opr0);	/* Lower 12b of dividend: AY0 */
					sint stemp1 = sRdReg(Opr1);		/* Upper 12b of dividend: AF  */
					sint stemp2 = sRdReg(Opr2);		/* Divisor 12b: AX0 */
					sint stemp5;

					for(int j = 0; j < NUMDP; j++) {
						sint msb0, msb1;

						if(rAstatR.AQ.dp[j]){
							stemp5.dp[j] = stemp1.dp[j] + stemp2.dp[j];	/* R = Y+X if AQ = 1 */
						}else{
							stemp5.dp[j] = stemp1.dp[j] - stemp2.dp[j];	/* R = Y-X if AQ = 0 */
						}

						msb0.dp[j] = isNeg12b(stemp5.dp[j]) ^ isNeg12b(stemp2.dp[j]);
						rAstatR.AQ.dp[j] = msb0.dp[j];			/* set AQ */

						msb1.dp[j] = isNeg12b(stemp0.dp[j]);	/* get msb of AY0 */
						stemp1.dp[j] = (((0x7FF & stemp5.dp[j]) << 1) | msb1.dp[j]);	/* AF  = (R << 1) | (msb of AY0) */
						stemp0.dp[j] = ((stemp0.dp[j] << 1) | (!msb0.dp[j]));  /* AY0 = (AY0 << 1) | (new AQ)    */
					}

					sWrReg(Opr1, stemp1, condMask);			/* update AF  */
					sWrReg(Opr0, stemp0, condMask);			/* update AY0 */
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iDO:
			/* type 11a */
			/* DO <IMM_UINT12> UNTIL [<TERM>] */
			/* DO Opr1        UNTIL [Opr0  ] */
			if(Opr1 != NULL){
				p->InstType = t11a;

				int LoopBeginAddr = p->PMA + 1;
				int LoopEndAddr   = getLabelAddr(p, symTable, Opr1);

				/* latency restriction: if single-instruction loop-body, DO-UNTIL becomes 2-cycle. */
				if(LoopBeginAddr == LoopEndAddr) p->LatencyAdded = 1;

				stackPush(&LoopBeginStack, LoopBeginAddr);	/* loop begin addr */
				stackPush(&LoopEndStack, LoopEndAddr);	/* loop end addr */
				if(!Opr0 || !strcasecmp(Opr0, "FOREVER")){	/* if FOREVER */
														/* if Opr0 omitted, it's also FOREVER */
					loopCntr = RdReg2(p, "_CNTR");
					stackPush(&LoopCounterStack, loopCntr);

					WrReg("_LPEVER", 1);
					stackPush(&LPEVERStack, 1);			//added 2009.05.22
				} else if(!strcasecmp(Opr0, "CE")){	/* if CE */
					loopCntr = RdReg2(p, "_CNTR");
					stackPush(&LoopCounterStack, loopCntr);

					WrReg("_LPEVER", 0);
					stackPush(&LPEVERStack, 0);			//added 2009.05.22
				}
				sFlagEffect(p->InstType, so2, so2, so2, so2, trueMask);
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iDPID:
			if(isDReg12(p, Opr0)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				if(ifCond(p->Cond)){
					/* type 18c */
					/* [IF COND] DPID DREG12 */
					p->InstType = t18c;

					sWrReg(Opr0, sRdReg("DID"), trueMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iENA:
			if(p->OperandCounter){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				if(ifCond(p->Cond)){
					/* type 18a */
					/* [IF COND] ENA |SR, BR, OL, AS, MM, TI, SD, MB, INT|, ... */
					/* [IF COND] ENA Opr0 [, Opr1][, Opr2], ... */
					p->InstType = t18a;

					for(int i = 0; i < p->OperandCounter; i++){
						if(!strcasecmp(p->Operand[i], "SR")){
							rMstat.SR = TRUE;
						} else if(!strcasecmp(p->Operand[i], "BR")){
							rMstat.BR = TRUE;
						} else if(!strcasecmp(p->Operand[i], "OL")){
							rMstat.OL = TRUE;
						} else if(!strcasecmp(p->Operand[i], "AS")){
							rMstat.AS = TRUE;
						} else if(!strcasecmp(p->Operand[i], "MM")){
							rMstat.MM = TRUE;
						} else if(!strcasecmp(p->Operand[i], "TI")){
							rMstat.TI = TRUE;
						} else if(!strcasecmp(p->Operand[i], "SD")){
							rMstat.SD = TRUE;
						} else if(!strcasecmp(p->Operand[i], "MB")){
							rMstat.MB = TRUE;
						} else if(!strcasecmp(p->Operand[i], "INT")){
							rICNTL.GIE = TRUE;
						} 
					}
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iENADP:
			if(p->OperandCounter){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				if(ifCond(p->Cond)){
					/* type 18b */
					/* [IF COND] ENADP IMM_UINT2, … (up to 4 IMM_UINT2 in parallel) */
					/* [IF COND] ENADP Opr0 [, Opr1][, Opr2][, Opr3] */
					p->InstType = t18b;

					int mst_mask, ena_mask = 0;
					int mst_id, ena_id;
					
					mst_id = atoi(p->Operand[0]);
					mst_mask = (1<<mst_id);

					for(int i = 0; i < p->OperandCounter; i++){
						ena_id = atoi(p->Operand[i]);  
						ena_mask += (1<<ena_id);
					}

					WrReg("_DSTAT0", ena_mask);
					WrReg("_DSTAT1", mst_mask);

				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iEXP:
			/* note: iEXP cannot be used in multifunction instructions */
			if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && Opr2){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 16c */
					/* [IF COND] EXP DREG12, XOP12 (|HIX, HI, LO|) */
					/* [IF COND] EXP Op0,    Op1,     Op2           */
					p->InstType = t16c;

					sint stemp1 = sRdReg(Opr1);
					sint stemp5;
					for(int j = 0; j < NUMDP; j++) {
						stemp5.dp[j] = 0;
					}

					if(!strcasecmp(Opr2, "HI") || !strcasecmp(Opr2, "HIX")){
						sint msb;
						sint mask;

						for(int j = 0; j < NUMDP; j++) {
							msb.dp[j] = 0x800 & stemp1.dp[j];

							mask.dp[j] = 0x400;

							for(int i = 0; i < 11; i++){
								if(stemp1.dp[j] & mask.dp[j]){	/* if current bit is 1 */
									if(msb.dp[j]) stemp5.dp[j]++;
									else
										break;
								}else{				/* if current bit is 0 */
									if(!msb.dp[j]) stemp5.dp[j]++;
									else
										break;
								}
								mask.dp[j] >>= 1;
							}

							/* if HIX, check AV */
							if(!strcasecmp(Opr2, "HIX")){
								if(rAstatR.AV.dp[j]){
									stemp5.dp[j] = -1;		/* indicates overflow happened 
												       just before this instruction */
								}
							}

							/* set SS flag */
							if(!rAstatR.AV.dp[j]){			/* if AV == 0 */
								rAstatR.SS.dp[j] = isNeg12b(stemp1.dp[j]);
							}else{					/* if AV == 1 */
								if(rAstatR.AV.dp[j] && !strcasecmp(Opr2, "HIX"))
									rAstatR.SS.dp[j] = (isNeg12b(stemp1.dp[j])? 0: 1);
							}
						}

						sWrReg(Opr0, stemp5, condMask);
					}else if(!strcasecmp(Opr2, "LO")){
						sint stemp5 = sRdReg(Opr0);

						for(int j = 0; j < NUMDP; j++) {
							if(stemp5.dp[j] == 11){	/* if upper word is all sign bits */
								sint sb;
								sint mask;

								sb.dp[j] = rAstatR.SS.dp[j];		/* sign bit of previous word */
								mask.dp[j] = 0x800;

								for(int i = 0; i < 12; i++){
									if(stemp1.dp[j] & mask.dp[j]){	/* if current bit is 1 */
										if(sb.dp[j]) stemp5.dp[j]++;
										else
											break;
									}else{				/* if current bit is 0 */
										if(!sb.dp[j]) stemp5.dp[j]++;
										else
											break;
									}
									mask.dp[j] >>= 1;
								}
							}else{				/* else return previous (HI/HIX) result */
								;
							}
							sWrReg(Opr0, stemp5, condMask);
						}
					}else{
						printRunTimeError(p->LineCntr, Opr2, 
							"Invalid option for EXP instruction.\n");
						break;
					}
				} 
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iEXP_C:
			/* note: iEXP_C cannot be used in multifunction instructions */
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && Opr2){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 16d */
					/* [IF COND] EXP.C DREG24, XOP24 (|HIX, HI, LO|) */
					/* [IF COND] EXP.C Op0,    Op1,     Op2           */
					p->InstType = t16d;

					scplx sct1 = scRdReg(Opr1);
					scplx sct5;
					for(int j = 0; j < NUMDP; j++) {
						sct5.r.dp[j] = sct5.i.dp[j] = 0;
					}

					if(!strcasecmp(Opr2, "HI") || !strcasecmp(Opr2, "HIX")){
						sint msb;
						sint mask;

						for(int j = 0; j < NUMDP; j++) {
							/* real part */
							msb.dp[j] = 0x800 & sct1.r.dp[j];
							mask.dp[j] = 0x400;

							for(int i = 0; i < 11; i++){
								if(sct1.r.dp[j] & mask.dp[j]){	/* if current bit is 1 */
									if(msb.dp[j]) sct5.r.dp[j]++;
									else
										break;
								}else{				/* if current bit is 0 */
									if(!msb.dp[j]) sct5.r.dp[j]++;
									else
										break;
								}
								mask.dp[j] >>= 1;
							}

							/* imaginary part */
							msb.dp[j] = 0x800 & sct1.i.dp[j];
							mask.dp[j] = 0x400;

							for(int i = 0; i < 11; i++){
								if(sct1.i.dp[j] & mask.dp[j]){	/* if current bit is 1 */
									if(msb.dp[j]) sct5.i.dp[j]++;
									else
										break;
								}else{				/* if current bit is 0 */
									if(!msb.dp[j]) sct5.i.dp[j]++;
									else
										break;
								}
								mask.dp[j] >>= 1;
							}

							/* if HIX, check AV */
							if(!strcasecmp(Opr2, "HIX")){
								if(rAstatR.AV.dp[j]){
									sct5.r.dp[j] = -1;		/* indicates overflow happened 
												       just before this instruction */
								}

								if(rAstatI.AV.dp[j]){
									sct5.i.dp[j] = -1;		/* indicates overflow happened 
												       just before this instruction */
								}
							}

							/* set SS flag */
							if(!rAstatR.AV.dp[j]){			/* if AV == 0 */
								rAstatR.SS.dp[j] = isNeg12b(sct1.r.dp[j]);
							}else{					/* if AV == 1 */
								if(rAstatR.AV.dp[j] && !strcasecmp(Opr2, "HIX"))
									rAstatR.SS.dp[j] = (isNeg12b(sct1.r.dp[j])? 0: 1);
							}
							if(!rAstatI.AV.dp[j]){			/* if AV == 0 */
								rAstatI.SS.dp[j] = isNeg12b(sct1.i.dp[j]);
							}else{					/* if AV == 1 */
								if(rAstatI.AV.dp[j] && !strcasecmp(Opr2, "HIX"))
									rAstatI.SS.dp[j] = (isNeg12b(sct1.i.dp[j])? 0: 1);
							}
						}

						scWrReg(Opr0, sct5.r, sct5.i, condMask);
					}else if(!strcasecmp(Opr2, "LO")){
						scplx sct5 = scRdReg(Opr0);

						for(int j = 0; j < NUMDP; j++) {
							if(sct5.r.dp[j] == 11){	/* if upper word is all sign bits */
								sint sb;
								sint mask;

								sb.dp[j] = rAstatR.SS.dp[j];		/* sign bit of previous word */
								mask.dp[j] = 0x800;

								for(int i = 0; i < 12; i++){
									if(sct1.r.dp[j] & mask.dp[j]){	/* if current bit is 1 */
										if(sb.dp[j]) sct5.r.dp[j]++;
										else
											break;
									}else{				/* if current bit is 0 */
										if(!sb.dp[j]) sct5.r.dp[j]++;
										else
											break;
									}
									mask.dp[j] >>= 1;
								}
							}else{				/* else return previous (HI/HIX) result */
								;
							}

							if(sct5.i.dp[j] == 11){	/* if upper word is all sign bits */
								sint sb;
								sint mask;

								sb.dp[j] = rAstatI.SS.dp[j];		/* sign bit of previous word */
								mask.dp[j] = 0x800;

								for(int i = 0; i < 12; i++){
									if(sct1.i.dp[j] & mask.dp[j]){	/* if current bit is 1 */
										if(sb.dp[j]) sct5.i.dp[j]++;
										else
											break;
									}else{				/* if current bit is 0 */
										if(!sb.dp[j]) sct5.i.dp[j]++;
										else
											break;
									}
									mask.dp[j] >>= 1;
								}
							}else{				/* else return previous (HI/HIX) result */
								;
							}
						}

						scWrReg(Opr0, sct5.r, sct5.i, condMask);
					}else{
						printRunTimeError(p->LineCntr, Opr2, 
							"Invalid option for EXP.C instruction.\n");
						break;
					}
				} 
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iEXPADJ:
			/* note: iEXPADJ cannot be used in multifunction instructions */
			if(isDReg12(p, Opr0) && isXOP12(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 16c */
					/* [IF COND] EXPADJ DREG12, XOP12 */
					/* [IF COND] EXPADJ Op0,    Op1    */
					p->InstType = t16c;

					sint stemp0 = sRdReg(Opr0);	/* DST Opr0 must be initialized to 12 */
					sint stemp1 = sRdReg(Opr1);
					sint stemp5;
					for(int j = 0; j < NUMDP; j++) {
						stemp5.dp[j] = 0;
					}

					for(int j = 0; j < NUMDP; j++) {
						sint msb;
						sint mask;

						msb.dp[j] = 0x800 & stemp1.dp[j];
						mask.dp[j] = 0x400;

						for(int i = 0; i < 11; i++){
							if(stemp1.dp[j] & mask.dp[j]){	/* if current bit is 1 */
								if(msb.dp[j]) stemp5.dp[j]++;
								else
									break;
							}else{				/* if current bit is 0 */
								if(!msb.dp[j]) stemp5.dp[j]++;
								else
									break;
							}
							mask.dp[j] >>= 1;
						}

						if(stemp5.dp[j] < stemp0.dp[j]) {		/* if current result is smaller than Opr0 */
							stemp0.dp[j] = stemp5.dp[j];		/* update Opr0 */
						}
					}
					sWrReg(Opr0, stemp0, condMask);	
				} 
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iEXPADJ_C:
			/* note: iEXPADJ_C cannot be used in multifunction instructions */
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 16d */
					/* [IF COND] EXPADJ.C DREG24, XOP24 */
					/* [IF COND] EXPADJ.C Op0,    Op1    */
					p->InstType = t16d;

					scplx sct0 = scRdReg(Opr0);	/* DST Opr0 must be initialized to (12,12) */

					scplx sct1 = scRdReg(Opr1);
					scplx sct5;
					for(int j = 0; j < NUMDP; j++) {
						sct5.r.dp[j] = sct5.i.dp[j] = 0;
					}

					for(int j = 0; j < NUMDP; j++) {
						sint msb;
						sint mask;

						msb.dp[j] = 0x800 & sct1.r.dp[j];
						mask.dp[j] = 0x400;

						for(int i = 0; i < 11; i++){
							if(sct1.r.dp[j] & mask.dp[j]){	/* if current bit is 1 */
								if(msb.dp[j]) sct5.r.dp[j]++;
								else
									break;
							}else{				/* if current bit is 0 */
								if(!msb.dp[j]) sct5.r.dp[j]++;
								else
									break;
							}
							mask.dp[j] >>= 1;
						}

						msb.dp[j] = 0x800 & sct1.i.dp[j];
						mask.dp[j] = 0x400;

						for(int i = 0; i < 11; i++){
							if(sct1.i.dp[j] & mask.dp[j]){	/* if current bit is 1 */
								if(msb.dp[j]) sct5.i.dp[j]++;
								else
									break;
							}else{				/* if current bit is 0 */
								if(!msb.dp[j]) sct5.i.dp[j]++;
								else
									break;
							}
							mask.dp[j] >>= 1;
						}

						if(sct5.r.dp[j] < sct0.r.dp[j]) {		/* if current result is smaller than Opr0 */
							sct0.r.dp[j] = sct5.r.dp[j];
						}
						if(sct5.i.dp[j] < sct0.i.dp[j]) {		/* if current result is smaller than Opr0 */
							sct0.i.dp[j] = sct5.i.dp[j];
						}
					}
					scWrReg(Opr0, sct0.r, sct0.i, condMask);	/* update Opr0 */
				} 
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case    iIDLE:
			if(!p->Cond) p->Cond = strdup("TRUE");
			/* now p->Cond always exists */

			if(ifCond(p->Cond)){
            	/* type 31a */
            	/* [IF COND] IDLE */
            	p->InstType = t31a;
			}
            break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iJUMP:
			if(!p->Cond) {
				if(isIReg(p, Opr0)){
					/* type 19a */
					/* [IF TRUE] JUMP (<IREG>) */
					/* [IF TRUE] JUMP (Op0   ) */
					p->InstType = t19a;

					int tAddr = 0xFFFF & RdReg(Opr0);
					
					if(!DelaySlotMode){		/* if delay slot not enabled */
						p->LatencyAdded = 3; 	/* 4 cycles if taken */
					}

					NextCode = sICodeListSearch(&iCode, tAddr);
					isBranchTaken = TRUE;

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						if(pn->isDelaySlot){
							pn->BrTarget = NextCode;	/* save branch target address in delay slot */
						}else{		/* error */
							printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
								"Delay slot processing error. Please report.\n");
						}
					}

				} else {
					/* type 10b */
					/* JUMP <IMM_INT16> */
					/* JUMP  Opr0       */
					p->InstType = t10b;

					if(!DelaySlotMode){		/* if delay slot not enabled */
						p->LatencyAdded = 3; 	/* 4 cycles if taken */
					}

					NextCode = sICodeListSearch(&iCode, getLabelAddr(p, symTable, Opr0));
					isBranchTaken = TRUE;

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						if(pn->isDelaySlot){
							pn->BrTarget = NextCode;	/* save branch target address in delay slot */
						}else{		/* error */
							printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
								"Delay slot processing error. Please report.\n");
						}
					}

				}
			} else if(ifCond(p->Cond)) {
				if(isIReg(p, Opr0)){
					/* type 19a */
					/* [IF COND] JUMP (<IREG>) */
					/* [IF COND] JUMP (Op0   ) */
					p->InstType = t19a;

					int tAddr = 0xFFFF & RdReg(Opr0);
					
					if(!DelaySlotMode){		/* if delay slot not enabled */
						p->LatencyAdded = 3; 	/* 4 cycles if taken */
					}

					NextCode = sICodeListSearch(&iCode, tAddr);
					isBranchTaken = TRUE;

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						if(pn->isDelaySlot){
							pn->BrTarget = NextCode;	/* save branch target address in delay slot */
						}else{		/* error */
							printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
								"Delay slot processing error. Please report.\n");
						}
					}

				} else {
					/* type 10a */
					/* [IF COND] JUMP <IMM_INT13> */
					/* [IF COND] JUMP  Opr0       */
					p->InstType = t10a;

					if(!DelaySlotMode){		/* if delay slot not enabled */
						p->LatencyAdded = 3; 	/* 4 cycles if taken */
					}

					NextCode = sICodeListSearch(&iCode, getLabelAddr(p, symTable, Opr0));
					isBranchTaken = TRUE;

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						if(pn->isDelaySlot){
							pn->BrTarget = NextCode;	/* save branch target address in delay slot */
						}else{		/* error */
							printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
								"Delay slot processing error. Please report.\n");
						}
					}

				}
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iLD:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isInt(Opr1) && (Opr2 != NULL) 
				&& !strcasecmp(Opr2, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 3a */
					/* [IF COND] LD DREG12, DM(IMM_UINT16) */
					/* [IF COND] LD Op0,    Op2(Op1)        */
					p->InstType = t03a;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					//int tAddr = 0x0FFFF & getIntSymAddr(p, symTable, Opr1);
					int tAddr = 0xFFFF & getIntImm(p, getIntSymAddr(p, symTable, Opr1), eIMM_UINT16);
					sint tData = sRdDataMem(tAddr);

#ifdef VHPI
					if(VhpiMode){	/* rh: read 12b */
						dsp_rh(tAddr, (short int *)&tData.dp[0], 1);
						dsp_wait(1);
					}
#endif

					sWrReg(Opr0, tData, condMask);
				}
			} else if(isDReg12(p, Opr0) && isInt(Opr1) 
				&& (Opr2 != NULL)
				&& (!strcmp(Opr2, "+") || !strcmp(Opr2, "["))
				&& isInt(Opr3) && (Opr4 != NULL) && !strcasecmp(Opr4, "DM")){
				/* type 3a */
				/* Variation due to .VAR usage */
				/* LD DREG12, DM(IMM_UINT16 +   const ) */
				/* LD DREG12, DM(IMM_UINT16 [   const]) */
				/* LD Op0,    Op4(Op1      Op2 Op3   ) */
				p->InstType = t03a;

				printRunTimeError(p->LineCntr, Opr0, 
					"This case should not happen. Please report.\n");
				break;
			} else if(isRReg16(p, Opr0) && isInt(Opr1) && (Opr2 != NULL) 
				&& !strcasecmp(Opr2, "DM")){
				/* type 3b */
				/* LD RREG16, DM(<IMM_INT16>) */
				/* LD Op0,    Op2(Op1)        */
				printRunTimeError(p->LineCntr, Opr0, 
					"LD RREG16, DM(<IMM_INT16>) syntax not allowed.\n");
				break;

				/*
				p->InstType = t03b;
				WrReg(Opr0, RdDataMem(0x0FFFF & getIntSymAddr(p, symTable, Opr1)));
				*/
			} else if(isACC32(p, Opr0) && isInt(Opr1) && (Opr2 != NULL) 
				&& !strcasecmp(Opr2, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 3d */
					/* [IF COND] LD ACC32, DM(IMM_UINT16) (|HI,LO|) */
					/* [IF COND] LD Op0,   Op2(Op1)        (Op3)       */
					/* Note: load data to upper or lower **24** bits of accumulator */
					p->InstType = t03d;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					if(!Opr3){
						printRunTimeError(p->LineCntr, Opr0, 
							"(HI) or (LO) required. Please check syntax.\n");
						break;
					}

					//int tAddr1 = 0x0FFFF & getIntSymAddr(p, symTable, Opr1);
					int tAddr1 = 0xFFFF & getIntImm(p, getIntSymAddr(p, symTable, Opr1), eIMM_UINT16);

					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr1, Opr1);

					sint tData1 = sRdDataMem(tAddr1);			/* read low word  */
					sint tData2 = sRdDataMem(tAddr2);			/* read high word */

#ifdef VHPI
					if(VhpiMode){	/* rw: read 24b */
						int tData;

						dsp_rw(tAddr1, &tData, 1);
						dsp_wait(1);

						/* Note: tData = (((0x0FFF & tData1) << 12) | (0x0FFF & tData2)); */
						tData1.dp[0] = (0x0FFF & (tData >> 12));
						tData2.dp[0] = 0xFFF & tData;
					}
#endif

					sint stemp1;
					for(int j = 0; j < NUMDP; j++) {
						stemp1.dp[j] = (((0x0FFF & tData2.dp[j]) << 12) | (0x0FFF & tData1.dp[j]));
						if(isNeg24b(stemp1.dp[j])) stemp1.dp[j] |= 0x0FF000000;

						if(!strcasecmp(Opr3, "HI")) stemp1.dp[j] <<= 8;
					}
					sWrReg(Opr0, stemp1, condMask);
				}
			} else if(isACC32(p, Opr0) && isInt(Opr1) 
				&& (Opr2 != NULL)
				&& (!strcmp(Opr2, "+") || !strcmp(Opr2, "["))
				&& isInt(Opr3) && (Opr4 != NULL) && !strcasecmp(Opr4, "DM")
				&& (!strcmp(Opr5, "HI") || !strcmp(Opr5, "LO"))){
				/* type 3d */
				/* Variation due to .VAR usage */
				/* LD ACC32, DM(IMM_UINT16 +   const ) (|HI,LO|) */
				/* LD ACC32, DM(IMM_UINT16 [   const]) (|HI,LO|) */
				/* LD Op0,   Op4(Op1       Op2 Op3   ) (Op5)     */
				/* Note: load data to upper or lower **24** bits of accumulator (complex pair) */
				p->InstType = t03d;

				printRunTimeError(p->LineCntr, Opr0, 
					"This case should not happen. Please report.\n");
				break;
			} else if(isRReg16(p, Opr0) && isInt(Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 6a */
					/* [IF COND] LD RREG16, <IMM_INT16> */
					/* [IF COND] LD Op0,    Op1         */
					p->InstType = t06a;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					sint stemp1;
					int imm16 = getIntImm(p, getIntSymAddr(p, symTable, Opr1),  eIMM_INT16);			
					for(int j = 0; j < NUMDP; j++) {
						stemp1.dp[j] = imm16;
					}
					if(!isReadOnlyReg(p, Opr0)){
						//WrReg(Opr0, getIntImm(p, getIntSymAddr(p, symTable, Opr1),  eIMM_INT16));			
						sWrReg(Opr0, stemp1, condMask);
					}
				}
			} else if(isDReg12(p, Opr0) && isInt(Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 6b */
					/* [IF COND] LD DREG12, <IMM_INT12> */
					/* [IF COND] LD Op0,    Op1         */
					p->InstType = t06b;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					sint stemp1;
					int imm12 = getIntImm(p, getIntSymAddr(p, symTable, Opr1),  eIMM_INT12);			
					for(int j = 0; j < NUMDP; j++) {
						stemp1.dp[j] = imm12;
					}
					//WrReg(Opr0, getIntImm(p, getIntSymAddr(p, symTable, Opr1),  eIMM_INT12));			
					sWrReg(Opr0, stemp1, condMask);
				}
			} else if(isACC32(p, Opr0) && isInt(Opr1)){
				/* type 6d */
				/* LD ACC32, <IMM_INT24> */
				/* LD Op0,   Op1         */
				p->InstType = t06d;

				/* latency: if LD comes just after ST, need +1 cycle */
				sICode *lp = p->LastExecuted;
				if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
					p->LatencyAdded = p->LatencyAdded +1;

				sint stemp1;
				int imm24 = getIntImm(p, getIntSymAddr(p, symTable, Opr1),  eIMM_INT24);			
					for(int j = 0; j < NUMDP; j++) {
						stemp1.dp[j] = imm24;
					}

				sWrReg(Opr0, stemp1, trueMask);			
			} else if(isRReg(p, Opr0) && !strcmp(Opr1, "+=") && isIReg(p, Opr2)
				&& isMReg(p, Opr3) && !strcasecmp(Opr4, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 32a */
					/* [IF COND] LD RREG, DM(IREG +=  MREG) */
					/* [IF COND] LD Op0,  Op4(Op2 Op1 Op3) */
					p->InstType = t32a;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					int tAddr = 0xFFFF & RdReg2(p, Opr2);
					sint tData = sRdDataMem((unsigned int)tAddr);

#ifdef VHPI
					if(VhpiMode){	/* rh: read 12b */
						dsp_rh(tAddr, (short int *)&tData.dp[0], 1);
						dsp_wait(1);
					}
#endif
					sWrReg(Opr0, tData, condMask);

					/* postmodify: update Ix */
					tAddr += RdReg2(p, Opr3);
					updateIReg(Opr2, tAddr);
				}
			} else if(isRReg(p, Opr0) && !strcmp(Opr1, "+") && isIReg(p, Opr2)
				&& isMReg(p, Opr3) && !strcasecmp(Opr4, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 32a */
					/* [IF COND] LD RREG, DM(IREG +   MREG) */
					/* [IF COND] LD Op0,  Op4(Op2 Op1 Op3) */
					p->InstType = t32a;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					/* premodify: don't update Ix */
					int tAddr = 0xFFFF & (RdReg2(p, Opr2) + RdReg2(p, Opr3));
					sint tData = sRdDataMem((unsigned int)tAddr);

#ifdef VHPI
					if(VhpiMode){	/* rh: read 12b */
						dsp_rh(tAddr, (short int *)&tData.dp[0], 1);
						dsp_wait(1);
					}
#endif
					sWrReg(Opr0, tData, condMask);
				}
			} else if(isDReg12(p, Opr0) && !strcmp(Opr1, "+=") && isIReg(p, Opr2)
				&& isInt(Opr3) && !strcasecmp(Opr4, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 29a */
					/* [IF COND] LD DREG12, DM(IREG +=  <IMM_INT8>) */
					/* [IF COND] LD Op0,    Op4(Op2 Op1 Op3) */
					p->InstType = t29a;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					int tAddr = 0xFFFF & RdReg2(p, Opr2);
					sint tData = sRdDataMem((unsigned int)tAddr);

#ifdef VHPI
					if(VhpiMode){	/* rh: read 12b */
						dsp_rh(tAddr, (short int *)&tData.dp[0], 1);
						dsp_wait(1);
					}
#endif
					sWrReg(Opr0, tData, condMask);

					/* postmodify: update Ix */
					//int tOffset = (0x0FF & getIntSymAddr(p, symTable, Opr3));
					int tOffset = getIntImm(p, getIntSymAddr(p, symTable, Opr3), eIMM_INT8);

					tAddr += tOffset;
					updateIReg(Opr2, tAddr);
				}
			} else if(isDReg12(p, Opr0) && !strcmp(Opr1, "+") && isIReg(p, Opr2)
				&& isInt(Opr3) && !strcasecmp(Opr4, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 29a */
					/* [IF COND] LD DREG12, DM(IREG +   <IMM_INT8>) */
					/* [IF COND] LD Op0,    Op4(Op2 Op1 Op3) */
					p->InstType = t29a;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					/* premodify: don't update Ix */
					//int tOffset =  (0x0FF & getIntSymAddr(p, symTable, Opr3));
					int tOffset = getIntImm(p, getIntSymAddr(p, symTable, Opr3), eIMM_INT8);

					int tAddr = 0xFFFF & (RdReg2(p, Opr2) + tOffset);
					sint tData = sRdDataMem((unsigned int)tAddr);

#ifdef VHPI
					if(VhpiMode){	/* rh: read 12b */
						dsp_rh(tAddr, (short int *)&tData.dp[0], 1);
						dsp_wait(1);
					}
#endif
					sWrReg(Opr0, tData, condMask);
				}
			} else if(isDReg12(p, Opr0) && isIReg(p, Opr1)
				&& !strcasecmp(Opr2, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 29a */
					/* [IF COND] LD DREG12, DM(IREG) */
					/* [IF COND] LD Op0,    Op2(Op1) */
					p->InstType = t29a;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					int tAddr = 0xFFFF & RdReg2(p, Opr1);
					sint tData = sRdDataMem((unsigned int)tAddr);

#ifdef VHPI
					if(VhpiMode){	/* rh: read 12b */
						dsp_rh(tAddr, (short int *)&tData.dp[0], 1);
						dsp_wait(1);
					}
#endif
					sWrReg(Opr0, tData, condMask);
				}
			} else if(isDReg12(p, Opr0) && isSysCtlReg(Opr1)){
				/* type 35a */
				/* LD DREG12, CACTL */
				/* ST Op0,    Op1    */
				printRunTimeError(p->LineCntr, Opr1, 
					"CACTL Register not supported.\n");
				break;
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iLD_C:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isDReg24(p, Opr0) && isInt(Opr1) && (Opr2 != NULL) 
				&& !strcasecmp(Opr2, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 3c */
					/* [IF COND] LD.C DREG24, DM(IMM_UINT16) */
					/* [IF COND] LD.C Op0,    Op2(Op1)        */
					p->InstType = t03c;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					//int tAddr = 0xFFFF & getIntSymAddr(p, symTable, Opr1);
					int tAddr = 0xFFFF & getIntImm(p, getIntSymAddr(p, symTable, Opr1), eIMM_UINT16);

					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr, Opr1);

					sint tData1 = sRdDataMem(tAddr);
					sint tData2 = sRdDataMem(tAddr2);

#ifdef VHPI
					if(VhpiMode){	/* rw: read 24b */
						int tData;

						dsp_rw(tAddr, &tData, 1);
						dsp_wait(1);

						/* Note: tData = (((0x0FFF & tData1) << 12) | (0x0FFF & tData2)); */
						tData1.dp[0] = (0x0FFF & (tData >> 12));
						tData2.dp[0] = 0xFFF & tData;
					}
#endif

					sint stemp1;
					for(int j = 0; j < NUMDP; j++) {
						stemp1.dp[j] = (((0x0FFF & tData2.dp[j]) << 12) | (0x0FFF & tData1.dp[j]));
						if(isNeg24b(stemp1.dp[j])) stemp1.dp[j] |= 0x0FF000000;
					}
					scWrReg(Opr0, tData1, tData2, condMask);
				}

			} else if(isDReg24(p, Opr0) && isInt(Opr1) 
				&& (Opr2 != NULL)
				&& (!strcmp(Opr2, "+") || !strcmp(Opr2, "["))
				&& isInt(Opr3) && (Opr4 != NULL) && !strcasecmp(Opr4, "DM")){
				/* type 3c */
				/* Variation due to .VAR usage */
				/* [IF COND] LD.C DREG24, DM(IMM_UINT16 +   const ) */
				/* [IF COND] LD.C DREG24, DM(IMM_UINT16 [   const]) */
				/* [IF COND] LD.C Op0,    Op4(Op1       Op2 Op3   ) */
				p->InstType = t03c;

				printRunTimeError(p->LineCntr, Opr0, 
					"This case should not happen. Please report.\n");
				break;
			} else if(isACC64(p, Opr0) && isInt(Opr1) && (Opr2 != NULL) 
				&& !strcasecmp(Opr2, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 3e */
					/* [IF COND] LD.C ACC64, DM(IMM_UINT16) (|HI,LO|) */
					/* [IF COND] LD.C Op0,   Op2(Op1)        (Op3)       */
					/* Note: load data to upper or lower **24** bits of accumulator (complex pair) */
					p->InstType = t03e;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					if(!Opr3){
						printRunTimeError(p->LineCntr, Opr0, 
							"(HI) or (LO) required. Please check syntax.\n");
						break;
					}

					//int tAddr1 = 0x0FFFF & getIntSymAddr(p, symTable, Opr1);
					int tAddr1 = 0xFFFF & getIntImm(p, getIntSymAddr(p, symTable, Opr1), eIMM_UINT16);

					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr1, Opr1);

					scplx scData1, scData2;

					/* read real part 24 bits */
					scData1.r = sRdDataMem(tAddr1);		/* read low word */
					scData1.i = sRdDataMem(tAddr2);		/* read high word */
					/* read imaginary part 24 bits */
					scData2.r = sRdDataMem(tAddr1+2);		/* read low word */
					scData2.i = sRdDataMem(tAddr2+2);		/* read high word */

#ifdef VHPI
					if(VhpiMode){	/* rw+rw: read 24b+24b */
						int tData1, tData2;

						dsp_rw_rw(tAddr1, &tData1, tAddr1+2, &tData2, 1);
						dsp_wait(1);

						/* Note: 
						tData1 = (((0x0FFF & cData1.r) << 12) | (0x0FFF & cData1.i));	
						tData2 = (((0x0FFF & cData2.r) << 12) | (0x0FFF & cData2.i));	
						*/
						scData1.r.dp[0] = (0x0FFF & (tData1 >> 12));
						scData1.i.dp[0] = 0xFFF & tData1;
						scData2.r.dp[0] = (0x0FFF & (tData2 >> 12));
						scData2.i.dp[0] = 0xFFF & tData2;
					}
#endif
					sint stemp1, stemp2;

					for(int j = 0; j < NUMDP; j++) {
						stemp1.dp[j] = (((0x0FFF & scData1.i.dp[j]) << 12) | (0x0FFF & scData1.r.dp[j]));	 
						stemp2.dp[j] = (((0x0FFF & scData2.i.dp[j]) << 12) | (0x0FFF & scData2.r.dp[j]));

						if(isNeg24b(stemp1.dp[j])) stemp1.dp[j] |= 0x0FF000000;	/* sign extension */
						if(isNeg24b(stemp2.dp[j])) stemp2.dp[j] |= 0x0FF000000;

						if(!strcasecmp(Opr3, "HI")) {
							stemp1.dp[j] <<= 8;
							stemp2.dp[j] <<= 8;
						}
					}
					scWrReg(Opr0, stemp1, stemp2, condMask);
				}

			} else if(isACC64(p, Opr0) && isInt(Opr1) 
				&& (Opr2 != NULL)
				&& (!strcmp(Opr2, "+") || !strcmp(Opr2, "["))
				&& isInt(Opr3) && (Opr4 != NULL) && !strcasecmp(Opr4, "DM")
				&& (!strcmp(Opr5, "HI") || !strcmp(Opr5, "LO"))){
				/* type 3e */
				/* Variation due to .VAR usage */
				/* [IF COND] LD.C ACC64, DM(IMM_UINT16 +   const ) (|HI,LO|) */
				/* [IF COND] LD.C ACC64, DM(IMM_UINT16 [   const]) (|HI,LO|) */
				/* [IF COND] LD.C Op0,   Op4(Op1       Op2 Op3   ) (Op5)     */
				/* Note: load data to upper or lower **24** bits of accumulator (complex pair) */
				p->InstType = t03e;

				printRunTimeError(p->LineCntr, Opr0, 
					"This case should not happen. Please report.\n");
				break;
			} else if(isDReg24(p, Opr0) && isInt(Opr1)){
				if(!isInt(Opr2)){
					printRunTimeError(p->LineCntr, Opr1, 
						"Complex constant should be in a pair, e.g. (3, 4)\n");
					break;
				}
				/* type 6c */
				/* LD.C DREG24, <IMM_COMPLEX24> */
				/* LD.C Op0,    (Op1, Op2)      */
				p->InstType = t06c;

				/* latency: if LD comes just after ST, need +1 cycle */
				sICode *lp = p->LastExecuted;
				if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
					p->LatencyAdded = p->LatencyAdded +1;

				sint stemp1, stemp2;
				int imm121 = getIntImm(p, getIntSymAddr(p, symTable, Opr1),  eIMM_INT12);			
				int imm122 = getIntImm(p, getIntSymAddr(p, symTable, Opr2),  eIMM_INT12);			
				for(int j = 0; j < NUMDP; j++) {
					stemp1.dp[j] = imm121;
					stemp2.dp[j] = imm122;
				}

				scWrReg(Opr0, stemp1, stemp2, trueMask);			
			} else if(isCReg(p, Opr0) && !strcmp(Opr1, "+=") && isIReg(p, Opr2)
				&& isMReg(p, Opr3) && !strcasecmp(Opr4, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
				/* type 32b */
				/* [IF COND] LD.C DREG24, DM(IREG +=  MREG) */
				/* [IF COND] LD.C Op0,  Op4(Op2 Op1 Op3) */
				p->InstType = t32b;

				/* latency: if LD comes just after ST, need +1 cycle */
				sICode *lp = p->LastExecuted;
				if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
					p->LatencyAdded = p->LatencyAdded +1;

				int tAddr = 0xFFFF & RdReg2(p, Opr2);

				int tAddr2 = checkUnalignedMemoryAccess(p, tAddr, Opr2);

				sint tData1 = sRdDataMem((unsigned int)tAddr);
				sint tData2 = sRdDataMem((unsigned int)tAddr2);

#ifdef VHPI
				if(VhpiMode){	/* rw: read 24b */
					int tData;

					dsp_rw(tAddr, &tData, 1);
					dsp_wait(1);

					/* Note: int tData = (((0x0FFF & tData1) << 12) | (0x0FFF & tData2)); */
					tData1.dp[0] = (0x0FFF & (tData >> 12));
					tData2.dp[0] = 0xFFF & tData;
				}
#endif

				scWrReg(Opr0, tData1, tData2, condMask);

				/* postmodify: update Ix */
				tAddr += RdReg2(p, Opr3);
				updateIReg(Opr2, tAddr);
				}
			} else if(isCReg(p, Opr0) && !strcmp(Opr1, "+") && isIReg(p, Opr2)
				&& isMReg(p, Opr3) && !strcasecmp(Opr4, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 32b */
					/* [IF COND] LD.C DREG24, DM(IREG +   MREG) */
					/* [IF COND] LD.C Op0,  Op4(Op2 Op1 Op3) */
					p->InstType = t32b;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					/* premodify: don't update Ix */
					int tAddr = 0xFFFF & (RdReg2(p, Opr2) + RdReg2(p, Opr3));

					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr, Opr2);

					sint tData1 = sRdDataMem((unsigned int)tAddr);
					sint tData2 = sRdDataMem((unsigned int)tAddr2);

#ifdef VHPI
					if(VhpiMode){	/* rw: read 24b */
						int tData;

						dsp_rw(tAddr, &tData, 1);
						dsp_wait(1);

						/* Note: int tData = (((0x0FFF & tData1) << 12) | (0x0FFF & tData2)); */
						tData1.dp[0] = (0x0FFF & (tData >> 12));
						tData2.dp[0] = 0xFFF & tData;
					}
#endif

					scWrReg(Opr0, tData1, tData2, condMask);
				}
			} else if(isDReg24(p, Opr0) && !strcmp(Opr1, "+=") && isIReg(p, Opr2)
				&& isInt(Opr3) && !strcasecmp(Opr4, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 29b */
					/* [IF COND] LD.C DREG24, DM(IREG +=  <IMM_INT8>) */
					/* [IF COND] LD.C Op0,    Op4(Op2 Op1 Op3) */
					p->InstType = t29b;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					int tAddr = 0xFFFF & RdReg2(p, Opr2);

					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr, Opr2);

					sint tData1 = sRdDataMem((unsigned int)tAddr);
					sint tData2 = sRdDataMem((unsigned int)tAddr2);

#ifdef VHPI
					if(VhpiMode){	/* rw: read 24b */
						int tData;

						dsp_rw(tAddr, &tData, 1);
						dsp_wait(1);

						/* Note: tData = (((0x0FFF & tData1) << 12) | (0x0FFF & tData2)); */
						tData1.dp[0] = (0x0FFF & (tData >> 12));
						tData2.dp[0] = 0xFFF & tData;
					}
#endif

					scWrReg(Opr0, tData1, tData2, condMask);

					/* postmodify: update Ix */
					//int tOffset = (0x0FF & getIntSymAddr(p, symTable, Opr3));
					int tOffset = getIntImm(p, getIntSymAddr(p, symTable, Opr3), eIMM_INT8);

					tAddr += tOffset;
					updateIReg(Opr2, tAddr);
				}
			} else if(isDReg24(p, Opr0) && !strcmp(Opr1, "+") && isIReg(p, Opr2)
				&& isInt(Opr3) && !strcasecmp(Opr4, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 29b */
					/* [IF COND] LD.C DREG24, DM(IREG +   <IMM_INT8>) */
					/* [IF COND] LD.C Op0,    Op4(Op2 Op1 Op3) */
					p->InstType = t29b;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					/* premodify: don't update Ix */
					//int tOffset = (0x0FF & getIntSymAddr(p, symTable, Opr3));
					int tOffset = getIntImm(p, getIntSymAddr(p, symTable, Opr3), eIMM_INT8);

					int tAddr = 0xFFFF & (RdReg2(p, Opr2) + tOffset);

					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr, Opr2);

					sint tData1 = sRdDataMem((unsigned int)tAddr);
					sint tData2 = sRdDataMem((unsigned int)tAddr2);

#ifdef VHPI
					if(VhpiMode){	/* rw: read 24b */
						int tData;

						dsp_rw(tAddr, &tData, 1);
						dsp_wait(1);

						/* Note: tData = (((0x0FFF & tData1) << 12) | (0x0FFF & tData2)); */
						tData1.dp[0] = (0x0FFF & (tData >> 12));
						tData2.dp[0] = 0xFFF & tData;
					}
#endif

					scWrReg(Opr0, tData1, tData2, condMask);
				}
			} else if(isDReg24(p, Opr0) && isIReg(p, Opr1)
				&& !strcasecmp(Opr2, "DM")){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 29b */
					/* [IF COND] LD.C DREG12, DM(IREG) */
					/* [IF COND] LD.C Op0,    Op2(Op1) */
					p->InstType = t29b;

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					int tAddr = 0xFFFF & RdReg2(p, Opr1);

					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr, Opr1);

					sint tData1 = sRdDataMem((unsigned int)tAddr);
					sint tData2 = sRdDataMem((unsigned int)tAddr2);

#ifdef VHPI
					if(VhpiMode){	/* rw: read 24b */
						int tData;

						dsp_rw(tAddr, &tData, 1);
						dsp_wait(1);

						/* Note: tData = (((0x0FFF & tData1) << 12) | (0x0FFF & tData2)); */
						tData1.dp[0] = (0x0FFF & (tData >> 12));
						tData2.dp[0] = 0xFFF & tData;
					}
#endif
					scWrReg(Opr0, tData1, tData2, condMask);
				}
			} else if(isRReg16(p, Opr0) && isInt(Opr1) && (Opr2 != NULL) 
				&& !strcasecmp(Opr2, "DM")){
				/* type: Not allowed!!  */
				/* LD.C RREG16, DM(<IMM_INT16>) */
				/* LD.C Op0,    Op2(Op1)        */
				printRunTimeError(p->LineCntr, Opr0, 
					"LD.C does not support this register operand.\n");
				break;
			} else if(isRReg16(p, Opr0) && isInt(Opr1)){
				/* type 6a */
				/* LD RREG16, <IMM_INT16> */
				/* LD Op0,    Op1         */
				printRunTimeError(p->LineCntr, Opr0, 
					"LD.C does not support this register operand.\n");
				break;
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iMAC:
		case	iMAS:
		case	iMPY:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isACC32(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 40e */
					p->InstType = t40e;
					/* [IF COND] MAC ACC32, XOP12, YOP12 (|RND, SS, SU, US, UU|) */
					/* [IF COND] MAC Opr0,  Opr1,  Opr2  (Opr3)                  */

					/* 
					* NOTE: this code does NOT consider delay slot. (2010.07.20)
					*/
					sICode *NCode;
					if(stackEmpty(&LoopBeginStack)){		/* no loop running */
						NCode = p->Next;
						if(NCode && !isAnyMAC(NCode))			/* if last MAC */
							p->LatencyAdded = p->LatencyAdded +1;
					}else{									/* loop running */
						if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
							if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{			/* DO UNTIL CE */
								loopCntr = stackTop(&LoopCounterStack);
								if(loopCntr > 1){ /* decrement counter */
									NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
								}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
									NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
								}else{	/* exit loop if loopCntr == 1 */
									NCode = p->Next;
								}
							}
						} else {	/* within the loop */
							NCode = p->Next;
						}
						if(NCode && !isAnyMAC(NCode))			/* if last MAC */
							p->LatencyAdded = p->LatencyAdded +1;
					}

					sint stemp1 = sRdReg(Opr1);
					sint stemp2 = sRdReg(Opr2);

					sProcessMACFunc(p, stemp1, stemp2, Opr0, Opr3, condMask);
				} 
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iMAC_C:
		case	iMAS_C:
		case	iMPY_C:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isACC64(p, Opr0) && isXOP24(p, Opr1) && isXOP24(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 40f */
					p->InstType = t40f;
					/* [IF COND] MAC.C ACC64, XOP24, YOP24[*] (|RND, SS, SU, US, UU|) */
					/* [IF COND] MAC.C Opr0,  Opr1,  Opr2[*]  (Opr3)                  */

					/* 
					* NOTE: this code does NOT consider delay slot. (2010.07.20)
					*/
					sICode *NCode;
					if(stackEmpty(&LoopBeginStack)){		/* no loop running */
						NCode = p->Next;
						if(NCode && !isAnyMAC(NCode))			/* if last MAC */
							p->LatencyAdded = p->LatencyAdded +1;
					}else{									/* loop running */
						if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
							if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{			/* DO UNTIL CE */
								loopCntr = stackTop(&LoopCounterStack);
								if(loopCntr > 1){ /* decrement counter */
									NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
								}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
									NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
								}else{	/* exit loop if loopCntr == 1 */
									NCode = p->Next;
								}
							}
						} else {	/* within the loop */
							NCode = p->Next;
						}
						if(NCode && !isAnyMAC(NCode))			/* if last MAC */
							p->LatencyAdded = p->LatencyAdded +1;
					}

					scplx sct1 = scRdReg(Opr1);
					scplx sct2 = scRdReg(Opr2);
					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct2.i.dp[j] = -sct2.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					sProcessMAC_CFunc(p, sct1, sct2, Opr0, Opr3, condMask);
				} 
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iMAC_RC:
		case	iMAS_RC:
		case	iMPY_RC:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isACC64(p, Opr0) && isXOP12(p, Opr1) && isXOP24(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 40g */
					p->InstType = t40g;
					/* [IF COND] MAC.RC ACC64, XOP12, YOP24[*] (|RND, SS, SU, US, UU|) */
					/* [IF COND] MAC.RC Opr0,  Opr1,  Opr2[*]  (Opr3)                  */

					/* 
					* NOTE: this code does NOT consider delay slot. (2010.07.20)
					*/
					sICode *NCode;
					if(stackEmpty(&LoopBeginStack)){		/* no loop running */
						NCode = p->Next;
						if(NCode && !isAnyMAC(NCode))			/* if last MAC */
							p->LatencyAdded = p->LatencyAdded +1;
					}else{									/* loop running */
						if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
							if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{			/* DO UNTIL CE */
								loopCntr = stackTop(&LoopCounterStack);
								if(loopCntr > 1){ /* decrement counter */
									NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
								}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
									NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
								}else{	/* exit loop if loopCntr == 1 */
									NCode = p->Next;
								}
							}
						} else {	/* within the loop */
							NCode = p->Next;
						}
						if(NCode && !isAnyMAC(NCode))			/* if last MAC */
							p->LatencyAdded = p->LatencyAdded +1;
					}

					sint stemp1 = sRdReg(Opr1);
					scplx sct2 = scRdReg(Opr2);
					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct2.i.dp[j] = -sct2.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					sProcessMAC_RCFunc(p, stemp1, sct2, Opr0, Opr3, condMask);
				} 
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iMAG_C:
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 47a */
					/* [IF COND] MAG.C DREG24, XOP24[*] */
					/* [IF COND] MAG.C Op0,    Op1[*]   */
					p->InstType = t47a;

					scplx sct1 = scRdReg(Opr1);
					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct1.i.dp[j] = -sct1.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					sProcessComplexFunc(p, sct1, Opr0, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iNOP:
			if(!p->Cond) p->Cond = strdup("TRUE");
			/* now p->Cond always exists */

			if(ifCond(p->Cond)){
				/* type 30a */
				/* [IF COND] NOP */
				p->InstType = t30a;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iPOLAR_C:
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 42a */
					/* [IF COND] POLAR.C DREG24, XOP24[*] */
					/* [IF COND] POLAR.C Op0,    Op1[*]   */
					p->InstType = t42a;

					scplx sct1 = scRdReg(Opr1);
					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct1.i.dp[j] = -sct1.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					sProcessCordicFunc(p, sct1, Opr0, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iPOP:
			/* type 26a */
			/* [IF COND] POP |PC, LOOP| [, STS] */
			/* [IF COND] POP Opr0,      [, STS] */
			if(p->OperandCounter){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				if(ifCond(p->Cond)){
					p->InstType = t26a;
					if(!strcasecmp(Opr0, "PC")){	/* if PC */
						WrReg("_PCSTACK", stackTop(&PCStack));
						stackPop(&PCStack);
					} else if(!strcasecmp(Opr0, "LOOP")){	/* if LOOP */
						WrReg("_PCSTACK", stackTop(&LoopBeginStack));
						stackPop(&LoopBeginStack);
						WrReg("_LPSTACK", stackTop(&LoopEndStack));
						stackPop(&LoopEndStack);
						WrReg("_CNTR", stackTop(&LoopCounterStack));
						stackPop(&LoopCounterStack);
						WrReg("_LPEVER", stackTop(&LPEVERStack));
						stackPop(&LPEVERStack);

						p->LatencyAdded = 4;		/* added 2010.07.20. */
					}
					if(Opr0 && !strcasecmp(Opr0, "STS")){	/* if STS */
						sWrReg("ASTAT.R", sStackTop(&ASTATStack[0]), trueMask);
						sStackPop(&ASTATStack[0]);
						sWrReg("ASTAT.I", sStackTop(&ASTATStack[1]), trueMask);
						sStackPop(&ASTATStack[1]);
						sWrReg("ASTAT.C", sStackTop(&ASTATStack[2]), trueMask);
						sStackPop(&ASTATStack[2]);
						WrReg("_MSTAT",   stackTop(&MSTATStack));
						stackPop(&MSTATStack);
					}
					if(Opr1 && !strcasecmp(Opr1, "STS")){	/* if STS */
						sWrReg("ASTAT.R", sStackTop(&ASTATStack[0]), trueMask);
						sStackPop(&ASTATStack[0]);
						sWrReg("ASTAT.I", sStackTop(&ASTATStack[1]), trueMask);
						sStackPop(&ASTATStack[1]);
						sWrReg("ASTAT.C", sStackTop(&ASTATStack[2]), trueMask);
						sStackPop(&ASTATStack[2]);
						WrReg("_MSTAT",   stackTop(&MSTATStack));
						stackPop(&MSTATStack);
					}
					sFlagEffect(p->InstType, so2, so2, so2, so2, trueMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iPUSH:
			/* type 26a */
			/* [IF COND] PUSH |PC, LOOP| [, STS] */
			/* [IF COND] PUSH Opr0,      [, STS] */
			if(p->OperandCounter){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				if(ifCond(p->Cond)){
					p->InstType = t26a;
					if(!strcasecmp(Opr0, "PC")){	/* if PC */
						stackPush(&PCStack, RdReg("_PCSTACK"));
					} else if(!strcasecmp(Opr0, "LOOP")){	/* if LOOP */
						stackPush(&LoopBeginStack, RdReg("_PCSTACK"));
						stackPush(&LoopEndStack, RdReg("_LPSTACK"));
						stackPush(&LoopCounterStack, RdReg("_CNTR"));
						stackPush(&LPEVERStack, RdReg("_LPEVER"));
					}
					if(Opr0 && !strcasecmp(Opr0, "STS")){	/* if STS */
						sStackPush(&ASTATStack[0], sRdReg("ASTAT.R"));
						sStackPush(&ASTATStack[1], sRdReg("ASTAT.I"));
						sStackPush(&ASTATStack[2], sRdReg("ASTAT.C"));
						stackPush(&MSTATStack, RdReg("_MSTAT"));
					}
					if(Opr1 && !strcasecmp(Opr1, "STS")){	/* if STS */
						sStackPush(&ASTATStack[0], sRdReg("ASTAT.R"));
						sStackPush(&ASTATStack[1], sRdReg("ASTAT.I"));
						sStackPush(&ASTATStack[2], sRdReg("ASTAT.C"));
						stackPush(&MSTATStack, RdReg("_MSTAT"));
					}
					sFlagEffect(p->InstType, so2, so2, so2, so2, trueMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iRCCW_C:
		case	iRCW_C:
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isXOP12(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 50a */
					/* [IF COND] RCCW.C DREG24, XOP24, YOP12 */
					/* [IF COND] RCCW.C Op0,    Op1,   Op2   */
					p->InstType = t50a;

					scplx sct1 = scRdReg(Opr1);
					scplx sct2;
					sct2.r = sRdReg(Opr2);

					sProcessALU_CFunc(p, sct1, sct2, Opr0, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			} 
			break;
		case	iRECT_C:
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 42a */
					/* [IF COND] RECT.C DREG24, XOP24[*] */
					/* [IF COND] RECT.C Op0,    Op1[*]   */
					p->InstType = t42a;

					scplx sct1 = scRdReg(Opr1);
					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct1.i.dp[j] = -sct1.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					sProcessCordicFunc(p, sct1, Opr0, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iRESET:
			if(!p->Cond) p->Cond = strdup("TRUE");
			/* now p->Cond always exists */

			if(ifCond(p->Cond)){
				/* type 48a */
				/* [IF COND] RESET */
				p->InstType = t48a;

				/* reset registers & stacks */
				resetSim();

				/* reset PC */
				/*
				NextCode = icode.FirstNode;
				isResetHappened = TRUE;
				*/
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iRNDACC:
			/* type 41a */
			/* [IF COND] RNDACC ACC32 */
			/* [IF COND] RNDACC Op0   */
			if(isACC32(p, Opr0)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					p->InstType = t41a;

					sint stemp5 = sRdReg(Opr0);

					/* Rounding operation */
					/* when result is 0x800, add 1 to bit 11 */
					/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
					/* Note that after rounding, the content of ACCx.L is INVALID. */
					stemp5 = sCalcRounding(stemp5);
					/*
					if(isUnbiasedRounding() && (temp5 == 0x800)) {	//unbiased rounding only
						temp5 = 0x0;
					} else {	
						temp5 += 0x800;
					}
					*/

					sint sv = sOVCheck(p->InstType, stemp5, so2);
					sWrReg(Opr0, stemp5, condMask);
			
					sFlagEffect(p->InstType, so2, so2, sv, so2, condMask);
				} 
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iRNDACC_C:
			/* type 41b */
			/* [IF COND] RNDACC.C ACC64[*] */
			/* [IF COND] RNDACC.C Op0[*]   */
			if(isACC64(p, Opr0)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					p->InstType = t41b;
	
					scplx sct5 = scRdReg(Opr0);
					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct5.i.dp[j] = -sct5.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					/* Rounding operation */
					/* when result is 0x800, add 1 to bit 11 */
					/* refer to pp.97 of ADSP-219x DSP Inst Set Reference Rev 2.0 */
					/* Note that after rounding, the content of ACCx.L is INVALID. */
					sct5.r = sCalcRounding(sct5.r);
					/*
					if(isUnbiasedRounding() && (ct5.r == 0x800)) {	//unbiased rounding only
						ct5.r = 0x0;
					} else {	
						ct5.r += 0x800;
					}
					*/
					sct5.i = sCalcRounding(sct5.i);
					/*
					if(isUnbiasedRounding() && (ct5.i == 0x800)) {	//unbiased rounding only 
						ct5.i = 0x0;
					} else {	
						ct5.i += 0x800;
					}
					*/

					scplx sv2;
					sv2.r = sOVCheck(p->InstType, sct5.r, so2);
					sv2.i = sOVCheck(p->InstType, sct5.i, so2);
					scWrReg(Opr0, sct5.r, sct5.i, condMask);
			
					scFlagEffect(p->InstType, sco2, sco2, sv2, sco2, condMask);
				} 
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iRTS:
		case	iRTI:
			/* type 20a */
			/* [IF COND] RTS/RTI */
			if(!p->Cond) p->Cond = strdup("TRUE");
			/* now p->Cond always exists */

			if(ifCond(p->Cond)){
				p->InstType = t20a;

				NextCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&PCStack));
				stackPop(&PCStack);
				isBranchTaken = TRUE;

				if(DelaySlotMode){
					sICode *pn = getNextCode(p);
					if(pn->isDelaySlot){
						pn->BrTarget = NextCode;	/* save branch target address in delay slot */
					}else{		/* error */
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Delay slot processing error. Please report.\n");
					}
				}

				sFlagEffect(p->InstType, so2, so2, so2, so2, trueMask);
			} 
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iSATACC:
			/* type 25a */
			/* [IF COND] SATACC ACC32 */
			/* [IF COND] SATACC Opr0  */
			if(isACC32(p, Opr0)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					p->InstType = t25a;

					sint stemp5 = sRdReg(Opr0);

					for(int j = 0; j < NUMDP; j++) {
						int mul_ov;
						if(((stemp5.dp[j] & 0x0FF800000) == 0x0FF800000) ||((stemp5.dp[j] & 0xFF800000) == 0)){
							mul_ov = FALSE;
						}else{
							mul_ov = TRUE;
						}

						if(mul_ov){			/* if overflow */
							if(stemp5.dp[j] >= 0){		/* if MSB of ACCx.H is 0 */
								stemp5.dp[j] = 0x007FFFFF;		/* set to max positive number */
							} else {			/* if MSB of ACCx.H is 1 */
								stemp5.dp[j] = 0xFF800000;		/* set to min negative number */
							}
						}
					}
					sWrReg(Opr0, stemp5, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iSATACC_C:
			/* type 25b */
			/* [IF COND] SATACC.C ACC64[*] */
			/* [IF COND] SATACC.C Opr0[*]  */
			if(isACC64(p, Opr0)){
				if(!p->Cond) p->Cond = strdup("TRUE");
					/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					p->InstType = t25b;

					scplx sct5 = scRdReg(Opr0);

					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct5.i.dp[j] = -sct5.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					for(int j = 0; j < NUMDP; j++) {
						cplx mul_ov;
						if(((sct5.r.dp[j] & 0x0FF800000) == 0x0FF800000) ||((sct5.r.dp[j] & 0xFF800000) == 0)){
							mul_ov.r = FALSE;
						}else{
							mul_ov.r = TRUE;
						}
						if(((sct5.i.dp[j] & 0x0FF800000) == 0x0FF800000) ||((sct5.i.dp[j] & 0xFF800000) == 0)){
							mul_ov.i = FALSE;
						}else{
							mul_ov.i = TRUE;
						}

						if(mul_ov.r){			/* if overflow */
							if(sct5.r.dp[j] >= 0){		/* if MSB of ACCx.H is 0 */
								sct5.r.dp[j] = 0x007FFFFF;		/* set to max positive number */
							} else {			/* if MSB of ACCx.H is 1 */
								sct5.r.dp[j] = 0xFF800000;		/* set to min negative number */
							}
						}
						if(mul_ov.i){			/* if overflow */
							if(sct5.i.dp[j] >= 0){		/* if MSB of ACCx.H is 0 */
								sct5.i.dp[j] = 0x007FFFFF;		/* set to max positive number */
							} else {			/* if MSB of ACCx.H is 1 */
								sct5.i.dp[j] = 0xFF800000;		/* set to min negative number */
							}
						}
					}
					scWrReg(Opr0, sct5.r, sct5.i, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iSCR:
			if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 46a */
					/* [IF COND] SCR DREG12, XOP12, YOP12 */
					/* [IF COND] SCR Op0,    Op1,   Op2   */
					p->InstType = t46a;

					sint stemp1 = sRdReg(Opr1);
					sint stemp2 = sRdReg(Opr2);
					sProcessALUFunc(p, stemp1, stemp2, Opr0, condMask);
				} 
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iSCR_C:
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isXOP24(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 46b */
					/* [IF COND] SCR.C DREG24, XOP24, YOP24[*] */
					/* [IF COND] SCR.C Op0,    Op1,   Op2[*]   */
					p->InstType = t46b;

					scplx sct1 = scRdReg(Opr1);
					scplx sct2 = scRdReg(Opr2);
					if(p->Conj) {
						for(int j = 0; j < NUMDP; j++) {
							sct2.i.dp[j] = -sct2.i.dp[j];	/* CONJ(*) modifier */
						}
					}

					sProcessALU_CFunc(p, sct1, sct2, Opr0, condMask);
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			} 
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
        case    iSETINT:
            if(isInt(Opr0)) {
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				if(ifCond(p->Cond)){
                	/* type 37a */
                	/* [IF COND] SETINT IMM_UINT4   */
                	/* [IF COND] SETINT Op0 */
                	p->InstType = t37a;

                	//temp1 = 0x0F & getIntSymAddr(p, symTable, Opr0);
					int temp1 = getIntImm(p, getIntSymAddr(p, symTable, Opr0), eIMM_UINT4);

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
					sFlagEffect(p->InstType, so2, so2, so2, so2, trueMask);
                }
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
            }
            break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iST:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isInt(Opr0) && (Opr1 != NULL) && !strcasecmp(Opr1, "DM") 
				&& (Opr2 != NULL) && isDReg12(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 3f */
					/* [IF COND] ST DM(IMM_UINT16), DREG12 */
					/* [IF COND] ST Op1(Op0),        Op2    */
					p->InstType = t03f;

					//int tAddr = 0x0FFFF & getIntSymAddr(p, symTable, Opr0);
					int tAddr = 0xFFFF & getIntImm(p, getIntSymAddr(p, symTable, Opr0), eIMM_UINT16);

					sint tData = sRdReg(Opr2);
					sWrDataMem(tData, tAddr, condMask);

#ifdef VHPI
					if(VhpiMode){	/* wh: write 12b */
						dsp_wh(tAddr, (short int *)&tData.dp[0], 1);
					}
#endif
				}
			} else if(isInt(Opr0) 
				&& (Opr1 != NULL)
				&& (!strcmp(Opr1, "+") || !strcmp(Opr1, "["))
				&& isInt(Opr2) && (Opr3 != NULL) && !strcasecmp(Opr3, "DM")
				&& isDReg12(p, Opr4)){
				/* type 3f */
				/* Variation due to .VAR usage */
				/* [IF COND] ST DM(IMM_UINT16 +   const ), DREG12 */
				/* [IF COND] ST DM(IMM_UINT16 [   const]), DREG12 */
				/* [IF COND] ST Op3(Op0       Op1 Op2   ), Op4    */
				p->InstType = t03f;

				printRunTimeError(p->LineCntr, Opr0, 
					"This case should not happen. Please report.\n");
				break;
			} else if(isInt(Opr0) && (Opr1 != NULL) && !strcasecmp(Opr1, "DM") 
				&& (Opr2 != NULL) && isRReg16(p, Opr2)){
				/* type 3b */
				/* ST DM(<IMM_INT16>), RREG16 */
				/* ST Op1(Op0),        Op2    */
				printRunTimeError(p->LineCntr, Opr2, 
					"ST DM(<IMM_INT16>), RREG16 syntax not allowed.\n");
				break;

				/*
				p->InstType = t03b;
				WrDataMem(RdReg(Opr2), 0x0FFFF & getIntSymAddr(p, symTable, Opr0));
				*/
			} else if(isInt(Opr0) && (Opr1 != NULL) && !strcasecmp(Opr1, "DM") 
				&& (Opr2 != NULL) && isACC32(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 3h */
					/* [IF COND] ST DM(IMM_UINT16), ACC32, (|HI,LO|) */
					/* [IF COND] ST Op1(Op0),        Op2,   (Op3)    */
					/* Note: store upper or lower **24** bits of 32-bit accumulator data */
					p->InstType = t03h;

					if(!Opr3){
						printRunTimeError(p->LineCntr, Opr0, 
							"(HI) or (LO) required. Please check syntax.\n");
						break;
					}

					sint stemp1;
					stemp1 = sRdReg(Opr2);

					sint tData1, tData2;
					for(int j = 0; j < NUMDP; j++) {
						if(!strcasecmp(Opr3, "HI")) stemp1.dp[j] >>= 8;

						tData1.dp[j] = 0x0FFF & stemp1.dp[j];				/* get low word of acc  */
						tData2.dp[j] = (0x0FFF000 & stemp1.dp[j]) >> 12;	/* get high word of acc */
					}

					//int tAddr = 0x0FFFF & (getIntSymAddr(p, symTable, Opr0));
					int tAddr = 0xFFFF & getIntImm(p, getIntSymAddr(p, symTable, Opr0), eIMM_UINT16);
					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr, Opr0);

					sWrDataMem(tData1, tAddr, condMask);					/* write low word  */
					sWrDataMem(tData2, tAddr2, condMask);					/* write high word */

#ifdef VHPI
					if(VhpiMode){	/* ww: write 24b */
						int tData = (((0x0FFF & tData1.dp[0]) << 12) | (0x0FFF & tData2.dp[0]));	
						dsp_ww(tAddr, &tData, 1);
					}
#endif
				}
			} else if(isInt(Opr0) 
				&& (Opr1 != NULL)
				&& (!strcmp(Opr1, "+") || !strcmp(Opr1, "["))
				&& isInt(Opr2) && (Opr3 != NULL) && !strcasecmp(Opr3, "DM")
				&& isACC32(p, Opr4)){
				/* type 3h */
				/* Variation due to .VAR usage */
				/* [IF COND] ST DM(IMM_UINT16 +   const ), ACC32, (|HI,LO|) */
				/* [IF COND] ST DM(IMM_UINT16 [   const]), ACC32, (|HI,LO|) */
				/* [IF COND] ST Op3(Op0       Op1 Op2),    Op4,   (Op5)    */
				/* Note: store upper or lower **24** bits of 32-bit accumulator data */
				p->InstType = t03h;

				printRunTimeError(p->LineCntr, Opr0, 
					"This case should not happen. Please report.\n");
				break;
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "DM") && isRReg(p, Opr4)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 32c */
					/* [IF COND] ST DM(IREG += MREG), RREG */
					/* [IF COND] ST Op3(Op1 Op0 Op2), Op4  */
					p->InstType = t32c;

					int tAddr = 0xFFFF & RdReg2(p, Opr1);
					sint tData = sRdReg(Opr4);
					sWrDataMem(tData, (unsigned int)tAddr, condMask);

#ifdef VHPI
					if(VhpiMode){	/* wh: write 12b */
						dsp_wh(tAddr, (short int *)&tData.dp[0], 1);
					}
#endif

					/* postmodify: update Ix */
					tAddr += RdReg2(p, Opr2);
					updateIReg(Opr1, tAddr);
				}
			} else if(!strcmp(Opr0, "+") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "DM") && isRReg(p, Opr4)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 32c */
					/* [IF COND] ST DM(IREG + MREG), RREG */
					/* [IF COND] ST Op3(Op1 Op0 Op2), Op4  */
					p->InstType = t32c;

					/* premodify: don't update Ix */
					int tAddr = 0xFFFF & (RdReg2(p, Opr1) + RdReg2(p, Opr2));
					sint tData = sRdReg(Opr4);
					sWrDataMem(tData, (unsigned int)tAddr, condMask);

#ifdef VHPI
					if(VhpiMode){	/* wh: write 12b */
						dsp_wh(tAddr, (short int *)&tData.dp[0], 1);
					}
#endif
				}
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "PM") && isRReg(p, Opr4)){
				/* type 32c */
				/* [IF COND] ST PM(IREG += MREG), RREG */
				/* [IF COND] ST Op3(Op1 Op0 Op2), Op4  */
				printRunTimeError(p->LineCntr, Opr4, 
					"Accessing Program Memory not supported.\n");
				break;
			} else if(!strcmp(Opr0, "+") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "PM") && isRReg(p, Opr4)){
				/* type 32c */
				/* [IF COND] ST PM(IREG + MREG), RREG */
				/* [IF COND] ST Op3(Op1 Op0 Op2), Op4  */
				printRunTimeError(p->LineCntr, Opr4, 
					"Accessing Program Memory not supported.\n");
				break;
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isInt(Opr2)
				&& !strcasecmp(Opr3, "DM") && isDReg12(p, Opr4)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 29c */
					/* [IF COND] ST DM(IREG += <IMM_INT8>), DREG12 */
					/* [IF COND] ST Op3(Op1 Op0 Op2      ), Op4    */
					p->InstType = t29c;

					int tAddr = 0xFFFF & RdReg2(p, Opr1);
					sint tData = sRdReg(Opr4);
					sWrDataMem(tData, (unsigned int)tAddr, condMask);

#ifdef VHPI
					if(VhpiMode){	/* wh: write 12b */
						dsp_wh(tAddr, (short int *)&tData.dp[0], 1);
					}
#endif

					/* postmodify: update Ix */
					//int tOffset = 0x0FF & getIntSymAddr(p, symTable, Opr2);
					int tOffset = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT8);

					tAddr += tOffset;
					updateIReg(Opr1, tAddr);
				}
			} else if(!strcmp(Opr0, "+") && isIReg(p, Opr1) && isInt(Opr2)
				&& !strcasecmp(Opr3, "DM") && isDReg12(p, Opr4)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 29c */
					/* [IF COND] ST DM(IREG + <IMM_INT8>), DREG12 */
					/* [IF COND] ST Op3(Op1 Op0 Op2     ), Op4    */
					p->InstType = t29c;

					/* premodify: don't update Ix */
					//int tOffset = 0x0FF & getIntSymAddr(p, symTable, Opr2);
					int tOffset = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT8);

					int tAddr = 0xFFFF & (RdReg2(p, Opr1) + tOffset);
					sint tData = sRdReg(Opr4);
					sWrDataMem(tData, (unsigned int)tAddr, condMask);

#ifdef VHPI
					if(VhpiMode){	/* wh: write 12b */
						dsp_wh(tAddr, (short int *)&tData.dp[0], 1);
					}
#endif
				}
			} else if(isIReg(p, Opr0) && !strcasecmp(Opr1, "DM")
				&& isDReg12(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 29c */
					/* [IF COND] ST DM(IREG), DREG12 */
					/* [IF COND] ST Op1(Op0), Op2    */
					p->InstType = t29c;

					int tAddr = 0xFFFF & RdReg2(p, Opr0);
					sint tData = sRdReg(Opr2);
					sWrDataMem(tData, (unsigned int)tAddr, condMask);

#ifdef VHPI
					if(VhpiMode){	/* wh: write 12b */
						dsp_wh(tAddr, (short int *)&tData.dp[0], 1);
					}
#endif
				}
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "DM") && isInt(Opr4)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 22a */
					/* [IF COND] ST DM(IREG += MREG), <IMM_INT12> */
					/* [IF COND] ST Op3(Op1 Op0 Op2), Op4         */
					p->InstType = t22a;

					int tAddr = 0xFFFF & RdReg2(p, Opr1);
					sint tData;

					int imm12 = getIntImm(p, getIntSymAddr(p, symTable, Opr4), eIMM_INT12);			
					for(int j = 0; j < NUMDP; j++) {
						tData.dp[j] = imm12;
					}
					sWrDataMem(tData, (unsigned int)tAddr, condMask);

#ifdef VHPI
					if(VhpiMode){	/* wh: write 12b */
						dsp_wh(tAddr, (short int *)&tData.dp[0], 1);
					}
#endif

					/* postmodify: update Ix */
					tAddr += RdReg2(p, Opr2);
					updateIReg(Opr1, tAddr);
				}
			} else if(!strcmp(Opr0, "+") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "DM") && isInt(Opr4)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 22a */
					/* [IF COND] ST DM(IREG + MREG), <IMM_INT12> */
					/* [IF COND] ST Op3(Op1 Op0 Op2), Op4         */
					p->InstType = t22a;

					/* premodify: don't update Ix */
					int tAddr = 0xFFFF & (RdReg2(p, Opr1) + RdReg2(p, Opr2));
					sint tData;

					int imm12 = getIntImm(p, getIntSymAddr(p, symTable, Opr4), eIMM_INT12);			
					for(int j = 0; j < NUMDP; j++) {
						tData.dp[j] = imm12;
					}
					sWrDataMem(tData, (unsigned int)tAddr, condMask);

#ifdef VHPI
					if(VhpiMode){	/* wh: write 12b */
						dsp_wh(tAddr, (short int *)&tData.dp[0], 1);
					}
#endif
				}
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "PM") && isInt(Opr4)){
				/* type 22b */
				/* ST PM(IREG += MREG), <IMM_INT16> */
				/* ST Op3(Op1 Op0 Op2), Op4         */
				printRunTimeError(p->LineCntr, Opr4, 
					"Accessing Program Memory not supported.\n");
				break;
			} else if(isSysCtlReg(Opr0) && isDReg12(p, Opr1)){
				/* type 35a */
				/* ST CACTL, DREG12 */
				/* ST Op0,   Op1    */
				printRunTimeError(p->LineCntr, Opr1, 
					"CACTL Register not supported.\n");
				break;
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iST_C:
			if(isMultiFunc(p)){
				p = asmSimOneStepMultiFunc(p);
				break;
			}

			if(isInt(Opr0) && (Opr1 != NULL) && !strcasecmp(Opr1, "DM") 
				&& (Opr2 != NULL) && isDReg24(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 3g */
					/* [IF COND] ST.C DM(IMM_UINT16), DREG24 */
					/* [IF COND] ST.C Op1(Op0),        Op2    */
					p->InstType = t03g;

					scplx scData;
					scData = scRdReg(Opr2);
					int tAddr = 0xFFFF & getIntImm(p, getIntSymAddr(p, symTable, Opr0), eIMM_UINT16);

					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr, Opr0);

					sWrDataMem(scData.r, tAddr, condMask);
					sWrDataMem(scData.i, tAddr2, condMask);

#ifdef VHPI
					if(VhpiMode){	/* ww: write 24b */
						int tData = (((0x0FFF & scData.r.dp[0]) << 12) | (0x0FFF & scData.i.dp[0]));	
						dsp_ww(tAddr, &tData, 1);
					}
#endif
				}
			} else if(isInt(Opr0) 
				&& (Opr1 != NULL)
				&& (!strcmp(Opr1, "+") || !strcmp(Opr1, "["))
				&& isInt(Opr2) && (Opr3 != NULL) && !strcasecmp(Opr3, "DM")
				&& isDReg24(p, Opr4)){
				/* type 3g */
				/* Variation due to .VAR usage */
				/* [IF COND] ST.C DM(IMM_UINT16 +   const ), DREG24 */
				/* [IF COND] ST.C DM(IMM_UINT16 [   const]), DREG24 */
				/* [IF COND] ST.C Op3(Op0       Op1 Op2   ), Op4    */
				p->InstType = t03g;

				printRunTimeError(p->LineCntr, Opr0, 
					"This case should not happen. Please report.\n");
				break;

			} else if(isInt(Opr0) && (Opr1 != NULL) && !strcasecmp(Opr1, "DM") 
				&& (Opr2 != NULL) && isACC64(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 3i */
					/* [IF COND] ST.C DM(IMM_UINT16), ACC64, (|HI,LO|) */
					/* [IF COND] ST.C Op1(Op0),        Op2,   (Op3)    */
					/* Note: store upper or lower **24** bits of 32-bit accumulator data (complex pair) */
					p->InstType = t03i;

					if(!Opr3){
						printRunTimeError(p->LineCntr, Opr0, 
							"(HI) or (LO) required. Please check syntax.\n");
						break;
					}

					scplx sct1 = scRdReg(Opr2);
					if(!strcasecmp(Opr3, "HI")) {
						for(int j = 0; j < NUMDP; j++) {
							sct1.r.dp[j] >>= 8;		/* real part */
							sct1.i.dp[j] >>= 8;		/* imaginary part */
						}
					}

					int tAddr = 0xFFFF & getIntImm(p, getIntSymAddr(p, symTable, Opr0), eIMM_UINT16);
					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr, Opr0);

					scplx scData1, scData2;
	
					for(int j = 0; j < NUMDP; j++) {
						/* read real part 24 bits */
						scData1.r.dp[j] = 0x0FFF & sct1.r.dp[j];					/* get low word of acc  */
						scData1.i.dp[j] = (0x0FFF000 & sct1.r.dp[j]) >> 12;		/* get high word of acc */
						/* read imaginary part 24 bits */
						scData2.r.dp[j] = 0x0FFF & sct1.i.dp[j];					/* get low word of acc  */
						scData2.i.dp[j] = (0x0FFF000 & sct1.i.dp[j]) >> 12;		/* get high word of acc */
					}

					/* write real part 24 bits */
					sWrDataMem(scData1.r, tAddr, condMask);					/* write low word  */
					sWrDataMem(scData1.i, tAddr2, condMask);				/* write high word */
					/* write imaginary part 24 bits */
					sWrDataMem(scData2.r, tAddr+2, condMask);				/* write low word  */
					sWrDataMem(scData2.i, tAddr2+2, condMask);				/* write high word */

#ifdef VHPI
					if(VhpiMode){	/* ww+ww: write 24b+24b */
						int tData1 = (((0x0FFF & scData1.r.dp[0]) << 12) | (0x0FFF & scData1.i.dp[0]));	
						int tData2 = (((0x0FFF & scData2.r.dp[0]) << 12) | (0x0FFF & scData2.i.dp[0]));	
						dsp_ww_ww(tAddr, &tData1, tAddr+2, &tData2, 1);
					}
#endif
				}
			} else if(isInt(Opr0) 
				&& (Opr1 != NULL)
				&& (!strcmp(Opr1, "+") || !strcmp(Opr1, "["))
				&& isInt(Opr2) && (Opr3 != NULL) && !strcasecmp(Opr3, "DM")
				&& isACC64(p, Opr4)){
				/* type 3i */
				/* Variation due to .VAR usage */
				/* [IF COND] ST.C DM(IMM_UINT16 +   const ), ACC64, (|HI,LO|) */
				/* [IF COND] ST.C DM(IMM_UINT16 [   const]), ACC64, (|HI,LO|) */
				/* [IF COND] ST.C Op3(Op0       Op1 Op2),    Op4,   (Op5)     */
				/* Note: store upper or lower **24** bits of 32-bit accumulator data */
				p->InstType = t03i;

				printRunTimeError(p->LineCntr, Opr0, 
					"This case should not happen. Please report.\n");
				break;
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "DM") && isCReg(p, Opr4)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 32d */
					/* [IF COND] ST.C DM(IREG += MREG), DREG24 */
					/* [IF COND] ST.C Op3(Op1 Op0 Op2), Op4  */
					p->InstType = t32d;

					scplx scData = scRdReg(Opr4);

					int tAddr = 0xFFFF & RdReg2(p, Opr1);
					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr, Opr1);

					sWrDataMem(scData.r, (unsigned int)tAddr, condMask);
					sWrDataMem(scData.i, (unsigned int)tAddr2, condMask);

#ifdef VHPI
					if(VhpiMode){	/* ww: write 24b */
						int tData = (((0x0FFF & scData.r.dp[0]) << 12) | (0x0FFF & scData.i.dp[0]));	
						dsp_ww(tAddr, &tData, 1);
					}
#endif

					/* postmodify: update Ix */
					tAddr += RdReg2(p, Opr2);
					updateIReg(Opr1, tAddr);
				}
			} else if(!strcmp(Opr0, "+") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "DM") && isCReg(p, Opr4)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 32d */
					/* [IF COND] ST.C DM(IREG + MREG),  DREG24 */
					/* [IF COND] ST.C Op3(Op1 Op0 Op2), Op4  */
					p->InstType = t32d;

					/* premodify: don't update Ix */
					scplx scData = scRdReg(Opr4);

					int tAddr = 0xFFFF & (RdReg2(p, Opr1) + RdReg2(p, Opr2));
					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr, Opr1);

					sWrDataMem(scData.r, (unsigned int)tAddr, condMask);
					sWrDataMem(scData.i, (unsigned int)tAddr2, condMask);

#ifdef VHPI
					if(VhpiMode){	/* ww: write 24b */
						int tData = (((0x0FFF & scData.r.dp[0]) << 12) | (0x0FFF & scData.i.dp[0]));	
						dsp_ww(tAddr, &tData, 1);
					}
#endif
				}
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isInt(Opr2)
				&& !strcasecmp(Opr3, "DM") && isDReg24(p, Opr4)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 29d */
					/* [IF COND] ST.C DM(IREG += <IMM_INT8>), DREG24 */
					/* [IF COND] ST.C Op3(Op1 Op0 Op2      ), Op4    */
					p->InstType = t29d;

					scplx scData = scRdReg(Opr4);

					int tAddr = 0xFFFF & RdReg2(p, Opr1);
					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr, Opr1);

					sWrDataMem(scData.r, (unsigned int)tAddr, condMask);
					sWrDataMem(scData.i, (unsigned int)tAddr2, condMask);

#ifdef VHPI
					if(VhpiMode){	/* ww: write 24b */
						int tData = (((0x0FFF & scData.r.dp[0]) << 12) | (0x0FFF & scData.i.dp[0]));	
						dsp_ww(tAddr, &tData, 1);
					}
#endif

					/* postmodify: update Ix */
					//int tOffset = 0x0FF & getIntSymAddr(p, symTable, Opr2);
					int tOffset = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT8);

					tAddr += tOffset;
					updateIReg(Opr1, tAddr);
				}
			} else if(!strcmp(Opr0, "+") && isIReg(p, Opr1) && isInt(Opr2)
				&& !strcasecmp(Opr3, "DM") && isDReg24(p, Opr4)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 29d */
					/* [IF COND] ST.C DM(IREG + <IMM_INT8>), DREG24 */
					/* [IF COND] ST.C Op3(Op1 Op0 Op2     ), Op4    */
					p->InstType = t29d;

					/* premodify: don't update Ix */
					scplx scData = scRdReg(Opr4);
					int tAddr = 0xFFFF & RdReg2(p, Opr1);
					//int tOffset = 0x0FF & getIntSymAddr(p, symTable, Opr2);
					int tOffset = getIntImm(p, getIntSymAddr(p, symTable, Opr2), eIMM_INT8);
					tAddr += tOffset;

					int tAddr2 = checkUnalignedMemoryAccess(p, tAddr, Opr1);

					sWrDataMem(scData.r, (unsigned int)tAddr, condMask);
					sWrDataMem(scData.i, (unsigned int)tAddr2, condMask);

#ifdef VHPI
					if(VhpiMode){	/* ww: write 24b */
						int tData = (((0x0FFF & scData.r.dp[0]) << 12) | (0x0FFF & scData.i.dp[0]));	
						dsp_ww(tAddr, &tData, 1);
					}
#endif
				}
			} else if(isIReg(p, Opr0) && !strcasecmp(Opr1, "DM")
				&& isDReg24(p, Opr2)){
				if(!p->Cond) p->Cond = strdup("TRUE");
				/* now p->Cond always exists */

				//if(ifCond(p->Cond)){
				if(sIfCond(p->Cond, &condMask)){
					/* type 29d */
					/* [IF COND] ST.C DM(IREG), DREG24 */
					/* [IF COND] ST.C Op1(Op0), Op2    */
					p->InstType = t29d;

					scplx scData = scRdReg(Opr2);
					int tAddr = 0xFFFF & RdReg2(p, Opr0);

					sWrDataMem(scData.r, (unsigned int)tAddr, condMask);
					sWrDataMem(scData.i, (unsigned int)tAddr+1, condMask);

#ifdef VHPI
					if(VhpiMode){	/* ww: write 24b */
						int tData = (((0x0FFF & scData.r.dp[0]) << 12) | (0x0FFF & scData.i.dp[0]));	
						dsp_ww(tAddr, &tData, 1);
					}
#endif
				}
			} else if(isInt(Opr0) && (Opr1 != NULL) && !strcasecmp(Opr1, "DM") 
				&& (Opr2 != NULL) && isRReg16(p, Opr2)){
				/* type: Not allowed!!  */
				/* ST.C DM(<IMM_INT16>), RREG16 */
				/* ST.C Op1(Op0),        Op2    */
				printRunTimeError(p->LineCntr, Opr2, 
					"ST.C does not support this register operand.\n");
				break;
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	i_CODE:
			/* assembler pseudo instructions: do nothing here */
			break;
		case	i_DATA:
			/* assembler pseudo instructions: do nothing here */
			break;
		case	i_VAR:
			/* assembler pseudo instructions: do nothing here */
			break;
		case	i_GLOBAL:
			/* assembler pseudo instructions: do nothing here */
			break;
		case	i_EXTERN:
			/* assembler pseudo instructions: do nothing here */
		case	i_EQU:
			/* assembler pseudo instructions: do nothing here */
			break;
		default:		
			printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
				"Invalid instruction! Please check instruction syntax.\n");
			break;
	}

	/* added for reset instruction (2010.02.11) */
	if(isResetHappened){		/* if reset */
		isResetHappened = FALSE;	/* toggle flag */
		;	/* NextCode = icode.FirstNode */
	} else if(stackEmpty(&LoopBeginStack)){		/* no loop running */
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
	}else{							/* loop running */
		if(!DelaySlotMode){			/* if delay slot disabled */
			if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
				if(isBranchTaken){			/* if branch taken */
					isBranchTaken = FALSE;	/* reset flag      */
				} else if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
					loopCntr = stackTop(&LoopCounterStack);
					stackPop(&LoopCounterStack);
					stackPush(&LoopCounterStack, loopCntr-1);
					NextCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
				}else{			/* DO UNTIL CE */
					loopCntr = stackTop(&LoopCounterStack);
					if(loopCntr > 1){ /* decrement counter */
						stackPop(&LoopCounterStack);
						stackPush(&LoopCounterStack, loopCntr-1);
						NextCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
					}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
						stackPop(&LoopCounterStack);
						loopCntr = 0xFFFF;
						stackPush(&LoopCounterStack, loopCntr);
						NextCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
					}else{	/* exit loop if loopCntr == 1 */
						stackPop(&LoopBeginStack);
						WrReg("_PCSTACK", stackTop(&LoopBeginStack));
						stackPop(&LoopEndStack);
						WrReg("_LPSTACK", stackTop(&LoopEndStack));
						stackPop(&LoopCounterStack);
						WrReg("_CNTR", stackTop(&LoopCounterStack));
						stackPop(&LPEVERStack);
						WrReg("_LPEVER", stackTop(&LPEVERStack));

						sFlagEffect(t11a, so2, so2, so2, so2, trueMask);
						NextCode = p->Next;
					}
				}
			} else {	/* within the loop */
				if(isBranchTaken){			/* if branch taken */
					isBranchTaken = FALSE;	/* reset flag      */
				}else{
					NextCode = p->Next;
				}
			}
		}else{						/* if delay slot enabled */
			if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
				if(isBranchTaken){			/* if branch taken */
					isBranchTaken = FALSE;	/* reset flag      */

					/* error */
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"This case should not happen - Branch at the end of do-until block.\n");

				} else if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
					loopCntr = stackTop(&LoopCounterStack);
					stackPop(&LoopCounterStack);
					stackPush(&LoopCounterStack, loopCntr-1);
					NextCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
				}else{			/* DO UNTIL CE */
					loopCntr = stackTop(&LoopCounterStack);
					if(loopCntr > 1){ /* decrement counter */
						stackPop(&LoopCounterStack);
						stackPush(&LoopCounterStack, loopCntr-1);
						NextCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
					}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
						stackPop(&LoopCounterStack);
						loopCntr = 0xFFFF;
						stackPush(&LoopCounterStack, loopCntr);
						NextCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
					}else{	/* exit loop if loopCntr == 1 */
						stackPop(&LoopBeginStack);
						WrReg("_PCSTACK", stackTop(&LoopBeginStack));
						stackPop(&LoopEndStack);
						WrReg("_LPSTACK", stackTop(&LoopEndStack));
						stackPop(&LoopCounterStack);
						WrReg("_CNTR", stackTop(&LoopCounterStack));
						stackPop(&LPEVERStack);
						WrReg("_LPEVER", stackTop(&LPEVERStack));

						sFlagEffect(t11a, so2, so2, so2, so2, trueMask);
						NextCode = p->Next;
					}
				}
			} else {	/* within the loop */
				if(isBranchTaken){			/* if branch taken */
					isBranchTaken = FALSE;	/* reset flag      */
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
* @brief Simulate one multifunction assembly source line 
* 
* @param *p Pointer to current instruction 
* 
* @return Pointer to current instruction
*/
sICode *asmSimOneStepMultiFunc(sICode *p)
{
	sICode *m1, *m2;
	sint	so2;

	sint	trueMask = { 1, 1, 1, 1 };
	int loopCntr;

	/* init o2 constant */
	for(int j = 0; j < NUMDP; j++) {
		so2.dp[j] = 0;
	//	sco2.r.dp[j] = 0;
	//	sco2.i.dp[j] = 0;
	}

	switch(p->MultiCounter){
		case	1:				/* two-opcode multifunction */
			m1 = p->Multi[0];

			if(isLD(p) && isLD(m1)){
				/* type 1a */
				p->InstType = t01a;
				/* LD || LD */

				/* latency: if LD comes just after ST, need +1 cycle */
				sICode *lp = p->LastExecuted;
				if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
					p->LatencyAdded = p->LatencyAdded +1;

				if(!isReg12S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of LD, only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD, only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}

				if(!isIx(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For IX operand of LD, only I0, I1, I2, I3 registers are allowed.\n");
					break;
				}
				if(!isMx(p, p->Operand[3])){
					printRunTimeError(p->LineCntr, p->Operand[3], 
						"For MX operand of LD, only M0, M1, M2, M3 registers are allowed.\n");
					break;
				}
				if(!isIy(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IY operand of LD, only I4, I5, I6, I7 registers are allowed.\n");
					break;
				}
				if(!isMy(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MY operand of LD, only M4, M5, M6, M7 registers are allowed.\n");
					break;
				}

				/* read first LD operands */
				/* LD XOP12S, DM(IREG +/+=  MREG) */
				/* LD Op0,  Op4(Op2 Op1   Op3) */
				int isPost1 = FALSE;
				if(!strcmp(p->Operand[1], "+=")) isPost1 = TRUE;

				int tAddr12 = 0xFFFF & RdReg2(p, p->Operand[2]);
				int tAddr13 = RdReg2(p, p->Operand[3]);

				/* if premodify: don't update Ix */
				if(!isPost1) tAddr12 += tAddr13;

				sint tData1 = sRdDataMem((unsigned int)tAddr12);

				/* read second LD operands */
				/* LD XOP12S, DM(IREG +/+=  MREG) */
				/* LD Op0,  Op4(Op2 Op1   Op3) */
				int isPost2 = FALSE;
				if(!strcmp(m1->Operand[1], "+=")) isPost2 = TRUE;

				int tAddr22 = 0xFFFF & RdReg2(p, m1->Operand[2]);
				int tAddr23 = RdReg2(p, m1->Operand[3]);

				/* if premodify: don't update Ix */
				if(!isPost2) tAddr22 += tAddr23;

				sint tData2 = sRdDataMem((unsigned int)tAddr22);

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

#ifdef VHPI
				if(VhpiMode){	/* rh+rh: read 12b+12b */
					dsp_rh_rh(tAddr12, (short int *)&tData1, tAddr22, (short int *)&tData2, 1);
					dsp_wait(1);
				}
#endif

				/* write first LD result */
				sWrReg(p->Operand[0], tData1, trueMask);

				/* write second LD result */
				sWrReg(m1->Operand[0], tData2, trueMask);

				/* if postmodify: update Ix */
				if(isPost1){
					tAddr12 += tAddr13;
					updateIReg(p->Operand[2], tAddr12);
				}

				/* if postmodify: update Ix */
				if(isPost2){
					tAddr22 += tAddr23;
					updateIReg(m1->Operand[2], tAddr22);
				}
			}else if(isLD_C(p) && isLD_C(m1)){
				/* type 1b */
				p->InstType = t01b;
				/* LD.C || LD.C */
				/* LD.C DREG24, DM(IREG +/+=  MREG) */
				/* LD.C Op0,    Op4(Op2 Op1 Op3)  */
				/*            or                  */
				/* LD.C DREG24, DM(IREG)          */
				/* LD.C Op0,    Op1(Op0)          */

				/* latency: if LD comes just after ST, need +1 cycle */
				sICode *lp = p->LastExecuted;
				if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
					p->LatencyAdded = p->LatencyAdded +1;

				if(!isReg24S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of LD.C, only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD.C, only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}

				if(!isIx(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For IX operand of LD.C, only I0, I1, I2, I3 registers are allowed.\n");
					break;
				}
				if(!isMx(p, p->Operand[3])){
					printRunTimeError(p->LineCntr, p->Operand[3], 
						"For MX operand of LD.C, only M0, M1, M2, M3 registers are allowed.\n");
					break;
				}
				if(!isIy(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IY operand of LD.C, only I4, I5, I6, I7 registers are allowed.\n");
					break;
				}
				if(!isMy(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MY operand of LD.C, only M4, M5, M6, M7 registers are allowed.\n");
					break;
				}

				/* read first LD operands */
				/* LD.C XOP24S, DM(IREG +/+=  MREG) */
				/* LD.C Op0,  Op4(Op2 Op1 Op3) */
				int isPost1 = FALSE;
				if(!strcmp(p->Operand[1], "+=")) isPost1 = TRUE;

				int tAddr12 = 0xFFFF & RdReg2(p, p->Operand[2]);
				int tAddr13 = RdReg2(p, p->Operand[3]);

				/* if premodify: don't update Ix */
				if(!isPost1) tAddr12  += tAddr13;

				int tAddr122 = checkUnalignedMemoryAccess(p, tAddr12, p->Operand[2]);

				sint tData11 = sRdDataMem((unsigned int)tAddr12);
				sint tData12 = sRdDataMem((unsigned int)tAddr122);

				/* read second LD operands */
				/* LD.C XOP24S, DM(IREG +/+= MREG) */
				/* LD.C Op0,    Op4(Op2 Op1  Op3)  */
				int isPost2 = FALSE;
				if(!strcmp(m1->Operand[1], "+=")) isPost2 = TRUE;

				int tAddr22 = 0xFFFF & RdReg2(p, m1->Operand[2]);
				int tAddr23 = RdReg2(p, m1->Operand[3]);

				/* if premodify: don't update Ix */
				if(!isPost2) tAddr22  += tAddr23;

				int tAddr222 = checkUnalignedMemoryAccess(p, tAddr22, m1->Operand[2]);

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				sint tData21 = sRdDataMem((unsigned int)tAddr22);
				sint tData22 = sRdDataMem((unsigned int)tAddr222);

#ifdef VHPI
				if(VhpiMode){	/* rw+rw: read 24b+24b */
					int tData1, tData2;

					dsp_rw_rw(tAddr12, &tData1, tAddr22, &tData2, 1);
					dsp_wait(1);

					/* Note:
					tData1 = (((0x0FFF & tData11) << 12) | (0x0FFF & tData12));	
					tData2 = (((0x0FFF & tData21) << 12) | (0x0FFF & tData22));	
					*/
					tData11.dp[0] = (0x0FFF & (tData1 >> 12));
					tData12.dp[0] = 0xFFF & tData1;
					tData21.dp[0] = (0x0FFF & (tData2 >> 12));
					tData22.dp[0] = 0xFFF & tData2;
				}
#endif

				/* write first LD result */
				scWrReg(p->Operand[0], tData11, tData12, trueMask);

				/* write second LD result */
				scWrReg(m1->Operand[0], tData21, tData22, trueMask);

				/* if postmodify: update Ix */
				if(isPost1){
					tAddr12 += tAddr13;
					updateIReg(p->Operand[2], tAddr12);
				}

				/* if postmodify: update Ix */
				if(isPost2){
					tAddr22 += tAddr23;
					updateIReg(m1->Operand[2], tAddr22);
				}
			}else if(isMAC(p) && isLD(m1)){
				/* type 4a */
				p->InstType = t04a;
				/* MAC || LD */

				int MoreLatencyRequired = FALSE;

				/* latency: if LD comes just after ST, need +1 cycle */
				sICode *lp = p->LastExecuted;
				if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
					MoreLatencyRequired = TRUE;

				/* latency: if last MAC (= next inst. is not MAC), need +1 cycle */
				/* 
				* NOTE: this code does NOT consider delay slot. (2010.07.20)
				*/
				sICode *NCode;
				if(stackEmpty(&LoopBeginStack)){		/* no loop running */
					NCode = p->Next;
					if(NCode && !isAnyMAC(NCode))
						MoreLatencyRequired = TRUE;
				}else{									/* loop running */
					if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
						if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
							NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
						}else{			/* DO UNTIL CE */
							loopCntr = stackTop(&LoopCounterStack);
							if(loopCntr > 1){ /* decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{	/* exit loop if loopCntr == 1 */
								NCode = p->Next;
							}
						}
					} else {	/* within the loop */
						NCode = p->Next;
					}
					if(NCode && !isAnyMAC(NCode))
						MoreLatencyRequired = TRUE;
				}

				if(MoreLatencyRequired)
					p->LatencyAdded = p->LatencyAdded +1;

				if(!isACC32S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC instruction, "
						"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
				if(isNONE(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of Multifunction instruction, "
						"NONE is not allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isDReg12(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}

				/* read MAC operands */
				sint stemp1 = sRdReg(p->Operand[1]);
				sint stemp2 = sRdReg(p->Operand[2]);

				/* read LD operands */
				/* LD DREG12, DM(IREG +/+= MREG) */
				/* LD Op0,    Op4(Op2 Op1  Op3)  */
				int isPost2 = FALSE;
				if(!strcmp(m1->Operand[1], "+=")) isPost2 = TRUE;

				int tAddr22 = 0xFFFF & RdReg2(p, m1->Operand[2]);
				int tAddr23 = RdReg2(p, m1->Operand[3]);

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				/* if premodify: don't update Ix */
				if(!isPost2) tAddr22 += tAddr23;

				sint tData2 = sRdDataMem((unsigned int)tAddr22);

				/* write MAC result */
				sProcessMACFunc(p, stemp1, stemp2, p->Operand[0], p->Operand[3], trueMask);

#ifdef VHPI
				if(VhpiMode){	/* rh: read 12b */
					dsp_rh(tAddr22, (short int *)&tData2.dp[0], 1);
					dsp_wait(1);
				}
#endif

				/* write LD result */
				sWrReg(m1->Operand[0], tData2, trueMask);

				/* if postmodify: update Ix */
				if(isPost2){
					tAddr22 += tAddr23;
					updateIReg(m1->Operand[2], tAddr22);
				}
			}else if(isALU(p) && isLD(m1)){
				/* type 4c */
				p->InstType = t04c;
				/* ALU || LD */

				/* latency: if LD comes just after ST, need +1 cycle */
				sICode *lp = p->LastExecuted;
				if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
					p->LatencyAdded = p->LatencyAdded +1;

				if(!isDReg12S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU instruction, "
						"only R0~R7 and ACC0.L~ACC7.L registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg12(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}

				/* read ALU operands */
				sint stemp1 = sRdReg(p->Operand[1]);
				sint stemp2;
				if(p->Operand[2]){
					stemp2 = sRdReg(p->Operand[2]);
				}else{			/* ABS/NOT/INC/DEC */
					stemp2 = so2;
				}

				/* read LD operands */
				/* LD DREG12, DM(IREG +/+= MREG) */
				/* LD Op0,    Op4(Op2 Op1  Op3) */
				int isPost2 = FALSE;
				if(!strcmp(m1->Operand[1], "+=")) isPost2 = TRUE;

				int tAddr22 = 0xFFFF & RdReg2(p, m1->Operand[2]);
				int tAddr23 = RdReg2(p, m1->Operand[3]);

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				/* if premodify: don't update Ix */
				if(!isPost2) tAddr22 += tAddr23;

				sint tData2 = sRdDataMem((unsigned int)tAddr22);

				/* write ALU result */
				sProcessALUFunc(p, stemp1, stemp2, p->Operand[0], trueMask);

#ifdef VHPI
				if(VhpiMode){	/* rh: read 12b */
					dsp_rh(tAddr22, (short int *)&tData2.dp[0], 1);
					dsp_wait(1);
				}
#endif
				/* write LD result */
				sWrReg(m1->Operand[0], tData2, trueMask);

				/* if postmodify: update Ix */
				if(isPost2){
					tAddr22 += tAddr23;
					updateIReg(m1->Operand[2], tAddr22);
				}
			}else if(isMAC_C(p) && isLD_C(m1)){
				/* type 4b */
				p->InstType = t04b;
				/* MAC.C || LD.C */

				int MoreLatencyRequired = FALSE;

				/* latency: if LD comes just after ST, need +1 cycle */
				sICode *lp = p->LastExecuted;
				if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
					MoreLatencyRequired = TRUE;

				/* latency: if last MAC (= next inst. is not MAC), need +1 cycle */
				/* 
				* NOTE: this code does NOT consider delay slot. (2010.07.20)
				*/
				sICode *NCode;
				if(stackEmpty(&LoopBeginStack)){		/* no loop running */
					NCode = p->Next;
					if(NCode && !isAnyMAC(NCode))
						MoreLatencyRequired = TRUE;
				}else{									/* loop running */
					if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
						if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
							NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
						}else{			/* DO UNTIL CE */
							loopCntr = stackTop(&LoopCounterStack);
							if(loopCntr > 1){ /* decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{	/* exit loop if loopCntr == 1 */
								NCode = p->Next;
							}
						}
					} else {	/* within the loop */
						NCode = p->Next;
					}
					if(NCode && !isAnyMAC(NCode))
						MoreLatencyRequired = TRUE;
				}

				if(MoreLatencyRequired)
					p->LatencyAdded = p->LatencyAdded +1;

				if(!isACC64S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC.C instruction, "
						"only ACC0, ACC2 registers are allowed.\n");
					break;
				}
				if(isNONE(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of Multifunction instruction, "
						"NONE is not allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isDReg24(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}

				/* read MAC operands */
				scplx sct1 = scRdReg(p->Operand[1]);
				scplx sct2 = scRdReg(p->Operand[2]);
				if(p->Conj) {
					for(int j = 0; j < NUMDP; j++) {
						sct2.i.dp[j] = -sct2.i.dp[j];	/* CONJ(*) modifier */
					}
				}

				/* read LD operands */
				/* LD.C DREG24, DM(IREG +/+= MREG) */
				/* LD.C Op0,    Op4(Op2 Op1  Op3) */
				int isPost2 = FALSE;
				if(!strcmp(m1->Operand[1], "+=")) isPost2 = TRUE;

				int tAddr22 = 0xFFFF & RdReg2(p, m1->Operand[2]);
				int tAddr23 = RdReg2(p, m1->Operand[3]);

				/* if premodify: don't update Ix */
				if(!isPost2) tAddr22 += tAddr23;

				int tAddr222 = checkUnalignedMemoryAccess(p, tAddr22, m1->Operand[2]);

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				sint tData21 = sRdDataMem((unsigned int)tAddr22);
				sint tData22 = sRdDataMem((unsigned int)tAddr222);

				/* write MAC result */
				sProcessMAC_CFunc(p, sct1, sct2, p->Operand[0], p->Operand[3], trueMask);

#ifdef VHPI
				if(VhpiMode){	/* rw: read 24b */
					int tData1;

					dsp_rw(tAddr22, &tData1, 1);
					dsp_wait(1);

					/* Note: tData1 = (((0x0FFF & tData21) << 12) | (0x0FFF & tData22)); */
					tData21.dp[0] = (0x0FFF & (tData1 >> 12));
					tData22.dp[0] = 0xFFF & tData1;
				}
#endif

				/* write LD result */
				scWrReg(m1->Operand[0], tData21, tData22, trueMask);

				/* if postmodify: update Ix */
				if(isPost2){
					tAddr22 += tAddr23;
					updateIReg(m1->Operand[2], tAddr22);
				}
			}else if(isALU_C(p) && isLD_C(m1)){
				/* type 4d */
				p->InstType = t04d;
				/* ALU.C || LD.C */

				/* latency: if LD comes just after ST, need +1 cycle */
				sICode *lp = p->LastExecuted;
				if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
					p->LatencyAdded = p->LatencyAdded +1;

				if(!isDReg24S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU.C instruction, "
						"only R[0/2/4/6] and ACC[0/2/4/6].L registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg24S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU.C instruction, "
							"only R0, R2, R4, R6 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg24(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}

				/* read ALU.C operands */
				scplx sct1 = scRdReg(p->Operand[1]);
				scplx sct2;
				if(p->Operand[2]){
					sct2 = scRdReg(p->Operand[2]);
				}else{			/* ABS.C/NOT.C */
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = sct2.i.dp[j] = 0;
					}
				}

				/* read LD operands */
				/* LD.C DREG24, DM(IREG +/+= MREG) */
				/* LD.C Op0,    Op4(Op2 Op1  Op3) */
				int isPost2 = FALSE;
				if(!strcmp(m1->Operand[1], "+=")) isPost2 = TRUE;

				int tAddr22 = 0xFFFF & RdReg2(p, m1->Operand[2]);
				int tAddr23 = RdReg2(p, m1->Operand[3]);

				/* if premodify: don't update Ix */
				if(!isPost2) tAddr22 += tAddr23;

				int tAddr222 = checkUnalignedMemoryAccess(p, tAddr22, m1->Operand[2]);

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				sint tData21 = sRdDataMem((unsigned int)tAddr22);
				sint tData22 = sRdDataMem((unsigned int)tAddr222);

				/* write ALU.C result */
				sProcessALU_CFunc(p, sct1, sct2, p->Operand[0], trueMask);

#ifdef VHPI
				if(VhpiMode){	/* rw: read 24b */
					int tData1;

					dsp_rw(tAddr22, &tData1, 1);
					dsp_wait(1);

					/* Note: tData1 = (((0x0FFF & tData21) << 12) | (0x0FFF & tData22)); */
					tData21.dp[0] = (0x0FFF & (tData1 >> 12));
					tData22.dp[0] = 0xFFF & tData1;
				}
#endif

				/* write LD result */
				scWrReg(m1->Operand[0], tData21, tData22, trueMask);

				/* if postmodify: update Ix */
				if(isPost2){
					tAddr22 += tAddr23;
					updateIReg(m1->Operand[2], tAddr22);
				}
			}else if(isMAC(p) && isST(m1)){
				/* type 4e */
				p->InstType = t04e;
				/* MAC || ST */

				/* 
				* NOTE: this code does NOT consider delay slot. (2010.07.20)
				*/
				sICode *NCode;
				if(stackEmpty(&LoopBeginStack)){		/* no loop running */
					NCode = p->Next;
					if(NCode && !isAnyMAC(NCode))			/* if last MAC */
						p->LatencyAdded = p->LatencyAdded +1;
				}else{									/* loop running */
					if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
						if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
							NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
						}else{			/* DO UNTIL CE */
							loopCntr = stackTop(&LoopCounterStack);
							if(loopCntr > 1){ /* decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{	/* exit loop if loopCntr == 1 */
								NCode = p->Next;
							}
						}
					} else {	/* within the loop */
						NCode = p->Next;
					}
					if(NCode && !isAnyMAC(NCode))			/* if last MAC */
						p->LatencyAdded = p->LatencyAdded +1;
				}

				if(!isACC32S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC instruction, "
						"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
				if(isNONE(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of Multifunction instruction, "
						"NONE is not allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isDReg12(p, m1->Operand[4])){
					printRunTimeError(p->LineCntr, m1->Operand[4], 
						"For destination of ST, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}

				/* read MAC operands */
				sint stemp1 = sRdReg(p->Operand[1]);
				sint stemp2 = sRdReg(p->Operand[2]);

				/* read ST operands */
				/* ST DM(IREG +/+= MREG), DREG12 */
                /* ST Op3(Op1 Op0  Op2),  Op4  */
				int isPost2 = FALSE;
				if(!strcmp(m1->Operand[0], "+=")) isPost2 = TRUE;

				int tAddr21 = 0xFFFF & RdReg2(p, m1->Operand[1]);
				int tAddr22 = RdReg2(p, m1->Operand[2]);

				sint tData2 = sRdReg(m1->Operand[4]);

				/* write MAC result */
				sProcessMACFunc(p, stemp1, stemp2, p->Operand[0], p->Operand[3], trueMask);

				/* if premodify: don't update Ix */
				if(!isPost2) tAddr21 += tAddr22;

				/* write ST result */
                sWrDataMem(tData2, (unsigned int)tAddr21, trueMask);

#ifdef VHPI
				if(VhpiMode){	/* wh: write 12b */
					dsp_wh(tAddr21, (short int *)&tData2.dp[0], 1);
				}
#endif

				/* if postmodify: update Ix */
				if(isPost2){
					tAddr21 += tAddr22;
					updateIReg(m1->Operand[1], tAddr21);
				}
			}else if(isALU(p) && isST(m1)){
				/* type 4g */
				p->InstType = t04g;
				/* ALU || ST */

				if(!isDReg12S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU instruction, "
						"only R0~R7 and ACC0.L~ACC7.L registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg12(p, m1->Operand[4])){
					printRunTimeError(p->LineCntr, m1->Operand[4], 
						"For destination of ST, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}

				/* read ALU operands */
				sint stemp1 = sRdReg(p->Operand[1]);
				sint stemp2;
				if(p->Operand[2]){
					stemp2 = sRdReg(p->Operand[2]);
				}else{			/* ABS/NOT/INC/DEC */
					stemp2 = so2;
				}

				/* read ST operands */
				/* ST DM(IREG +/+= MREG), DREG12 */
                /* ST Op3(Op1 Op0  Op2),  Op4  */
				int isPost2 = FALSE;
				if(!strcmp(m1->Operand[0], "+=")) isPost2 = TRUE;

				int tAddr21 = 0xFFFF & RdReg2(p, m1->Operand[1]);
				int tAddr22 = RdReg2(p, m1->Operand[2]);

				sint tData2 = sRdReg(m1->Operand[4]);

				/* write ALU result */
				sProcessALUFunc(p, stemp1, stemp2, p->Operand[0], trueMask);

				/* if premodify: don't update Ix */
				if(!isPost2) tAddr21 += tAddr22;

				/* write ST result */
                sWrDataMem(tData2, (unsigned int)tAddr21, trueMask);

#ifdef VHPI
				if(VhpiMode){	/* wh: write 12b */
					dsp_wh(tAddr21, (short int *)&tData2.dp[0], 1);
				}
#endif

				/* if postmodify: update Ix */
				if(isPost2){
					tAddr21 += tAddr22;
					updateIReg(m1->Operand[1], tAddr21);
				}
			}else if(isMAC_C(p) && isST_C(m1)){
				/* type 4f */
				p->InstType = t04f;
				/* MAC.C || ST.C */

				/* 
				* NOTE: this code does NOT consider delay slot. (2010.07.20)
				*/
				sICode *NCode;
				if(stackEmpty(&LoopBeginStack)){		/* no loop running */
					NCode = p->Next;
					if(NCode && !isAnyMAC(NCode))			/* if last MAC */
						p->LatencyAdded = p->LatencyAdded +1;
				}else{									/* loop running */
					if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
						if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
							NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
						}else{			/* DO UNTIL CE */
							loopCntr = stackTop(&LoopCounterStack);
							if(loopCntr > 1){ /* decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{	/* exit loop if loopCntr == 1 */
								NCode = p->Next;
							}
						}
					} else {	/* within the loop */
						NCode = p->Next;
					}
					if(NCode && !isAnyMAC(NCode))			/* if last MAC */
						p->LatencyAdded = p->LatencyAdded +1;
				}

				if(!isACC64S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC.C instruction, "
						"only ACC0, ACC2 registers are allowed.\n");
					break;
				}
				if(isNONE(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of Multifunction instruction, "
						"NONE is not allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isDReg24(p, m1->Operand[4])){
					printRunTimeError(p->LineCntr, m1->Operand[4], 
						"For destination of ST.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}

				/* read MAC operands */
				scplx sct1 = scRdReg(p->Operand[1]);
				scplx sct2 = scRdReg(p->Operand[2]);

				/* read ST.C operands */
				/* ST.C DM(IREG +/+= MREG), DREG24 */
                /* ST.C Op3(Op1 Op0  Op2),  Op4    */
				int isPost2 = FALSE;
				if(!strcmp(m1->Operand[0], "+=")) isPost2 = TRUE;

				int tAddr21 = 0xFFFF & RdReg2(p, m1->Operand[1]);
				int tAddr22 = RdReg2(p, m1->Operand[2]);

				scplx scData2 = scRdReg(m1->Operand[4]);

				/* write MAC result */
				sProcessMAC_CFunc(p, sct1, sct2, p->Operand[0], p->Operand[3], trueMask);

				/* if premodify: don't update Ix */
				if(!isPost2) tAddr21 += tAddr22;

				int tAddr212 = checkUnalignedMemoryAccess(p, tAddr21, m1->Operand[1]);

				/* write ST result */
                sWrDataMem(scData2.r, (unsigned int)tAddr21, trueMask);
                sWrDataMem(scData2.i, (unsigned int)tAddr212, trueMask);

#ifdef VHPI
				if(VhpiMode){	/* ww: write 24b */
					int tData2 = (((0x0FFF & scData2.r.dp[0]) << 12) | (0x0FFF & scData2.i.dp[0]));	
					dsp_ww(tAddr21, &tData2, 1);
				}
#endif

				/* if postmodify: update Ix */
				if(isPost2){
					tAddr21 += tAddr22;
					updateIReg(m1->Operand[1], tAddr21);
				}
			}else if(isALU_C(p) && isST_C(m1)){
				/* type 4h */
				p->InstType = t04h;
				/* ALU.C || ST.C */

				if(!isDReg24S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU.C instruction, "
						"only R[0/2/4/6] and ACC[0/2/4/6].L registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg24S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU.C instruction, "
							"only R0, R2, R4, R6 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg24(p, m1->Operand[4])){
					printRunTimeError(p->LineCntr, m1->Operand[4], 
						"For destination of ST.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}

				/* read ALU.C operands */
				scplx sct1 = scRdReg(p->Operand[1]);
				scplx sct2;
				if(p->Operand[2]){
					sct2 = scRdReg(p->Operand[2]);
				}else{			/* ABS.C/NOT.C */
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = sct2.i.dp[j] = 0;
					}
				}

				/* read ST.C operands */
				/* ST.C DM(IREG +/+= MREG), DREG24 */
                /* ST.C Op3(Op1 Op0  Op2),  Op4    */
				int isPost2 = FALSE;
				if(!strcmp(m1->Operand[0], "+=")) isPost2 = TRUE;

				int tAddr21 = 0xFFFF & RdReg2(p, m1->Operand[1]);
				int tAddr22 = RdReg2(p, m1->Operand[2]);

				scplx scData2 = scRdReg(m1->Operand[4]);

				/* write ALU.C result */
				sProcessALU_CFunc(p, sct1, sct2, p->Operand[0], trueMask);

				/* if premodify: don't update Ix */
				if(!isPost2) tAddr21 += tAddr22;

				int tAddr212 = checkUnalignedMemoryAccess(p, tAddr21, m1->Operand[1]);

				/* write ST result */
                sWrDataMem(scData2.r, (unsigned int)tAddr21, trueMask);
                sWrDataMem(scData2.i, (unsigned int)tAddr212, trueMask);

#ifdef VHPI
				if(VhpiMode){	/* ww: write 24b */
					int tData2 = (((0x0FFF & scData2.r.dp[0]) << 12) | (0x0FFF & scData2.i.dp[0]));	
					dsp_ww(tAddr21, &tData2, 1);
				}
#endif

				/* if postmodify: update Ix */
				if(isPost2){
					tAddr21 += tAddr22;
					updateIReg(m1->Operand[1], tAddr21);
				}
			}else if(isMAC(p) && isCP(m1)){
				/* type 8a */
				p->InstType = t08a;
				/* MAC || CP */

				/* 
				* NOTE: this code does NOT consider delay slot. (2010.07.20)
				*/
				sICode *NCode;
				if(stackEmpty(&LoopBeginStack)){		/* no loop running */
					NCode = p->Next;
					if(NCode && !isAnyMAC(NCode))			/* if last MAC */
						p->LatencyAdded = p->LatencyAdded +1;
				}else{									/* loop running */
					if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
						if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
							NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
						}else{			/* DO UNTIL CE */
							loopCntr = stackTop(&LoopCounterStack);
							if(loopCntr > 1){ /* decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{	/* exit loop if loopCntr == 1 */
								NCode = p->Next;
							}
						}
					} else {	/* within the loop */
						NCode = p->Next;
					}
					if(NCode && !isAnyMAC(NCode))			/* if last MAC */
						p->LatencyAdded = p->LatencyAdded +1;
				}

				if(!isACC32S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC instruction, "
						"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
				if(isNONE(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of Multifunction instruction, "
						"NONE is not allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isDReg12(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of CP, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}
				if(!isDReg12(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For source of CP, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}
				/* read MAC operands */
				sint stemp1 = sRdReg(p->Operand[1]);
				sint stemp2 = sRdReg(p->Operand[2]);

				/* read CP operands */
				/* CP RREG, RREG */
				/* CP Opr0, Opr1 */
				sint tData1 = sRdReg(m1->Operand[1]);

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				/* write MAC result */
				sProcessMACFunc(p, stemp1, stemp2, p->Operand[0], p->Operand[3], trueMask);

				/* write CP result */
				sWrReg(m1->Operand[0], tData1, trueMask);
			}else if(isALU(p) && isCP(m1)){
				/* type 8c */
				p->InstType = t08c;
				/* ALU || CP */

				if(!isDReg12S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU instruction, "
						"only R0~R7 and ACC0.L~ACC7.L registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg12(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of CP, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}
				if(!isDReg12(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For source of CP, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}

				/* read ALU operands */
				sint stemp1 = sRdReg(p->Operand[1]);
				sint stemp2;
				if(p->Operand[2]){
					stemp2 = sRdReg(p->Operand[2]);
				}else{			/* ABS/NOT/INC/DEC */
					stemp2 = so2;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				/* read CP operands */
				/* CP RREG, RREG */
				/* CP Opr0, Opr1 */
				sint tData1 = sRdReg(m1->Operand[1]);

				/* write ALU result */
				sProcessALUFunc(p, stemp1, stemp2, p->Operand[0], trueMask);

				/* write CP result */
				sWrReg(m1->Operand[0], tData1, trueMask);
			}else if(isMAC_C(p) && isCP_C(m1)){
				/* type 8b */
				p->InstType = t08b;
				/* MAC.C || CP.C */

				/* 
				* NOTE: this code does NOT consider delay slot. (2010.07.20)
				*/
				sICode *NCode;
				if(stackEmpty(&LoopBeginStack)){		/* no loop running */
					NCode = p->Next;
					if(NCode && !isAnyMAC(NCode))			/* if last MAC */
						p->LatencyAdded = p->LatencyAdded +1;
				}else{									/* loop running */
					if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
						if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
							NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
						}else{			/* DO UNTIL CE */
							loopCntr = stackTop(&LoopCounterStack);
							if(loopCntr > 1){ /* decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{	/* exit loop if loopCntr == 1 */
								NCode = p->Next;
							}
						}
					} else {	/* within the loop */
						NCode = p->Next;
					}
					if(NCode && !isAnyMAC(NCode))			/* if last MAC */
						p->LatencyAdded = p->LatencyAdded +1;
				}

				if(!isACC64S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC.C instruction, "
						"only ACC0, ACC2 registers are allowed.\n");
					break;
				}
				if(isNONE(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of Multifunction instruction, "
						"NONE is not allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isDReg24(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of CP.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}
				if(!isDReg24(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For source of CP.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}

				/* read MAC.C operands */
				scplx sct1 = scRdReg(p->Operand[1]);
				scplx sct2 = scRdReg(p->Operand[2]);

				/* read CP.C operands */
				/* CP.C DREG24, DREG24 */
				/* CP.C Opr0, Opr1 */
				scplx scData1 = scRdReg(m1->Operand[1]);

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				/* write MAC.C result */
				sProcessMAC_CFunc(p, sct1, sct2, p->Operand[0], p->Operand[3], trueMask);

				/* write CP.C result */
				scWrReg(m1->Operand[0], scData1.r, scData1.i, trueMask);
			}else if(isALU_C(p) && isCP_C(m1)){
				/* type 8d */
				p->InstType = t08d;
				/* ALU.C || CP.C */

				if(!isDReg24S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU.C instruction, "
						"only R[0/2/4/6] and ACC[0/2/4/6].L registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg24S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU.C instruction, "
							"only R0, R2, R4, R6 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg24(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of CP.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}
				if(!isDReg24(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For source of CP.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}

				/* read ALU.C operands */
				scplx sct1 = scRdReg(p->Operand[1]);
				scplx sct2;
				if(p->Operand[2]){
					sct2 = scRdReg(p->Operand[2]);
				}else{			/* ABS.C/NOT.C */
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = sct2.i.dp[j] = 0;
					}
				}

				/* read CP.C operands */
				/* CP.C DREG24, DREG24 */
				/* CP.C Opr0, Opr1 */
				scplx scData1 = scRdReg(m1->Operand[1]);

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				/* write ALU.C result */
				sProcessALU_CFunc(p, sct1, sct2, p->Operand[0], trueMask);

				/* write CP.C result */
				scWrReg(m1->Operand[0], scData1.r, scData1.i, trueMask);
			}else if(isSHIFT(p) && isLD(m1)){
				if(isReg12S(p, p->Operand[1])){
					/* type 12e */
					p->InstType = t12e;
					/* SHIFT || LD */
					/* SHIFT ACC32S, XOP12S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) || LD DREG12, DM(IREG+/+=MREG) */

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of LD, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}
	
					/* read SHIFT operands */
					sint stemp1 = sRdReg(p->Operand[1]);
					sint stemp2;
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, p->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm5;
					}
	
					/* read LD operands */
					/* LD DREG12, DM(IREG +/+= MREG) */
					/* LD Op0,    Op4(Op2 Op1  Op3)  */
					int isPost2 = FALSE;
					if(!strcmp(m1->Operand[1], "+=")) isPost2 = TRUE;

					int tAddr22 = 0xFFFF & RdReg2(p, m1->Operand[2]);
					int tAddr23 = RdReg2(p, m1->Operand[3]);

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
	
					/* if premodify: don't update Ix */
					if(!isPost2) tAddr22 += tAddr23;

					sint tData2 = sRdDataMem((unsigned int)tAddr22);
	
					/* write SHIFT result */
					sProcessSHIFTFunc(p, stemp1, stemp2, p->Operand[0], p->Operand[3], trueMask);

#ifdef VHPI
					if(VhpiMode){	/* rh: read 12b */
						dsp_rh(tAddr22, (short int *)&tData2.dp[0], 1);
						dsp_wait(1);
					}
#endif

					/* write LD result */
					sWrReg(m1->Operand[0], tData2, trueMask);

					/* if postmodify: update Ix */
					if(isPost2){
						tAddr22 += tAddr23;
						updateIReg(m1->Operand[2], tAddr22);
					}
				} else if(isACC32S(p, p->Operand[1])){
					/* type 12m */
					p->InstType = t12m;
					/* SHIFT || LD */
					/* SHIFT ACC32S, ACC32S, IMM_INT5 ( |NORND, RND| ) || LD DREG12, DM(IREG+/+=MREG) */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of LD, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}
	
					/* read SHIFT operands */
					sint stemp1 = sRdReg(p->Operand[1]);
					sint stemp2;
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, p->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm5;
					}
	
					/* read LD operands */
					/* LD DREG12, DM(IREG +/+= MREG) */
					/* LD Op0,    Op4(Op2 Op1  Op3)  */
					int isPost2 = FALSE;
					if(!strcmp(m1->Operand[1], "+=")) isPost2 = TRUE;

					int tAddr22 = 0xFFFF & RdReg2(p, m1->Operand[2]);
					int tAddr23 = RdReg2(p, m1->Operand[3]);

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
	
					/* if premodify: don't update Ix */
					if(!isPost2) tAddr22 += tAddr23;

					sint tData2 = sRdDataMem((unsigned int)tAddr22);
	
					/* write SHIFT result */
					sProcessSHIFTFunc(p, stemp1, stemp2, p->Operand[0], p->Operand[3], trueMask);

#ifdef VHPI
					if(VhpiMode){	/* rh: read 12b */
						dsp_rh(tAddr22, (short int *)&tData2.dp[0], 1);
						dsp_wait(1);
					}
#endif

					/* write LD result */
					sWrReg(m1->Operand[0], tData2, trueMask);

					/* if postmodify: update Ix */
					if(isPost2){
						tAddr22 += tAddr23;
						updateIReg(m1->Operand[2], tAddr22);
					}
				} else {
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 or R0, R1, ..., R7 registers are allowed.\n");
						break;
				}
			}else if(isSHIFT_C(p) && isLD_C(m1)){
				if(isReg24S(p, p->Operand[1])){
					/* type 12f */
					p->InstType = t12f;
					/* SHIFT.C || LD.C */
					/* SHIFT.C ACC64S, XOP24S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) || LD.C DREG24, DM(IREG+/+=MREG) */

					/* latency: if LD comes just after ST, need +1 cycle */
					sICode *lp = p->LastExecuted;
					if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
						p->LatencyAdded = p->LatencyAdded +1;

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC64S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT.C instruction, "
							"only ACC0, ACC2, ACC4, ACC6 registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of LD.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}

					/* read SHIFT.C operands */
					scplx sct1 = scRdReg(p->Operand[1]);
					scplx sct2; 
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, p->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = sct2.i.dp[j] = imm5;
					}

					/* read LD operands */
					/* LD.C DREG24, DM(IREG +/+= MREG) */
					/* LD.C Op0,    Op4(Op2 Op1  Op3) */
					int isPost2 = FALSE;
					if(!strcmp(m1->Operand[1], "+=")) isPost2 = TRUE;

					int tAddr22 = 0xFFFF & RdReg2(p, m1->Operand[2]);
					int tAddr23 = RdReg2(p, m1->Operand[3]);

					/* if premodify: don't update Ix */
					if(!isPost2) tAddr22 += tAddr23;

					int tAddr222 = checkUnalignedMemoryAccess(p, tAddr22, m1->Operand[2]);

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}

					sint tData21 = sRdDataMem((unsigned int)tAddr22);
					sint tData22 = sRdDataMem((unsigned int)tAddr222);

					/* write SHIFT.C result */
					sProcessSHIFT_CFunc(p, sct1, sct2, p->Operand[0], p->Operand[3], trueMask);

#ifdef VHPI
					if(VhpiMode){	/* rw: read 24b */
						int tData1;

						dsp_rw(tAddr22, &tData1, 1);
						dsp_wait(1);

						/* Note: tData1 = (((0x0FFF & tData21) << 12) | (0x0FFF & tData22)); */
						tData21.dp[0] = (0x0FFF & (tData1 >> 12));
						tData22.dp[0] = 0xFFF & tData1;
					}
#endif

					/* write LD result */
					scWrReg(m1->Operand[0], tData21, tData22, trueMask);

					/* if postmodify: update Ix */
					if(isPost2){
						tAddr22 += tAddr23;
						updateIReg(m1->Operand[2], tAddr22);
					}
				} else if(isACC64S(p, p->Operand[1])){
					/* type 12n */
					p->InstType = t12n;
					/* SHIFT.C || LD.C */
					/* SHIFT.C ACC64S, ACC64S, IMM_INT5 ( |NORND, RND| ) || LD.C DREG24, DM(IREG+/+=MREG) */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC64S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT.C instruction, "
							"only ACC0, ACC2, ACC4, ACC6 registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of LD.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}

					/* read SHIFT.C operands */
					scplx sct1 = scRdReg(p->Operand[1]);
					scplx sct2; 
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, p->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = sct2.i.dp[j] = imm5;
					}

					/* read LD operands */
					/* LD.C DREG24, DM(IREG +/+= MREG) */
					/* LD.C Op0,    Op4(Op2 Op1  Op3) */
					int isPost2 = FALSE;
					if(!strcmp(m1->Operand[1], "+=")) isPost2 = TRUE;

					int tAddr22 = 0xFFFF & RdReg2(p, m1->Operand[2]);
					int tAddr23 = RdReg2(p, m1->Operand[3]);

					/* if premodify: don't update Ix */
					if(!isPost2) tAddr22 += tAddr23;

					int tAddr222 = checkUnalignedMemoryAccess(p, tAddr22, m1->Operand[2]);

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}

					sint tData21 = sRdDataMem((unsigned int)tAddr22);
					sint tData22 = sRdDataMem((unsigned int)tAddr222);

					/* write SHIFT.C result */
					sProcessSHIFT_CFunc(p, sct1, sct2, p->Operand[0], p->Operand[3], trueMask);

#ifdef VHPI
					if(VhpiMode){	/* rw: read 24b */
						int tData1;

						dsp_rw(tAddr22, &tData1, 1);
						dsp_wait(1);

						/* Note: tData1 = (((0x0FFF & tData21) << 12) | (0x0FFF & tData22)); */
						tData21.dp[0] = (0x0FFF & (tData1 >> 12));
						tData22.dp[0] = 0xFFF & tData1;
					}
#endif

					/* write LD result */
					scWrReg(m1->Operand[0], tData21, tData22, trueMask);

					/* if postmodify: update Ix */
					if(isPost2){
						tAddr22 += tAddr23;
						updateIReg(m1->Operand[2], tAddr22);
					}
				} else {
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of SHIFT instruction, "
							"only ACC0, ACC2, ACC4, ACC6 or R0, R2, R4, R6 registers are allowed.\n");
						break;
				}
			}else if(isSHIFT(p) && isST(m1)){
				if(isReg12S(p, p->Operand[1])){
					/* type 12g */
					p->InstType = t12g;
					/* SHIFT || ST */
					/* SHIFT ACC32S, XOP12S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) || ST DM(IREG+/+=MREG), DREG12 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[4])){
						printRunTimeError(p->LineCntr, m1->Operand[4], 
							"For destination of ST, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}

					/* read SHIFT operands */
					sint stemp1 = sRdReg(p->Operand[1]);
					sint stemp2;
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, p->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm5;
					}

					/* read ST operands */
					/* ST DM(IREG +/+= MREG), DREG12 */
                	/* ST Op3(Op1 Op0  Op2),  Op4  */
					int isPost2 = FALSE;
					if(!strcmp(m1->Operand[0], "+=")) isPost2 = TRUE;

					int tAddr21 = 0xFFFF & RdReg2(p, m1->Operand[1]);
					int tAddr22 = RdReg2(p, m1->Operand[2]);

					sint tData2 = sRdReg(m1->Operand[4]);

					/* write SHIFT result */
					sProcessSHIFTFunc(p, stemp1, stemp2, p->Operand[0], p->Operand[3], trueMask);

					/* if premodify: don't update Ix */
					if(!isPost2) tAddr21 += tAddr22;

					/* write ST result */
                	sWrDataMem(tData2, (unsigned int)tAddr21, trueMask);

#ifdef VHPI
					if(VhpiMode){	/* wh: write 12b */
						dsp_wh(tAddr21, (short int *)&tData2.dp[0], 1);
					}
#endif

					/* if postmodify: update Ix */
					if(isPost2){
						tAddr21 += tAddr22;
						updateIReg(m1->Operand[1], tAddr21);
					}
				} else if(isACC32S(p, p->Operand[1])){
					/* type 12o */
					p->InstType = t12o;
					/* SHIFT || ST */
					/* SHIFT ACC32S, ACC32S, IMM_INT5 ( |NORND, RND| ) || ST DM(IREG+/+=MREG), DREG12 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[4])){
						printRunTimeError(p->LineCntr, m1->Operand[4], 
							"For destination of ST, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}

					/* read SHIFT operands */
					sint stemp1 = sRdReg(p->Operand[1]);
					sint stemp2;
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, p->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm5;
					}

					/* read ST operands */
					/* ST DM(IREG +/+= MREG), DREG12 */
                	/* ST Op3(Op1 Op0  Op2),  Op4  */
					int isPost2 = FALSE;
					if(!strcmp(m1->Operand[0], "+=")) isPost2 = TRUE;

					int tAddr21 = 0xFFFF & RdReg2(p, m1->Operand[1]);
					int tAddr22 = RdReg2(p, m1->Operand[2]);

					sint tData2 = sRdReg(m1->Operand[4]);

					/* write SHIFT result */
					sProcessSHIFTFunc(p, stemp1, stemp2, p->Operand[0], p->Operand[3], trueMask);

					/* if premodify: don't update Ix */
					if(!isPost2) tAddr21 += tAddr22;

					/* write ST result */
                	sWrDataMem(tData2, (unsigned int)tAddr21, trueMask);

#ifdef VHPI
					if(VhpiMode){	/* wh: write 12b */
						dsp_wh(tAddr21, (short int *)&tData2.dp[0], 1);
					}
#endif

					/* if postmodify: update Ix */
					if(isPost2){
						tAddr21 += tAddr22;
						updateIReg(m1->Operand[1], tAddr21);
					}
				} else {
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 or R0, R1, ..., R7 registers are allowed.\n");
						break;
				}
			}else if(isSHIFT_C(p) && isST_C(m1)){
				if(isReg24S(p, p->Operand[1])){
					/* type 12h */
					p->InstType = t12h;
					/* SHIFT.C || ST.C */
					/* SHIFT.C ACC64S, XOP24S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) || ST.C DM(IREG+/+=MREG), DREG24 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC64S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT.C instruction, "
							"only ACC0, ACC2 registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[4])){
						printRunTimeError(p->LineCntr, m1->Operand[4], 
							"For destination of ST.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}

					/* read SHIFT.C operands */
					scplx sct1 = scRdReg(p->Operand[1]);
					scplx sct2; 
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, p->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = sct2.i.dp[j] = imm5;
					}

					/* read ST.C operands */
					/* ST.C DM(IREG +/+= MREG), DREG24 */
                	/* ST.C Op3(Op1 Op0  Op2),  Op4    */
					int isPost2 = FALSE;
					if(!strcmp(m1->Operand[0], "+=")) isPost2 = TRUE;

					int tAddr21 = 0xFFFF & RdReg2(p, m1->Operand[1]);
					int tAddr22 = RdReg2(p, m1->Operand[2]);

					scplx scData2 = scRdReg(m1->Operand[4]);

					/* write SHIFT.C result */
					sProcessSHIFT_CFunc(p, sct1, sct2, p->Operand[0], p->Operand[3], trueMask);

					/* if premodify: don't update Ix */
					if(!isPost2) tAddr21 += tAddr22;

					int tAddr212 = checkUnalignedMemoryAccess(p, tAddr21, m1->Operand[1]);

					/* write ST result */
                	sWrDataMem(scData2.r, (unsigned int)tAddr21, trueMask);
                	sWrDataMem(scData2.i, (unsigned int)tAddr212, trueMask);

#ifdef VHPI
					if(VhpiMode){	/* ww: write 24b */
						int tData2 = (((0x0FFF & scData2.r.dp[0]) << 12) | (0x0FFF & scData2.i.dp[0]));	
						dsp_ww(tAddr21, &tData2, 1);
					}
#endif

					/* if postmodify: update Ix */
					if(isPost2){
						tAddr21 += tAddr22;
						updateIReg(m1->Operand[1], tAddr21);
					}
				} else if(isACC64S(p, p->Operand[1])){
					/* type 12p */
					p->InstType = t12p;
					/* SHIFT.C || ST.C */
					/* SHIFT.C ACC64S, ACC64S, IMM_INT5 ( |NORND, RND| ) || ST.C DM(IREG+/+=MREG), DREG24 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC64S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT.C instruction, "
							"only ACC0, ACC2 registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[4])){
						printRunTimeError(p->LineCntr, m1->Operand[4], 
							"For destination of ST.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}

					/* read SHIFT.C operands */
					scplx sct1 = scRdReg(p->Operand[1]);
					scplx sct2; 
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, p->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = sct2.i.dp[j] = imm5;
					}

					/* read ST.C operands */
					/* ST.C DM(IREG +/+= MREG), DREG24 */
                	/* ST.C Op3(Op1 Op0  Op2),  Op4    */
					int isPost2 = FALSE;
					if(!strcmp(m1->Operand[0], "+=")) isPost2 = TRUE;

					int tAddr21 = 0xFFFF & RdReg2(p, m1->Operand[1]);
					int tAddr22 = RdReg2(p, m1->Operand[2]);

					scplx scData2 = scRdReg(m1->Operand[4]);

					/* write SHIFT.C result */
					sProcessSHIFT_CFunc(p, sct1, sct2, p->Operand[0], p->Operand[3], trueMask);

					/* if premodify: don't update Ix */
					if(!isPost2) tAddr21 += tAddr22;

					int tAddr212 = checkUnalignedMemoryAccess(p, tAddr21, m1->Operand[1]);

					/* write ST result */
                	sWrDataMem(scData2.r, (unsigned int)tAddr21, trueMask);
                	sWrDataMem(scData2.i, (unsigned int)tAddr212, trueMask);

#ifdef VHPI
					if(VhpiMode){	/* ww: write 24b */
						int tData2 = (((0x0FFF & scData2.r.dp[0]) << 12) | (0x0FFF & scData2.i.dp[0]));	
						dsp_ww(tAddr21, &tData2, 1);
					}
#endif
					/* if postmodify: update Ix */
					if(isPost2){
						tAddr21 += tAddr22;
						updateIReg(m1->Operand[1], tAddr21);
					}
				} else {
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of SHIFT instruction, "
							"only ACC0, ACC2, ACC4, ACC6 or R0, R2, R4, R6 registers are allowed.\n");
						break;
				}
			}else if(isSHIFT(p) && isCP(m1)){
				if(isReg12S(p, p->Operand[1])){
					/* type 14c */
					p->InstType = t14c;
					/* SHIFT || CP */
					/* SHIFT ACC32S, XOP12S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) || CP DREG12, DREG12 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of CP, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[1])){
						printRunTimeError(p->LineCntr, m1->Operand[1], 
							"For source of CP, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}

					/* read SHIFT operands */
					sint stemp1 = sRdReg(p->Operand[1]);
					sint stemp2;
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, p->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm5;
					}

					/* read CP operands */
					/* CP DREG12, DREG12 */
					/* CP Opr0, Opr1 */
					sint tData1 = sRdReg(m1->Operand[1]);

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}

					/* write SHIFT result */
					sProcessSHIFTFunc(p, stemp1, stemp2, p->Operand[0], p->Operand[3], trueMask);

					/* write CP result */
					sWrReg(m1->Operand[0], tData1, trueMask);
				} else if(isACC32S(p, p->Operand[1])){
					/* type 14k */
					p->InstType = t14k;
					/* SHIFT || CP */
					/* SHIFT ACC32S, ACC32S, IMM_INT5 ( |NORND, RND| ) || CP DREG12, DREG12 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of CP, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[1])){
						printRunTimeError(p->LineCntr, m1->Operand[1], 
							"For source of CP, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}

					/* read SHIFT operands */
					sint stemp1 = sRdReg(p->Operand[1]);
					sint stemp2;
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, p->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm5;
					}

					/* read CP operands */
					/* CP DREG12, DREG12 */
					/* CP Opr0, Opr1 */
					sint tData1 = sRdReg(m1->Operand[1]);

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}

					/* write SHIFT result */
					sProcessSHIFTFunc(p, stemp1, stemp2, p->Operand[0], p->Operand[3], trueMask);

					/* write CP result */
					sWrReg(m1->Operand[0], tData1, trueMask);
				} else {
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 or R0, R1, ..., R7 registers are allowed.\n");
						break;
				}
			}else if(isSHIFT_C(p) && isCP_C(m1)){
				if(isReg24S(p, p->Operand[1])){
					/* type 14d */
					p->InstType = t14d;
					/* SHIFT.C || CP.C */
					/* SHIFT.C ACC64S, XOP24S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) || CP.C DREG24, DREG24 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC64S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT.C instruction, "
							"only ACC0, ACC2 registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of CP.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[1])){
						printRunTimeError(p->LineCntr, m1->Operand[1], 
							"For source of CP.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}

					/* read SHIFT.C operands */
					scplx sct1 = scRdReg(p->Operand[1]);
					scplx sct2; 
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, p->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = sct2.i.dp[j] = imm5;
					}

					/* read CP.C operands */
					/* CP.C DREG24, DREG24 */
					/* CP.C Opr0, Opr1 */
					scplx scData1 = scRdReg(m1->Operand[1]);

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}

					/* write SHIFT.C result */
					sProcessSHIFT_CFunc(p, sct1, sct2, p->Operand[0], p->Operand[3], trueMask);

					/* write CP.C result */
					scWrReg(m1->Operand[0], scData1.r, scData1.i, trueMask);
				} else if(isACC64S(p, p->Operand[1])){
					/* type 14l */
					p->InstType = t14l;
					/* SHIFT.C || CP.C */
					/* SHIFT.C ACC64S, ACC64S, IMM_INT5 ( |NORND, RND| ) || CP.C DREG24, DREG24 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC64S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT.C instruction, "
							"only ACC0, ACC2 registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of CP.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[1])){
						printRunTimeError(p->LineCntr, m1->Operand[1], 
							"For source of CP.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}

					/* read SHIFT.C operands */
					scplx sct1 = scRdReg(p->Operand[1]);
					scplx sct2; 
					int imm5 = getIntImm(p, getIntSymAddr(p, symTable, p->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						sct2.r.dp[j] = sct2.i.dp[j] = imm5;
					}

					/* read CP.C operands */
					/* CP.C DREG24, DREG24 */
					/* CP.C Opr0, Opr1 */
					scplx scData1 = scRdReg(m1->Operand[1]);

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}

					/* write SHIFT.C result */
					sProcessSHIFT_CFunc(p, sct1, sct2, p->Operand[0], p->Operand[3], trueMask);

					/* write CP.C result */
					scWrReg(m1->Operand[0], scData1.r, scData1.i, trueMask);
				} else {
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of SHIFT instruction, "
							"only ACC0, ACC2, ACC4, ACC6 or R0, R2, R4, R6 registers are allowed.\n");
						break;
				}
			}else if(isALU(p) && isMAC(m1)){
				/* type 43a */
				p->InstType = t43a;
				/* ALU || MAC */

				/* 
				* NOTE: this code does NOT consider delay slot. (2010.07.20)
				*/
				sICode *NCode;
				if(stackEmpty(&LoopBeginStack)){		/* no loop running */
					NCode = p->Next;
					if(NCode && !isAnyMAC(NCode))			/* if last MAC */
						p->LatencyAdded = p->LatencyAdded +1;
				}else{									/* loop running */
					if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
						if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
							NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
						}else{			/* DO UNTIL CE */
							loopCntr = stackTop(&LoopCounterStack);
							if(loopCntr > 1){ /* decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{	/* exit loop if loopCntr == 1 */
								NCode = p->Next;
							}
						}
					} else {	/* within the loop */
						NCode = p->Next;
					}
					if(NCode && !isAnyMAC(NCode))			/* if last MAC */
						p->LatencyAdded = p->LatencyAdded +1;
				}

				if(!isDReg12S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU instruction, "
						"only R0~R7, ACC0.L~ACC7.L registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
				}
				if(!isACC32S(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of this MAC instruction, "
						"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
				if(isNONE(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of Multifunction instruction, "
						"NONE is not allowed.\n");
					break;
				}
				if(!isReg12S(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}

				/* read ALU operands */
				sint stemp1 = sRdReg(p->Operand[1]);
				sint stemp2;
				if(p->Operand[2]){
					stemp2 = sRdReg(p->Operand[2]);
				}else{			/* ABS/NOT/INC/DEC */
					stemp2 = so2;
				}

				/* read MAC operands */
				sint stemp3 = sRdReg(m1->Operand[1]);
				sint stemp4 = sRdReg(m1->Operand[2]);

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				/* write ALU result */
				sProcessALUFunc(p, stemp1, stemp2, p->Operand[0], trueMask);

				/* write MAC result */
				m1->InstType = t40e;
				sProcessMACFunc(m1, stemp3, stemp4, m1->Operand[0], m1->Operand[3], trueMask);
			}else if(isALU_C(p) && isMAC_C(m1)){
				/* type 43b */
				//p->InstType = t43b;
				/* ALU.C || MAC.C */
				/* this multifunction format is no longer supported: 2008.12.5 */
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
				break;
			}else if(isALU(p) && isSHIFT(m1)){
				if(isReg12S(p, m1->Operand[1])){
					/* type 44b */
					p->InstType = t44b;
					/* ALU || SHIFT */
					/* ALU DREG12S, XOP12S, YOP12S || SHIFT ACC32S, XOP12S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) */

					if(!(m1->Operand[3])){
						printRunTimeError(p->LineCntr, m1->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(m1->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[m1->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isDReg12S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of ALU instruction, "
							"only R0~R7, ACC0.L~ACC7.L registers are allowed.\n");
						break;
					}
					if(!isReg12S(p, p->Operand[1])){
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
					if(p->Operand[2]){
						if(!isReg12S(p, p->Operand[2])){
							printRunTimeError(p->LineCntr, p->Operand[2], 
								"For source of ALU instruction, "
								"only R0, R1, ..., R7 registers are allowed.\n");
							break;
						}
					}
					if(!isACC32S(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}

					/* read ALU operands */
					sint stemp1 = sRdReg(p->Operand[1]);
					sint stemp2;
					if(p->Operand[2]){
						stemp2 = sRdReg(p->Operand[2]);
					}else{			/* ABS/NOT/INC/DEC */
						stemp2 = so2;
					}

					/* read SHIFT operands */
					sint stemp3 = sRdReg(m1->Operand[1]);
					sint stemp4;
					int imm5 = getIntImm(p, getIntSymAddr(m1, symTable, m1->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						stemp4.dp[j] = imm5;
					}

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}

					/* write ALU result */
					sProcessALUFunc(p, stemp1, stemp2, p->Operand[0], trueMask);

					/* write SHIFT result */
					m1->InstType = t15a;
					for(int j = 0; j < NUMDP; j++) {
						if(stemp4.dp[j] > 12) stemp4.dp[j] = 12;
						else if(stemp4.dp[j] < -12) stemp4.dp[j] = -12;
					}

					sProcessSHIFTFunc(m1, stemp3, stemp4, m1->Operand[0], m1->Operand[3], trueMask);
				} else if(isACC32S(p, m1->Operand[1])){
					/* type 44d */
					p->InstType = t44d;
					/* ALU || SHIFT */
					/* ALU DREG12S, XOP12S, YOP12S || SHIFT ACC32S, ACC32S, IMM_INT5 ( |NORND, RND| ) */

					if(!(m1->Operand[3])){
						printRunTimeError(p->LineCntr, m1->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(m1->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[m1->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isDReg12S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of ALU instruction, "
							"only R0~R7, ACC0.L~ACC7.L registers are allowed.\n");
						break;
					}
					if(!isReg12S(p, p->Operand[1])){
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
					if(p->Operand[2]){
						if(!isReg12S(p, p->Operand[2])){
							printRunTimeError(p->LineCntr, p->Operand[2], 
								"For source of ALU instruction, "
								"only R0, R1, ..., R7 registers are allowed.\n");
							break;
						}
					}
					if(!isACC32S(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}

					/* read ALU operands */
					sint stemp1 = sRdReg(p->Operand[1]);
					sint stemp2;
					if(p->Operand[2]){
						stemp2 = sRdReg(p->Operand[2]);
					}else{			/* ABS/NOT/INC/DEC */
						stemp2 = so2;
					}

					/* read SHIFT operands */
					sint stemp3 = sRdReg(m1->Operand[1]);
					sint stemp4;
					int imm5 = getIntImm(p, getIntSymAddr(m1, symTable, m1->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						stemp2.dp[j] = imm5;
					}
	

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}

					/* write ALU result */
					sProcessALUFunc(p, stemp1, stemp2, p->Operand[0], trueMask);

					/* write SHIFT result */
					m1->InstType = t15a;
					for(int j = 0; j < NUMDP; j++) {
						if(stemp4.dp[j] > 12) stemp4.dp[j] = 12;
						else if(stemp4.dp[j] < -12) stemp4.dp[j] = -12;
					}

					sProcessSHIFTFunc(m1, stemp3, stemp4, m1->Operand[0], m1->Operand[3], trueMask);
				} else {		/* if NOT XOP12S or ACC32S */
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For 1st source of SHIFT instruction, "
						"only R0, R1, ..., R7 or ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
			}else if(isMAC(p) && isSHIFT(m1)){
				if(isReg12S(p, m1->Operand[1])){
					/* type 45b */
					p->InstType = t45b;
					/* MAC || SHIFT */
					/* MAC ACC32S, XOP12S, YOP12S [( |RND, SS, SU, US, UU| )] || SHIFT ACC32S, XOP12S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) */

					/* 
					* NOTE: this code does NOT consider delay slot. (2010.07.20)
					*/
					sICode *NCode;
					if(stackEmpty(&LoopBeginStack)){		/* no loop running */
						NCode = p->Next;
						if(NCode && !isAnyMAC(NCode))			/* if last MAC */
							p->LatencyAdded = p->LatencyAdded +1;
					}else{									/* loop running */
						if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
							if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{			/* DO UNTIL CE */
								loopCntr = stackTop(&LoopCounterStack);
								if(loopCntr > 1){ /* decrement counter */
									NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
								}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
									NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
								}else{	/* exit loop if loopCntr == 1 */
									NCode = p->Next;
								}
							}
						} else {	/* within the loop */
							NCode = p->Next;
						}
						if(NCode && !isAnyMAC(NCode))			/* if last MAC */
							p->LatencyAdded = p->LatencyAdded +1;
					}

					if(!(m1->Operand[3])){
						printRunTimeError(p->LineCntr, m1->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(m1->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[m1->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of ALU instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(isNONE(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of Multifunction instruction, "
							"NONE is not allowed.\n");
						break;
					}
					if(!isReg12S(p, p->Operand[1])){
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
					if(!isACC32S(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}

					/* read MAC operands */
					sint stemp1 = sRdReg(p->Operand[1]);
					sint stemp2 = sRdReg(p->Operand[2]);

					/* read SHIFT operands */
					sint stemp3 = sRdReg(m1->Operand[1]);
					sint stemp4;
					int imm5 = getIntImm(p, getIntSymAddr(m1, symTable, m1->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						stemp4.dp[j] = imm5;
					}


					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}

					/* write MAC result */
					sProcessMACFunc(p, stemp1, stemp2, p->Operand[0], p->Operand[3], trueMask);

					/* write SHIFT result */
					m1->InstType = t16a;
					for(int j = 0; j < NUMDP; j++) {
						if(stemp4.dp[j] > 12) stemp4.dp[j] = 12;
						else if(stemp4.dp[j] < -12) stemp4.dp[j] = -12;
					}

					sProcessSHIFTFunc(m1, stemp3, stemp4, m1->Operand[0], m1->Operand[3], trueMask);
				} else if(isACC32S(p, m1->Operand[1])){
					/* type 45d */
					p->InstType = t45d;
					/* MAC || SHIFT */
					/* MAC ACC32S, XOP12S, YOP12S [( |RND, SS, SU, US, UU| )] || SHIFT ACC32S, ACC32S, IMM_INT5 ( |NORND, RND| ) */

					/* 
					* NOTE: this code does NOT consider delay slot. (2010.07.20)
					*/
					sICode *NCode;
					if(stackEmpty(&LoopBeginStack)){		/* no loop running */
						NCode = p->Next;
						if(NCode && !isAnyMAC(NCode))			/* if last MAC */
							p->LatencyAdded = p->LatencyAdded +1;
					}else{									/* loop running */
						if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
							if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{			/* DO UNTIL CE */
								loopCntr = stackTop(&LoopCounterStack);
								if(loopCntr > 1){ /* decrement counter */
									NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
								}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
									NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
								}else{	/* exit loop if loopCntr == 1 */
									NCode = p->Next;
								}
							}
						} else {	/* within the loop */
							NCode = p->Next;
						}
						if(NCode && !isAnyMAC(NCode))			/* if last MAC */
							p->LatencyAdded = p->LatencyAdded +1;
					}

					if(!(m1->Operand[3])){
						printRunTimeError(p->LineCntr, m1->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(m1->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[m1->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of ALU instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(isNONE(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of Multifunction instruction, "
							"NONE is not allowed.\n");
						break;
					}
					if(!isReg12S(p, p->Operand[1])){
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
					if(!isACC32S(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}

					/* read MAC operands */
					sint stemp1 = sRdReg(p->Operand[1]);
					sint stemp2 = sRdReg(p->Operand[2]);

					/* read SHIFT operands */
					sint stemp3 = sRdReg(m1->Operand[1]);
					sint stemp4;
					int imm5 = getIntImm(p, getIntSymAddr(m1, symTable, m1->Operand[2]), eIMM_INT5);
					for(int j = 0; j < NUMDP; j++) {
						stemp4.dp[j] = imm5;
					}

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}

					/* write MAC result */
					sProcessMACFunc(p, stemp1, stemp2, p->Operand[0], p->Operand[3], trueMask);

					/* write SHIFT result */
					m1->InstType = t16a;
					for(int j = 0; j < NUMDP; j++) {
						if(stemp4.dp[j] > 12) stemp4.dp[j] = 12;
						else if(stemp4.dp[j] < -12) stemp4.dp[j] = -12;
					}

					sProcessSHIFTFunc(m1, stemp3, stemp4, m1->Operand[0], m1->Operand[3], trueMask);
				} else {		/* if NOT XOP12S or ACC32S */
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For 1st source of SHIFT instruction, "
						"only R0, R1, ..., R7 or ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
			}
			break;
		case	2:			/* three-opcode multifunction */
			m1 = p->Multi[0];
			m2 = p->Multi[1];

			if(isALU(p) && isLD(m1) && isLD(m2)){
				/* type 1c */
				p->InstType = t01c;
				/* ALU || LD || LD */
				/* ALU DREG12S, XOP12S, YOP12S || LD XOP12S, DM(IX+= MX) || LD YOP12S, DM(IY+=MY) */

				/* latency: if LD comes just after ST, need +1 cycle */
				sICode *lp = p->LastExecuted;
				if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
					p->LatencyAdded = p->LatencyAdded +1;

				if(!isDReg12S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU instruction, "
						"only R0~R7 and ACC0.L~ACC7.L registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg12(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}
				if(!isDReg12(p, m2->Operand[0])){
					printRunTimeError(p->LineCntr, m2->Operand[0], 
						"For destination of LD, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}

				if(!isIx(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IX operand of LD, only I0, I1, I2, I3 registers are allowed.\n");
					break;
				}
				if(!isMx(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MX operand of LD, only M0, M1, M2, M3 registers are allowed.\n");
					break;
				}
				if(!isIy(p, m2->Operand[2])){
					printRunTimeError(p->LineCntr, m2->Operand[2], 
						"For IY operand of LD, only I4, I5, I6, I7 registers are allowed.\n");
					break;
				}
				if(!isMy(p, m2->Operand[3])){
					printRunTimeError(p->LineCntr, m2->Operand[3], 
						"For MY operand of LD, only M4, M5, M6, M7 registers are allowed.\n");
					break;
				}

				/* read ALU operands */
				sint stemp1 = sRdReg(p->Operand[1]);
				sint stemp2;
				if(p->Operand[2]){
					stemp2 = sRdReg(p->Operand[2]);
				}else{			/* ABS/NOT/INC/DEC */
					stemp2 = so2;
				}

				/* read first LD operands */
				/* LD XOP12S, DM(IREG +=  MREG) */
				/* LD Op0,  Op4(Op2 Op1   Op3) */
				int isPost1 = TRUE;

				int tAddr12 = 0xFFFF & RdReg2(p, m1->Operand[2]);
				int tAddr13 = RdReg2(p, m1->Operand[3]);

				sint tData1 = sRdDataMem((unsigned int)tAddr12);

				/* read second LD operands */
				/* LD XOP12S, DM(IREG +=  MREG) */
				/* LD Op0,  Op4(Op2 Op1   Op3) */
				int isPost2 = TRUE;

				int tAddr22 = 0xFFFF & RdReg2(p, m2->Operand[2]);
				int tAddr23 = RdReg2(p, m2->Operand[3]);

				sint tData2 = sRdDataMem((unsigned int)tAddr22);

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
				if(!strcasecmp(p->Operand[0], m2->Operand[0])){
					printRunTimeError(p->LineCntr, m2->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
				if(!strcasecmp(m1->Operand[0], m2->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				/* write ALU result */
				sProcessALUFunc(p, stemp1, stemp2, p->Operand[0], trueMask);

#ifdef VHPI
				if(VhpiMode){	/* rh+rh: read 12b+12b */
					dsp_rh_rh(tAddr12, (short int *)&tData1.dp[0], tAddr22, (short int *)&tData2.dp[0], 1);
					dsp_wait(1);
				}
#endif

				/* write first LD result */
				sWrReg(m1->Operand[0], tData1, trueMask);

				/* write second LD result */
				sWrReg(m2->Operand[0], tData2, trueMask);

				/* if postmodify: update Ix */
				if(isPost1){
					tAddr12 += tAddr13;
					updateIReg(m1->Operand[2], tAddr12);
				}

				/* if postmodify: update Ix */
				if(isPost2){
					tAddr22 += tAddr23;
					updateIReg(m2->Operand[2], tAddr22);
				}
			}else if(isALU_C(p) && isLD_C(m1) && isLD_C(m2)){
				/* type 1d */
				//p->InstType = t01d;
				/* ALU.C || LD.C || LD.C */
				/* this multifunction format is no longer supported: 2008.12.5 */
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
				break;
			}else if(isMAC(p) && isLD(m1) && isLD(m2)){
				/* type 1a */
				p->InstType = t01a;
				/* MAC || LD || LD */
				/* MAC ACC32S, XOP12S, YOP12S [( |RND, SS, SU, US, UU| )] || LD XOP12S, DM(IX+/+= MX) || LD YOP12S, DM(IY+/+=MY) */

				int MoreLatencyRequired = FALSE;

				/* latency: if LD comes just after ST, need +1 cycle */
				sICode *lp = p->LastExecuted;
				if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
					MoreLatencyRequired = TRUE;

				/* latency: if last MAC (= next inst. is not MAC), need +1 cycle */
				/* 
				* NOTE: this code does NOT consider delay slot. (2010.07.20)
				*/
				sICode *NCode;
				if(stackEmpty(&LoopBeginStack)){		/* no loop running */
					NCode = p->Next;
					if(NCode && !isAnyMAC(NCode))
						MoreLatencyRequired = TRUE;
				}else{									/* loop running */
					if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
						if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
							NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
						}else{			/* DO UNTIL CE */
							loopCntr = stackTop(&LoopCounterStack);
							if(loopCntr > 1){ /* decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{	/* exit loop if loopCntr == 1 */
								NCode = p->Next;
							}
						}
					} else {	/* within the loop */
						NCode = p->Next;
					}
					if(NCode && !isAnyMAC(NCode))
						MoreLatencyRequired = TRUE;
				}

				if(MoreLatencyRequired)
					p->LatencyAdded = p->LatencyAdded +1;

				if(!isACC32S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC instruction, "
						"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
				if(isNONE(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of Multifunction instruction, "
						"NONE is not allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isDReg12(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}
				if(!isDReg12(p, m2->Operand[0])){
					printRunTimeError(p->LineCntr, m2->Operand[0], 
						"For destination of LD, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}

				if(!isIx(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IX operand of LD, only I0, I1, I2, I3 registers are allowed.\n");
					break;
				}
				if(!isMx(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MX operand of LD, only M0, M1, M2, M3 registers are allowed.\n");
					break;
				}
				if(!isIy(p, m2->Operand[2])){
					printRunTimeError(p->LineCntr, m2->Operand[2], 
						"For IY operand of LD, only I4, I5, I6, I7 registers are allowed.\n");
					break;
				}
				if(!isMy(p, m2->Operand[3])){
					printRunTimeError(p->LineCntr, m2->Operand[3], 
						"For MY operand of LD, only M4, M5, M6, M7 registers are allowed.\n");
					break;
				}

				/* read MAC operands */
				sint stemp1 = sRdReg(p->Operand[1]);
				sint stemp2 = sRdReg(p->Operand[2]);

				/* read first LD operands */
				/* LD XOP12S, DM(IREG +/+=  MREG) */
				/* LD Op0,  Op4(Op2 Op1   Op3) */
				int isPost1 = FALSE;
				if(!strcmp(m1->Operand[1], "+=")) isPost1 = TRUE;

				int tAddr12 = 0xFFFF & RdReg2(p, m1->Operand[2]);
				int tAddr13 = RdReg2(p, m1->Operand[3]);

				/* if premodify: don't update Ix */
				if(!isPost1) tAddr12 += tAddr13;

				sint tData1 = sRdDataMem((unsigned int)tAddr12);

				/* read second LD operands */
				/* LD XOP12S, DM(IREG +/+=  MREG) */
				/* LD Op0,  Op4(Op2 Op1   Op3) */
				int isPost2 = FALSE;
				if(!strcmp(m2->Operand[1], "+=")) isPost2 = TRUE;

				int tAddr22 = 0xFFFF & RdReg2(p, m2->Operand[2]);
				int tAddr23 = RdReg2(p, m2->Operand[3]);

				/* if premodify: don't update Ix */
				if(!isPost2) tAddr22 += tAddr23;

				sint tData2 = sRdDataMem((unsigned int)tAddr22);

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
				if(!strcasecmp(p->Operand[0], m2->Operand[0])){
					printRunTimeError(p->LineCntr, m2->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
				if(!strcasecmp(m1->Operand[0], m2->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				/* write MAC result */
				sProcessMACFunc(p, stemp1, stemp2, p->Operand[0], p->Operand[3], trueMask);

#ifdef VHPI
				if(VhpiMode){	/* rh+rh: read 12b+12b */
					dsp_rh_rh(tAddr12, (short int *)&tData1.dp[0], tAddr22, (short int *)&tData2.dp[0], 1);
					dsp_wait(1);
				}
#endif

				/* write first LD result */
				sWrReg(m1->Operand[0], tData1, trueMask);

				/* write second LD result */
				sWrReg(m2->Operand[0], tData2, trueMask);

				/* if postmodify: update Ix */
				if(isPost1){
					tAddr12 += tAddr13;
					updateIReg(m1->Operand[2], tAddr12);
				}

				/* if postmodify: update Ix */
				if(isPost2){
					tAddr22 += tAddr23;
					updateIReg(m2->Operand[2], tAddr22);
				}
			}else if(isMAC_C(p) && isLD_C(m1) && isLD_C(m2)){
				/* type 1b */
				p->InstType = t01b;
				/* MAC.C || LD.C || LD.C */
				/* MAC.C ACC64S, XOP24S, YOP24S [( |RND, SS, SU, US, UU| )] || LD.C XOP24S, DM(IX+/+= MX) || LD.C YOP24S, DM(IY+/+=MY) */

				int MoreLatencyRequired = FALSE;

				/* latency: if LD comes just after ST, need +1 cycle */
				sICode *lp = p->LastExecuted;
				if(lp && (isST(lp) || isST_C(lp) || isSTMulti(lp)))
					MoreLatencyRequired = TRUE;

				/* latency: if last MAC (= next inst. is not MAC), need +1 cycle */
				/* 
				* NOTE: this code does NOT consider delay slot. (2010.07.20)
				*/
				sICode *NCode;
				if(stackEmpty(&LoopBeginStack)){		/* no loop running */
					NCode = p->Next;
					if(NCode && !isAnyMAC(NCode))
						MoreLatencyRequired = TRUE;
				}else{									/* loop running */
					if(p->PMA == stackTop(&LoopEndStack)){	 /* if end of the loop */
						if(RdReg("_LPEVER"))	{	/* DO UNTIL FOREVER */
							NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
						}else{			/* DO UNTIL CE */
							loopCntr = stackTop(&LoopCounterStack);
							if(loopCntr > 1){ /* decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else if(loopCntr == 0){ /* 0 is regarded as 0x10000, decrement counter */
								NCode = sICodeListSearch(&iCode, (unsigned int)stackTop(&LoopBeginStack));
							}else{	/* exit loop if loopCntr == 1 */
								NCode = p->Next;
							}
						}
					} else {	/* within the loop */
						NCode = p->Next;
					}
					if(NCode && !isAnyMAC(NCode))
						MoreLatencyRequired = TRUE;
				}

				if(MoreLatencyRequired)
					p->LatencyAdded = p->LatencyAdded +1;

				if(!isACC64S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC.C instruction, "
						"only ACC0, ACC2, ACC4, ACC6 registers are allowed.\n");
					break;
				}
				if(isNONE(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of Multifunction instruction, "
						"NONE is not allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isDReg24(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}
				if(!isDReg24(p, m2->Operand[0])){
					printRunTimeError(p->LineCntr, m2->Operand[0], 
						"For destination of LD.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}

				if(!isIx(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IX operand of LD.C, only I0, I1, I2, I3 registers are allowed.\n");
					break;
				}
				if(!isMx(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MX operand of LD.C, only M0, M1, M2, M3 registers are allowed.\n");
					break;
				}
				if(!isIy(p, m2->Operand[2])){
					printRunTimeError(p->LineCntr, m2->Operand[2], 
						"For IY operand of LD.C, only I4, I5, I6, I7 registers are allowed.\n");
					break;
				}
				if(!isMy(p, m2->Operand[3])){
					printRunTimeError(p->LineCntr, m2->Operand[3], 
						"For MY operand of LD.C, only M4, M5, M6, M7 registers are allowed.\n");
					break;
				}

				/* read MAC operands */
				scplx sct1 = scRdReg(p->Operand[1]);
				scplx sct2 = scRdReg(p->Operand[2]);

				/* read first LD.C operands */
				/* LD.C XOP24S, DM(IREG +/+=  MREG) */
				/* LD.C Op0,  Op4(Op2 Op1 Op3) */
				int isPost1 = FALSE;
				if(!strcmp(m1->Operand[1], "+=")) isPost1 = TRUE;

				int tAddr12 = 0xFFFF & RdReg2(p, m1->Operand[2]);
				int tAddr13 = RdReg2(p, m1->Operand[3]);

				/* if premodify: don't update Ix */
				if(!isPost1) tAddr12 += tAddr13;

				int tAddr122 = checkUnalignedMemoryAccess(p, tAddr12, m1->Operand[2]);

				sint tData11 = sRdDataMem((unsigned int)tAddr12);
				sint tData12 = sRdDataMem((unsigned int)tAddr122);

				/* read second LD operands */
				/* LD.C XOP24S, DM(IREG +/+=  MREG) */
				/* LD.C Op0,  Op4(Op2 Op1 Op3) */
				int isPost2 = FALSE;
				if(!strcmp(m2->Operand[1], "+=")) isPost2 = TRUE;

				int tAddr22 = 0xFFFF & RdReg2(p, m2->Operand[2]);
				int tAddr23 = RdReg2(p, m2->Operand[3]);

				/* if premodify: don't update Ix */
				if(!isPost2) tAddr22 += tAddr23;

				int tAddr222 = checkUnalignedMemoryAccess(p, tAddr22, m2->Operand[2]);

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
				if(!strcasecmp(p->Operand[0], m2->Operand[0])){
					printRunTimeError(p->LineCntr, m2->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
				if(!strcasecmp(m1->Operand[0], m2->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				sint tData21 = sRdDataMem((unsigned int)tAddr22);
				sint tData22 = sRdDataMem((unsigned int)tAddr222);

				/* write MAC result */
				sProcessMAC_CFunc(p, sct1, sct2, p->Operand[0], p->Operand[3], trueMask);

#ifdef VHPI
				if(VhpiMode){	/* rw+rw: read 24b+24b */
					int tData1, tData2;

					dsp_rw_rw(tAddr12, &tData1, tAddr22, &tData2, 1);
					dsp_wait(1);

					/* Note:
					tData1 = (((0x0FFF & tData11) << 12) | (0x0FFF & tData12));	
					tData2 = (((0x0FFF & tData21) << 12) | (0x0FFF & tData22));	
					*/
					tData11.dp[0] = (0x0FFF & (tData1 >> 12));
					tData12.dp[0] = 0xFFF & tData1;
					tData21.dp[0] = (0x0FFF & (tData2 >> 12));
					tData22.dp[0] = 0xFFF & tData2;
				}
#endif

				/* write first LD.C result */
				scWrReg(m1->Operand[0], tData11, tData12, trueMask);

				/* write second LD.C result */
				scWrReg(m2->Operand[0], tData21, tData22, trueMask);

				/* if postmodify: update Ix */
				if(isPost1){
					tAddr12 += tAddr13;
					updateIReg(m1->Operand[2], tAddr12);
				}

				/* if postmodify: update Ix */
				if(isPost2){
					tAddr22 += tAddr23;
					updateIReg(m2->Operand[2], tAddr22);
				}
			}
			break;
		default:
			break;
	}
	return p;
}


/** 
* @brief Initialize simulation variables: registers & stacks. Affect all data paths (including disabled DPs).
*/
void initSim(void)
{
	int i, j;
	int data;

	/* initialize stacks */
	stackInit(&PCStack, iPC);
	stackInit(&LoopBeginStack, iLoopBegin);
	stackInit(&LoopEndStack, iLoopEnd);
	stackInit(&LoopCounterStack, iLoopCounter);
	for(i = 0; i < 3; i++)
		sStackInit(&ASTATStack[i], iAStatus);
	stackInit(&MSTATStack, iStatus);
	stackInit(&LPEVERStack, iLPEVER);

	/* initialize registers */
	/* init Rx */
	for(i = 0; i < 32; i++){
		for(j = 0; j < NUMDP; j++){
			rR[i].dp[j] = 0;
		}
	}
	/* init ACCx */
	for(i = 0; i < 8; i++){
		for(j = 0; j < NUMDP; j++){
			rAcc[i].value.dp[j] = 0;
			rAcc[i].H.dp[j] = 0;
			rAcc[i].M.dp[j] = 0;
			rAcc[i].L.dp[j] = 0;
		}
	}
	/* init Ix, Mx, Lx, Bx */
	for(i = 0; i < 8; i++){
		rI[i] = 0;
		rM[i] = 0;
		rL[i] = 0;
		rB[i] = 0;
	}
	/* init _PCSTACK, _LPSTACK, _CNTR, _LPEVER */
	rPCSTACK = 0;
	rLPSTACK = 0;
	rCNTR = 0;
	rLPEVER = 0;
	/* init _MSTAT */
	rMstat.MB = 0;		/* bit 7 */
	rMstat.SD = 0;		/* bit 6 */
	rMstat.TI = 0;		/* bit 5 */
	rMstat.MM = 0;		/* bit 4 */
	rMstat.AS = 0;		/* bit 3 */
	rMstat.OL = 0;		/* bit 2 */
	rMstat.BR = 0;		/* bit 1 */
	rMstat.SR = 0;		/* bit 0 */
	/* init _SSAT */
	rSstat.SOV = 0;		/* bit 7 */
	rSstat.SSE = 1;		/* bit 6 */
	rSstat.LSF = 0;		/* bit 7 */
	rSstat.LSE = 1;		/* bit 4 */
	rSstat.Reserved = 0;	/* bit 3 */
	rSstat.PCL = 1;		/* bit 2 */
	rSstat.PCF = 0;		/* bit 1 */
	rSstat.PCE = 1;		/* bit 0 */
	/* init _DSTAT0, _DSTAT1 */
	if(SIMD1Mode){
		//rDstat0 = 1; (Enable)
		WrReg("_DSTAT0", 1);
		//rDstat1 = 1; (Master)
		WrReg("_DSTAT1", 1);
	}else if(SIMD2Mode){
		//rDstat0 = 3; (Enable)
		WrReg("_DSTAT0", 3);
		//rDstat1 = 1; (Master)
		WrReg("_DSTAT1", 1);
	}else if(SIMD4Mode){
		//rDstat0 = 0xF; (Enable)
		WrReg("_DSTAT0", 0xF);
		//rDstat1 = 1; (Master)
		WrReg("_DSTAT1", 1);
	}
	/* init ASTAT.R, ASTAT.I, ASTAT.C */
	for(j = 0; j < NUMDP; j++){
		rAstatR.UM.dp[j] = 0;		/* bit 9 */
		rAstatR.SV.dp[j] = 0;		/* bit 8 */
		rAstatR.SS.dp[j] = 0;		/* bit 7 */
		rAstatR.MV.dp[j] = 0;		/* bit 6 */
		rAstatR.AQ.dp[j] = 0;		/* bit 5 */
		rAstatR.AS.dp[j] = 0;		/* bit 4 */
		rAstatR.AC.dp[j] = 0;		/* bit 3 */
		rAstatR.AV.dp[j] = 0;		/* bit 2 */
		rAstatR.AN.dp[j] = 0;		/* bit 1 */
		rAstatR.AZ.dp[j] = 0;		/* bit 0 */
	}
	/* init ASTAT.I */
	for(j = 0; j < NUMDP; j++){
		rAstatI.UM.dp[j] = 0;		/* bit 9 */
		rAstatI.SV.dp[j] = 0;		/* bit 8 */
		rAstatI.SS.dp[j] = 0;		/* bit 7 */
		rAstatI.MV.dp[j] = 0;		/* bit 6 */
		rAstatI.AQ.dp[j] = 0;		/* bit 5 */
		rAstatI.AS.dp[j] = 0;		/* bit 4 */
		rAstatI.AC.dp[j] = 0;		/* bit 3 */
		rAstatI.AV.dp[j] = 0;		/* bit 2 */
		rAstatI.AN.dp[j] = 0;		/* bit 1 */
		rAstatI.AZ.dp[j] = 0;		/* bit 0 */
	}
	/* init ASTAT.C */
	for(j = 0; j < NUMDP; j++){
		rAstatC.UM.dp[j] = 0;		/* bit 9 */
		rAstatC.SV.dp[j] = 0;		/* bit 8 */
		rAstatC.SS.dp[j] = 0;		/* bit 7 */
		rAstatC.MV.dp[j] = 0;		/* bit 6 */
		rAstatC.AQ.dp[j] = 0;		/* bit 5 */
		rAstatC.AS.dp[j] = 0;		/* bit 4 */
		rAstatC.AC.dp[j] = 0;		/* bit 3 */
		rAstatC.AV.dp[j] = 0;		/* bit 2 */
		rAstatC.AN.dp[j] = 0;		/* bit 1 */
		rAstatC.AZ.dp[j] = 0;		/* bit 0 */
	}
	/* init _ICNTL */
	rICNTL.GIE = 0;		/* bit 5 */
	rICNTL.INE = 0;		/* bit 4 (TBD) */
	/* init _IMASK */
	for(i = 0; i < 12; i++){
		rIMASK.UserDef[i] = 0;		/* bit 15 to 4 */
	}
	rIMASK.STACK = 0;	/* bit 3 */
	rIMASK.SSTEP = 0;	/* bit 2 */
	rIMASK.PWDN = 0;	/* bit 1 */
	rIMASK.EMU = 0;		/* bit 0 */
	/* init _IRPTL */
	for(i = 0; i < 12; i++){
		rIRPTL.UserDef[i] = 0;		/* bit 15 to 4 */
	}
	rIRPTL.STACK = 0;	/* bit 3 */
	rIRPTL.SSTEP = 0;	/* bit 2 */
	rIRPTL.PWDN = 0;	/* bit 1 */
	rIRPTL.EMU = 0;		/* bit 0 */
	/* init _IVEC0, _IVEC1, _IVEC2, _IVEC3 */
	for(i = 0; i < 4; i++){
		rIVEC[i] = 0;
	}
	/* init UMCOUNT */
	for(j = 0; j < NUMDP; j++){
		rUMCOUNT.dp[j] = 0;
	}
	/* init DID */
	for(j = 0; j < NUMDP; j++){
		rDID.dp[j] = j;		/* data path ID */
	}
	/* init _PC */
	oldPC = UNDEFINED;
	/* init NextPC */
	PC = 0;
}

/** 
* @brief Initialize some simulation variables for RESET instruction.
*/
void resetSim(void)
{
	int i, j;
	int data;

	/* initialize stacks */
	stackInit(&PCStack, iPC);
	stackInit(&LoopBeginStack, iLoopBegin);
	stackInit(&LoopEndStack, iLoopEnd);
	stackInit(&LoopCounterStack, iLoopCounter);
	for(i = 0; i < 3; i++)
		sStackInit(&ASTATStack[i], iAStatus);
	stackInit(&MSTATStack, iStatus);
	stackInit(&LPEVERStack, iLPEVER);

	/* init _PCSTACK, _LPSTACK, _LPEVER */
	rPCSTACK = 0;
	rLPSTACK = 0;
	rLPEVER = 0;
	/* init _MSTAT */
	rMstat.MB = 0;		/* bit 7 */
	rMstat.SD = 0;		/* bit 6 */
	rMstat.TI = 0;		/* bit 5 */
	rMstat.MM = 0;		/* bit 4 */
	rMstat.AS = 0;		/* bit 3 */
	rMstat.OL = 0;		/* bit 2 */
	rMstat.BR = 0;		/* bit 1 */
	rMstat.SR = 0;		/* bit 0 */
	/* init _SSAT */
	rSstat.SOV = 0;		/* bit 7 */
	rSstat.SSE = 1;		/* bit 6 */
	rSstat.LSF = 0;		/* bit 7 */
	rSstat.LSE = 1;		/* bit 4 */
	rSstat.Reserved = 0;	/* bit 3 */
	rSstat.PCL = 1;		/* bit 2 */
	rSstat.PCF = 0;		/* bit 1 */
	rSstat.PCE = 1;		/* bit 0 */
	/* init _DSTAT0, _DSTAT1 */
	if(SIMD1Mode){
		//rDstat0 = 1; (Enable)
		WrReg("_DSTAT0", 1);
		//rDstat1 = 1; (Master)
		WrReg("_DSTAT1", 1);
	}else if(SIMD2Mode){
		//rDstat0 = 3; (Enable)
		WrReg("_DSTAT0", 3);
		//rDstat1 = 1; (Master)
		WrReg("_DSTAT1", 1);
	}else if(SIMD4Mode){
		//rDstat0 = 0xF; (Enable)
		WrReg("_DSTAT0", 0xF);
		//rDstat1 = 1; (Master)
		WrReg("_DSTAT1", 1);
	}
	/* init ASTAT.R, ASTAT.I, ASTAT.C */
	for(j = 0; j < NUMDP; j++){
		rAstatR.UM.dp[j] = 0;		/* bit 9 */
		rAstatR.SV.dp[j] = 0;		/* bit 8 */
		rAstatR.SS.dp[j] = 0;		/* bit 7 */
		rAstatR.MV.dp[j] = 0;		/* bit 6 */
		rAstatR.AQ.dp[j] = 0;		/* bit 5 */
		rAstatR.AS.dp[j] = 0;		/* bit 4 */
		rAstatR.AC.dp[j] = 0;		/* bit 3 */
		rAstatR.AV.dp[j] = 0;		/* bit 2 */
		rAstatR.AN.dp[j] = 0;		/* bit 1 */
		rAstatR.AZ.dp[j] = 0;		/* bit 0 */
	}
	/* init ASTAT.I */
	for(j = 0; j < NUMDP; j++){
		rAstatI.UM.dp[j] = 0;		/* bit 9 */
		rAstatI.SV.dp[j] = 0;		/* bit 8 */
		rAstatI.SS.dp[j] = 0;		/* bit 7 */
		rAstatI.MV.dp[j] = 0;		/* bit 6 */
		rAstatI.AQ.dp[j] = 0;		/* bit 5 */
		rAstatI.AS.dp[j] = 0;		/* bit 4 */
		rAstatI.AC.dp[j] = 0;		/* bit 3 */
		rAstatI.AV.dp[j] = 0;		/* bit 2 */
		rAstatI.AN.dp[j] = 0;		/* bit 1 */
		rAstatI.AZ.dp[j] = 0;		/* bit 0 */
	}
	/* init ASTAT.C */
	for(j = 0; j < NUMDP; j++){
		rAstatC.UM.dp[j] = 0;		/* bit 9 */
		rAstatC.SV.dp[j] = 0;		/* bit 8 */
		rAstatC.SS.dp[j] = 0;		/* bit 7 */
		rAstatC.MV.dp[j] = 0;		/* bit 6 */
		rAstatC.AQ.dp[j] = 0;		/* bit 5 */
		rAstatC.AS.dp[j] = 0;		/* bit 4 */
		rAstatC.AC.dp[j] = 0;		/* bit 3 */
		rAstatC.AV.dp[j] = 0;		/* bit 2 */
		rAstatC.AN.dp[j] = 0;		/* bit 1 */
		rAstatC.AZ.dp[j] = 0;		/* bit 0 */
	}
	/* init _ICNTL */
	rICNTL.GIE = 0;		/* bit 5 */
	rICNTL.INE = 0;		/* bit 4 (TBD) */
	/* init _IMASK */
	for(i = 0; i < 12; i++){
		rIMASK.UserDef[i] = 0;		/* bit 15 to 4 */
	}
	rIMASK.STACK = 0;	/* bit 3 */
	rIMASK.SSTEP = 0;	/* bit 2 */
	rIMASK.PWDN = 0;	/* bit 1 */
	rIMASK.EMU = 0;		/* bit 0 */
	/* init _IRPTL */
	for(i = 0; i < 12; i++){
		rIRPTL.UserDef[i] = 0;		/* bit 15 to 4 */
	}
	rIRPTL.STACK = 0;	/* bit 3 */
	rIRPTL.SSTEP = 0;	/* bit 2 */
	rIRPTL.PWDN = 0;	/* bit 1 */
	rIRPTL.EMU = 0;		/* bit 0 */
	/* init UMCOUNT */
	for(j = 0; j < NUMDP; j++){
		rUMCOUNT.dp[j] = 0;
	}
}


/** 
* @brief Read memory init. data if needed.
*/
void initDumpIn(void)
{
	int i;
	sint data = { UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED};
	char ch;
	char *ret;

#define	MAX_SBUF	(8*NUMDP+1)
	char tstr[MAX_SBUF];

	sint	trueMask = { 1, 1, 1, 1 };

/*********************************************************
*
* Input data format: (for now, only decimal number allowed)
*
*	; comment allowed with ';' at the line beginning
*	dp0_dec0 dp1_dec0 dp2_dec0 dp3_dec0
*	dp0_dec1 dp1_dec1 dp2_dec1 dp3_dec1
*	dp0_dec2 dp1_dec2 dp2_dec2 dp3_dec2
*	...
*
*********************************************************/
	/* memory dump input */
	if(dumpInFP){
		//if -ad option set (dataSegAddr)
		//dumpInStart -= dataSegAddr;

		for(i = 0; i < dumpInSize; i++){		
			ret = fgets(tstr, MAX_SBUF, dumpInFP);
			if(ret == NULL){	/* if end of input file, */
				break;			/* early exit */
			}

			ch = tstr[0];
			if(ch == ';'){
				i--;		/* don't count this line */
				continue;	/* skip comment line */
			}

			//for(int j = 0; j < NUMDP; j++){
				//fscanf(dumpInFP, "%d ", &data.dp[j]);	/* read decimal number */
			//}
			sscanf(tstr, "%d %d %d %d", &data.dp[0], &data.dp[1], &data.dp[2], &data.dp[3]);
			for(int j = 0; j < NUMDP; j++){
				data.dp[j] &= 0xFFF;
			}
			sWrDataMem(data, dumpInStart + i, trueMask);
		}
	}
}


/** 
* @brief Close files.
*/
void closeSim(void)
{
	int i;
	int data;

	/* memory dump output */
	closeDumpOut();

	/* close file */
	fclose(yyin);
	if(dumpInFP)  fclose(dumpInFP);
	if(dumpOutFP) fclose(dumpOutFP);
	if(dumpBinFP) fclose(dumpBinFP);
	if(dumpMemFP) fclose(dumpMemFP);
	if(dumpTxtFP) fclose(dumpTxtFP);
	if(dumpSymFP) fclose(dumpSymFP);
	if(dumpErrFP) fclose(dumpErrFP);
	if(dumpLstFP) fclose(dumpLstFP);
}

/** 
* @brief Write memory data if needed.
*/
void closeDumpOut(void)
{
	int i;
	sint data;

	/* memory dump output */
	if(dumpOutFP){
		//if -ad option set (dataSegAddr)
		//dumpOutStart -= dataSegAddr;

		for(i = 0; i < dumpOutSize; i++){
			data = sBriefRdDataMem(dumpOutStart + i);
			for(int j = 0; j < NUMDP; j++){
				if(data.dp[j] >= 0x800){
					data.dp[j] -= 0x1000;
				}
				fprintf(dumpOutFP, "%d ", data.dp[j]);
			}
			fprintf(dumpOutFP, "\n");
		}
	}
}

/** 
* @brief Scan intermediate code one-by-one to determine the type of each instruction.
* 
* @param icode Pointer to user program converted to intermediate code format by lexer & parser
* 
* @return 0 if the simulation successfully ends.
*/
int codeScan(sICodeList *icode)
{
	sICode *p = icode->FirstNode;

	curSecInfo = NULL;
	curaddr = 0;

	/* skip comment and/or label only lines */
	if(p != NULL){
		while(isCommentLabelInst(p->Index)){
			p = p->Next;
		}
	}

	while(p != NULL) {
		p = codeScanOneInst(p);
	}

	if(curSecInfo){
		curSecInfo->Size = curaddr - codeSegAddr;	/* update size of current segement */
	}

	return 0;	/* successfully ended */
}


/** 
* @brief Scan one assembly source line for instruction type resolution
* 
* @param *p Pointer to current instruction 
* 
* @return Pointer to next instruction
*/
sICode *codeScanOneInst(sICode *p)
{
	sICode *NextCode;
	sTab *sp;
	sSecInfo *sip;

	char *Opr0 = p->Operand[0]; 
	char *Opr1 = p->Operand[1];			
	char *Opr2 = p->Operand[2];			
	char *Opr3 = p->Operand[3];			
	char *Opr4 = p->Operand[4];			
	char *Opr5 = p->Operand[5];			

	/* default: 1 cycle */
	if(!isNotRealInst(p->Index)) {
		p->Latency = 1;
	}else{		/* pseudo/comment/label */
		p->Latency = 0;
	}

	/* update pointer to segment info */
	p->Sec = curSecInfo;
	/* update segment address for .DATA segment */
	p->DMA = curaddr;

	/* update program or data memory address */
	if(VerboseMode){
		if(!isNotRealInst(p->Index) || (p->Index == i_VAR)){
			if(p->Sec)
				printf("Line: %4d - PMA: %04X, DMA: %04X, segment: %s\n", p->LineCntr, p->PMA, p->DMA, p->Sec->Name);
			else
				printf("Line: %4d - PMA: %04X, DMA: %04X, segment: ----\n", p->LineCntr, p->PMA, p->DMA);
		}else{
			printf("Line: %4d - PMA: ----, DMA: ----\n", p->LineCntr);
		}
	}

	switch(p->Index){
		case	iABS:
		case	iNOT:
		case	iINC:
		case	iDEC:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isXOP12(p, Opr1)){
				/* type 9c */
				/* [IF COND] ABS DREG12, XOP12 */
				/* [IF COND] ABS Op0,    Op1   */
				p->InstType = t09c;
			}
			break;
		case	iABS_C:
		case	iNOT_C:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isDReg24(p, Opr0) && isXOP24(p, Opr1)){
				/* type 9d */
				/* [IF COND] ABS.C DREG24, XOP24[*] */
				/* [IF COND] ABS.C Op0,    Op1[*]   */
				p->InstType = t09d;
			}
			break;
		case	iADD:
		case	iSUB:
		case	iSUBB:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				/* type 9c */
				/* [IF COND] ADD DREG12, XOP12, YOP12 */
				/* [IF COND] ADD Op0,    Op1,   Op2   */
				p->InstType = t09c;
			} else if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isInt(Opr2)){
				if(isIntSignedN(p, symTable, Opr2, 4)){
					/* type 9e */
					/* [IF COND] ADD DREG12, XOP12, IMM_INT4 */
					/* [IF COND] ADD Op0,    Op1,   Op2   */
					p->InstType = t09e;
				}
			} else if(isACC32(p, Opr0) && isACC32(p, Opr1) && isACC32(p, Opr2)){
				/* type 9g */
				/* [IF COND] ADD ACC32, ACC32, ACC32 */
				/* [IF COND] ADD Op0,   Op1,   Op2   */
				p->InstType = t09g;
			}
			break;
		case	iADD_C:
		case	iSUB_C:
		case	iSUBB_C:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isXOP24(p, Opr2)){
				/* type 9d */
				/* [IF COND] ADD.C DREG24, XOP24, YOP24[*] */
				/* [IF COND] ADD.C Op0,    Op1,   Op2[*]   */
				p->InstType = t09d;
			} else if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isInt(Opr2)){
				if(!isInt(Opr3)){
					printRunTimeError(p->LineCntr, Opr2, 
						"Complex constant should be in a pair, e.g. (3, 4)\n");
					break;
				}

				if(isIntSignedN(p, symTable, Opr2, 4) && isIntSignedN(p, symTable, Opr3, 4)){
					/* type 9f */
					/* [IF COND] ADD.C DREG24, XOP24[*], IMM_COMPLEX8 */
					/* [IF COND] ADD.C Op0,    Op1[*],    (Op2, Op3)     */
					p->InstType = t09f;
				}
			} else if(isACC64(p, Opr0) && isACC64(p, Opr1) && isACC64(p, Opr2)){
				/* type 9h */
				/* [IF COND] ADD.C ACC64, ACC64, ACC64[*] */
				/* [IF COND] ADD.C Op0,   Op1,   Op2      */
				p->InstType = t09h;
			} 
			break;
		case	iADDC:
		case	iSUBC:
		case	iSUBBC:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				/* type 9c */
				/* [IF COND] ADDC DREG12, XOP12, YOP12 */
				/* [IF COND] ADDC Op0,    Op1,   Op2   */
				p->InstType = t09c;
			} else if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isInt(Opr2)){
				if(isIntSignedN(p, symTable, Opr2, 4)){
					int temp2 = getIntSymAddr(p, symTable, Opr2);

					/* type 9e */
					/* [IF COND] ADDC DREG12, XOP12, IMM_INT4 */
					/* [IF COND] ADDC Op0,    Op1,    Op2        */
					p->InstType = t09e;
				}
			} 
			break;
		case	iADDC_C:
		case	iSUBC_C:
		case	iSUBBC_C:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isXOP24(p, Opr2)){
				/* type 9d */
				/* [IF COND] ADDC.C DREG24, XOP24, YOP24[*] */
				/* [IF COND] ADDC.C Op0,    Op1,   Op2[*]   */
				p->InstType = t09d;
			} else if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isInt(Opr2)){
				if(!isInt(Opr3)){
					printRunTimeError(p->LineCntr, Opr2, 
						"Complex constant should be in a pair, e.g. (3, 4)\n");
					break;
				}

				if(isIntSignedN(p, symTable, Opr2, 4) && isIntSignedN(p, symTable, Opr3, 4)){
					cplx ct2;
					ct2.r = getIntSymAddr(p, symTable, Opr2);
					ct2.i = getIntSymAddr(p, symTable, Opr3);

					/* type 9f */
					/* [IF COND] ADDC.C DREG24, XOP24[*], IMM_COMPLEX8 */
					/* [IF COND] ADDC.C Op0,    Op1[*],    (Op2, Op3)     */
					p->InstType = t09f;
				}
			} 
			break;
		case	iAND:
		case	iOR:
		case	iXOR:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				/* type 9c */
				/* [IF COND] AND DREG12, XOP12, YOP12 */
				/* [IF COND] AND Op0,    Op1,   Op2   */
				p->InstType = t09c;
			} else if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isInt(Opr2)){
				if(isIntUnsignedN(p, symTable, Opr2, 4)){
					/* type 9i */
					/* [IF COND] AND DREG12, XOP12, IMM_UINT4 */
					/* [IF COND] AND Op0,    Op1,   Op2        */
					p->InstType = t09i;
				}
			}
			break;
		case	iASHIFT:
		case	iASHIFTOR:
		case	iLSHIFT:
		case	iLSHIFTOR:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isACC32(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2) && Opr3){
				/* type 16a */
				/* [IF COND] ASHIFT ACC32, XOP12, YOP12 (|HI, LO, HIRND, LORND|) */
				/* [IF COND] ASHIFT Op0,   Op1,   Op2     Op3                    */
				p->InstType = t16a;
			} else if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2) && Opr3){
				/* type 16e */
				/* [IF COND] ASHIFT DREG12, XOP12, YOP12 (|NORND, RND|) */
				/* [IF COND] ASHIFT Op0,   Op1,   Op2      Op3          */
				p->InstType = t16e;
			} else if(isACC32(p, Opr0) && isXOP12(p, Opr1) && isInt(Opr2) && Opr3){
				if(isIntNM(p, symTable, Opr2, -11, 11)){
					/* type 15a */
					/* [IF COND] ASHIFT ACC32, XOP12, IMM_INT5 (|HI, LO, HIRND, LORND|) */
					/* [IF COND] ASHIFT Op0,   Op1,   Op2        Op3      */
					p->InstType = t15a;
				}else{
					printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid shift amount (number of bits)! Please check instruction syntax.\n");
					break;
				}
			} else if(isACC32(p, Opr0) && isACC32(p, Opr1) && isInt(Opr2) && Opr3){
				if(isIntNM(p, symTable, Opr2, -31, 31)){
					/* type 15c */
					/* [IF COND] ASHIFT ACC32, ACC32, IMM_INT6 (|NORND, RND|) */
					/* [IF COND] ASHIFT Op0,   Op1,   Op2        Op3          */
					p->InstType = t15c;
				}else{
					printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid shift amount (number of bits)! Please check instruction syntax.\n");
					break;
				}
			} else if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isInt(Opr2) && Opr3){
				if(isIntNM(p, symTable, Opr2, -11, 11)){
					/* type 15e */
					/* [IF COND] ASHIFT DREG12, XOP12, IMM_INT5 (|NORND, RND|) */
					/* [IF COND] ASHIFT Op0,   Op1,   Op2        Op3           */
					p->InstType = t15e;
				}else{
					printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid shift amount (number of bits)! Please check instruction syntax.\n");
					break;
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	iASHIFT_C:
		case	iASHIFTOR_C:
		case	iLSHIFT_C:
		case	iLSHIFTOR_C:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isACC64(p, Opr0) && isXOP24(p, Opr1) && isXOP12(p, Opr2) && Opr3){
				/* type 16b */
				/* [IF COND] ASHIFT.C ACC64, XOP24, YOP12 (|HI, LO, HIRND, LORND|) */
				/* [IF COND] ASHIFT.C Op0,   Op1,   Op2     Op3      */
				p->InstType = t16b;
			} else if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isXOP12(p, Opr2) && Opr3){
				/* type 16f */
				/* [IF COND] ASHIFT.C DREG24, XOP24, YOP12 (|NORND, RND|) */
				/* [IF COND] ASHIFT.C Op0,    Op1,   Op2     Op3                    */
				p->InstType = t16f;
			} else if(isACC64(p, Opr0) && isXOP24(p, Opr1) && isInt(Opr2) && Opr3){
				if(isIntNM(p, symTable, Opr2, -11, 11)){
					/* type 15b */
					/* [IF COND] ASHIFT.C ACC64, XOP24, IMM_INT5 (|HI, LO, HIRND, LORND|) */
					/* [IF COND] ASHIFT.C Op0,    Op1,  Op2        Op3      */
					p->InstType = t15b;
				}else{
					printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid shift amount (number of bits)! Please check instruction syntax.\n");
					break;
				}
			} else if(isACC64(p, Opr0) && isACC64(p, Opr1) && isInt(Opr2) && Opr3){
				if(isIntNM(p, symTable, Opr2, -31, 31)){
					/* type 15d */
					/* [IF COND] ASHIFT.C ACC64, ACC64, IMM_INT6 (|NORND, RND|) */
					/* [IF COND] ASHIFT.C Op0,   Op1,   Op2        Op3 */
					p->InstType = t15d;
				}else{
					printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid shift amount (number of bits)! Please check instruction syntax.\n");
					break;
				}
			} else if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isInt(Opr2) && Opr3){
				if(isIntNM(p, symTable, Opr2, -11, 11)){
					/* type 15f */
					/* [IF COND] ASHIFT.C DREG24, XOP24, IMM_INT5 (|NORRND, RND|) */
					/* [IF COND] ASHIFT.C Op0,    Op1,   Op2        Op3           */
					p->InstType = t15f;
				}else{
					printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid shift amount (number of bits)! Please check instruction syntax.\n");
					break;
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iCALL:
			if(!p->Cond){
				if(isIReg(p, Opr0)){
					/* type 19a */
					/* [IF TRUE] CALL (<IREG>) */
					/* [IF TRUE] CALL (Op0   ) */
					p->InstType = t19a;
					p->MemoryRefType = tABS;	/* offset: LOCAL  var - current PC */

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						pn->isDelaySlot = TRUE;			/* identify delay slot */
					}else{		/* if delay slot disabled */
						p->Latency = 1;		/* default: 1 cycles (not taken) */
					}
				}else {
					/* type 10b */
					/* CALL <IMM_INT16> */
					/* CALL  Opr0       */
					p->InstType = t10b;

					sTab *sp = sTabHashSearch(symTable, Opr0);
					getLabelAddr(p, symTable, Opr0);	/* to make extern reference list of this label */
					if(sp->Type == tEXTERN){
						p->MemoryRefType = tREL;	/* offset: EXTERN var - current PC */
					}else
						p->MemoryRefType = tABS;	/* offset: LOCAL  var - current PC */

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						pn->isDelaySlot = TRUE;			/* identify delay slot */
					}else{		/* if delay slot disabled */
						p->Latency = 1;		/* default: 1 cycles (not taken) */
					}
				}
			} else {
				if(isIReg(p, Opr0)){
					/* type 19a */
					/* [IF COND] CALL (<IREG>) */
					/* [IF COND] CALL (Op0   ) */
					p->InstType = t19a;
					p->MemoryRefType = tABS;	/* offset: LOCAL  var - current PC */

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						pn->isDelaySlot = TRUE;			/* identify delay slot */
					}else{		/* if delay slot disabled */
						p->Latency = 1;		/* default: 1 cycles (not taken) */
					}
				} else {
					/* type 10a */
					/* [IF COND] CALL <IMM_INT13> */
					/* [IF COND] CALL  Opr0       */
					p->InstType = t10a;

					sTab *sp = sTabHashSearch(symTable, Opr0);
					getLabelAddr(p, symTable, Opr0);	/* to make extern reference list of this label */
					if(sp->Type == tEXTERN){
						p->MemoryRefType = tREL;	/* offset: EXTERN var - current PC */
					}else
						p->MemoryRefType = tABS;	/* offset: LOCAL  var - current PC */

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						pn->isDelaySlot = TRUE;			/* identify delay slot */
					}else{		/* if delay slot disabled */
						p->Latency = 1;		/* default: 1 cycles (not taken) */
					}
				}
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iCLRACC:
			if(isACC32(p, Opr0)){
				/* type 41c */
				/* [IF COND] CLRACC ACC32 */
				/* [IF COND] CLRACC Op0   */
				p->InstType = t41c;
			}
			break;
		case	iCLRACC_C:
			if(isACC64(p, Opr0)){
				/* type 41d */
				/* [IF COND] CLRACC.C ACC64 */
				/* [IF COND] CLRACC.C Op0   */
				p->InstType = t41d;
			}
			break;
		case	iCLRBIT:
		case	iSETBIT:
		case	iTGLBIT:
		case	iTSTBIT:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isInt(Opr2)){
				if(isIntNM(p, symTable, Opr2, 0, 11)){
					/* type 9i */
					/* [IF COND] CLRBIT DREG12, XOP12, IMM_UINT4   */
					/* [IF COND] CLRBIT Op0,    Op1,   Op2 */
					p->InstType = t09i;
				}else{
					printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"Invalid bit position (should be 0 ~ 11)! Please check instruction syntax.\n");
					break;
				}
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
        case    iCLRINT:
            if(isInt(Opr0)) {
				if(isIntUnsignedN(p, symTable, Opr0, 4)) {
               		/* type 37a */
               		/* [IF COND] CLRINT IMM_UINT4   */
               	 	/* [IF COND] CLRINT Op0 */
               	 	p->InstType = t37a;
				}
            }
            break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iCONJ_C:
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1)){
				/* type 42b */
				/* [IF COND] CONJ.C DREG24, XOP24 */
				/* [IF COND] CONJ.C Op0,    Op1    */
				p->InstType = t42b;
			}
			break;
		case	iCP:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isDReg12(p, Opr1)){
				/* type 17a */
				/* [IF COND] CP DREG12, DREG12 */
				/* [IF COND] CP Opr0, Opr1 */
				p->InstType = t17a;
			} else if(isDReg12(p, Opr0) && isRReg16(p, Opr1)){
				/* type 17b */
				/* [IF COND] CP DREG12, RREG16 */
				/* [IF COND] CP Opr0, Opr1 */
				p->InstType = t17b;
			} else if(isRReg16(p, Opr0) && isDReg12(p, Opr1)){
				/* type 17b */
				/* [IF COND] CP RREG16, DREG12 */
				/* [IF COND] CP Opr0, Opr1 */
				p->InstType = t17b;
			} else if(isRReg16(p, Opr0) && isRReg16(p, Opr1)){
				/* type 17g */
				/* [IF COND] CP RREG16, RREG16 */
				/* [IF COND] CP Opr0, Opr1 */
				p->InstType = t17g;
			} else if(isACC32(p, Opr0) && isACC32(p, Opr1)){
				/* type 17c */
				/* [IF COND] CP ACC32, ACC32 */
				/* [IF COND] CP Opr0,  Opr1 */
				p->InstType = t17c;
			} else if(isACC32(p, Opr0) && isDReg12(p, Opr1)){
				/* type 17h */
				/* [IF COND] CP ACC32, DReg12 */
				/* [IF COND] CP Opr0,  Opr1 */
				p->InstType = t17h;
			}
			break;
		case	iCP_C:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isDReg24(p, Opr0) && isDReg24(p, Opr1)){
				/* type 17d */
				/* [IF COND] CP.C DREG24, DREG24 */
				/* [IF COND] CP.C Opr0, Opr1 */
				p->InstType = t17d;
			} else if(isDReg24(p, Opr0) && isRReg16(p, Opr1)){
				/* type 17e */
				/* [IF COND] CP.C DREG24, RREG16 */
				/* [IF COND] CP.C Opr0, Opr1 */
				p->InstType = t17e;
			} else if(isRReg16(p, Opr0) && isDReg24(p, Opr1)){
				/* type 17e */
				/* [IF COND] CP.C RREG16, DREG24 */
				/* [IF COND] CP.C Opr0, Opr1 */
				p->InstType = t17e;
			} else if(isACC64(p, Opr0) && isACC64(p, Opr1)){
				/* type 17f */
				/* [IF COND] CP.C ACC64, ACC64 */
				/* [IF COND] CP.C Opr0,  Opr1 */
				p->InstType = t17f;
			}
			break;
		case	iCPXI:
			if(isDReg12(p, Opr0) && isXReg12(p, Opr1) && isInt(Opr2) && isIDN(Opr3)){
				/* type 49a */
				/* [IF COND] CPXI DREG12, XREG12, IMM_UINT2, IMM_UINT4 */
				/* [IF COND] CPXI Opr0,   Opr1,   Opr2,      Opr3      */
				p->InstType = t49a;
			}
			break;
		case	iCPXI_C:
			if(isDReg24(p, Opr0) && isXReg24(p, Opr1) && isInt(Opr2) && isIDN(Opr3)){
				/* type 49b */
				/* [IF COND] CPXI.C DREG24, XREG24, IMM_UINT2, IMM_UINT4 */
				/* [IF COND] CPXI.C Opr0,   Opr1,   Opr2,      Opr3      */
				p->InstType = t49b;
			}
			break;
		case	iCPXO:
			if(isXReg12(p, Opr0) && isDReg12(p, Opr1) && isInt(Opr2) && isIDN(Opr3)){
				/* type 49c	*/
				/* [IF COND] CPXO XREG12, DREG12, IMM_UINT2, IMM_UINT4 */
				/* [IF COND] CPXO Opr0,   Opr1,   Opr2,      Opr3      */
				p->InstType = t49c;
			}
			break;
		case	iCPXO_C:
			if(isXReg24(p, Opr0) && isDReg24(p, Opr1) && isInt(Opr2) && isIDN(Opr3)){
				/* type 49d */
				/* [IF COND] CPXO.C XREG24, DREG24, IMM_UINT2, IMM_UINT4 */
				/* [IF COND] CPXO.C Opr0,   Opr1,   Opr2,      Opr3      */
				p->InstType = t49d;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iDIS:
			/* type 18a */
			/* [IF COND] DIS |SR, BR, OL, AS, MM, TI, SD, MB, INT|, ... */
			/* [IF COND] DIS Opr0 [, Opr1][, Opr2], ... */
			if(p->OperandCounter)
				p->InstType = t18a;
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iDIVS:
			if(isReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				/* type 23a */
				/* [IF COND] DIVS REG12, XOP12, YOP12 */
				/* [IF COND] DIVS Op0,   Op1,   Op2   */
				p->InstType = t23a;
			}
			break;
		case	iDIVQ:
			if(isReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				/* type 23a */
				/* [IF COND] DIVQ REG12, XOP12, YOP12 */
				/* [IF COND] DIVQ Op0,   Op1,   Op2   */
				p->InstType = t23a;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iDO:
			if(p->Cond){
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"IF-COND not allowed. Please check instruction syntax.\n");
				break;
			}

			/* type 11a */
			/* DO <IMM_UINT12> UNTIL [<TERM>] */
			/* DO Opr1        UNTIL [Opr0  ] */
			if(Opr1 != NULL){
				p->InstType = t11a;	//for type checking
				if(isIntUnsignedN(p, symTable, Opr1, 12)){
					p->InstType = t11a;
					sTab *sp = sTabHashSearch(symTable, Opr1);
					getLabelAddr(p, symTable, Opr1);	/* to make extern reference list of this label */
					if(sp->Type == tEXTERN){
						p->MemoryRefType = tREL;	/* offset: EXTERN var - current PC */
					}else
						p->MemoryRefType = tABS;	/* offset: LOCAL  var - current PC */
				}else
					p->InstType = 0;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iDPID:
			/* type 18c */
			/* [IF COND] DPID DREG12 */
			if(isDReg12(p, Opr0)){
				p->InstType = t18c;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iENA:
			/* type 18a */
			/* [IF COND] ENA |SR, BR, OL, AS, MM, TI, SD, MB, INT|, ... */
			/* [IF COND] ENA Opr0 [, Opr1][, Opr2], ... */
			if(p->OperandCounter)
				p->InstType = t18a;
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iENADP:
			/* type 18b */
			/* [IF COND] ENADP IMM_UINT2, … (up to 4 IMM_UINT2 in parallel) */
			/* [IF COND] ENADP Opr0 [, Opr1][, Opr2][, Opr3] */
			if(p->OperandCounter)
				p->InstType = t18b;
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iEXP:
			if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && Opr2){
				/* type 16c */
				/* [IF COND] EXP DREG12, XOP12 (|HIX, HI, LO|) */
				/* [IF COND] EXP Op0,    Op1,     Op2           */
				p->InstType = t16c;
			}
			break;
		case	iEXP_C:
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && Opr2){
				/* type 16d */
				/* [IF COND] EXP.C DREG24, XOP24 (|HIX, HI, LO|) */
				/* [IF COND] EXP.C Op0,    Op1,     Op2           */
				p->InstType = t16d;
			}
			break;
		case	iEXPADJ:
			if(isDReg12(p, Opr0) && isXOP12(p, Opr1)){
				/* type 16c */
				/* [IF COND] EXPADJ DREG12, XOP12 */
				/* [IF COND] EXPADJ Op0,    Op1    */
				p->InstType = t16c;
			}
			break;
		case	iEXPADJ_C:
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1)){
				/* type 16d */
				/* [IF COND] EXPADJ.C DREG24, XOP24 */
				/* [IF COND] EXPADJ.C Op0,    Op1    */
				p->InstType = t16d;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case    iIDLE:
            /* type 31a */
            /* [IF COND] IDLE */
            p->InstType = t31a;
            break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iJUMP:
			if(!p->Cond) {
				if(isIReg(p, Opr0)){
					/* type 19a */
					/* [IF TRUE] JUMP (<IREG>) */
					/* [IF TRUE] JUMP (Op0   ) */
					p->InstType = t19a;
					p->MemoryRefType = tABS;	/* offset: LOCAL  var - current PC */

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						pn->isDelaySlot = TRUE;			/* identify delay slot */
					}else{		/* if delay slot disabled */
						p->Latency = 1;		/* default: 1 cycles (not taken) */
					}
				} else {
					/* type 10b */
					/* JUMP <IMM_INT16> */
					/* JUMP  Opr0       */
					p->InstType = t10b;

					sTab *sp = sTabHashSearch(symTable, Opr0);
					getLabelAddr(p, symTable, Opr0);	/* to make extern reference list of this label */
					if(sp->Type == tEXTERN){
						p->MemoryRefType = tREL;	/* offset: EXTERN var - current PC */
					}else
						p->MemoryRefType = tABS;	/* offset: LOCAL  var - current PC */

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						pn->isDelaySlot = TRUE;			/* identify delay slot */
					}else{		/* if delay slot disabled */
						p->Latency = 1;		/* default: 1 cycles (not taken) */
					}
				}
			} else {
				if(isIReg(p, Opr0)){
					/* type 19a */
					/* [IF COND] JUMP (<IREG>) */
					/* [IF COND] JUMP (Op0   ) */
					p->InstType = t19a;
					p->MemoryRefType = tABS;	/* offset: LOCAL  var - current PC */

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						pn->isDelaySlot = TRUE;			/* identify delay slot */
					}else{		/* if delay slot disabled */
						p->Latency = 1;		/* default: 1 cycles (not taken) */
					}
				} else {
					/* type 10a */
					/* [IF COND] JUMP <IMM_INT13> */
					/* [IF COND] JUMP  Opr0       */
					p->InstType = t10a;

					sTab *sp = sTabHashSearch(symTable, Opr0);
					getLabelAddr(p, symTable, Opr0);	/* to make extern reference list of this label */
					if(sp->Type == tEXTERN){
						p->MemoryRefType = tREL;	/* offset: EXTERN var - current PC */
					}else
						p->MemoryRefType = tABS;	/* offset: LOCAL  var - current PC */

					if(DelaySlotMode){
						sICode *pn = getNextCode(p);
						pn->isDelaySlot = TRUE;			/* identify delay slot */
					}else{		/* if delay slot disabled */
						p->Latency = 1;		/* default: 1 cycles (not taken) */
					}
				}
			}
			break;
		case	iLD:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isDReg12(p, Opr0) && isInt(Opr1) && (Opr2 != NULL) 
				&& !strcasecmp(Opr2, "DM")){
				if(isIntUnsignedN(p, symTable, Opr1, 16)){
					/* type 3a */
					/* [IF COND] LD DREG12, DM(IMM_UINT16) */
					/* [IF COND] LD Op0,    Op2(Op1)        */
					p->InstType = t03a;
				}
			} else if(isDReg12(p, Opr0) && isInt(Opr1) 
				&& (Opr2 != NULL)
				&& (!strcmp(Opr2, "+") || !strcmp(Opr2, "["))
				&& isInt(Opr3) && (Opr4 != NULL) && !strcasecmp(Opr4, "DM")){
				if(isIntUnsignedN(p, symTable, Opr1, 16)){
					/* type 3a */
					/* Variation due to .VAR usage */
					/* [IF COND] LD DREG12, DM(IMM_UINT16 +   const ) */
					/* [IF COND] LD DREG12, DM(IMM_UINT16 [   const]) */
					/* [IF COND] LD Op0,    Op4(Op1      Op2 Op3   ) */
					p->InstType = t03a;

					/* preprocessing: back to normal type 3a */
					/* after:                                */
					/* LD DREG12, DM(IMM_INT16)            */
					/* LD Op0,    Op2(Op1)                   */
					int iop1 = 0xFFFF & (getIntSymAddr(p, symTable, Opr1) + atoi(Opr3));
	
					/* free operands */
					free(p->Operand[1]); 
					free(p->Operand[2]); 
					free(p->Operand[3]); 

					char sop1[10];
					sprintf(sop1, "%d", iop1);
	
					/* new operands */
					p->Operand[1] = strdup(sop1);
					p->Operand[2] = p->Operand[4];
					p->OperandCounter = 3;

					/* make unused operands NULL */
					p->Operand[3] = NULL;
					p->Operand[4] = NULL;
				}
			} else if(isRReg16(p, Opr0) && isInt(Opr1) && (Opr2 != NULL) 
				&& !strcasecmp(Opr2, "DM")){
				/* type 3b */
				/* [IF COND] LD RREG16, DM(<IMM_INT16>) */
				/* [IF COND] LD Op0,    Op2(Op1)        */
				printRunTimeError(p->LineCntr, Opr0, 
					"LD RREG16, DM(<IMM_INT16>) syntax not allowed.\n");
				break;

				/*
				p->InstType = t03b;
				*/
			} else if(isACC32(p, Opr0) && isInt(Opr1) && (Opr2 != NULL) 
				&& !strcasecmp(Opr2, "DM")){
				if(isIntUnsignedN(p, symTable, Opr1, 16)){
					/* type 3d */
					/* [IF COND] LD ACC32, DM(IMM_UINT16) (|HI,LO|) */
					/* [IF COND] LD Op0,   Op2(Op1)        (Op3)     */
					/* Note: load data to upper or lower **24** bits of accumulator */
					p->InstType = t03d;

					if(!Opr3){
						printRunTimeError(p->LineCntr, Opr0, 
							"(HI) or (LO) required. Please check syntax.\n");
						break;
					}
				}
			} else if(isACC32(p, Opr0) && isInt(Opr1) 
				&& (Opr2 != NULL)
				&& (!strcmp(Opr2, "+") || !strcmp(Opr2, "["))
				&& isInt(Opr3) && (Opr4 != NULL) && !strcasecmp(Opr4, "DM")
				&& (!strcmp(Opr5, "HI") || !strcmp(Opr5, "LO"))){
				if(isIntUnsignedN(p, symTable, Opr1, 16)){
					/* type 3d */
					/* Variation due to .VAR usage */
					/* [IF COND] LD ACC32, DM(IMM_UINT16 +   const ) (|HI,LO|) */
					/* [IF COND] LD ACC32, DM(IMM_UINT16 [   const]) (|HI,LO|) */
					/* [IF COND] LD Op0,   Op4(Op1       Op2 Op3   ) (Op5)     */
					/* Note: load data to upper or lower **24** bits of accumulator (complex pair) */
					p->InstType = t03d;

					/* preprocessing: back to normal type 3d */
					/* after:                                */
					/* LD ACC32, DM(IMM_UINT16) (|HI,LO|)   */
					/* LD Op0,   Op2(Op1)        (Op3)       */
					//int iop1 = 0xFFFF & (getIntSymAddr(p, symTable, Opr1) + atoi(Opr3));
					int iop1 = getIntImm(p, (getIntSymAddr(p, symTable, Opr1) + atoi(Opr3)), eIMM_UINT16);

					/* free operands */
					free(p->Operand[1]); 
					free(p->Operand[2]); 
					free(p->Operand[3]); 

					char sop1[10];
					sprintf(sop1, "%d", iop1);

					/* new operands */
					p->Operand[1] = strdup(sop1);
					p->Operand[2] = p->Operand[4];
					p->Operand[3] = p->Operand[5];
					p->OperandCounter = 4;

					/* make unused operands NULL */
					p->Operand[4] = NULL;
					p->Operand[5] = NULL;
				}
			} else if(isRReg16(p, Opr0) && isInt(Opr1)
				&& (Opr2 != NULL)
				&& (!strcmp(Opr2, "+") || !strcmp(Opr2, "["))
				&& isInt(Opr3)){

				if(isIntUnsignedN(p, symTable, Opr1, 16)){
					/* type 6a */
					/* Variation due to .VAR usage */
					/* [IF COND] LD RREG16, <IMM_UINT16> +   const  */
					/* [IF COND] LD RREG16, <IMM_UINT16> [   const] */
					/* [IF COND] LD Op0,    Op1          Op2 Op3    */
					p->InstType = t06a;

					/* preprocessing: back to normal type 6a */
					/* after:                                */
					/* LD RREG16, <IMM_INT16> */
					/* LD Op0,    Op1         */
					int iop1 = 0xFFFF & (getIntSymAddr(p, symTable, Opr1) + atoi(Opr3));
	
					/* free operands */
					free(p->Operand[1]); 
					free(p->Operand[2]); 
					free(p->Operand[3]); 

					char sop1[10];
					sprintf(sop1, "%d", iop1);
	
					/* new operands */
					p->Operand[1] = strdup(sop1);
					p->OperandCounter = 2;

					/* make unused operands NULL */
					p->Operand[2] = NULL;
					p->Operand[3] = NULL;
				}
			} else if(isRReg16(p, Opr0) && isInt(Opr1)){
				if(isIntSignedN(p, symTable, Opr1, 16)){
					/* type 6a */
					/* [IF COND] LD RREG16, <IMM_INT16> */
					/* [IF COND] LD Op0,    Op1         */
					p->InstType = t06a;
				}
			} else if(isDReg12(p, Opr0) && isInt(Opr1)
				&& (Opr2 != NULL)
				&& (!strcmp(Opr2, "+") || !strcmp(Opr2, "["))
				&& isInt(Opr3)){

				if(isIntUnsignedN(p, symTable, Opr1, 12)){
					/* type 6b */
					/* Variation due to .VAR usage */
					/* [IF COND] LD DREG12, <IMM_UINT12> +   const  */
					/* [IF COND] LD DREG12, <IMM_UINT12> [   const] */
					/* [IF COND] LD Op0,    Op1          Op2 Op3    */
					p->InstType = t06b;

					/* preprocessing: back to normal type 6b */
					/* after:                                */
					/* LD DREG12, <IMM_INT12> */
					/* LD Op0,    Op1         */
					int iop1 = 0xFFF & (getIntSymAddr(p, symTable, Opr1) + atoi(Opr3));
	
					/* free operands */
					free(p->Operand[1]); 
					free(p->Operand[2]); 
					free(p->Operand[3]); 

					char sop1[10];
					sprintf(sop1, "%d", iop1);
	
					/* new operands */
					p->Operand[1] = strdup(sop1);
					p->OperandCounter = 2;

					/* make unused operands NULL */
					p->Operand[2] = NULL;
					p->Operand[3] = NULL;
				}
			} else if(isDReg12(p, Opr0) && isInt(Opr1)){
				if(isIntSignedN(p, symTable, Opr1, 12)){
					/* type 6b */
					/* [IF COND] LD DREG12, <IMM_INT12> */
					/* [IF COND] LD Op0,    Op1         */
					p->InstType = t06b;
				}
			} else if(isACC32(p, Opr0) && isInt(Opr1)){
				if(p->Cond){
					printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"IF-COND not allowed. Please check instruction syntax.\n");
					break;
				}

				if(isIntSignedN(p, symTable, Opr1, 24)){
					/* type 6d */
					/* LD ACC32, <IMM_INT24> */
					/* LD Op0,   Op1         */
					p->InstType = t06d;
				}
			} else if(isRReg(p, Opr0) && !strcmp(Opr1, "+=") && isIReg(p, Opr2)
				&& isMReg(p, Opr3) && !strcasecmp(Opr4, "DM")){
				/* type 32a */
				/* [IF COND] LD RREG, DM(IREG +=  MREG) */
				/* [IF COND] LD Op0,  Op4(Op2 Op1 Op3) */
				p->InstType = t32a;
			} else if(isRReg(p, Opr0) && !strcmp(Opr1, "+") && isIReg(p, Opr2)
				&& isMReg(p, Opr3) && !strcasecmp(Opr4, "DM")){
				/* type 32a */
				/* [IF COND] LD RREG, DM(IREG +   MREG) */
				/* [IF COND] LD Op0,  Op4(Op2 Op1 Op3) */
				p->InstType = t32a;
			} else if(isRReg(p, Opr0) && !strcmp(Opr1, "+=") && isIReg(p, Opr2)
				&& isMReg(p, Opr3) && !strcasecmp(Opr4, "PM")){
				/* type 32a */
				/* [IF COND] LD RREG, PM(IREG +=  MREG) */
				/* [IF COND] LD Op0,  Op4(Op2 Op1 Op3) */
				printRunTimeError(p->LineCntr, Opr4, 
					"Accessing Program Memory not supported.\n");
				break;

				/*
				p->InstType = t32a;
				*/
			} else if(isRReg(p, Opr0) && !strcmp(Opr1, "+") && isIReg(p, Opr2)
				&& isMReg(p, Opr3) && !strcasecmp(Opr4, "PM")){
				/* type 32a */
				/* [IF COND] LD RREG, PM(IREG +   MREG) */
				/* [IF COND] LD Op0,  Op4(Op2 Op1 Op3) */
				printRunTimeError(p->LineCntr, Opr4, 
					"Accessing Program Memory not supported.\n");
				break;

				/*
				p->InstType = t32a;
				*/
			} else if(isDReg12(p, Opr0) && !strcmp(Opr1, "+=") && isIReg(p, Opr2)
				&& isInt(Opr3) && !strcasecmp(Opr4, "DM")){
				if(isIntSignedN(p, symTable, Opr3, 8)){
					/* type 29a */
					/* [IF COND] LD DREG12, DM(IREG +=  <IMM_INT8>) */
					/* [IF COND] LD Op0,    Op4(Op2 Op1 Op3) */
					p->InstType = t29a;
				}
			} else if(isDReg12(p, Opr0) && !strcmp(Opr1, "+") && isIReg(p, Opr2)
				&& isInt(Opr3) && !strcasecmp(Opr4, "DM")){
				if(isIntSignedN(p, symTable, Opr3, 8)){
					/* type 29a */
					/* [IF COND] LD DREG12, DM(IREG +   <IMM_INT8>) */
					/* [IF COND] LD Op0,    Op4(Op2 Op1 Op3) */
					p->InstType = t29a;
				}
			} else if(isDReg12(p, Opr0) && isIReg(p, Opr1)
				&& !strcasecmp(Opr2, "DM")){
				/* type 29a */
				/* [IF COND] LD DREG12, DM(IREG) */
				/* [IF COND] LD Op0,    Op2(Op1) */
				p->InstType = t29a;
			} else if(isDReg12(p, Opr0) && isSysCtlReg(Opr1)){
				/* type 35a */
				/* LD DREG12, CACTL */
				/* ST Op0,    Op1    */
				printRunTimeError(p->LineCntr, Opr1, 
					"CACTL Register not supported.\n");
				break;

				/*
				p->InstType = t35a;
				*/
			}
			break;
		case	iLD_C:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isDReg24(p, Opr0) && isInt(Opr1) && (Opr2 != NULL) 
				&& !strcasecmp(Opr2, "DM")){
				if(isIntUnsignedN(p, symTable, Opr1, 16)){
					/* type 3c */
					/* [IF COND] LD.C DREG24, DM(IMM_UINT16) */
					/* [IF COND] LD.C Op0,    Op2(Op1)        */
					p->InstType = t03c;
				}
			} else if(isDReg24(p, Opr0) && isInt(Opr1) 
				&& (Opr2 != NULL)
				&& (!strcmp(Opr2, "+") || !strcmp(Opr2, "["))
				&& isInt(Opr3) && (Opr4 != NULL) && !strcasecmp(Opr4, "DM")){
				if(isIntUnsignedN(p, symTable, Opr1, 16)){
					/* type 3c */
					/* Variation due to .VAR usage */
					/* [IF COND] LD.C DREG24, DM(IMM_UINT16 +   const ) */
					/* [IF COND] LD.C DREG24, DM(IMM_UINT16 [   const]) */
					/* [IF COND] LD.C Op0,    Op4(Op1        Op2 Op3   ) */
					p->InstType = t03c;

					/* preprocessing: back to normal type 3c */
					/* after:                                */
					/* LD.C DREG24, DM(IMM_UINT16)          */
					/* LD.C Op0,    Op2(Op1)                 */
					//int iop1 = 0xFFFF & (getIntSymAddr(p, symTable, Opr1) + atoi(Opr3));
					int iop1 = getIntImm(p, (getIntSymAddr(p, symTable, Opr1) + atoi(Opr3)), eIMM_UINT16);

					/* free operands */
					free(p->Operand[1]); 
					free(p->Operand[2]); 
					free(p->Operand[3]); 

					char sop1[10];
					sprintf(sop1, "%d", iop1);

					/* new operands */
					p->Operand[1] = strdup(sop1);
					p->Operand[2] = p->Operand[4];
					p->OperandCounter = 3;

					/* make unused operands NULL */
					p->Operand[3] = NULL;
					p->Operand[4] = NULL;
				}
			} else if(isACC64(p, Opr0) && isInt(Opr1) && (Opr2 != NULL) 
				&& !strcasecmp(Opr2, "DM")){
				if(isIntUnsignedN(p, symTable, Opr1, 16)){
					/* type 3e */
					/* [IF COND] LD.C ACC64, DM(IMM_UINT16) (|HI,LO|) */
					/* [IF COND] LD.C Op0,   Op2(Op1)       (Op3)       */
					/* Note: load data to upper or lower **24** bits of accumulator (complex pair) */
					p->InstType = t03e;

					/* latency restriction: t03e: 2-cycle instruction */
					p->Latency = 2;
				}
			} else if(isACC64(p, Opr0) && isInt(Opr1) 
				&& (Opr2 != NULL)
				&& (!strcmp(Opr2, "+") || !strcmp(Opr2, "["))
				&& isInt(Opr3) && (Opr4 != NULL) && !strcasecmp(Opr4, "DM")
				&& (!strcmp(Opr5, "HI") || !strcmp(Opr5, "LO"))){
				if(isIntUnsignedN(p, symTable, Opr1, 16)){
					/* type 3e */
					/* Variation due to .VAR usage */
					/* [IF COND] LD.C ACC64, DM(IMM_UINT16 +   const ) (|HI,LO|) */
					/* [IF COND] LD.C ACC64, DM(IMM_UINT16 [   const]) (|HI,LO|) */
					/* [IF COND] LD.C Op0,   Op4(Op1       Op2 Op3   ) (Op5)     */
					/* Note: load data to upper or lower **24** bits of accumulator (complex pair) */
					p->InstType = t03e;

					/* latency restriction: t03e: 2-cycle instruction */
					p->Latency = 2;

					/* preprocessing: back to normal type 3e */
					/* after:                                */
					/* LD.C ACC64, DM(IMM_UINT16) (|HI,LO|) */
					/* LD.C Op0,   Op2(Op1)        (Op3)       */
					//int iop1 = 0xFFFF & (getIntSymAddr(p, symTable, Opr1) + atoi(Opr3));
					int iop1 = getIntImm(p, (getIntSymAddr(p, symTable, Opr1) + atoi(Opr3)), eIMM_UINT16);

					/* free operands */
					free(p->Operand[1]); 
					free(p->Operand[2]); 
					free(p->Operand[3]); 

					char sop1[10];
					sprintf(sop1, "%d", iop1);

					/* new operands */
					p->Operand[1] = strdup(sop1);
					p->Operand[2] = p->Operand[4];
					p->Operand[3] = p->Operand[5];
					p->OperandCounter = 4;

					/* make unused operands NULL */
					p->Operand[4] = NULL;
					p->Operand[5] = NULL;
				}
			} else if(isDReg24(p, Opr0) && isInt(Opr1)){
				if(p->Cond){
					printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
						"IF-COND not allowed. Please check instruction syntax.\n");
					break;
				}

				if(!isInt(Opr2)){
					printRunTimeError(p->LineCntr, Opr1, 
						"Complex constant should be in a pair, e.g. (3, 4)\n");
					break;
				}
				/* type 6c */
				/* LD.C DREG24, <IMM_COMPLEX24> */
				/* LD.C Op0,    (Op1, Op2)      */
				p->InstType = t06c;
			} else if(isCReg(p, Opr0) && !strcmp(Opr1, "+=") && isIReg(p, Opr2)
				&& isMReg(p, Opr3) && !strcasecmp(Opr4, "DM")){
				/* type 32b */
				/* [IF COND] LD.C DREG24, DM(IREG +=  MREG) */
				/* [IF COND] LD.C Op0,  Op4(Op2 Op1 Op3) */
				p->InstType = t32b;
			} else if(isCReg(p, Opr0) && !strcmp(Opr1, "+") && isIReg(p, Opr2)
				&& isMReg(p, Opr3) && !strcasecmp(Opr4, "DM")){
				/* type 32b */
				/* [IF COND] LD.C DREG24, DM(IREG +   MREG) */
				/* [IF COND] LD.C Op0,  Op4(Op2 Op1 Op3) */
				p->InstType = t32b;
			} else if(isCReg(p, Opr0) && !strcmp(Opr1, "+=") && isIReg(p, Opr2)
				&& isMReg(p, Opr3) && !strcasecmp(Opr4, "PM")){
				/* type 32b */
				/* [IF COND] LD.C DREG24, PM(IREG +=  MREG) */
				/* [IF COND] LD.C Op0,  Op4(Op2 Op1 Op3) */
				printRunTimeError(p->LineCntr, Opr4, 
					"Accessing Program Memory not supported.\n");
				break;

				/*
				p->InstType = t32b;
				*/
			} else if(isCReg(p, Opr0) && !strcmp(Opr1, "+") && isIReg(p, Opr2)
				&& isMReg(p, Opr3) && !strcasecmp(Opr4, "PM")){
				/* type 32b */
				/* [IF COND] LD.C DREG24, PM(IREG +   MREG) */
				/* [IF COND] LD.C Op0,  Op4(Op2 Op1 Op3) */
				printRunTimeError(p->LineCntr, Opr4, 
					"Accessing Program Memory not supported.\n");
				break;

				/*
				p->InstType = t32b;
				*/
			} else if(isDReg24(p, Opr0) && !strcmp(Opr1, "+=") && isIReg(p, Opr2)
				&& isInt(Opr3) && !strcasecmp(Opr4, "DM")){
				if(isIntSignedN(p, symTable, Opr3, 8)){
					/* type 29b */
					/* [IF COND] LD.C DREG24, DM(IREG +=  <IMM_INT8>) */
					/* [IF COND] LD.C Op0,    Op4(Op2 Op1 Op3) */
					p->InstType = t29b;
				}
			} else if(isDReg24(p, Opr0) && !strcmp(Opr1, "+") && isIReg(p, Opr2)
				&& isInt(Opr3) && !strcasecmp(Opr4, "DM")){
				if(isIntSignedN(p, symTable, Opr3, 8)){
					/* type 29b */
					/* [IF COND] LD.C DREG24, DM(IREG +   <IMM_INT8>) */
					/* [IF COND] LD.C Op0,    Op4(Op2 Op1 Op3) */
					p->InstType = t29b;
				}
			} else if(isDReg24(p, Opr0) && isIReg(p, Opr1)
				&& !strcasecmp(Opr2, "DM")){
				/* type 29b */
				/* [IF COND] LD.C DREG12, DM(IREG) */
				/* [IF COND] LD.C Op0,    Op2(Op1) */
				p->InstType = t29b;
			} else if(isRReg16(p, Opr0) && isInt(Opr1) && (Opr2 != NULL) 
				&& !strcasecmp(Opr2, "DM")){
				/* type: Not allowed!!  */
				/* LD.C RREG16, DM(<IMM_INT16>) */
				/* LD.C Op0,    Op2(Op1)        */
				printRunTimeError(p->LineCntr, Opr0, 
					"LD.C does not support this register operand.\n");
				break;
			} else if(isRReg16(p, Opr0) && isInt(Opr1)){
				/* type 6a */
				/* [IF COND] LD RREG16, <IMM_INT16> */
				/* [IF COND] LD Op0,    Op1         */
				printRunTimeError(p->LineCntr, Opr0, 
					"LD.C does not support this register operand.\n");
				break;
			}
			break;
		case	iMAC:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isACC32(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				/* type 40e */
				/* [IF COND] MAC ACC32, XOP12, YOP12 (|RND, SS, SU, US, UU|) */
				/* [IF COND] MAC Opr0,  Opr1,  Opr2  (Opr3)                  */
				p->InstType = t40e;

				if(!Opr3){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
				}
			}
			break;
		case	iMAC_C:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isACC64(p, Opr0) && isXOP24(p, Opr1) && isXOP24(p, Opr2)){
				/* type 40f */
				/* [IF COND] MAC.C ACC64, XOP24, YOP24[*] (|RND, SS, SU, US, UU|) */
				/* [IF COND] MAC.C Opr0,  Opr1,  Opr2[*]  (Opr3)                  */
				p->InstType = t40f;

				if(!Opr3){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
				}
			}
			break;
		case	iMAC_RC:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isACC64(p, Opr0) && isXOP12(p, Opr1) && isXOP24(p, Opr2)){
				/* type 40g */
				/* [IF COND] MAC.RC ACC64, XOP12, YOP24[*] (|RND, SS, SU, US, UU|) */
				/* [IF COND] MAC.RC Opr0,  Opr1,  Opr2[*]  (Opr3)                  */
				p->InstType = t40g;

				if(!Opr3){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
				}
			}
			break;
		case	iMAS:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isACC32(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				/* type 40e */
				/* [IF COND] MAS ACC32, XOP12, YOP12 (|RND, SS, SU, US, UU|) */
				/* [IF COND] MAS Opr0,  Opr1,  Opr2  (Opr3)                  */
				p->InstType = t40e;

				if(!Opr3){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
				}
			}
			break;
		case	iMAS_C:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isACC64(p, Opr0) && isXOP24(p, Opr1) && isXOP24(p, Opr2)){
				/* type 40f */
				/* [IF COND] MAS.C ACC64, XOP24, YOP24[*] (|RND, SS, SU, US, UU|) */
				/* [IF COND] MAS.C Opr0,  Opr1,  Opr2[*]  (Opr3)                  */
				p->InstType = t40f;

				if(!Opr3){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
				}
			}
			break;
		case	iMAS_RC:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isACC64(p, Opr0) && isXOP12(p, Opr1) && isXOP24(p, Opr2)){
				/* type 40g */
				/* [IF COND] MAS.RC ACC64, XOP12, YOP24[*] (|RND, SS, SU, US, UU|) */
				/* [IF COND] MAS.RC Opr0,  Opr1,  Opr2[*]  (Opr3)                  */
				p->InstType = t40g;

				if(!Opr3){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
				}
			}
			break;
		case	iMPY:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isACC32(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				/* type 40e */
				/* [IF COND] MPY ACC32, XOP12, YOP12 (|RND, SS, SU, US, UU|) */
				/* [IF COND] MPY Opr0,  Opr1,  Opr2  (Opr3)                  */
				p->InstType = t40e;

				if(!Opr3){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
				}
			}
			break;
		case	iMPY_C:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isACC64(p, Opr0) && isXOP24(p, Opr1) && isXOP24(p, Opr2)){
				/* type 40f */
				/* [IF COND] MPY.C ACC64, XOP24, YOP24[*] (|RND, SS, SU, US, UU|) */
				/* [IF COND] MPY.C Opr0,  Opr1,  Opr2[*]  (Opr3)                  */
				p->InstType = t40f;

				if(!Opr3){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
				}
			}
			break;
		case	iMPY_RC:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isACC64(p, Opr0) && isXOP12(p, Opr1) && isXOP24(p, Opr2)){
				/* type 40g */
				/* [IF COND] MPY.RC ACC64, XOP12, YOP24[*] (|RND, SS, SU, US, UU|) */
				/* [IF COND] MPY.RC Opr0,  Opr1,  Opr2[*]  (Opr3)                  */
				p->InstType = t40g;

				if(!Opr3){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
				}
			}
			break;
		case	iMAG_C:
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1)){
				/* type 47a */
				/* [IF COND] MAG.C DREG24, XOP24[*] */
				/* [IF COND] MAG.C Op0,    Op1[*]   */
				p->InstType = t47a;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iNOP:
			/* type 30a */
			/* [IF COND] NOP */
			p->InstType = t30a;
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iPOLAR_C:
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1)){
				/* type 42a */
				/* [IF COND] POLAR.C DREG24, XOP24[*] */
				/* [IF COND] POLAR.C Op0,    Op1[*]   */
				p->InstType = t42a;
				p->Latency = 8;			
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iPOP:
			/* type 26a */
			/* [IF COND] POP |PC, LOOP| [, STS] */
			/* [IF COND] POP Opr0,      [, STS] */
			if(p->OperandCounter)
				p->InstType = t26a;
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iPUSH:
			/* type 26a */
			/* [IF COND] PUSH |PC, LOOP| [, STS] */
			/* [IF COND] PUSH Opr0,      [, STS] */
			if(p->OperandCounter)
				p->InstType = t26a;
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iRCCW_C:
		case	iRCW_C:
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isXOP12(p, Opr2)){
				/* type 50a */
				/* [IF COND] RCCW.C DREG24, XOP24, YOP12 */
				/* [IF COND] RCCW.C Op0,    Op1,   Op2   */
				p->InstType = t50a;
			} 
			break;
		case	iRECT_C:
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1)){
				/* type 42a */
				/* [IF COND] RECT.C DREG24, XOP24[*] */
				/* [IF COND] RECT.C Op0,    Op1[*]   */
				p->InstType = t42a;
				p->Latency = 8;		
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iRESET:
			/* type 48a */
			/* [IF COND] RESET */
			p->InstType = t48a;
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iRNDACC:
			/* type 41a */
			/* [IF COND] RNDACC ACC32 */
			/* [IF COND] RNDACC Op0   */
			if(isACC32(p, Opr0)){
				p->InstType = t41a;
			}
			break;
		case	iRNDACC_C:
			/* type 41b */
			/* [IF COND] RNDACC.C ACC64[*] */
			/* [IF COND] RNDACC.C Op0[*]   */
			if(isACC64(p, Opr0)){
				p->InstType = t41b;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
		case	iRTS:
		case	iRTI:
			/* type 20a */
			/* [IF COND] RTS/RTI */
			p->InstType = t20a;

			if(DelaySlotMode){
				sICode *pn = getNextCode(p);
				pn->isDelaySlot = TRUE;			/* identify delay slot */
			}else{		/* if delay slot diasbled, 1-cycle stall */
				p->Latency = 2;
			}
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iSATACC:
			/* type 25a */
			/* [IF COND] SATACC ACC32 */
			/* [IF COND] SATACC Opr0  */
			if(isACC32(p, Opr0)){
				p->InstType = t25a;
			}
			break;
		case	iSATACC_C:
			/* type 25b */
			/* [IF COND] SATACC.C ACC64[*] */
			/* [IF COND] SATACC.C Opr0[*]  */
			if(isACC64(p, Opr0)){
				p->InstType = t25b;
			}
			break;
		case	iSCR:
			/* type 46a */
			/* [IF COND] SCR DREG12, XOP12, YOP12 */
			/* [IF COND] SCR Op0,    Op1,   Op2   */
			if(isDReg12(p, Opr0) && isXOP12(p, Opr1) && isXOP12(p, Opr2)){
				p->InstType = t46a;
			}
			break;
		case	iSCR_C:
			/* type 46b */
			/* [IF COND] SCR.C DREG24, XOP24, YOP24[*] */
			/* [IF COND] SCR.C Op0,    Op1,   Op2[*]   */
			if(isDReg24(p, Opr0) && isXOP24(p, Opr1) && isXOP24(p, Opr2)){
				p->InstType = t46b;
			} 
			break;
		///////////////////////////////////////////////////////////////////////////////////////
		// Program Flow Instruction
		///////////////////////////////////////////////////////////////////////////////////////
        case    iSETINT:
            if(isInt(Opr0)) {
				if(isIntUnsignedN(p, symTable, Opr0, 4)) {
               		/* type 37a */
					/* [IF COND] SETINT IMM_UINT4   */
					/* [IF COND] SETINT Op0 */
					p->InstType = t37a;
				}
            }
            break;
		///////////////////////////////////////////////////////////////////////////////////////
		case	iST:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isInt(Opr0) && (Opr1 != NULL) && !strcasecmp(Opr1, "DM") 
				&& (Opr2 != NULL) && isDReg12(p, Opr2)){
				if(isIntUnsignedN(p, symTable, Opr0, 16)){
					/* type 3f */
					/* [IF COND] ST DM(IMM_UINT16), DREG12 */
					/* [IF COND] ST Op1(Op0),        Op2    */
					p->InstType = t03f;
				}
			} else if(isInt(Opr0) 
				&& (Opr1 != NULL)
				&& (!strcmp(Opr1, "+") || !strcmp(Opr1, "["))
				&& isInt(Opr2) && (Opr3 != NULL) && !strcasecmp(Opr3, "DM")
				&& isDReg12(p, Opr4)){
				if(isIntUnsignedN(p, symTable, Opr0, 16)){
					/* type 3f */
					/* Variation due to .VAR usage */
					/* [IF COND] ST DM(IMM_UINT16 +   const ), DREG12 */
					/* [IF COND] ST DM(IMM_UINT16 [   const]), DREG12 */
					/* [IF COND] ST Op3(Op0        Op1 Op2   ), Op4    */
					p->InstType = t03f;

					/* preprocessing: back to normal type 3f */
					/* after:                                */
					/* ST DM(IMM_UINT16), DREG12            */
					/* ST Op1(Op0),        Op2               */
					//int iop0 = 0xFFFF & (getIntSymAddr(p, symTable, Opr0) + atoi(Opr2));
					int iop0 = getIntImm(p, (getIntSymAddr(p, symTable, Opr0) + atoi(Opr2)), eIMM_UINT16);

					/* free operands */
					free(p->Operand[0]); 
					free(p->Operand[1]); 
					free(p->Operand[2]); 

					char sop0[10];
					sprintf(sop0, "%d", iop0);

					/* new operands */
					p->Operand[0] = strdup(sop0);
					p->Operand[1] = p->Operand[3];
					p->Operand[2] = p->Operand[4];
					p->OperandCounter = 3;

					/* make unused operands NULL */
					p->Operand[3] = NULL;
					p->Operand[4] = NULL;
				}
			} else if(isInt(Opr0) && (Opr1 != NULL) && !strcasecmp(Opr1, "DM") 
				&& (Opr2 != NULL) && isRReg16(p, Opr2)){
				/* type 3b */
				/* ST DM(<IMM_INT16>), RREG16 */
				/* ST Op1(Op0),        Op2    */
				printRunTimeError(p->LineCntr, Opr2, 
					"ST DM(<IMM_INT16>), RREG16 syntax not allowed.\n");
				break;

				/*
				p->InstType = t03b;
				*/
			} else if(isInt(Opr0) && (Opr1 != NULL) && !strcasecmp(Opr1, "DM") 
				&& (Opr2 != NULL) && isACC32(p, Opr2)){
				if(isIntUnsignedN(p, symTable, Opr0, 16)){
					/* type 3h */
					/* [IF COND] ST DM(IMM_UINT16), ACC32, (|HI,LO|) */
					/* [IF COND] ST Op1(Op0),        Op2,   (Op3)    */
					/* Note: store upper or lower **24** bits of 32-bit accumulator data */
					p->InstType = t03h;

					if(!Opr3){
						printRunTimeError(p->LineCntr, Opr0, 
							"(HI) or (LO) required. Please check syntax.\n");
						break;
					}
				}
			} else if(isInt(Opr0) 
				&& (Opr1 != NULL)
				&& (!strcmp(Opr1, "+") || !strcmp(Opr1, "["))
				&& isInt(Opr2) && (Opr3 != NULL) && !strcasecmp(Opr3, "DM")
				&& isACC32(p, Opr4)){
				if(isIntUnsignedN(p, symTable, Opr0, 16)){
					/* type 3h */
					/* Variation due to .VAR usage */
					/* [IF COND] ST DM(IMM_UINT16 +   const ), ACC32, (|HI,LO|) */
					/* [IF COND] ST DM(IMM_UINT16 [   const]), ACC32, (|HI,LO|) */
					/* [IF COND] ST Op3(Op0       Op1 Op2),    Op4,   (Op5)    */
					/* Note: store upper or lower **24** bits of 32-bit accumulator data */
					p->InstType = t03h;

					/* preprocessing: back to normal type 3h */
					/* after:                                */
					/* ST DM(<IMM_INT16>), ACC32, (|HI,LO|) */
					/* ST Op1(Op0),        Op2,   (Op3)    */
					//int iop0 = 0xFFFF & (getIntSymAddr(p, symTable, Opr0) + atoi(Opr2));
					int iop0 = getIntImm(p, (getIntSymAddr(p, symTable, Opr0) + atoi(Opr2)), eIMM_UINT16);

					/* free operands */
					free(p->Operand[0]); 
					free(p->Operand[1]); 
					free(p->Operand[2]); 

					char sop0[10];
					sprintf(sop0, "%d", iop0);

					/* new operands */
					p->Operand[0] = strdup(sop0);
					p->Operand[1] = p->Operand[3];
					p->Operand[2] = p->Operand[4];
					p->Operand[3] = p->Operand[5];
					p->OperandCounter = 4;

					/* make unused operands NULL */
					p->Operand[4] = NULL;
					p->Operand[5] = NULL;
				}
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "DM") && isRReg(p, Opr4)){
				/* type 32c */
				/* [IF COND] ST DM(IREG += MREG), RREG */
				/* [IF COND] ST Op3(Op1 Op0 Op2), Op4  */
				p->InstType = t32c;
			} else if(!strcmp(Opr0, "+") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "DM") && isRReg(p, Opr4)){
				/* type 32c */
				/* [IF COND] ST DM(IREG + MREG), RREG */
				/* [IF COND] ST Op3(Op1 Op0 Op2), Op4  */
				p->InstType = t32c;
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "PM") && isRReg(p, Opr4)){
				/* type 32c */
				/* [IF COND] ST PM(IREG += MREG), RREG */
				/* [IF COND] ST Op3(Op1 Op0 Op2), Op4  */
				printRunTimeError(p->LineCntr, Opr4, 
					"Accessing Program Memory not supported.\n");
				break;

				/*
				p->InstType = t32c;
				*/
			} else if(!strcmp(Opr0, "+") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "PM") && isRReg(p, Opr4)){
				/* type 32c */
				/* [IF COND] ST PM(IREG + MREG), RREG */
				/* [IF COND] ST Op3(Op1 Op0 Op2), Op4  */
				printRunTimeError(p->LineCntr, Opr4, 
					"Accessing Program Memory not supported.\n");
				break;

				/*
				p->InstType = t32c;
				*/
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isInt(Opr2)
				&& !strcasecmp(Opr3, "DM") && isDReg12(p, Opr4)){
				if(isIntSignedN(p, symTable, Opr2, 8)){
					/* type 29c */
					/* [IF COND] ST DM(IREG += <IMM_INT8>), DREG12 */
					/* [IF COND] ST Op3(Op1 Op0 Op2      ), Op4    */
					p->InstType = t29c;
				}
			} else if(!strcmp(Opr0, "+") && isIReg(p, Opr1) && isInt(Opr2)
				&& !strcasecmp(Opr3, "DM") && isDReg12(p, Opr4)){
				if(isIntSignedN(p, symTable, Opr2, 8)){
					/* type 29c */
					/* [IF COND] ST DM(IREG + <IMM_INT8>), DREG12 */
					/* [IF COND] ST Op3(Op1 Op0 Op2     ), Op4    */
					p->InstType = t29c;
				}
			} else if(isIReg(p, Opr0) && !strcasecmp(Opr1, "DM")
				&& isDReg12(p, Opr2)){
				/* type 29c */
				/* [IF COND] ST DM(IREG), DREG12 */
				/* [IF COND] ST Op1(Op0), Op2    */
				p->InstType = t29c;
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "DM") && isInt(Opr4)){
				if(isIntSignedN(p, symTable, Opr4, 12)){
					/* type 22a */
					/* [IF COND] ST DM(IREG += MREG), <IMM_INT12> */
					/* [IF COND] ST Op3(Op1 Op0 Op2), Op4         */
					p->InstType = t22a;
				}
			} else if(!strcmp(Opr0, "+") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "DM") && isInt(Opr4)){
				if(isIntSignedN(p, symTable, Opr4, 12)){
					/* type 22a */
					/* [IF COND] ST DM(IREG + MREG), <IMM_INT12> */
					/* [IF COND] ST Op3(Op1 Op0 Op2), Op4         */
					p->InstType = t22a;
				}
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "PM") && isInt(Opr4)){
				/* type 22b */
				/* ST PM(IREG += MREG), <IMM_INT16> */
				/* ST Op3(Op1 Op0 Op2), Op4         */
				printRunTimeError(p->LineCntr, Opr4, 
					"Accessing Program Memory not supported.\n");
				break;

				/*
				p->InstType = t22b;
				*/
			} else if(isSysCtlReg(Opr0) && isDReg12(p, Opr1)){
				/* type 35a */
				/* ST CACTL, DREG12 */
				/* ST Op0,   Op1    */
				printRunTimeError(p->LineCntr, Opr1, 
					"CACTL Register not supported.\n");
				break;
				/*
				p->InstType = t35a;
				*/
			}
			break;
		case	iST_C:
			if(isMultiFunc(p)){
				p = codeScanOneInstMultiFunc(p);
				break;
			}

			if(isInt(Opr0) && (Opr1 != NULL) && !strcasecmp(Opr1, "DM") 
				&& (Opr2 != NULL) && isDReg24(p, Opr2)){
				if(isIntUnsignedN(p, symTable, Opr0, 16)){
					/* type 3g */
					/* [IF COND] ST.C DM(IMM_UINT16), DREG24 */
					/* [IF COND] ST.C Op1(Op0),        Op2    */
					p->InstType = t03g;
				}
			} else if(isInt(Opr0) 
				&& (Opr1 != NULL)
				&& (!strcmp(Opr1, "+") || !strcmp(Opr1, "["))
				&& isInt(Opr2) && (Opr3 != NULL) && !strcasecmp(Opr3, "DM")
				&& isDReg24(p, Opr4)){
				if(isIntUnsignedN(p, symTable, Opr0, 16)){
					/* type 3g */
					/* Variation due to .VAR usage */
					/* [IF COND] ST.C DM(IMM_UINT16 +   const ), DREG24 */
					/* [IF COND] ST.C DM(IMM_UINT16 [   const]), DREG24 */
					/* [IF COND] ST.C Op3(Op0       Op1 Op2   ), Op4    */
					p->InstType = t03g;

					/* preprocessing: back to normal type 3g */
					/* after:                                */
					/* ST.C DM(<IMM_INT16>), DREG24          */
					/* ST.C Op1(Op0),        Op2             */
					//int iop0 = 0xFFFF & (getIntSymAddr(p, symTable, Opr0) + atoi(Opr2));
					int iop0 = getIntImm(p, (getIntSymAddr(p, symTable, Opr0) + atoi(Opr2)), eIMM_UINT16);

					/* free operands */
					free(p->Operand[0]); 
					free(p->Operand[1]); 
					free(p->Operand[2]); 

					char sop0[10];
					sprintf(sop0, "%d", iop0);

					/* new operands */
					p->Operand[0] = strdup(sop0);
					p->Operand[1] = p->Operand[3];
					p->Operand[2] = p->Operand[4];
					p->OperandCounter = 3;

					/* make unused operands NULL */
					p->Operand[3] = NULL;
					p->Operand[4] = NULL;
				}
			} else if(isInt(Opr0) && (Opr1 != NULL) && !strcasecmp(Opr1, "DM") 
				&& (Opr2 != NULL) && isACC64(p, Opr2)){
				if(isIntUnsignedN(p, symTable, Opr0, 16)){
					/* type 3i */
					/* [IF COND] ST.C DM(IMM_UINT16), ACC64, (|HI,LO|) */
					/* [IF COND] ST.C Op1(Op0),        Op2,   (Op3)    */
					/* Note: store upper or lower **24** bits of 32-bit accumulator data (complex pair) */
					p->InstType = t03i;

					/* latency restriction: t03i: 2-cycle instruction */
					p->Latency = 2;

					if(!Opr3){
						printRunTimeError(p->LineCntr, Opr0, 
							"(HI) or (LO) required. Please check syntax.\n");
						break;
					}
				}
			} else if(isInt(Opr0) 
				&& (Opr1 != NULL)
				&& (!strcmp(Opr1, "+") || !strcmp(Opr1, "["))
				&& isInt(Opr2) && (Opr3 != NULL) && !strcasecmp(Opr3, "DM")
				&& isACC64(p, Opr4)){
				if(isIntUnsignedN(p, symTable, Opr0, 16)){
					/* type 3i */
					/* Variation due to .VAR usage */
					/* [IF COND] ST.C DM(IMM_UINT16 +   const ), ACC64, (|HI,LO|) */
					/* [IF COND] ST.C DM(IMM_UINT16 [   const]), ACC64, (|HI,LO|) */
					/* [IF COND] ST.C Op3(Op0        Op1 Op2),    Op4,   (Op5)     */
					/* Note: store upper or lower **24** bits of 32-bit accumulator data */
					p->InstType = t03i;

					/* latency restriction: t03i: 2-cycle instruction */
					p->Latency = 2;

					/* preprocessing: back to normal type 3i  */
					/* after:                                 */
					/* ST.C DM(IMM_UINT16), ACC64, (|HI,LO|) */
					/* ST.C Op1(Op0),        Op2,   (Op3)     */
					//int iop0 = 0xFFFF & (getIntSymAddr(p, symTable, Opr0) + atoi(Opr2));
					int iop0 = getIntImm(p, (getIntSymAddr(p, symTable, Opr0) + atoi(Opr2)), eIMM_UINT16);

					/* free operands */
					free(p->Operand[0]); 
					free(p->Operand[1]); 
					free(p->Operand[2]); 

					char sop0[10];
					sprintf(sop0, "%d", iop0);

					/* new operands */
					p->Operand[0] = strdup(sop0);
					p->Operand[1] = p->Operand[3];
					p->Operand[2] = p->Operand[4];
					p->Operand[3] = p->Operand[5];
					p->OperandCounter = 4;

					/* make unused operands NULL */
					p->Operand[4] = NULL;
					p->Operand[5] = NULL;
				}
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "DM") && isCReg(p, Opr4)){
				/* type 32d */
				/* [IF COND] ST.C DM(IREG += MREG), DREG24 */
				/* [IF COND] ST.C Op3(Op1 Op0 Op2), Op4  */
				p->InstType = t32d;
			} else if(!strcmp(Opr0, "+") && isIReg(p, Opr1) && isMReg(p, Opr2)
				&& !strcasecmp(Opr3, "DM") && isCReg(p, Opr4)){
				/* type 32d */
				/* [IF COND] ST.C DM(IREG + MREG),  DREG24 */
				/* [IF COND] ST.C Op3(Op1 Op0 Op2), Op4  */
				p->InstType = t32d;
			} else if(!strcmp(Opr0, "+=") && isIReg(p, Opr1) && isInt(Opr2)
				&& !strcasecmp(Opr3, "DM") && isDReg24(p, Opr4)){
				if(isIntSignedN(p, symTable, Opr2, 8)){
					/* type 29d */
					/* [IF COND] ST.C DM(IREG += <IMM_INT8>), DREG24 */
					/* [IF COND] ST.C Op3(Op1 Op0 Op2      ), Op4    */
					p->InstType = t29d;
				}
			} else if(!strcmp(Opr0, "+") && isIReg(p, Opr1) && isInt(Opr2)
				&& !strcasecmp(Opr3, "DM") && isDReg24(p, Opr4)){
				if(isIntSignedN(p, symTable, Opr2, 8)){
					/* type 29d */
					/* [IF COND] ST.C DM(IREG + <IMM_INT8>), DREG24 */
					/* [IF COND] ST.C Op3(Op1 Op0 Op2     ), Op4    */
					p->InstType = t29d;
				}
			} else if(isIReg(p, Opr0) && !strcasecmp(Opr1, "DM")
				&& isDReg24(p, Opr2)){
				/* type 29d */
				/* [IF COND] ST.C DM(IREG), DREG24 */
				/* [IF COND] ST.C Op1(Op0), Op2    */
				p->InstType = t29d;
			} else if(isInt(Opr0) && (Opr1 != NULL) && !strcasecmp(Opr1, "DM") 
				&& (Opr2 != NULL) && isRReg16(p, Opr2)){
				/* type: Not allowed!!  */
				/* [IF COND] ST.C DM(<IMM_INT16>), RREG16 */
				/* [IF COND] ST.C Op1(Op0),        Op2    */
				printRunTimeError(p->LineCntr, Opr2, 
					"ST.C does not support this register operand.\n");
				break;
			}
			break;
		case	i_CODE:
			/* assembler pseudo instructions */
			if(curSecInfo)
				curSecInfo->Size = curaddr - dataSegAddr;	/* update size of current segement */

			sip = sSecInfoListSearch(&secInfo, "CODE");
			if(sip){	/* existing segment */
				curSecInfo = sip;
				curaddr = curSecInfo->Size + codeSegAddr;	/* set start address of current segment */
			}else{	/* new segment */
				curSecInfo = sSecInfoListAdd(&secInfo, "CODE", tCODE);
				curaddr = codeSegAddr;				/* set start address of current segment */
			}

			/* search entire symbol table and modify its Sec(pointer to secinfo) */
			/* Since .CODE always after .DATA, 
			   every symbol (not assigned secinfo by .VAR) belongs to .CODE */
			for(int i = 0; i < MAX_HASHTABLE; i++){
				if(symTable[i].FirstNode){
					sTab *n = symTable[i].FirstNode;
					while(n != NULL){
						if(n->Sec == NULL && n->Type != tEXTERN){
							n->Sec = curSecInfo;
						}
						n = n->Next;
					}
				}
			}

			break;
		case	i_DATA:
			/* assembler pseudo instructions */
			if(curSecInfo)
				curSecInfo->Size = curaddr - codeSegAddr;	/* update size of current segement */

			sip = sSecInfoListSearch(&secInfo, "DATA");
			if(sip){	/* existing segment */
				curSecInfo = sip;
				curaddr = curSecInfo->Size + dataSegAddr;	/* set start address of current segment */
			}else{	/* new segment */
				curSecInfo = sSecInfoListAdd(&secInfo, "DATA", tDATA);
				curaddr = dataSegAddr;				/* set start address of current segment */
			}
			break;
		case	i_VAR:
			/* assembler pseudo instructions */
			/* .VAR varname                  */
			/* .VAR Op0                      */
			/*             or                */
			/* .VAR varname [   arraysize ]  */
			/* .VAR Op0     Op1 Op2       ]  */
			if(p->OperandCounter){
				int arraySize;

				for(int i = 0; i < p->OperandCounter; i++){
					/* put it in the symTab */
					sp = sTabHashSearch(symTable, p->Operand[i]);
					sp->Addr = curaddr; 
					sp->Sec = curSecInfo;	/* attach segment info, in this case, DATA */

					/* reserve data memory */
					if(p->Operand[i+1] && p->Operand[i+1][0] == '['){		/* if array */
						arraySize = getIntSymAddr(p, symTable, p->Operand[i+2]);
						if(arraySize < 1){
							printRunTimeError(p->LineCntr, p->Operand[i+2], 
								"Array size must be a positive integer number.\n");
							break;
						}

						sint sUNDEFINED;                                                                             
						for(int j = 0; j < NUMDP; j++) {                                                             
							sUNDEFINED.dp[j] = 0x0FFF & UNDEFINED;                                                   
						}                                                      

						for(int j = 0; j < arraySize; j++){
							dMemHashAdd(dataMem, sUNDEFINED, curaddr+j);
						}

						/* set size in symbol table */
						sp->Size = arraySize;
					}else{		/* if not array (scalar variable) */
						sint sUNDEFINED;                                                                             
						for(int j = 0; j < NUMDP; j++) {                                                             
							sUNDEFINED.dp[j] = 0x0FFF & UNDEFINED;                                                   
						}                                                      

						dMemHashAdd(dataMem, sUNDEFINED, curaddr);

						/* set size in symbol table */
						sp->Size = 1;
					}

					/* calculate curaddr(DMA) */
					if(p->Operand[i+1] && p->Operand[i+1][0] == '['){		/* if array */
						if(arraySize > 0){
							curaddr += arraySize;
						}
						i += 2;
					}else{		/* if not array */
						curaddr ++;
					}
				}
				curaddr--;	
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	i_GLOBAL:
			/* assembler pseudo instructions */
			/* .GLOBAL varname                  */
			/* .GLOBAL Op0                      */
			if(p->OperandCounter){
				for(int i = 0; i < p->OperandCounter; i++){
					sp = sTabHashSearch(symTable, p->Operand[i]);	/* must already be in the table */
					sp->Type = tGLOBAL;
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	i_EXTERN:
			/* assembler pseudo instructions */
			/* .EXTERN varname                  */
			/* .EXTERN Op0                      */
			if(p->OperandCounter){
				for(int i = 0; i < p->OperandCounter; i++){
					sp = sTabHashSearch(symTable, p->Operand[i]);	/* must be new */
					sp->Type = tEXTERN;
					sp->Addr = 0;		/* addr of EXTERN variable: defaults to 0 */
					sp->Sec = NULL;
				}
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		case	i_EQU:
			/* assembler pseudo instructions */
			/* .EQU varname, value           */
			/* .EQU Op0,     Op1             */
			if(p->OperandCounter == 2){
				/* put it in the symTab */
				sp = sTabHashSearch(symTable, p->Operand[0]);
				sp->Sec = sSecInfoListSearch(&secInfo, "CODE");
				sp->isConst = TRUE;		/* since it's constant, it's ABSOLUTE always */
				/* attach segment info as CODE (because of 0-size) */
				sp->Addr = getIntSymAddr(p, symTable, p->Operand[1]);
			}else{
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Invalid operands! Please check instruction syntax.\n");
				break;
			}
			break;
		default:		/* only for iLABEL, iCOMMENT */	
			break;
	}

	NextCode = p->Next;

	if(!isNotRealInst(p->Index) || (p->Index == i_VAR))
		curaddr++;

	/* skip comment and/or label only lines */
	if(p != NULL){
		while(p != NULL && isCommentLabelInst(p->Index)){
			p = p->Next;
		}
	}
	return (NextCode);
}

/** 
* @brief Scan one multifunction assembly source line for instruction type resolution
* 
* @param *p Pointer to current instruction 
* 
* @return Pointer to next instruction
*/
sICode *codeScanOneInstMultiFunc(sICode *p)
{
	sICode *m1, *m2;

	switch(p->MultiCounter){
		case	1:				/* two-opcode multifunction */
			m1 = p->Multi[0];

			if(isLD(p) && isLD(m1)){
				/* type 1a */
				p->InstType = t01a;
				/* LD || LD */

				/* read first LD operands */
				/* LD RREG, DM(IREG +=  MREG) */
				/* LD Op0,  Op4(Op2 Op1 Op3) */
				if(!isReg12S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of LD, only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD, only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}

				if(!isIx(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For IX operand of LD, only I0, I1, I2, I3 registers are allowed.\n");
					break;
				}
				if(!isMx(p, p->Operand[3])){
					printRunTimeError(p->LineCntr, p->Operand[3], 
						"For MX operand of LD, only M0, M1, M2, M3 registers are allowed.\n");
					break;
				}
				if(!isIy(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IY operand of LD, only I4, I5, I6, I7 registers are allowed.\n");
					break;
				}
				if(!isMy(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MY operand of LD, only M4, M5, M6, M7 registers are allowed.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
			}else if(isLD_C(p) && isLD_C(m1)){
				/* type 1b */
				p->InstType = t01b;
				/* LD.C || LD.C */
				/* LD.C DREG24, DM(IREG +=  MREG) */
				/* LD.C Op0,    Op4(Op2 Op1 Op3)  */
				/*            or                  */
				/* LD.C DREG24, DM(IREG)          */
				/* LD.C Op0,    Op1(Op0)          */

				if(!isReg24S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of LD.C, only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD.C, only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}

				if(!isIx(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For IX operand of LD.C, only I0, I1, I2, I3 registers are allowed.\n");
					break;
				}
				if(!isMx(p, p->Operand[3])){
					printRunTimeError(p->LineCntr, p->Operand[3], 
						"For MX operand of LD.C, only M0, M1, M2, M3 registers are allowed.\n");
					break;
				}
				if(!isIy(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IY operand of LD.C, only I4, I5, I6, I7 registers are allowed.\n");
					break;
				}
				if(!isMy(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MY operand of LD.C, only M4, M5, M6, M7 registers are allowed.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
			}else if(isMAC(p) && isLD(m1)){
				/* type 4a */
				p->InstType = t04a;
				/* MAC || LD */

				if(!isACC32S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC instruction, "
						"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!p->Operand[3]){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
					break;
				}
				if(!isDReg12(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}
				if(!isIReg(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IREG operand of LD, only I0~7 registers are allowed.\n");
					break;
				}
				if(!isMReg(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MREG operand of LD, only M0~7 registers are allowed.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
			}else if(isALU(p) && isLD(m1)){
				/* type 4c */
				p->InstType = t04c;
				/* ALU || LD */

				if(!isDReg12S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU instruction, "
						"only R0~R7 and ACC0.L~ACC7.L registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg12(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}
				if(!isIReg(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IREG operand of LD, only I0~7 registers are allowed.\n");
					break;
				}
				if(!isMReg(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MREG operand of LD, only M0~7 registers are allowed.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
			}else if(isMAC_C(p) && isLD_C(m1)){
				/* type 4b */
				p->InstType = t04b;
				/* MAC.C || LD.C */

				if(!isACC64S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC.C instruction, "
						"only ACC0, ACC2 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!p->Operand[3]){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
					break;
				}
				if(!isDReg24(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}
				if(!isIReg(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IREG operand of LD, only I0~7 registers are allowed.\n");
					break;
				}
				if(!isMReg(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MREG operand of LD, only M0~7 registers are allowed.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
			}else if(isALU_C(p) && isLD_C(m1)){
				/* type 4d */
				p->InstType = t04d;
				/* ALU.C || LD.C */

				if(!isDReg24S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU.C instruction, "
						"only R[0/2/4/6] and ACC[0/2/4/6].L registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg24S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU.C instruction, "
							"only R0, R2, R4, R6 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg24(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}
				if(!isIReg(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IREG operand of LD, only I0~7 registers are allowed.\n");
					break;
				}
				if(!isMReg(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MREG operand of LD, only M0~7 registers are allowed.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
			}else if(isMAC(p) && isST(m1)){
				/* type 4e */
				p->InstType = t04e;
				/* MAC || ST */

				if(!isACC32S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC instruction, "
						"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!p->Operand[3]){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
					break;
				}
				if(!isDReg12(p, m1->Operand[4])){
					printRunTimeError(p->LineCntr, m1->Operand[4], 
						"For destination of ST, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}
				if(!isIReg(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For IREG operand of ST, only I0~7 registers are allowed.\n");
					break;
				}
				if(!isMReg(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For MREG operand of ST, only M0~7 registers are allowed.\n");
					break;
				}
			}else if(isALU(p) && isST(m1)){
				/* type 4g */
				p->InstType = t04g;
				/* ALU || ST */

				if(!isDReg12S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU instruction, "
						"only R0~R7 and ACC0.L~ACC7.L registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg12(p, m1->Operand[4])){
					printRunTimeError(p->LineCntr, m1->Operand[4], 
						"For destination of ST, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}
				if(!isIReg(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For IREG operand of ST, only I0~7 registers are allowed.\n");
					break;
				}
				if(!isMReg(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For MREG operand of ST, only M0~7 registers are allowed.\n");
					break;
				}
			}else if(isMAC_C(p) && isST_C(m1)){
				/* type 4f */
				p->InstType = t04f;
				/* MAC.C || ST.C */

				if(!isACC64S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC.C instruction, "
						"only ACC0, ACC2 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!p->Operand[3]){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
					break;
				}
				if(!isDReg24(p, m1->Operand[4])){
					printRunTimeError(p->LineCntr, m1->Operand[4], 
						"For destination of ST.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}
				if(!isIReg(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For IREG operand of ST, only I0~7 registers are allowed.\n");
					break;
				}
				if(!isMReg(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For MREG operand of ST, only M0~7 registers are allowed.\n");
					break;
				}
			}else if(isALU_C(p) && isST_C(m1)){
				/* type 4h */
				p->InstType = t04h;
				/* ALU.C || ST.C */

				if(!isDReg24S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU.C instruction, "
						"only R[0/2/4/6] and ACC[0/2/4/6].L registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg24S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU.C instruction, "
							"only R0, R2, R4, R6 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg24(p, m1->Operand[4])){
					printRunTimeError(p->LineCntr, m1->Operand[4], 
						"For destination of ST.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}
				if(!isIReg(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For IREG operand of ST, only I0~7 registers are allowed.\n");
					break;
				}
				if(!isMReg(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For MREG operand of ST, only M0~7 registers are allowed.\n");
					break;
				}
			}else if(isMAC(p) && isCP(m1)){
				/* type 8a */
				p->InstType = t08a;
				/* MAC || CP */

				if(!isACC32S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC instruction, "
						"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!p->Operand[3]){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
					break;
				}
				if(!isDReg12(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of CP, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}
				if(!isDReg12(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For source of CP, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
			}else if(isALU(p) && isCP(m1)){
				/* type 8c */
				p->InstType = t08c;
				/* ALU || CP */

				if(!isDReg12S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU instruction, "
						"only R0~R7 and ACC0.L~ACC7.L registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg12(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of CP, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}
				if(!isDReg12(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For source of CP, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
			}else if(isMAC_C(p) && isCP_C(m1)){
				/* type 8b */
				p->InstType = t08b;
				/* MAC.C || CP.C */

				if(!isACC64S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC.C instruction, "
						"only ACC0, ACC2 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!p->Operand[3]){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
					break;
				}
				if(!isDReg24(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of CP.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}
				if(!isDReg24(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For source of CP.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
			}else if(isALU_C(p) && isCP_C(m1)){
				/* type 8d */
				p->InstType = t08d;
				/* ALU.C || CP.C */

				if(!isDReg24S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU.C instruction, "
						"only R[0/2/4/6] and ACC[0/2/4/6].L registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg24S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU.C instruction, "
							"only R0, R2, R4, R6 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg24(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of CP.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}
				if(!isDReg24(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For source of CP.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
			}else if(isSHIFT(p) && isLD(m1)){
				if(isReg12S(p, p->Operand[1])){
					/* type 12e */
					p->InstType = t12e;
					/* SHIFT || LD */
					/* SHIFT ACC32S, XOP12S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) || LD DREG12, DM(IREG+/+=MREG) */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of LD, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}
					if(!isIReg(p, m1->Operand[2])){
						printRunTimeError(p->LineCntr, m1->Operand[2], 
							"For IREG operand of LD, only I0~7 registers are allowed.\n");
						break;
					}
					if(!isMReg(p, m1->Operand[3])){
						printRunTimeError(p->LineCntr, m1->Operand[3], 
							"For MREG operand of LD, only M0~7 registers are allowed.\n");
						break;
					}
	
					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
				} else if(isACC32S(p, p->Operand[1])){
					/* type 12m */
					p->InstType = t12m;
					/* SHIFT || LD */
					/* SHIFT ACC32S, ACC32S, IMM_INT5 ( |NORND, RND| ) || LD DREG12, DM(IREG+/+=MREG) */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of LD, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}
					if(!isIReg(p, m1->Operand[2])){
						printRunTimeError(p->LineCntr, m1->Operand[2], 
							"For IREG operand of LD, only I0~7 registers are allowed.\n");
						break;
					}
					if(!isMReg(p, m1->Operand[3])){
						printRunTimeError(p->LineCntr, m1->Operand[3], 
							"For MREG operand of LD, only M0~7 registers are allowed.\n");
						break;
					}
	
					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
				} else {
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 or R0, R1, ..., R7 registers are allowed.\n");
						break;
				}
			}else if(isSHIFT_C(p) && isLD_C(m1)){
				if(isReg24S(p, p->Operand[1])){
					/* type 12f */
					p->InstType = t12f;
					/* SHIFT.C || LD.C */
					/* SHIFT.C ACC64S, XOP24S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) || LD.C DREG24, DM(IREG+/+=MREG) */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC64S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT.C instruction, "
							"only ACC0, ACC2, ACC4, ACC6 registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of LD.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}
					if(!isIReg(p, m1->Operand[2])){
						printRunTimeError(p->LineCntr, m1->Operand[2], 
							"For IREG operand of LD, only I0~7 registers are allowed.\n");
						break;
					}
					if(!isMReg(p, m1->Operand[3])){
						printRunTimeError(p->LineCntr, m1->Operand[3], 
							"For MREG operand of LD, only M0~7 registers are allowed.\n");
						break;
					}

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
				} else if(isACC64S(p, p->Operand[1])){
					/* type 12n */
					p->InstType = t12n;
					/* SHIFT.C || LD.C */
					/* SHIFT.C ACC64S, ACC64S, IMM_INT5 ( |NORND, RND| ) || LD.C DREG24, DM(IREG+/+=MREG) */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC64S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT.C instruction, "
							"only ACC0, ACC2, ACC4, ACC6 registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of LD.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}
					if(!isIReg(p, m1->Operand[2])){
						printRunTimeError(p->LineCntr, m1->Operand[2], 
							"For IREG operand of LD, only I0~7 registers are allowed.\n");
						break;
					}
					if(!isMReg(p, m1->Operand[3])){
						printRunTimeError(p->LineCntr, m1->Operand[3], 
							"For MREG operand of LD, only M0~7 registers are allowed.\n");
						break;
					}

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
				} else {
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of SHIFT instruction, "
							"only ACC0, ACC2, ACC4, ACC6 or R0, R2, R4, R6 registers are allowed.\n");
						break;
				}
			}else if(isSHIFT(p) && isST(m1)){
				if(isReg12S(p, p->Operand[1])){
					/* type 12g */
					p->InstType = t12g;
					/* SHIFT || ST */
					/* SHIFT ACC32S, XOP12S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) || ST DM(IREG+/+=MREG), DREG12 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[4])){
						printRunTimeError(p->LineCntr, m1->Operand[4], 
							"For destination of ST, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}
					if(!isIReg(p, m1->Operand[1])){
						printRunTimeError(p->LineCntr, m1->Operand[1], 
							"For IREG operand of ST, only I0~7 registers are allowed.\n");
						break;
					}
					if(!isMReg(p, m1->Operand[2])){
						printRunTimeError(p->LineCntr, m1->Operand[2], 
							"For MREG operand of ST, only M0~7 registers are allowed.\n");
						break;
					}
				} else if(isACC32S(p, p->Operand[1])){
					/* type 12o */
					p->InstType = t12o;
					/* SHIFT || ST */
					/* SHIFT ACC32S, ACC32S, IMM_INT5 ( |NORND, RND| ) || ST DM(IREG+/+=MREG), DREG12 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[4])){
						printRunTimeError(p->LineCntr, m1->Operand[4], 
							"For destination of ST, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}
					if(!isIReg(p, m1->Operand[1])){
						printRunTimeError(p->LineCntr, m1->Operand[1], 
							"For IREG operand of ST, only I0~7 registers are allowed.\n");
						break;
					}
					if(!isMReg(p, m1->Operand[2])){
						printRunTimeError(p->LineCntr, m1->Operand[2], 
							"For MREG operand of ST, only M0~7 registers are allowed.\n");
						break;
					}
				} else {
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 or R0, R1, ..., R7 registers are allowed.\n");
						break;
				}
			}else if(isSHIFT_C(p) && isST_C(m1)){
				if(isReg24S(p, p->Operand[1])){
					/* type 12h */
					p->InstType = t12h;
					/* SHIFT.C || ST.C */
					/* SHIFT.C ACC64S, XOP24S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) || ST.C DM(IREG+/+=MREG), DREG24 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC64S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT.C instruction, "
							"only ACC0, ACC2 registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[4])){
						printRunTimeError(p->LineCntr, m1->Operand[4], 
							"For destination of ST.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}
					if(!isIReg(p, m1->Operand[1])){
						printRunTimeError(p->LineCntr, m1->Operand[1], 
							"For IREG operand of ST, only I0~7 registers are allowed.\n");
						break;
					}
					if(!isMReg(p, m1->Operand[2])){
						printRunTimeError(p->LineCntr, m1->Operand[2], 
							"For MREG operand of ST, only M0~7 registers are allowed.\n");
						break;
					}
				} else if(isACC64S(p, p->Operand[1])){
					/* type 12p */
					p->InstType = t12p;
					/* SHIFT.C || ST.C */
					/* SHIFT.C ACC64S, ACC64S, IMM_INT5 ( |NORND, RND| ) || ST.C DM(IREG+/+=MREG), DREG24 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC64S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT.C instruction, "
							"only ACC0, ACC2 registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[4])){
						printRunTimeError(p->LineCntr, m1->Operand[4], 
							"For destination of ST.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}
					if(!isIReg(p, m1->Operand[1])){
						printRunTimeError(p->LineCntr, m1->Operand[1], 
							"For IREG operand of ST, only I0~7 registers are allowed.\n");
						break;
					}
					if(!isMReg(p, m1->Operand[2])){
						printRunTimeError(p->LineCntr, m1->Operand[2], 
							"For MREG operand of ST, only M0~7 registers are allowed.\n");
						break;
					}
				} else {
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of SHIFT instruction, "
							"only ACC0, ACC2, ACC4, ACC6 or R0, R2, R4, R6 registers are allowed.\n");
						break;
				}
			}else if(isSHIFT(p) && isCP(m1)){
				if(isReg12S(p, p->Operand[1])){
					/* type 14c */
					p->InstType = t14c;
					/* SHIFT || CP */
					/* SHIFT ACC32S, XOP12S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) || CP DREG12, DREG12 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of CP, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[1])){
						printRunTimeError(p->LineCntr, m1->Operand[1], 
							"For source of CP, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
				} else if(isACC32S(p, p->Operand[1])){
					/* type 14k */
					p->InstType = t14k;
					/* SHIFT || CP */
					/* SHIFT ACC32S, ACC32S, IMM_INT5 ( |NORND, RND| ) || CP DREG12, DREG12 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of CP, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}
					if(!isDReg12(p, m1->Operand[1])){
						printRunTimeError(p->LineCntr, m1->Operand[1], 
							"For source of CP, only R0~R31, "
							"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
						break;
					}

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
				} else {
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 or R0, R1, ..., R7 registers are allowed.\n");
						break;
				}
			}else if(isSHIFT_C(p) && isCP_C(m1)){
				if(isReg24S(p, p->Operand[1])){
					/* type 14d */
					p->InstType = t14d;
					/* SHIFT.C || CP.C */
					/* SHIFT.C ACC64S, XOP24S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) || CP.C DREG24, DREG24 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC64S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT.C instruction, "
							"only ACC0, ACC2 registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of CP.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[1])){
						printRunTimeError(p->LineCntr, m1->Operand[1], 
							"For source of CP.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
				} else if(isACC64S(p, p->Operand[1])){
					/* type 14l */
					p->InstType = t14l;
					/* SHIFT.C || CP.C */
					/* SHIFT.C ACC64S, ACC64S, IMM_INT5 ( |NORND, RND| ) || CP.C DREG24, DREG24 */

					if(!(p->Operand[3])){
						printRunTimeError(p->LineCntr, p->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(p->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC64S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of SHIFT.C instruction, "
							"only ACC0, ACC2 registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of CP.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}
					if(!isDReg24(p, m1->Operand[1])){
						printRunTimeError(p->LineCntr, m1->Operand[1], 
							"For source of CP.C, only R0, R2, ..., R30,\n"
							"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
						break;
					}

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
				} else {
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of SHIFT instruction, "
							"only ACC0, ACC2, ACC4, ACC6 or R0, R2, R4, R6 registers are allowed.\n");
						break;
				}
			}else if(isALU(p) && isMAC(m1)){
				/* type 43a */
				p->InstType = t43a;
				/* ALU || MAC */

				if(!isDReg12S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU instruction, "
						"only R0~R7, ACC0.L~ACC7.L registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
				}
				if(!isACC32S(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of this MAC instruction, "
						"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, m1->Operand[1])){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!m1->Operand[3]){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
			}else if(isALU_C(p) && isMAC_C(m1)){
				/* type 43b */
				//p->InstType = t43b;
				/* ALU.C || MAC.C */
				/* this multifunction format is no longer supported: 2008.12.5 */
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
				break;
			}else if(isALU(p) && isSHIFT(m1)){
				if(isReg12S(p, m1->Operand[1])){
					/* type 44b */
					p->InstType = t44b;
					/* ALU || SHIFT */
					/* ALU DREG12S, XOP12S, YOP12S || SHIFT ACC32S, XOP12S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) */

					if(!(m1->Operand[3])){
						printRunTimeError(p->LineCntr, m1->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(m1->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[m1->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isDReg12S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of ALU instruction, "
							"only R0~R7, ACC0.L~ACC7.L registers are allowed.\n");
						break;
					}
					if(!isReg12S(p, p->Operand[1])){
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
					if(p->Operand[2]){
						if(!isReg12S(p, p->Operand[2])){
							printRunTimeError(p->LineCntr, p->Operand[2], 
								"For source of ALU instruction, "
								"only R0, R1, ..., R7 registers are allowed.\n");
							break;
						}
					}
					if(!isACC32S(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
				} else if(isACC32S(p, m1->Operand[1])){
					/* type 44d */
					p->InstType = t44d;
					/* ALU || SHIFT */
					/* ALU DREG12S, XOP12S, YOP12S || SHIFT ACC32S, ACC32S, IMM_INT5 ( |NORND, RND| ) */

					if(!(m1->Operand[3])){
						printRunTimeError(p->LineCntr, m1->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(m1->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[m1->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isDReg12S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of ALU instruction, "
							"only R0~R7, ACC0.L~ACC7.L registers are allowed.\n");
						break;
					}
					if(!isReg12S(p, p->Operand[1])){
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
					if(p->Operand[2]){
						if(!isReg12S(p, p->Operand[2])){
							printRunTimeError(p->LineCntr, p->Operand[2], 
								"For source of ALU instruction, "
								"only R0, R1, ..., R7 registers are allowed.\n");
							break;
						}
					}
					if(!isACC32S(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
				} else {		/* if NOT XOP12S or ACC32S */
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For 1st source of SHIFT instruction, "
						"only R0, R1, ..., R7 or ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
			}else if(isMAC(p) && isSHIFT(m1)){
				if(isReg12S(p, m1->Operand[1])){
					/* type 45b */
					p->InstType = t45b;
					/* MAC || SHIFT */
					/* MAC ACC32S, XOP12S, YOP12S [( |RND, SS, SU, US, UU| )] || SHIFT ACC32S, XOP12S, IMM_INT5 ( |HI, LO, HIRND, LORND| ) */

					if(!(m1->Operand[3])){
						printRunTimeError(p->LineCntr, m1->Operand[3], 
							"For the SHIFT instruction, "
							"one of HI, LO, HIRND, LORND options must be specified.\n");
						break;
					}

					if(!isInt(m1->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[m1->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of ALU instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(isNONE(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of Multifunction instruction, "
							"NONE is not allowed.\n");
						break;
					}
					if(!isReg12S(p, p->Operand[1])){
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
					if(!isACC32S(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
				} else if(isACC32S(p, m1->Operand[1])){
					/* type 45d */
					p->InstType = t45d;
					/* MAC || SHIFT */
					/* MAC ACC32S, XOP12S, YOP12S [( |RND, SS, SU, US, UU| )] || SHIFT ACC32S, ACC32S, IMM_INT5 ( |NORND, RND| ) */

					if(!(m1->Operand[3])){
						printRunTimeError(p->LineCntr, m1->Operand[3], 
							"For the SHIFT instruction, "
							"one of NORND, RND options must be specified.\n");
						break;
					}

					if(!isInt(m1->Operand[2])){
						printRunTimeError(p->LineCntr, (char *)sOp[m1->Index], 
							"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
						break;
					}

					if(!isACC32S(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of ALU instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}
					if(isNONE(p, p->Operand[0])){
						printRunTimeError(p->LineCntr, p->Operand[0], 
							"For destination of Multifunction instruction, "
							"NONE is not allowed.\n");
						break;
					}
					if(!isReg12S(p, p->Operand[1])){
						printRunTimeError(p->LineCntr, p->Operand[1], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
					if(!isACC32S(p, m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"For destination of SHIFT instruction, "
							"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
						break;
					}

					if(!strcasecmp(p->Operand[0], m1->Operand[0])){
						printRunTimeError(p->LineCntr, m1->Operand[0], 
							"Destination registers should not be identical.\n");
						break;
					}
				} else {		/* if NOT XOP12S or ACC32S */
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"For 1st source of SHIFT instruction, "
						"only R0, R1, ..., R7 or ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
			}
			break;
		case	2:			/* three-opcode multifunction */
			m1 = p->Multi[0];
			m2 = p->Multi[1];

			if(isALU(p) && isLD(m1) && isLD(m2)){
				/* type 1c */
				p->InstType = t01c;
				/* ALU || LD || LD */

				if(!isDReg12S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of ALU instruction, "
						"only R0~R7 and ACC0.L~ACC7.L registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of ALU instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(p->Operand[2]){
					if(!isReg12S(p, p->Operand[2])){
						printRunTimeError(p->LineCntr, p->Operand[2], 
							"For source of ALU instruction, "
							"only R0, R1, ..., R7 registers are allowed.\n");
						break;
					}
				}
				if(!isDReg12(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}
				if(!isDReg12(p, m2->Operand[0])){
					printRunTimeError(p->LineCntr, m2->Operand[0], 
						"For destination of LD, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}

				if(!isIx(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IX operand of LD, only I0, I1, I2, I3 registers are allowed.\n");
					break;
				}
				if(!isMx(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MX operand of LD, only M0, M1, M2, M3 registers are allowed.\n");
					break;
				}
				if(!isIy(p, m2->Operand[2])){
					printRunTimeError(p->LineCntr, m2->Operand[2], 
						"For IY operand of LD, only I4, I5, I6, I7 registers are allowed.\n");
					break;
				}
				if(!isMy(p, m2->Operand[3])){
					printRunTimeError(p->LineCntr, m2->Operand[3], 
						"For MY operand of LD, only M4, M5, M6, M7 registers are allowed.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
				if(!strcasecmp(p->Operand[0], m2->Operand[0])){
					printRunTimeError(p->LineCntr, m2->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
				if(!strcasecmp(m1->Operand[0], m2->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}

				/* now supports only postmodify */
				if(!strcmp(m1->Operand[1], "+")){
					printRunTimeError(p->LineCntr, m1->Operand[1], 
						"Premodify addressing is not supported in this instruction type.\n");
					break;
				}
				if(!strcmp(m2->Operand[1], "+")){
					printRunTimeError(p->LineCntr, m2->Operand[1], 
						"Premodify addressing is not supported in this instruction type.\n");
					break;
				}

			}else if(isALU_C(p) && isLD_C(m1) && isLD_C(m2)){
				/* type 1d */
				//p->InstType = t01d;
				/* ALU.C || LD.C || LD.C */
				/* this multifunction format is no longer supported: 2008.12.5 */
				printRunTimeError(p->LineCntr, (char *)sOp[p->Index], 
					"Sorry, this multifuction format is no longer supported. Please check instruction syntax.\n");
				break;
			}else if(isMAC(p) && isLD(m1) && isLD(m2)){
				/* type 1a */
				p->InstType = t01a;
				/* MAC || LD || LD */

				if(!isACC32S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC instruction, "
						"only ACC0, ACC1, ACC2, ACC3 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!isReg12S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC instruction, "
						"only R0, R1, ..., R7 registers are allowed.\n");
					break;
				}
				if(!p->Operand[3]){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
					break;
				}
				if(!isDReg12(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}
				if(!isDReg12(p, m2->Operand[0])){
					printRunTimeError(p->LineCntr, m2->Operand[0], 
						"For destination of LD, only R0~R31, "
						"ACC0.H/M/L~ACC7.H/M/L registers are allowed.\n");
					break;
				}

				if(!isIx(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IX operand of LD, only I0, I1, I2, I3 registers are allowed.\n");
					break;
				}
				if(!isMx(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MX operand of LD, only M0, M1, M2, M3 registers are allowed.\n");
					break;
				}
				if(!isIy(p, m2->Operand[2])){
					printRunTimeError(p->LineCntr, m2->Operand[2], 
						"For IY operand of LD, only I4, I5, I6, I7 registers are allowed.\n");
					break;
				}
				if(!isMy(p, m2->Operand[3])){
					printRunTimeError(p->LineCntr, m2->Operand[3], 
						"For MY operand of LD, only M4, M5, M6, M7 registers are allowed.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
				if(!strcasecmp(p->Operand[0], m2->Operand[0])){
					printRunTimeError(p->LineCntr, m2->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
				if(!strcasecmp(m1->Operand[0], m2->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
			}else if(isMAC_C(p) && isLD_C(m1) && isLD_C(m2)){
				/* type 1b */
				p->InstType = t01b;
				/* MAC.C || LD.C || LD.C */

				if(!isACC64S(p, p->Operand[0])){
					printRunTimeError(p->LineCntr, p->Operand[0], 
						"For destination of this MAC.C instruction, "
						"only ACC0, ACC2 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[1])){
					printRunTimeError(p->LineCntr, p->Operand[1], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!isReg24S(p, p->Operand[2])){
					printRunTimeError(p->LineCntr, p->Operand[2], 
						"For source of MAC.C instruction, "
						"only R0, R2, R4, R6 registers are allowed.\n");
					break;
				}
				if(!p->Operand[3]){
					printRunTimeError(p->LineCntr, "(RND), (SS), (SU), (US), (UU)", 
						"One of these multiplier options must be given.\n");
					break;
				}
				if(!isDReg24(p, m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"For destination of LD.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}
				if(!isDReg24(p, m2->Operand[0])){
					printRunTimeError(p->LineCntr, m2->Operand[0], 
						"For destination of LD.C, only R0, R2, ..., R30,\n"
						"ACC[0/2/4/6].[H/M/L] registers are allowed.\n");
					break;
				}

				if(!isIx(p, m1->Operand[2])){
					printRunTimeError(p->LineCntr, m1->Operand[2], 
						"For IX operand of LD.C, only I0, I1, I2, I3 registers are allowed.\n");
					break;
				}
				if(!isMx(p, m1->Operand[3])){
					printRunTimeError(p->LineCntr, m1->Operand[3], 
						"For MX operand of LD.C, only M0, M1, M2, M3 registers are allowed.\n");
					break;
				}
				if(!isIy(p, m2->Operand[2])){
					printRunTimeError(p->LineCntr, m2->Operand[2], 
						"For IY operand of LD.C, only I4, I5, I6, I7 registers are allowed.\n");
					break;
				}
				if(!isMy(p, m2->Operand[3])){
					printRunTimeError(p->LineCntr, m2->Operand[3], 
						"For MY operand of LD.C, only M4, M5, M6, M7 registers are allowed.\n");
					break;
				}

				if(!strcasecmp(p->Operand[0], m1->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
				if(!strcasecmp(p->Operand[0], m2->Operand[0])){
					printRunTimeError(p->LineCntr, m2->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
				if(!strcasecmp(m1->Operand[0], m2->Operand[0])){
					printRunTimeError(p->LineCntr, m1->Operand[0], 
						"Destination registers should not be identical.\n");
					break;
				}
			}
			break;
		default:
			break;
	}
	return p;
}

