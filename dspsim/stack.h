/*
All Rights Reserved.
*/
/** 
* @file stack.h
* @brief Header for sStack stack implementation
* @date 2008-09-29
*/

#ifndef _STACK_H
#define _STACK_H

#define STACKDEPTH	8


#ifndef _SINT_
#define _SINT_

#define NUMDP   4   /* number of SIMD data paths */

/** definition for SIMD int data structure */
typedef struct ssint {
	int dp[NUMDP];                  /* integer value for each data path */
} sint;

#endif  /* _SINT_ */
	
/** 
* @brief Every sStack instance needs a unique ID 
*/
enum eSt {
	iPC,
	iLoopBegin,
	iLoopEnd,
	iLoopCounter,
	iLPEVER,
	iStatus,
	iAStatus,
};

/** 
* @brief Common data structure for stack variables
*/
typedef struct sStack {
	int ID;		/* stack ID */
	int Stack[STACKDEPTH];
	int StackT;
	unsigned char Overflow;
	unsigned char Underflow;
} sStack;

/** 
* @brief Common data structure for stack variables (SIMD version)
*/
typedef struct ssStack {
	int ID;		/* stack ID */
	sint Stack[STACKDEPTH];
	int StackT;
	unsigned char Overflow;
	unsigned char Underflow;
} ssStack;

void stackInit(sStack *s, int id);
void stackPush(sStack *s, int x);
int stackTop(sStack *s);
void stackPop(sStack *s);
int stackEmpty(sStack *s);
int stackFull(sStack *s);
int stackCheckLevel(sStack *s);
void stackPrint(sStack *s);

void sStackInit(ssStack *s, int id);
void sStackPush(ssStack *s, sint x);
sint sStackTop(ssStack *s);
void sStackPop(ssStack *s);
int sStackEmpty(ssStack *s);
int sStackFull(ssStack *s);
int sStackCheckLevel(ssStack *s);
void sStackPrint(ssStack *s);
#endif /* _STACK_H */
