/*
All Rights Reserved.
*/
/**
* @file cordic.h
* @brief Header for CORDIC-related functions
* @date 2008-11-24
*/

#ifndef _CORDIC_H
#define _CORDIC_H

#include "optab.h"		/* for iPOLAR_C, iRECT_C */

#define	N_W		14			/* wordlength */
#define	N_ITR	(N_W+1)		/* number of iteration: wordlength+1 */
#define	N_Q		25			/* number of angle p in Q1; total number of p is 4*N_Q */

#define	PSCALEBITS	(N_W-3)
#define SCALE11		(1<<PSCALEBITS)		/* scale factor for cordicInitRotate(): 3.11 format */
//#define SCALE9		(SCALE11/4)			/* scale factor 12-bit in/out: 3.9 format */
#define SCALE14		(SCALE11*8)			/* scale factor for An, 1/An: 0.14 format */
#define SCALE13     (SCALE11*4)         /* scale factor 12-bit in/out: 1.13 format */

#define	D_An	1.646760257098622171	/* An, n = 15 */
#define	D_1An	0.607252935385913517	/* 1/An */
#define	F_An	0x00006964				/* An in 0.14 format */
#define	F_1An	0x000026DD				/* 1/An in 0.14 format */

extern int scaled_cordic_ctab[];

int cordicInitRotate(int *x, int *y, int *z, int mode);
void cordicCompute(int *x, int *y, int *z, int mode);
void cordicBackwardRotate(int *x, int *y, int *z, int dir, int mode);
double computeAn(int n);
cplx runCordicRotationMode(int angle);
int runCordicVectoringMode(int x, int y);

#endif /* _CORDIC_H */
