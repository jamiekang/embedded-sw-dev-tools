#ifndef lint
static char const 
yyrcsid[] = "$FreeBSD: src/usr.bin/yacc/skeleton.c,v 1.28 2000/01/17 02:04:06 bde Exp $";
#endif
#include <stdlib.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING() (yyerrflag!=0)
static int yygrowstack();
#define YYPREFIX "yy"
#line 2 "dspsim.y"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dspsim.h"
#include "symtab.h"
#include "icode.h"
#include "optab.h"
#include "simsupport.h"
#include "dspdef.h"

#line 14 "dspsim.y"
typedef union {
    struct sTab *symp;	/* symbol table pointer */
	int	ival;			/* integer value */
	int subtok;			/* keyword */
	int	op;				/* opcode */
	char *tstr;			/* string */
} YYSTYPE;
#line 36 "y.tab.cc"
#define YYERRCODE 256
#define LABEL 257
#define COMMENT 258
#define OFFSET 259
#define OPCODE 260
#define DEC_NUMBER 261
#define BIN_NUMBER 262
#define HEX_NUMBER 263
#define UMINUS 264
#define FRND 265
#define FSS 266
#define FSU 267
#define FUS 268
#define FUU 269
#define FC1 270
#define FC2 271
#define CIF 272
#define CEQC 273
#define CEQ 274
#define CNEC 275
#define CNE 276
#define CGT 277
#define CLE 278
#define CLT 279
#define CGE 280
#define CAVC 281
#define CNOT_AVC 282
#define CAV 283
#define CNOT_AV 284
#define CACC 285
#define CNOT_ACC 286
#define CAC 287
#define CNOT_AC 288
#define CSVC 289
#define CNOT_SVC 290
#define CSV 291
#define CNOT_SV 292
#define CMVC 293
#define CNOT_MVC 294
#define CMV 295
#define CNOT_MV 296
#define CNOT_CE 297
#define CTRUE 298
#define CUMC 299
#define CNOT_UMC 300
#define MCONJ 301
#define FHI 302
#define FLO 303
#define FHIRND 304
#define FLORND 305
#define FNORND 306
#define FHIX 307
#define RCNTR 308
#define RLPEVER 309
#define RCACTL 310
#define RLPSTACK 311
#define RPCSTACK 312
#define RASTATR 313
#define RASTATI 314
#define RASTATC 315
#define RMSTAT 316
#define RSSTAT 317
#define RPX 318
#define RICNTL 319
#define RIMASK 320
#define RIRPTL 321
#define RIVEC0 322
#define RIVEC1 323
#define RIVEC2 324
#define RIVEC3 325
#define RDSTAT0 326
#define RDSTAT1 327
#define RUMCOUNT 328
#define RID 329
#define RIX 330
#define RMX 331
#define RLX 332
#define RBX 333
#define RRX 334
#define RACC 335
#define MSEC_REG 336
#define MBIT_REV 337
#define MAV_LATCH 338
#define MAL_SAT 339
#define MM_MODE 340
#define MTIMER 341
#define MSEC_DAG 342
#define MM_BIAS 343
#define MINT 344
#define KNONE 345
#define KSEPARATOR 346
#define KPM 347
#define KDM 348
#define KUNTIL 349
#define KCE 350
#define KFOREVER 351
#define KPC 352
#define KLOOP 353
#define KSTS 354
#define KPOST 355
#define KPRE 356
const short yylhs[] = {                                        -1,
    0,    0,   25,   25,   26,   26,   26,   26,   26,   26,
   26,   26,   13,   12,   12,   12,   22,   22,   23,   23,
   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
   23,   23,   23,   23,   23,   23,   10,   16,    6,    9,
    9,    9,    7,    7,    8,    8,    8,    8,    8,    8,
   27,   27,   27,   27,   27,   27,   27,   27,   27,   24,
   24,   17,   17,   17,   18,   18,   18,   19,   19,   19,
   20,   20,   20,   11,   11,   11,   21,   21,   14,   14,
    2,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    1,   15,   15,   15,   15,   15,   15,   15,   15,   15,
   15,   15,    3,    3,    3,    4,    4,    4,
};
const short yylen[] = {                                         2,
    0,    2,    3,    2,    0,    2,    1,    1,    3,    4,
    5,    6,    2,    3,    2,    1,    0,    2,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    2,    1,    1,    0,
    1,    1,    4,    2,    2,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    0,
    1,    4,    4,    1,    1,    1,    1,    3,    3,    1,
    3,    4,    1,    1,    1,    1,    1,    1,    0,    3,
    5,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    2,    3,    1,    1,    1,    1,
};
const short yydefred[] = {                                      1,
    0,    0,    8,    0,    0,    0,    2,    0,    0,   19,
   20,   21,   22,   23,   24,   25,   26,   27,   28,   29,
   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,
   40,   41,   42,   43,   44,   45,   46,   18,    6,    0,
   49,    0,   13,    4,    3,    0,   48,  136,  137,  138,
    0,   98,   99,  100,  101,  102,  103,  104,  105,  106,
  107,  108,  109,  110,  111,  112,  113,  114,  115,  116,
  117,  118,  119,   92,   93,   94,   95,   96,   97,   61,
   62,   63,   64,   65,   66,   67,   68,   69,  120,    0,
    0,   84,   85,   86,    0,   56,   59,  121,  135,    0,
    0,    0,   57,    0,   58,   74,   60,   10,    0,    0,
  133,    0,    0,    0,    0,   71,   55,    0,    0,   54,
    0,    0,    0,   14,    0,    0,    0,   77,    0,    0,
   75,   76,    0,    0,  134,    0,  122,  123,  124,  125,
  126,  127,  128,  129,  130,  131,  132,    0,   51,   52,
   47,   81,    0,   12,   87,   88,    0,   73,   72,    0,
   53,   90,   82,   78,   79,   91,
};
const short yydgoto[] = {                                       1,
   96,   97,   98,   99,  100,   42,  101,  102,  151,  124,
  103,   43,    5,  120,  148,  129,  105,  130,  131,  106,
  157,    6,   38,  117,    7,    8,  107,
};
const short yysindex[] = {                                      0,
 -232, -214,    0, -170, -247, -237,    0,   29,   35,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -237,
    0,  -33,    0,    0,    0, -244,    0,    0,    0,    0,
  -30,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    6,
    7,    0,    0,    0,  -30,    0,    0,    0,    0, -253,
    5,   10,    0,  -83,    0,    0,    0,    0, -237,  -30,
    0,   -2,   -2,   15,   19,    0,    0,  -33, -212,    0,
 -332,  -30,  -30,    0, -196,   19, -335,    0,  -82,   23,
    0,    0,   24,  -30,    0,   10,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   25,    0,    0,
    0,    0,  -26,    0,    0,    0,  -18,    0,    0,   27,
    0,    0,    0,    0,    0,    0,
};
const short yyrindex[] = {                                      0,
    3,    3,    0,    0,   59,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   -6,    0,    0,    0,   60,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   -8,
   -5,   -9,    0,   -7,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   28,    0,    0,    0,    0,    0,
   -4,    0,    0,    0,   61,    0,   32,    0,  -10,    0,
    0,    0,    0,    0,    0,   -9,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,
};
const short yygindex[] = {                                      0,
  -71,    0,  -34,    0,    0,    0,    0,  -44,    0,    0,
    0,  -24,    0,  -61,    0,   36,    0,  -36,    0,  -84,
    0,    0,    0,    0,    0,   77,    0,
};
#define YYTABLESIZE 342
const short yytable[] = {                                      83,
   89,   70,   83,   16,   15,   50,   95,  123,  123,  110,
   39,   51,    5,  108,   51,   46,  111,  149,  150,  155,
  156,  110,   41,  114,    2,    3,   51,  132,  132,   83,
   83,   70,   83,   83,   89,   70,   83,  110,   44,    4,
  128,  128,   51,    3,   45,  112,  113,  116,  118,  119,
  152,  153,  137,  138,  139,  140,  141,    4,  134,  135,
  115,  154,  160,  158,  159,  162,  163,  166,    7,    9,
   11,  121,   80,  136,  161,  126,  133,  104,    9,    0,
    0,    0,    0,    0,  125,  165,    0,    0,    0,  142,
  143,  144,  145,  146,  147,    0,    0,    0,   40,    0,
    0,  109,   10,   11,   12,   13,   14,   15,   16,   17,
   18,   19,   20,   21,   22,   23,   24,   25,   26,   27,
   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   47,    0,   48,   49,   50,
   48,   49,   50,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   48,   49,   50,    0,    0,   83,   89,   70,
   83,   16,   15,   50,    0,    0,   47,    0,   48,   49,
   50,    0,   17,    0,    0,  121,    0,    0,    0,    0,
    0,    0,  122,  122,   52,   53,   54,   55,   56,   57,
   58,   59,   60,   61,   62,   63,   64,   65,   66,   67,
   68,   69,   70,   71,   72,   73,   74,   75,   76,   77,
   78,   79,   80,   81,   82,   83,   84,   85,   86,   87,
   88,   89,  164,   90,   91,    0,    0,    0,   92,   93,
   94,    0,    0,    0,    0,    0,    0,  127,    0,    0,
    0,    0,    0,    0,    0,   83,   89,   70,   83,   16,
   15,   50,
};
const short yycheck[] = {                                      10,
   10,   10,   10,   10,   10,   10,   40,   91,   91,   40,
  258,   45,   10,  258,   45,   40,   51,  350,  351,  355,
  356,   40,  260,   95,  257,  258,   45,  112,  113,   40,
   41,   40,   40,   44,   44,   44,   44,   40,   10,  272,
  112,  113,   45,  258,   10,   40,   40,  301,   44,   40,
  122,  123,  265,  266,  267,  268,  269,  272,   44,   41,
   95,  258,  134,   41,   41,   41,   93,   41,   10,   10,
   10,   44,   41,  118,  136,  110,  113,   42,    2,   -1,
   -1,   -1,   -1,   -1,  109,  157,   -1,   -1,   -1,  302,
  303,  304,  305,  306,  307,   -1,   -1,   -1,  346,   -1,
   -1,  346,  273,  274,  275,  276,  277,  278,  279,  280,
  281,  282,  283,  284,  285,  286,  287,  288,  289,  290,
  291,  292,  293,  294,  295,  296,  297,  298,  299,  300,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  259,   -1,  261,  262,  263,
  261,  262,  263,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  261,  262,  263,   -1,   -1,  258,  258,  258,
  258,  258,  258,  258,   -1,   -1,  259,   -1,  261,  262,
  263,   -1,  260,   -1,   -1,  349,   -1,   -1,   -1,   -1,
   -1,   -1,  356,  356,  308,  309,  310,  311,  312,  313,
  314,  315,  316,  317,  318,  319,  320,  321,  322,  323,
  324,  325,  326,  327,  328,  329,  330,  331,  332,  333,
  334,  335,  336,  337,  338,  339,  340,  341,  342,  343,
  344,  345,  331,  347,  348,   -1,   -1,   -1,  352,  353,
  354,   -1,   -1,   -1,   -1,   -1,   -1,  330,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  346,  346,  346,  346,  346,
  346,  346,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 356
#if YYDEBUG
const char * const yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,"'\\n'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,"'('","')'","'*'","'+'","','","'-'",0,"'/'",0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'['",0,
"']'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"LABEL","COMMENT","OFFSET","OPCODE","DEC_NUMBER","BIN_NUMBER",
"HEX_NUMBER","UMINUS","FRND","FSS","FSU","FUS","FUU","FC1","FC2","CIF","CEQC",
"CEQ","CNEC","CNE","CGT","CLE","CLT","CGE","CAVC","CNOT_AVC","CAV","CNOT_AV",
"CACC","CNOT_ACC","CAC","CNOT_AC","CSVC","CNOT_SVC","CSV","CNOT_SV","CMVC",
"CNOT_MVC","CMV","CNOT_MV","CNOT_CE","CTRUE","CUMC","CNOT_UMC","MCONJ","FHI",
"FLO","FHIRND","FLORND","FNORND","FHIX","RCNTR","RLPEVER","RCACTL","RLPSTACK",
"RPCSTACK","RASTATR","RASTATI","RASTATC","RMSTAT","RSSTAT","RPX","RICNTL",
"RIMASK","RIRPTL","RIVEC0","RIVEC1","RIVEC2","RIVEC3","RDSTAT0","RDSTAT1",
"RUMCOUNT","RID","RIX","RMX","RLX","RBX","RRX","RACC","MSEC_REG","MBIT_REV",
"MAV_LATCH","MAL_SAT","MM_MODE","MTIMER","MSEC_DAG","MM_BIAS","MINT","KNONE",
"KSEPARATOR","KPM","KDM","KUNTIL","KCE","KFOREVER","KPC","KLOOP","KSTS","KPOST",
"KPRE",
};
const char * const yyrule[] = {
"$accept : source",
"source :",
"source : source sentence",
"sentence : LABEL statement '\\n'",
"sentence : statement '\\n'",
"statement :",
"statement : instruction_cond COMMENT",
"statement : instruction_cond",
"statement : COMMENT",
"statement : instruction_cond KSEPARATOR instruction",
"statement : instruction_cond KSEPARATOR instruction COMMENT",
"statement : instruction_cond KSEPARATOR instruction KSEPARATOR instruction",
"statement : instruction_cond KSEPARATOR instruction KSEPARATOR instruction COMMENT",
"instruction_cond : ifcond instruction",
"instruction : opcode offset do_until",
"instruction : opcode operands",
"instruction : opcode",
"ifcond :",
"ifcond : CIF ccode",
"ccode : CEQC",
"ccode : CEQ",
"ccode : CNEC",
"ccode : CNE",
"ccode : CGT",
"ccode : CLE",
"ccode : CLT",
"ccode : CGE",
"ccode : CAVC",
"ccode : CNOT_AVC",
"ccode : CAV",
"ccode : CNOT_AV",
"ccode : CACC",
"ccode : CNOT_ACC",
"ccode : CAC",
"ccode : CNOT_AC",
"ccode : CSVC",
"ccode : CNOT_SVC",
"ccode : CSV",
"ccode : CNOT_SV",
"ccode : CMVC",
"ccode : CNOT_MVC",
"ccode : CMV",
"ccode : CNOT_MV",
"ccode : CNOT_CE",
"ccode : CTRUE",
"ccode : CUMC",
"ccode : CNOT_UMC",
"do_until : KUNTIL term",
"offset : OFFSET",
"opcode : OPCODE",
"term :",
"term : KCE",
"term : KFOREVER",
"operands : operands ',' operand options",
"operands : operand options",
"operand : registers conjugate",
"operand : constant",
"operand : keywords",
"operand : addr",
"operand : complex_constant",
"operand : ena",
"ena : MSEC_REG",
"ena : MBIT_REV",
"ena : MAV_LATCH",
"ena : MAL_SAT",
"ena : MM_MODE",
"ena : MTIMER",
"ena : MSEC_DAG",
"ena : MM_BIAS",
"ena : MINT",
"conjugate :",
"conjugate : MCONJ",
"addr : KDM '(' addr_expr ')'",
"addr : KPM '(' addr_expr ')'",
"addr : addr_expr_no_reg",
"addr_expr : addr_expr_reg",
"addr_expr : addr_expr_no_reg",
"addr_expr : constant",
"addr_expr_reg : RIX addr_op RMX",
"addr_expr_reg : RIX addr_op constant",
"addr_expr_reg : RIX",
"addr_expr_no_reg : offset KPRE constant",
"addr_expr_no_reg : offset '[' constant ']'",
"addr_expr_no_reg : offset",
"keywords : KPC",
"keywords : KLOOP",
"keywords : KSTS",
"addr_op : KPOST",
"addr_op : KPRE",
"options :",
"options : '(' moptions ')'",
"complex_constant : '(' constant ',' constant ')'",
"registers : RIX",
"registers : RMX",
"registers : RLX",
"registers : RBX",
"registers : RRX",
"registers : RACC",
"registers : RCNTR",
"registers : RLPEVER",
"registers : RCACTL",
"registers : RLPSTACK",
"registers : RPCSTACK",
"registers : RASTATR",
"registers : RASTATI",
"registers : RASTATC",
"registers : RMSTAT",
"registers : RSSTAT",
"registers : RPX",
"registers : RICNTL",
"registers : RIMASK",
"registers : RIRPTL",
"registers : RIVEC0",
"registers : RIVEC1",
"registers : RIVEC2",
"registers : RIVEC3",
"registers : RDSTAT0",
"registers : RDSTAT1",
"registers : RUMCOUNT",
"registers : RID",
"registers : KNONE",
"constant : expression",
"moptions : FRND",
"moptions : FSS",
"moptions : FSU",
"moptions : FUS",
"moptions : FUU",
"moptions : FHI",
"moptions : FLO",
"moptions : FHIRND",
"moptions : FLORND",
"moptions : FNORND",
"moptions : FHIX",
"expression : '-' expression",
"expression : '(' expression ')'",
"expression : number",
"number : DEC_NUMBER",
"number : BIN_NUMBER",
"number : HEX_NUMBER",
};
#endif
#if YYDEBUG
#include <stdio.h>
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH 10000
#endif
#endif
#define YYINITSTACKSIZE 200
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short *yyss;
short *yysslim;
YYSTYPE *yyvs;
int yystacksize;
#line 562 "dspsim.y"

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
#line 530 "y.tab.cc"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack()
{
    int newsize, i;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;
    i = yyssp - yyss;
    newss = yyss ? (short *)realloc(yyss, newsize * sizeof *newss) :
      (short *)malloc(newsize * sizeof *newss);
    if (newss == NULL)
        return -1;
    yyss = newss;
    yyssp = newss + i;
    newvs = yyvs ? (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs) :
      (YYSTYPE *)malloc(newsize * sizeof *newvs);
    if (newvs == NULL)
        return -1;
    yyvs = newvs;
    yyvsp = newvs + i;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
}

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab

#ifndef YYPARSE_PARAM
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG void
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif	/* ANSI-C/C++ */
#else	/* YYPARSE_PARAM */
#ifndef YYPARSE_PARAM_TYPE
#define YYPARSE_PARAM_TYPE void *
#endif
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG YYPARSE_PARAM_TYPE YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL YYPARSE_PARAM_TYPE YYPARSE_PARAM;
#endif	/* ANSI-C/C++ */
#endif	/* ! YYPARSE_PARAM */

int
yyparse (YYPARSE_PARAM_ARG)
    YYPARSE_PARAM_DECL
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register const char *yys;

    if ((yys = getenv("YYDEBUG")))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate])) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yysslim && yygrowstack())
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#if defined(lint) || defined(__GNUC__)
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#if defined(lint) || defined(__GNUC__)
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yysslim && yygrowstack())
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 2:
#line 101 "dspsim.y"
{
				lineno++;			/**< increment line counter */
				curicode = NULL;	/**< this is end of processing 1 line */
				strcpy(condbuf, "");
			}
break;
case 3:
#line 109 "dspsim.y"
{	
				if(!curicode) 	/* when statment is null(blank): iLABEL */
				{
					if(isParsingMultiFunc){
						isParsingMultiFunc = FALSE;
						curaddr++;
						yyvsp[-2].symp->Addr = yyvsp[-2].symp->Addr + 1;	/* update symbol table, too */
						if(VerboseMode) printf("> CURADDR4: 0x%04X -> 0x%04X\n", curaddr-1, curaddr);
					}

					curicode = sICodeListAdd(&iCode, iLABEL, curaddr, lineno);

					char linebuf2[MAX_LINEBUF];
					sprintf(linebuf2, "%s:", yyvsp[-2].symp->Name);
					curicode->Line = strdup(linebuf2);
				}else{		/* when statement is not blank */
					if(yyvsp[-2].symp->Addr != curaddr){		/* happens just after multifunction */
						/* example:
							ADD R2, R0, R1 || CP R4, R0
						t1: SUB R3, R0, 1
						*/
						if(VerboseMode) printf("LABEL3:\t%s(0x%04X) -> (0x%04X) %p\n", 
							yyvsp[-2].symp->Name, yyvsp[-2].symp->Addr, curaddr-1, yyvsp[-2].symp); 

						yyvsp[-2].symp->Addr = curaddr-1;		/* correct label addr */
					}
				}

				if(VerboseMode) printf("> LINE2:\t%s\n", linebuf); 

				curicode->Label = yyvsp[-2].symp;

				if(VerboseMode) printf("LABEL:\t%s(0x%04X) %p\n", yyvsp[-2].symp->Name, yyvsp[-2].symp->Addr, yyvsp[-2].symp); 
				if(VerboseMode) printf("curaddr = %X\n", curaddr);

				if(VerboseMode) printf("** line: %d\n\n", lineno); 
			}
break;
case 4:
#line 147 "dspsim.y"
{	if(VerboseMode) printf("** line: %d\n\n", lineno);	}
break;
case 6:
#line 152 "dspsim.y"
{
				curicode->Comment = yyvsp[0].tstr;	
			}
break;
case 8:
#line 157 "dspsim.y"
{
				curicode = sICodeListAdd(&iCode, iCOMMENT, curaddr, lineno);
				curicode->Comment = yyvsp[0].tstr;	
			}
break;
case 9:
#line 162 "dspsim.y"
{
				if(VerboseMode) printf("MULTIFUNC1:\n");
			}
break;
case 10:
#line 166 "dspsim.y"
{
				curicode->Comment = yyvsp[0].tstr;	
				if(VerboseMode) printf("MULTIFUNC3:\n");
				if(VerboseMode) printf("COMMENT4: %s at PMA %04X\n", yyvsp[0].tstr, curicode->PMA);
			}
break;
case 11:
#line 172 "dspsim.y"
{
				if(VerboseMode) printf("MULTIFUNC2:\n");
			}
break;
case 12:
#line 176 "dspsim.y"
{
				curicode->Comment = yyvsp[0].tstr;	
				if(VerboseMode) printf("MULTIFUNC4:\n");
				if(VerboseMode) printf("COMMENT6: %s at PMA %04X\n", yyvsp[0].tstr, curicode->PMA);
			}
break;
case 13:
#line 183 "dspsim.y"
{}
break;
case 14:
#line 187 "dspsim.y"
{	
				addOperand(yyvsp[-1].symp->Name);

				if(yyvsp[-1].symp->Addr == UNDEFINED)
					if(VerboseMode) printf("OPCODE:\t%s %s(\?\?)\n", 
						sOp[yyvsp[-2].op], yyvsp[-1].symp->Name); 
				else
					if(VerboseMode) printf("OPCODE:\t%s %s(0x%04X)\n", 
						sOp[yyvsp[-2].op], yyvsp[-1].symp->Name, yyvsp[-1].symp->Addr); 

				curicode->MemoryRefType = tABS;
				if(VerboseMode) printf("MemoryRefType0: ABS\n");
			}
break;
case 15:
#line 201 "dspsim.y"
{	if(VerboseMode) printf("OPCODE:\t%s\n", sOp[yyvsp[-1].op]); }
break;
case 16:
#line 203 "dspsim.y"
{	if(VerboseMode) printf("OPCODE:\t%s\n", sOp[yyvsp[0].op]); }
break;
case 17:
#line 207 "dspsim.y"
{ 
				strcpy(condbuf, "");
			}
break;
case 18:
#line 211 "dspsim.y"
{
				strcpy(condbuf, yyvsp[0].tstr);
				if(VerboseMode) printf("CCODE: %s\n", yyvsp[0].tstr);	
			}
break;
case 47:
#line 248 "dspsim.y"
{	
			}
break;
case 48:
#line 253 "dspsim.y"
{
				/*addOperand($1->Name);*/
			}
break;
case 49:
#line 258 "dspsim.y"
{	yyval.op = yyvsp[0].op; }
break;
case 50:
#line 262 "dspsim.y"
{	
				addOperand("FOREVER");
				if(VerboseMode) printf("TERM: FOREVER\n");
			}
break;
case 51:
#line 267 "dspsim.y"
{	
				addOperand(yyvsp[0].tstr);
				if(VerboseMode) printf("TERM: CE\n");	
			}
break;
case 52:
#line 272 "dspsim.y"
{	
				addOperand(yyvsp[0].tstr);
				if(VerboseMode) printf("TERM: FOREVER\n");
			}
break;
case 55:
#line 283 "dspsim.y"
{	
				addOperand(yyvsp[-1].tstr);
				if(VerboseMode) printf("REGISTER:\t%s\n", yyvsp[-1].tstr); 
			}
break;
case 56:
#line 288 "dspsim.y"
{	
				char s[MAX_LINEBUF]; 
				sprintf(s, "%d", yyvsp[0].ival);
				addOperand(s);
				if(VerboseMode) printf("CONST: %d\n", yyvsp[0].ival); 
			}
break;
case 57:
#line 295 "dspsim.y"
{
				addOperand(yyvsp[0].tstr);
				if(VerboseMode) printf("KEYWORDS: %s\n", yyvsp[0].tstr); 
			}
break;
case 58:
#line 300 "dspsim.y"
{
			}
break;
case 59:
#line 303 "dspsim.y"
{
			}
break;
case 60:
#line 306 "dspsim.y"
{
			}
break;
case 61:
#line 311 "dspsim.y"
{
				addOperand("SR");
				if(VerboseMode) printf("ENA: %s\n", yyvsp[0].tstr); 
			}
break;
case 62:
#line 316 "dspsim.y"
{
				addOperand("BR");
				if(VerboseMode) printf("ENA: %s\n", yyvsp[0].tstr); 
			}
break;
case 63:
#line 321 "dspsim.y"
{
				addOperand("OL");
				if(VerboseMode) printf("ENA: %s\n", yyvsp[0].tstr); 
			}
break;
case 64:
#line 326 "dspsim.y"
{
				addOperand("AS");
				if(VerboseMode) printf("ENA: %s\n", yyvsp[0].tstr); 
			}
break;
case 65:
#line 331 "dspsim.y"
{
				addOperand("MM");
				if(VerboseMode) printf("ENA: %s\n", yyvsp[0].tstr); 
			}
break;
case 66:
#line 336 "dspsim.y"
{
				addOperand("TI");
				if(VerboseMode) printf("ENA: %s\n", yyvsp[0].tstr); 
			}
break;
case 67:
#line 341 "dspsim.y"
{
				addOperand("SD");
				if(VerboseMode) printf("ENA: %s\n", yyvsp[0].tstr); 
			}
break;
case 68:
#line 346 "dspsim.y"
{
				addOperand("MB");
				if(VerboseMode) printf("ENA: %s\n", yyvsp[0].tstr); 
			}
break;
case 69:
#line 351 "dspsim.y"
{
				addOperand("INT");
				if(VerboseMode) printf("ENA: %s\n", yyvsp[0].tstr); 
			}
break;
case 70:
#line 357 "dspsim.y"
{ }
break;
case 71:
#line 359 "dspsim.y"
{
				addConjugate();
				if(VerboseMode) printf("CONJUGATE: *\n"); 
			}
break;
case 72:
#line 366 "dspsim.y"
{ 
				addOperand("DM");
			}
break;
case 73:
#line 370 "dspsim.y"
{ 
				addOperand("PM");
			}
break;
case 74:
#line 374 "dspsim.y"
{
			}
break;
case 75:
#line 379 "dspsim.y"
{
			}
break;
case 76:
#line 382 "dspsim.y"
{
			}
break;
case 77:
#line 385 "dspsim.y"
{
				char s[MAX_LINEBUF]; 
				sprintf(s, "%d", yyvsp[0].ival);
				addOperand(s);
				if(VerboseMode) printf("CONST: %d\n", yyvsp[0].ival); 

				curicode->MemoryRefType = tABS;
				if(VerboseMode) printf("MemoryRefType1: ABS\n");
			}
break;
case 78:
#line 397 "dspsim.y"
{	
				addOperand(yyvsp[-2].tstr);
				if(VerboseMode) printf("DAG: %s\n", yyvsp[-2].tstr); 
				addOperand(yyvsp[0].tstr);
				if(VerboseMode) printf("DAG: %s\n", yyvsp[0].tstr); 

				curicode->MemoryRefType = tABS;
				if(VerboseMode) printf("MemoryRefType2: ABS\n");
			}
break;
case 79:
#line 407 "dspsim.y"
{	
				char s[MAX_LINEBUF]; 
				addOperand(yyvsp[-2].tstr);
				if(VerboseMode) printf("DAG: %s\n", yyvsp[-2].tstr); 
				sprintf(s, "%d", yyvsp[0].ival);
				addOperand(s);
				if(VerboseMode) printf("CONST: %d\n", yyvsp[0].ival); 

				curicode->MemoryRefType = tABS;
				if(VerboseMode) printf("MemoryRefType3: ABS\n");
			}
break;
case 80:
#line 419 "dspsim.y"
{	
				addOperand(yyvsp[0].tstr);
				if(VerboseMode) printf("DAG: %s\n", yyvsp[0].tstr); 

				curicode->MemoryRefType = tABS;
				if(VerboseMode) printf("MemoryRefType4: ABS\n");
			}
break;
case 81:
#line 428 "dspsim.y"
{
				char s[MAX_LINEBUF]; 

				int t = addOperand(yyvsp[-2].symp->Name);
				if(VerboseMode) printf("VAR: %s\n", yyvsp[-2].symp->Name); 

				addOperand(yyvsp[-1].tstr);

				sprintf(s, "%d", yyvsp[0].ival);
				addOperand(s);
				if(VerboseMode) printf("CONST: %d\n", yyvsp[0].ival); 

				curicode->MemoryRefType = tREL;
				if(VerboseMode) printf("MemoryRefType5: REL\n");
			}
break;
case 82:
#line 444 "dspsim.y"
{
				/* this syntax is only for .VAR */
				char s[MAX_LINEBUF]; 

				int t = addOperand(yyvsp[-3].symp->Name);
				if(VerboseMode) printf("VAR_ARRAY: %s\n", yyvsp[-3].symp->Name); 

				addOperand("[");

				sprintf(s, "%d", yyvsp[-1].ival);
				addOperand(s);
				if(VerboseMode) printf("CONST: %d\n", yyvsp[-1].ival); 

				curicode->MemoryRefType = tREL;
				if(VerboseMode) printf("MemoryRefType6: REL\n");
			}
break;
case 83:
#line 461 "dspsim.y"
{	
				int t = addOperand(yyvsp[0].symp->Name);

				curicode->MemoryRefType = tREL;
				if(VerboseMode) printf("MemoryRefType7: REL\n");
			}
break;
case 87:
#line 475 "dspsim.y"
{	addOperand(yyvsp[0].tstr); }
break;
case 88:
#line 477 "dspsim.y"
{	addOperand(yyvsp[0].tstr); }
break;
case 89:
#line 480 "dspsim.y"
{ yyval.tstr = NULL; }
break;
case 90:
#line 482 "dspsim.y"
{ 
				addOperand(yyvsp[-1].tstr);
				if(VerboseMode) printf("Option: %s\n", yyvsp[-1].tstr); 
				yyval.tstr = yyvsp[-1].tstr; 
			}
break;
case 91:
#line 490 "dspsim.y"
{
				char sr[MAX_LINEBUF]; 
				char si[MAX_LINEBUF]; 
				sprintf(sr, "%d", yyvsp[-3].ival);
				addOperand(sr);
				sprintf(si, "%d", yyvsp[-1].ival);
				addOperand(si);
				if(VerboseMode) printf("COMPLEX_CONST: (%d, %d)\n", yyvsp[-3].ival, yyvsp[-1].ival); 
			}
break;
case 121:
#line 532 "dspsim.y"
{
			}
break;
case 132:
#line 547 "dspsim.y"
{
				yyval.tstr = yyvsp[0].tstr;
			}
break;
case 133:
#line 552 "dspsim.y"
{ yyval.ival = -yyvsp[0].ival; }
break;
case 134:
#line 553 "dspsim.y"
{ yyval.ival = yyvsp[-1].ival; }
break;
case 135:
#line 554 "dspsim.y"
{ yyval.ival = yyvsp[0].ival; }
break;
#line 1183 "y.tab.cc"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yysslim && yygrowstack())
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
