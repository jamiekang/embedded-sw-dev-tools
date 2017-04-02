%{
#include "y.tab.h"
#include <stdio.h>	/* perror() */
#include <stdlib.h>	/* strtol(), exit() */
#include <string.h>	/* strcpy() */
#include "dspsim.h"
#include "symtab.h"	
#include "icode.h"	
#include "optab.h"
#include "secinfo.h"
#include "dspdef.h"

extern char linebuf[MAX_LINEBUF];
extern int lineno;
extern unsigned int curaddr;
extern sICode *curicode;
extern char	condbuf[MAX_CONDBUF];

%}

%%
	/* literal keyword tokens */
	/* AMF, AF, MF */
("RND"|"rnd")	{	
				yylval.tstr = strdup(yytext);
				return FRND;	
			}

("SS"|"ss")		{	
				yylval.tstr = strdup(yytext);
				return FSS;	
			}

("SU"|"su")		{	
				yylval.tstr = strdup(yytext);
				return FSU;	
			}

("US"|"us")		{	
				yylval.tstr = strdup(yytext);
				return FUS;	
			}

("UU"|"uu")		{	
				yylval.tstr = strdup(yytext);
				return FUU;	
			}

("C1"|"c1")	{	
				yylval.tstr = strdup(yytext);
				return FC1;	}

("C2"|"c2")	{	
				yylval.tstr = strdup(yytext);
				return FC2;	}

	/* COND */
("IF"|"if")	{	return CIF;	}

("EQ.C"|"eq.c")	{	
				yylval.tstr = strdup(yytext);
				return CEQC;	}

("EQ"|"eq")	{	
				yylval.tstr = strdup(yytext);
				return CEQ;	}

("NE.C"|"ne.c")	{	
				yylval.tstr = strdup(yytext);
				return CNEC;	}

("NE"|"ne")	{	
				yylval.tstr = strdup(yytext);
				return CNE;	}

("GT"|"gt")	{	
				yylval.tstr = strdup(yytext);
				return CGT;	}

("LE"|"le")	{	
				yylval.tstr = strdup(yytext);
				return CLE;	}

("LT"|"lt")	{	
				yylval.tstr = strdup(yytext);
				return CLT;	}

("GE"|"ge")	{	
				yylval.tstr = strdup(yytext);
				return CGE;	}

("AV.C"|"av.c")	{		
				yylval.tstr = strdup(yytext);
				return CAVC;	}

("NOT AV.C"|"not av.c")	{	
				yylval.tstr = strdup(yytext);
				return CNOT_AVC;	}

("AV"|"av")	{	
				yylval.tstr = strdup(yytext);
				return CAV;	}

("NOT AV"|"not av")	{	
				yylval.tstr = strdup(yytext);
				return CNOT_AV;	}

("AC.C"|"ac.c")	{	
				yylval.tstr = strdup(yytext);
				return CACC;	}

("NOT AC.C"|"not ac.c")	{	
				yylval.tstr = strdup(yytext);
				return CNOT_ACC;	}

("AC"|"ac")	{	
				yylval.tstr = strdup(yytext);
				return CAC;	}

("NOT AC"|"not ac")	{	
				yylval.tstr = strdup(yytext);
				return CNOT_AC;	}

("SV.C"|"sv.c")	{	
				yylval.tstr = strdup(yytext);
				return CSVC;	}

("NOT SV.C"|"not sv.c")	{	
				yylval.tstr = strdup(yytext);
				return CNOT_SVC;	}

("SV"|"sv")	{	
				yylval.tstr = strdup(yytext);
				return CSV;	}

("NOT SV"|"not sv")	{	
				yylval.tstr = strdup(yytext);
				return CNOT_SV;	}

("MV.C"|"mv.c")	{	
				yylval.tstr = strdup(yytext);
				return CMVC;	}

("NOT MV.C"|"not mv.c")	{	
				yylval.tstr = strdup(yytext);
				return CNOT_MVC;	}

("MV"|"mv")	{	
				yylval.tstr = strdup(yytext);
				return CMV;	}

("NOT MV"|"not mv")	{	
				yylval.tstr = strdup(yytext);
				return CNOT_MV;	}

("NOT CE"|"not ce")	{	
				yylval.tstr = strdup(yytext);
				return CNOT_CE;	}

("TRUE"|"true")	{	
				yylval.tstr = strdup(yytext);
				return CTRUE;	}

("UM.C"|"um.c")	{		
				yylval.tstr = strdup(yytext);
				return CUMC;	}

("NOT UM.C"|"not um.c")	{	
				yylval.tstr = strdup(yytext);
				return CNOT_UMC;	}

	/* PUSH/POP stack */
("PC"|"pc")	{	
				yylval.tstr = strdup(yytext);
				return KPC;	
			}

("LOOP"|"loop")	{	
				yylval.tstr = strdup(yytext);
				return KLOOP;	
			}

("STS"|"sts") {	
				yylval.tstr = strdup(yytext);
				return KSTS;	
			}

	/* CONJ modifier */
"*"	{	return MCONJ;	}

	/* SF */
("HI"|"hi")	{	
				yylval.tstr = strdup(yytext);
				return FHI;	}

("LO"|"lo")	{	
				yylval.tstr = strdup(yytext);
				return FLO;	}

("HIRND"|"hirnd")	{	
				yylval.tstr = strdup(yytext);
				return FHIRND;	}

("LORND"|"lornd")	{	
				yylval.tstr = strdup(yytext);
				return FLORND;	}

("NORND"|"nornd")	{	
				yylval.tstr = strdup(yytext);
				return FNORND;	
			}

("HIX"|"hix")	{	
				yylval.tstr = strdup(yytext);
				return FHIX;	}

	/* Registers */
("_CNTR"|"_cntr")	{	
				yylval.tstr = strdup(yytext);
				return RCNTR;	}

("_LPEVER"|"_lpever")	{	
				yylval.tstr = strdup(yytext);
				return RLPEVER;	}

("_LPSTACK"|"_lpstack")	{	
				yylval.tstr = strdup(yytext);
				return RLPSTACK;	}

("_PCSTACK"|"_PCstack")	{	
				yylval.tstr = strdup(yytext);
				return RPCSTACK;	}

("ASTAT.R"|"astat.r")	{	
				yylval.tstr = strdup(yytext);
				return RASTATR;	}
("ASTAT.I"|"astat.i")	{	
				yylval.tstr = strdup(yytext);
				return RASTATI;	}
("ASTAT.C"|"astat.c")	{	
				yylval.tstr = strdup(yytext);
				return RASTATC;	}

("_MSTAT"|"_mstat")	{	
				yylval.tstr = strdup(yytext);
				return RMSTAT;	}

("_SSTAT"|"_sstat")	{	
				yylval.tstr = strdup(yytext);
				return RSSTAT;	}

("_DSTAT0"|"_dstat0")	{	
				yylval.tstr = strdup(yytext);
				return RDSTAT0;	}

("_DSTAT1"|"_dstat1")	{	
				yylval.tstr = strdup(yytext);
				return RDSTAT1;	}

("_ICNTL"|"_icntl")	{	
				yylval.tstr = strdup(yytext);
				return RICNTL;	}

("_IMASK"|"_imask")	{	
				yylval.tstr = strdup(yytext);
				return RIMASK;	}

("_IRPTL"|"_irptl")	{	
				yylval.tstr = strdup(yytext);
				return RIRPTL;	}

("_IVEC0"|"_ivec0")	{	
				yylval.tstr = strdup(yytext);
				return RIVEC0;	}

("_IVEC1"|"_ivec1")	{	
				yylval.tstr = strdup(yytext);
				return RIVEC1;	}

("_IVEC2"|"_ivec2")	{	
				yylval.tstr = strdup(yytext);
				return RIVEC2;	}

("_IVEC3"|"_ivec3")	{	
				yylval.tstr = strdup(yytext);
				return RIVEC3;	}

("UMCOUNT"|"umcount")	{	
				yylval.tstr = strdup(yytext);
				return RUMCOUNT;	}

("ID"|"id")	{	
				yylval.tstr = strdup(yytext);
				return RID;	}

("_CACTL"|"_cactl")	{	
				yylval.tstr = strdup(yytext);
				return RCACTL;	}

("_PX"|"_px")	{	
				yylval.tstr = strdup(yytext);
				return RPX;	}

(I|i)[0-9]	{		/* Ix registers */
		yylval.tstr = strdup(yytext);
		return RIX;
	}

(M|m)[0-9]	{		/* Mx registers */
		yylval.tstr = strdup(yytext);
		return RMX;
	}

(L|l)[0-9]	{		/* Lx registers */
		yylval.tstr = strdup(yytext);
		return RLX;
	}

(B|b)[0-9]	{		/* Bx registers */
		yylval.tstr = strdup(yytext);
		return RBX;
	}

(R|r)[0-9][0-9]?	{		/* Rx registers */
		yylval.tstr = strdup(yytext);
		return RRX;
	}

("ACC"|"acc")[0-9](".H"|".h"|".M"|".m"|".L"|".l")?	{		/* ACCx registers */
		yylval.tstr = strdup(yytext);
		return RACC;
	}

	/* MSTAT Fields: TBD */

("SEC_REG"|"sec_reg")	{	
		yylval.tstr = strdup(yytext);
		return MSEC_REG;	
	}

("SR"|"sr")	{	
		yylval.tstr = strdup(yytext);
		return MSEC_REG;
	}

("BIT_REV"|"bit_rev")	{	
		yylval.tstr = strdup(yytext);
		return MBIT_REV;	
	}

("BR"|"br")	{	
		yylval.tstr = strdup(yytext);
		return MBIT_REV;	
	}

("AV_LATCH"|"av_latch")	{	
		yylval.tstr = strdup(yytext);
		return MAV_LATCH;	
	}

("OL"|"ol")	{	
		yylval.tstr = strdup(yytext);
		return MAV_LATCH;	
	}

("AL_SAT"|"al_sat")	{	
		yylval.tstr = strdup(yytext);
		return MAL_SAT;	
	}

("AS"|"as")	{	
		yylval.tstr = strdup(yytext);
		return MAL_SAT;	
	}

("M_MODE"|"m_mode")	{	
		yylval.tstr = strdup(yytext);
		return MM_MODE;	
	}

("MM"|"mm")	{	
		yylval.tstr = strdup(yytext);
		return MM_MODE;	
	}

("TIMER"|"timer")	{	
		yylval.tstr = strdup(yytext);
		return MTIMER;	
	}

("TI"|"ti")	{	
		yylval.tstr = strdup(yytext);
		return MTIMER;	
	}

("SEC_DAG"|"sec_dag")	{	
		yylval.tstr = strdup(yytext);
		return MSEC_DAG;	
	}

("SD"|"sd")	{	
		yylval.tstr = strdup(yytext);
		return MSEC_DAG;	
	}

("M_BIAS"|"m_bias")	{	
		yylval.tstr = strdup(yytext);
		return MM_BIAS;	
	}

("MB"|"mb")	{	
		yylval.tstr = strdup(yytext);
		return MM_BIAS;	
	}

("INT"|"int")	{	
		yylval.tstr = strdup(yytext);
		return MINT;	
	}

	/* other keywords */
("NONE"|"none")	{	
		yylval.tstr = strdup(yytext);
		return KNONE;	
	}

"||"	{	return KSEPARATOR;	}

("PM"|"pm")	{	return KPM;	}

("DM"|"dm")	{	return KDM;	}

("UNTIL"|"until")	{	return KUNTIL;	}

("CE"|"ce")	{	
				yylval.tstr = strdup(yytext);
				return KCE;	
			}

("FOREVER"|"forever") {	
				yylval.tstr = strdup(yytext);
				return KFOREVER;
			}
	
	/* address operator */
"+="	{	
			yylval.tstr = strdup(yytext);
			return KPOST;	
		}

"+"		{	
			yylval.tstr = strdup(yytext);
			return KPRE;	
		}

	/* numbers */
[0-9]+	{
		yylval.ival = (int)strtol(yytext, NULL, 10);
		return DEC_NUMBER;
	}
	
(0x|0X)[0-9A-Fa-f]+	{
		yylval.ival = (int)strtol(&yytext[2], NULL, 16);
		return HEX_NUMBER;
	}
	
(B#|b#)[01]+	{
		yylval.ival = (int)strtol(&yytext[2], NULL, 2);
		return BIN_NUMBER;
	}

[ \t]	;	/* ignore whitespace */

[A-Za-z_][-_A-Za-z0-9]*:	{	/* return symbol pointer */
		yytext[yyleng-1] = '\0';	/* remove trailing colon */

		/* check if this keyword is a reserved one. */
		sTab *rsp = isResSym(yytext);
		if(rsp){	/* if reserved, mark as error */
			yyerror("Reserved keyword used");
			AssemblerError++;
			if(VerboseMode) printf("> RESSYM0: %s (0x%04X)\n", yytext, curaddr);
		}

		sTab *sp0 = sTabHashSearch(symTable, yytext);
		/* check if this label is new */
		if(sp0 && sp0->Defined){	/* not new: duplicated */
			yyerror("Duplicated LABEL defined");
			AssemblerError++;
		}

		sTab *sp = symlook(yytext, curaddr);
		if(VerboseMode) printf("> symlook: %s (0x%04X)\n", yytext, curaddr);
		sp->Defined = TRUE;
		yylval.symp = sp;
		if(VerboseMode) printf("> LABEL0: %p (0x%04X)\n", sp, curaddr);
		return LABEL;
	}

[A-Za-z\.\/_]([-_A-Za-z0-9]*)|([-_A-Za-z0-9]+\.([Cc]|[Rr][Cc]))	{	/* should return opcode */
		unsigned int code = isOpcode(yytext);
		if(VerboseMode) printf("yytext:\t%s\n", yytext);
		if(VerboseMode) printf("code:\t%d\n", code);
		if(code != FALSE){		/* opcode */
			yylval.op = code;
			if(!curicode){ 
				if(isParsingMultiFunc){
					isParsingMultiFunc = FALSE;
					curaddr++;
					if(VerboseMode) printf("> CURADDR0: 0x%04X -> 0x%04X\n", curaddr-1, curaddr);
				}

				/* if .CODE, reset curaddr */
				if(code == i_CODE) curaddr = codeSegAddr;

				curicode = sICodeListAdd(&iCode, code, curaddr, lineno);
				if(VerboseMode) printf("curaddr = %X\n", curaddr);

				if(!isPseudoInst(code) || code == i_VAR){
					curaddr++;	/**< increment Address Counter */
					if(VerboseMode) printf("> CURADDR1: 0x%04X -> 0x%04X\n", curaddr-1, curaddr);
				}

				curicode->Line = strdup(linebuf);
				if(VerboseMode) printf("> LINE1:\t%s\n", linebuf);
			}else{
				isParsingMultiFunc = TRUE;

				sICode *oldicode = curicode;	/* save pointer to first 
													instruction of multifunction */

				if(VerboseMode) printf("> CURADDR2: 0x%04X -> 0x%04X\n", curaddr, oldicode->PMA);

				curaddr = oldicode->PMA;
				curicode = sICodeListMultiAdd(&iCode, code, curaddr, lineno);
				if(VerboseMode) printf("NOTE: curicode not NULL at addr %X, line %d\n", curaddr, lineno);
			}
			if(condbuf[0] != '\0') {
				curicode->Cond = strdup(condbuf);
				if(VerboseMode) printf("COND:\t%s\n", condbuf);
			}
			return OPCODE;
		} else {
			/* check if this keyword is a reserved one. */
			sTab *rsp = isResSym(yytext);
			if(rsp){	/* if reserved, mark as error */
				yyerror("Reserved keyword used");
				AssemblerError++;
				if(VerboseMode) printf("> RESSYM1: %s (0x%04X)\n", yytext, curaddr);
			}

			sTab *sp = sTabHashSearch(symTable, yytext);
			if(sp){	/* it's already in symbol table */
				yylval.symp = sp;
				if(VerboseMode) printf("> LABEL2: %p\n", sp);
			}else{	/* make new one */
				sp = symlook(yytext, UNDEFINED);	/* symbol offset (should be symTable) */
				yylval.symp = sp;
				if(VerboseMode) printf("> LABEL1: %p\n", sp);
			}
			return OFFSET;
		}
	}

;.*	{	/* return comment */
		yylval.tstr = strdup(yytext);
		return COMMENT;
	}

"$"	{ return 0; }

\n.*	{	
			int c = yytext[0];
			strcpy(linebuf, yytext+1);
			yyless(1);

			if(VerboseMode) printf("> %s\n", linebuf);
			return c;
	}

.	{ return yytext[0]; }
%%

int yyerror(char *s)
{
	fprintf(stderr, "%d: %s at \"%s\"\n", lineno, s, yytext);
	return (1);
}

/* look up a symbol table entry, add if not present */
struct sTab* symlook(char *s, unsigned int n)
{
    struct sTab *sp;

	return sTabHashAdd(symTable, s, n);
} /* symlook */

void addlabel(char *name, unsigned int address)
{
	struct sTab	*sp = symlook(name, address);
}

void yyerror2(char *s)
{
	fprintf(stderr,"%d: %s\n",lineno, s);
}

unsigned int isOpcode(char *s)
{
	oTab* p;
	p = oTabHashSearch(opTable, s);
	if(p == NULL) return FALSE;
	else 
		return p->Index;
}

struct sTab* isResSym(char *s)
{
	struct sTab* p;
	p = sTabHashSearch(resSymTable, s);
	if(p == NULL) return NULL;
	else 
		return p;
}
