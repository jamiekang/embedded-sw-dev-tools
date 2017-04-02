/*
All Rights Reserved.
*/
/** 
* @file stack.cc
* @brief Stack structure (sStack) implementation
* @date 2008-09-29
*/

#include <stdio.h>
#include <assert.h>
#include "stack.h"
	
void stackInit(sStack *s, int id)
{
	s->StackT = -1;
	s->ID = id;
	s->Overflow = s->Underflow = 0;
}

void stackPush(sStack *s, int x)
{
	s->StackT++;
	if(s->StackT >= STACKDEPTH) { 	/* stack overflow */
		s->Overflow++;
		return;
	}
	s->Stack[s->StackT] = x;
}

int stackTop(sStack *s)
{
	if(s->StackT < 0) {		/* check not empty: stack underflow */
		s->Underflow++;
		return (-1);
	}
	return s->Stack[s->StackT];
}

void stackPop(sStack *s)
{
	if(s->StackT < 0) {		/* check not empty: stack underflow */
		s->Underflow++;
		return;
	}
	s->StackT--;
}

int stackEmpty(sStack *s)
{
	return (s->StackT < 0);
}

int stackFull(sStack *s)
{
	return (s->StackT == (STACKDEPTH-1));
}

int stackCheckLevel(sStack *s)
{
	if((s->StackT >= (STACKDEPTH-2)) || (s->StackT <= 2)) return 1;
	else return 0;
}

void stackPrint(sStack *s)
{
	int i;
	 
	if(!stackEmpty(s)){
	
		switch(s->ID){
			case iPC:
				printf("PC Stack:           ");
				//printf("Number of Stack Elements: %d\n", (s->StackT +1));
				for(i = 0; i < (s->StackT+1); i++){
					printf("%04X ", s->Stack[i]);
				}
				printf("(Top)\n");
				if(stackFull(s)) printf("Stack Full!\n");
				break;
			case iLoopBegin:
				printf("Loop Begin Stack:   ");
				//printf("Number of Stack Elements: %d\n", (s->StackT +1));
				for(i = 0; i < (s->StackT+1); i++){
					printf("%04X ", s->Stack[i]);
				}
				printf("(Top)\n");
				if(stackFull(s)) printf("Stack Full!\n");
				break;
			case iLoopEnd:
				printf("Loop End Stack:     ");
				//printf("Number of Stack Elements: %d\n", (s->StackT +1));
				for(i = 0; i < (s->StackT+1); i++){
					printf("%04X ", s->Stack[i]);
				}
				printf("(Top)\n");
				if(stackFull(s)) printf("Stack Full!\n");
				break;
			case iLoopCounter:
				printf("Loop Counter Stack: ");
				//printf("Number of Stack Elements: %d\n", (s->StackT +1));
				for(i = 0; i < (s->StackT+1); i++){
					printf("%d ", s->Stack[i]);
				}
				printf("(Top)\n");
				if(stackFull(s)) printf("Stack Full!\n");
				break;
			case iLPEVER:
				printf("LPEVER Stack:       ");
				//printf("Number of Stack Elements: %d\n", (s->StackT +1));
				for(i = 0; i < (s->StackT+1); i++){
					printf("%X ", s->Stack[i]);
				}
				printf("(Top)\n");
				if(stackFull(s)) printf("Stack Full!\n");
				break;
			case iStatus:
				printf("Status Stack:       ");
				//printf("Number of Stack Elements: %d\n", (s->StackT +1));
				for(i = 0; i < (s->StackT+1); i++){
					printf("%04X ", s->Stack[i]);
				}
				printf("(Top)\n");
				if(stackFull(s)) printf("Stack Full!\n");
				break;
			default:
				break;
		}
	}
}

