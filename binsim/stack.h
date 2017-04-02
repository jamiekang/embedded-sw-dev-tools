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

void stackInit(sStack *s, int id);
void stackPush(sStack *s, int x);
int stackTop(sStack *s);
void stackPop(sStack *s);
int stackEmpty(sStack *s);
int stackFull(sStack *s);
int stackCheckLevel(sStack *s);
void stackPrint(sStack *s);

#endif /* _STACK_H */
