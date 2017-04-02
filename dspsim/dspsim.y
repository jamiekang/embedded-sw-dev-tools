%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dspsim.h"
#include "symtab.h"
#include "icode.h"
#include "optab.h"
#include "simsupport.h"
#include "dspdef.h"

%}

%union {
    struct sTab *symp;	/* symbol table pointer */
	int	ival;			/* integer value */
	int subtok;			/* keyword */
	int	op;				/* opcode */
	char *tstr;			/* string */
}
%token <symp> LABEL
%token <tstr> COMMENT
%token <symp> OFFSET
%token <op> OPCODE
%token <ival> DEC_NUMBER
%token <ival> BIN_NUMBER
%token <ival> HEX_NUMBER
%left '-' '+'
%left '*' '/'
%nonassoc UMINUS

	/* literal keyword tokens */
	/* AMF, AF, MF */
%token	<tstr> FRND FSS FSU FUS FUU FC1 FC2

	/* COND */
%token	<subtok> CIF 
%token	<tstr> CEQC CEQ CNEC CNE 
%token	<tstr> CGT CLE CLT CGE 
%token	<tstr> CAVC CNOT_AVC CAV CNOT_AV 
%token	<tstr> CACC CNOT_ACC CAC CNOT_AC 
%token	<tstr> CSVC CNOT_SVC CSV CNOT_SV 
%token	<tstr> CMVC CNOT_MVC CMV CNOT_MV 
%token	<tstr> CNOT_CE CTRUE
%token	<tstr> CUMC CNOT_UMC 

	/* CONJ modifier */
%token	<subtok> MCONJ

	/* SF */	
%token	<tstr> FHI FLO FHIRND FLORND FNORND FHIX

	/* Registers */
%token	<tstr> RCNTR RLPEVER RCACTL RLPSTACK RPCSTACK
%token	<tstr> RASTATR RASTATI RASTATC RMSTAT RSSTAT RPX
%token	<tstr> RICNTL RIMASK RIRPTL
%token  <tstr> RIVEC0 RIVEC1 RIVEC2 RIVEC3
%token  <tstr> RDSTAT0 RDSTAT1 RUMCOUNT RID
%token	<tstr> RIX RMX RLX RBX RRX RACC

	/* MSTAT Fields: TBD */
%token	<tstr> MSEC_REG MBIT_REV MAV_LATCH MAL_SAT MM_MODE MTIMER MSEC_DAG MM_BIAS MINT

	/* other keywords */
%token	<tstr> KNONE
%token	<subtok> KSEPARATOR
%token	<subtok> KPM KDM
%token	<tstr> KUNTIL KCE KFOREVER
%token	<tstr> KPC KLOOP KSTS

%type <ival> constant
%type <ival> complex_constant
%type <ival> expression
%type <ival> number
%type <tstr> registers
%type <op> opcode
%type <symp> operands
%type <symp> operand
%type <tstr> term
%type <tstr> do_until
%type <tstr> keywords
%type <tstr> instruction
%type <tstr> instruction_cond
%type <tstr> options
%type <tstr> moptions
%type <symp> offset
%type <ival> addr
%type <tstr> addr_expr
%type <tstr> addr_expr_reg
%type <tstr> addr_expr_no_reg
%type <tstr> addr_op
%type <tstr> ifcond
%type <tstr> ccode
%type <ival> conjugate

%token	<tstr> KPOST KPRE 

%%
source:	/* empty */
	|	source	sentence
			{
				lineno++;			/**< increment line counter */
				curicode = NULL;	/**< this is end of processing 1 line */
				strcpy(condbuf, "");
			}
	;

sentence:	LABEL	statement '\n'	
			{	
				if(!curicode) 	/* when statment is null(blank): iLABEL */
				{
					if(isParsingMultiFunc){
						isParsingMultiFunc = FALSE;
						curaddr++;
						$1->Addr = $1->Addr + 1;	/* update symbol table, too */
						if(VerboseMode) printf("> CURADDR4: 0x%04X -> 0x%04X\n", curaddr-1, curaddr);
					}

					curicode = sICodeListAdd(&iCode, iLABEL, curaddr, lineno);

					char linebuf2[MAX_LINEBUF];
					sprintf(linebuf2, "%s:", $1->Name);
					curicode->Line = strdup(linebuf2);
				}else{		/* when statement is not blank */
					if($1->Addr != curaddr){		/* happens just after multifunction */
						/* example:
							ADD R2, R0, R1 || CP R4, R0
						t1: SUB R3, R0, 1
						*/
						if(VerboseMode) printf("LABEL3:\t%s(0x%04X) -> (0x%04X) %p\n", 
							$1->Name, $1->Addr, curaddr-1, $1); 

						$1->Addr = curaddr-1;		/* correct label addr */
					}
				}

				if(VerboseMode) printf("> LINE2:\t%s\n", linebuf); 

				curicode->Label = $1;

				if(VerboseMode) printf("LABEL:\t%s(0x%04X) %p\n", $1->Name, $1->Addr, $1); 
				if(VerboseMode) printf("curaddr = %X\n", curaddr);

				if(VerboseMode) printf("** line: %d\n\n", lineno); 
			}
	|	statement '\n' 
			{	if(VerboseMode) printf("** line: %d\n\n", lineno);	}
	;

statement:	/* empty */
	|	instruction_cond	COMMENT	
			{
				curicode->Comment = $2;	
			}
	|	instruction_cond	
	|	COMMENT 
			{
				curicode = sICodeListAdd(&iCode, iCOMMENT, curaddr, lineno);
				curicode->Comment = $1;	
			}
	|	instruction_cond	KSEPARATOR instruction
			{
				if(VerboseMode) printf("MULTIFUNC1:\n");
			}
	|	instruction_cond	KSEPARATOR instruction COMMENT
			{
				curicode->Comment = $4;	
				if(VerboseMode) printf("MULTIFUNC3:\n");
				if(VerboseMode) printf("COMMENT4: %s at PMA %04X\n", $4, curicode->PMA);
			}
	|	instruction_cond	KSEPARATOR instruction KSEPARATOR instruction
			{
				if(VerboseMode) printf("MULTIFUNC2:\n");
			}
	|	instruction_cond	KSEPARATOR instruction KSEPARATOR instruction COMMENT
			{
				curicode->Comment = $6;	
				if(VerboseMode) printf("MULTIFUNC4:\n");
				if(VerboseMode) printf("COMMENT6: %s at PMA %04X\n", $6, curicode->PMA);
			}
	;

instruction_cond:	ifcond	instruction	{}
	;
	
instruction:	opcode	offset do_until
			{	
				addOperand($2->Name);

				if($2->Addr == UNDEFINED)
					if(VerboseMode) printf("OPCODE:\t%s %s(\?\?)\n", 
						sOp[$1], $2->Name); 
				else
					if(VerboseMode) printf("OPCODE:\t%s %s(0x%04X)\n", 
						sOp[$1], $2->Name, $2->Addr); 

				curicode->MemoryRefType = tABS;
				if(VerboseMode) printf("MemoryRefType0: ABS\n");
			}
	|	opcode	operands
			{	if(VerboseMode) printf("OPCODE:\t%s\n", sOp[$1]); }
	|	opcode
			{	if(VerboseMode) printf("OPCODE:\t%s\n", sOp[$1]); }
	;

ifcond:		/* empty */ 
			{ 
				strcpy(condbuf, "");
			}
	|	CIF	ccode
			{
				strcpy(condbuf, $2);
				if(VerboseMode) printf("CCODE: %s\n", $2);	
			}
	;

ccode:	CEQC
	|	CEQ
	|	CNEC
	|	CNE
	|	CGT
	|	CLE
	|	CLT
	|	CGE
	|	CAVC
	|	CNOT_AVC
	|	CAV
	|	CNOT_AV
	|	CACC
	|	CNOT_ACC
	|	CAC
	|	CNOT_AC
	|	CSVC
	|	CNOT_SVC
	|	CSV
	|	CNOT_SV
	|	CMVC
	|	CNOT_MVC
	|	CMV
	|	CNOT_MV
	|	CNOT_CE
	|	CTRUE
	|	CUMC
	|	CNOT_UMC
	;

do_until:	KUNTIL term
			{	
			}
	;

offset:	OFFSET
			{
				//addOperand($1->Name);
			}
	;
opcode:	OPCODE
			{	$$ = $1; }
	;

term:		/* blank: same as FOREVER */
			{	
				addOperand("FOREVER");
				if(VerboseMode) printf("TERM: FOREVER\n");
			}
	|	KCE
			{	
				addOperand($1);
				if(VerboseMode) printf("TERM: CE\n");	
			}
	|	KFOREVER
			{	
				addOperand($1);
				if(VerboseMode) printf("TERM: FOREVER\n");
			}
	;

operands:	operands ',' operand options
	|	operand options
	;

operand:	registers conjugate
			{	
				addOperand($1);
				if(VerboseMode) printf("REGISTER:\t%s\n", $1); 
			}
	|	constant
			{	
				char s[MAX_LINEBUF]; 
				sprintf(s, "%d", $1);
				addOperand(s);
				if(VerboseMode) printf("CONST: %d\n", $1); 
			}
	| 	keywords
			{
				addOperand($1);
				if(VerboseMode) printf("KEYWORDS: %s\n", $1); 
			}
	|	addr
			{
			}
	| complex_constant
			{
			}
	|	ena
			{
			}
	;

ena:	MSEC_REG
			{
				addOperand("SR");
				if(VerboseMode) printf("ENA: %s\n", $1); 
			}
	|	MBIT_REV
			{
				addOperand("BR");
				if(VerboseMode) printf("ENA: %s\n", $1); 
			}
	|	MAV_LATCH
			{
				addOperand("OL");
				if(VerboseMode) printf("ENA: %s\n", $1); 
			}
	|	MAL_SAT
			{
				addOperand("AS");
				if(VerboseMode) printf("ENA: %s\n", $1); 
			}
	|	MM_MODE
			{
				addOperand("MM");
				if(VerboseMode) printf("ENA: %s\n", $1); 
			}
	|	MTIMER
			{
				addOperand("TI");
				if(VerboseMode) printf("ENA: %s\n", $1); 
			}
	|	MSEC_DAG
			{
				addOperand("SD");
				if(VerboseMode) printf("ENA: %s\n", $1); 
			}
	|	MM_BIAS
			{
				addOperand("MB");
				if(VerboseMode) printf("ENA: %s\n", $1); 
			}
	|	MINT
			{
				addOperand("INT");
				if(VerboseMode) printf("ENA: %s\n", $1); 
			}
	;

conjugate:	{ }
	|	MCONJ
			{
				addConjugate();
				if(VerboseMode) printf("CONJUGATE: *\n"); 
			}
	;

addr:	KDM '(' addr_expr ')'	
			{ 
				addOperand("DM");
			}
	|	KPM '(' addr_expr ')'	
			{ 
				addOperand("PM");
			}
	|	addr_expr_no_reg
			{
			}
	;

addr_expr:	addr_expr_reg
			{
			}
	|		addr_expr_no_reg
			{
			}
	|		constant
			{
				char s[MAX_LINEBUF]; 
				sprintf(s, "%d", $1);
				addOperand(s);
				if(VerboseMode) printf("CONST: %d\n", $1); 

				curicode->MemoryRefType = tABS;
				if(VerboseMode) printf("MemoryRefType1: ABS\n");
			}
	;

addr_expr_reg: 	RIX	addr_op RMX
			{	
				addOperand($1);
				if(VerboseMode) printf("DAG: %s\n", $1); 
				addOperand($3);
				if(VerboseMode) printf("DAG: %s\n", $3); 

				curicode->MemoryRefType = tABS;
				if(VerboseMode) printf("MemoryRefType2: ABS\n");
			}
	|	RIX	addr_op constant
			{	
				char s[MAX_LINEBUF]; 
				addOperand($1);
				if(VerboseMode) printf("DAG: %s\n", $1); 
				sprintf(s, "%d", $3);
				addOperand(s);
				if(VerboseMode) printf("CONST: %d\n", $3); 

				curicode->MemoryRefType = tABS;
				if(VerboseMode) printf("MemoryRefType3: ABS\n");
			}
	|	RIX 
			{	
				addOperand($1);
				if(VerboseMode) printf("DAG: %s\n", $1); 

				curicode->MemoryRefType = tABS;
				if(VerboseMode) printf("MemoryRefType4: ABS\n");
			}

addr_expr_no_reg:	offset KPRE constant
			{
				char s[MAX_LINEBUF]; 

				int t = addOperand($1->Name);
				if(VerboseMode) printf("VAR: %s\n", $1->Name); 

				addOperand($2);

				sprintf(s, "%d", $3);
				addOperand(s);
				if(VerboseMode) printf("CONST: %d\n", $3); 

				curicode->MemoryRefType = tREL;
				if(VerboseMode) printf("MemoryRefType5: REL\n");
			}
	|	offset '[' constant ']'
			{
				/* this syntax is only for .VAR */
				char s[MAX_LINEBUF]; 

				int t = addOperand($1->Name);
				if(VerboseMode) printf("VAR_ARRAY: %s\n", $1->Name); 

				addOperand("[");

				sprintf(s, "%d", $3);
				addOperand(s);
				if(VerboseMode) printf("CONST: %d\n", $3); 

				curicode->MemoryRefType = tREL;
				if(VerboseMode) printf("MemoryRefType6: REL\n");
			}
	|	offset
			{	
				int t = addOperand($1->Name);

				curicode->MemoryRefType = tREL;
				if(VerboseMode) printf("MemoryRefType7: REL\n");
			}
	;

keywords:	KPC
	|	KLOOP
	|	KSTS
	;

addr_op:	KPOST	
			{	addOperand($1); }
	|	KPRE	
			{	addOperand($1); }
	;

options:	/* empty */	{ $$ = NULL; }
	|	'(' moptions ')'	
			{ 
				addOperand($2);
				if(VerboseMode) printf("Option: %s\n", $2); 
				$$ = $2; 
			}
	;

complex_constant:	'(' constant ',' constant ')'
			{
				char sr[MAX_LINEBUF]; 
				char si[MAX_LINEBUF]; 
				sprintf(sr, "%d", $2);
				addOperand(sr);
				sprintf(si, "%d", $4);
				addOperand(si);
				if(VerboseMode) printf("COMPLEX_CONST: (%d, %d)\n", $2, $4); 
			}

registers:	RIX
	|	RMX
	|	RLX
	|	RBX
	|	RRX
	|	RACC
	|	RCNTR	
	|	RLPEVER
	|	RCACTL
	|	RLPSTACK
	|	RPCSTACK
	|	RASTATR
	|	RASTATI
	|	RASTATC
	|	RMSTAT
	|	RSSTAT
	|	RPX
	|	RICNTL
	|	RIMASK
	|	RIRPTL
	|	RIVEC0
	|	RIVEC1
	|	RIVEC2
	|	RIVEC3
	|	RDSTAT0
	|	RDSTAT1
	|	RUMCOUNT
	|	RID
	|	KNONE	
	;
			
constant:	expression	
			{
			}
	;

moptions:	FRND
	|	FSS
	|	FSU
	|	FUS
	|	FUU
	|	FHI
	|	FLO
	|	FHIRND
	|	FLORND
	|	FNORND
	|	FHIX
			{
				$$ = $1;
			}
	;

expression:		'-' expression %prec UMINUS	{ $$ = -$2; }
	|	'(' expression ')'	{ $$ = $2; }
	|	number	{ $$ = $1; }
	;

number: DEC_NUMBER
	|	BIN_NUMBER
	|	HEX_NUMBER
	;
%%

int addOperand(char *s)
{
	int t = curicode->OperandCounter++;

	if(t > MAX_OPERAND){	/* too many operands!! */
		printRunTimeError(curicode->LineCntr, s,
			"Too many operands! Please check instruction syntax.\n");
		exit(1);
	}

	curicode->Operand[t] = strdup(s);
	if(VerboseMode) printf("Operand[%d]:\t%s\n", t, curicode->Operand[t]);

	return t;
}

void addConjugate(void)
{
	curicode->Conj = TRUE;
}
