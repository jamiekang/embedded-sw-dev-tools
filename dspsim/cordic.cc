/*
All Rights Reserved.
*/
/**
* @file cordic.cc
* @brief Header for CORDIC-related functions
* @date 2008-11-24
*/

#include <stdio.h>
#include <math.h>
#include "dspsim.h"
#include "dspdef.h"
#include "cordic.h"

//CORDIC table in 0.14 format 
/*
int cordic_ctab [] = {
	0x00003243, 0x00001DAC, 0x00000FAD, 0x000007F5, 
	0x000003FE, 0x000001FF, 0x000000FF, 0x0000007F, 
	0x0000003F, 0x0000001F, 0x0000000F, 0x00000007, 
	0x00000003, 0x00000001, 0x00000000, 0x00000000, 
};
*/

//scaled CORDIC table in 0.14 format: cordic_ctab[]/M_PI
int scaled_cordic_ctab [] = {
	0x00000FFF, 0x00000972, 0x000004FD, 0x00000288,
	0x00000145, 0x000000A2, 0x00000051, 0x00000028,
	0x00000014, 0x0000000A, 0x00000005, 0x00000002,
	0x00000001, 0x00000000, 0x00000000, 0x00000000,
};

/** 
* Angle parameter z, if outside Q1 & Q4, are rotated by (+/-)1/2(PI),
* because cordicCompute() can accept only angles in [-1/2, 1/2] range,
*
* @param x Pointer to x0 (integer: 14.0 format)
* @param y Pointer to y0 (integer: 14.0 format)
* @param z Pointer to angle z0 (1.13 format) - [-1, 1]
* @param mode Rotation("iRECT_C") or Vectoring("iPOLAR_C")
* 
* @return 1 if 1/2 added, -1 if 1/2 subtracted
*/
int cordicInitRotate(int *x, int *y, int *z, int mode)
{
	int x0, y0, z0;
	int direction = 0;

	x0 = *x;
	y0 = *y;
	z0 = *z;

	switch(mode){
		case iRECT_C:
			//Rotation mode: iRECT_C: init rotate by angle
			if(z0 < (int)(SCALE13/2) && z0 > (int)(-SCALE13/2)) {	//Q1 or Q4
				direction = 0;
			} else if(z0 >= (int)(SCALE13/2)){	//Q2
				*z = z0 - (int)(SCALE13/2);
				direction = 1;
			} else {								//Q3
				*z = z0 + (int)(SCALE13/2);
				direction = -1;
			}
			break;
		case iPOLAR_C:
			//Vectoring mode: iPOLAR_C: init rotate by (x, y)
			if (x0 > 0) 	//Q1 or Q4
				direction = 0;
			else if(y0 <= 0) {		//Q3	
				//if it's in Q3, move it to Q4 by adding Pi/2
				*x = -y0;
				*y = x0;
				direction = 1;
			} else {				//Q2
				//if it's in Q2, move it to Q1 by subtracting Pi/2
				*x = y0;
				*y = -x0;
				direction = -1;
			}
			break;
		default:
			/* should not happen */
			break;
	}
	return direction;
}

/** 
* CORDIC in 14-bit (0.14 format) signed fixed point math
* angle z parameter must be in [-1/2, 1/2] range.
*
* @param x Pointer to x0 (integer: 14.0 format)
* @param y Pointer to y0 (integer: 14.0 format)
* @param z Pointer to angle z0 (0.14 format)
* @param mode Rotation("iRECT_C") or Vectoring("iPOLAR_C")
*/
void cordicCompute(int *x, int *y, int *z, int mode)
{
	int d;
	int x0, y0, z0;
	int xn, yn, zn;

	x0 = *x;
	y0 = *y;
	z0 = *z;

	for (int i = 0; i < N_ITR; i++)
	{
		/* decide direction */
		switch(mode){
			case iRECT_C:
				//Rotation mode: iRECT_C
				if(z0 < 0) 
					d = -1;
				else 
					d = 1;
				break;
			case iPOLAR_C:
				//Vectoring mode: iPOLAR_C
				if(y0 < 0) 
					d = 1;
				else 
					d = -1;
				break;
			default:
				/* should not happen */
				break;
		}

		/* compute x_i+1, y_i+1, z_i+1 */
		xn = x0 - (y0>>i)*d;
		yn = y0 + (x0>>i)*d;
		zn = z0 - (int)(scaled_cordic_ctab[i]*d);	/* 0.14 format (IWL:-1) - [-0.5,0.5] */

		x0 = xn; y0 = yn; z0 = zn;
	}  

	*x = xn;
	*y = yn;
	*z = zn;
}

/** 
* @brief Since cordicCompute() can accept only angles in [-1/2, 1/2] range,
*        angles outside Q1 & Q4 are rotated by (+/-)1/2.
* @param x Pointer to x0 (integer: 14.0 format)
* @param y Pointer to y0 (integer: 14.0 format)
* @param z Pointer to angle z0 (1.13 format) - [-1, 1]
* @param dir Direction of pre-rotation
* @param mode Rotation("iRECT_C") or Vectoring("iPOLAR_C")
*/
void cordicBackwardRotate(int *x, int *y, int *z, int dir, int mode)
{
	int x0, y0, z0;

	x0 = *x;
	y0 = *y;
	z0 = *z;

	switch(mode){
		case iRECT_C:
			//Rotation mode: iRECT_C: init rotate by angle
			if(dir == 1){				//back to Q2 by adding Pi/2
				*x = -y0;
				*y = x0;
			}else if(dir == -1){		//back to Q3 by subtracting Pi/2
				*x = y0;
				*y = -x0;
			}
			break;
		case iPOLAR_C:
			//Vectoring mode: iPOLAR_C: init rotate by (x, y)
			if(dir == 1){				//back to Q2 by subtracting Pi/2
				*z = z0 - (int)(SCALE13/2);
			}else if(dir == -1){		//back to Q3 by adding Pi/2
				*z = z0 + (int)(SCALE13/2);
			}
			break;
		default:
			/* should not happen */
			break;
	}
}

/** 
* @brief Run CORDIC in rotation mode (iRECT_C): theta -> (cos(theta), sin(theta))
* 
* @param angle Angle in normalized radian, 1.11 format fixed-point [-1, 1)
*
* @return (cos, sin) value in cplx structure
*/
cplx runCordicRotationMode(int angle)
{
	int preRotate;
	int x, y, z;		/* x, y: 12.0 format */
	cplx rect;

	switch(angle){
		case -(SCALE11):		/* angle: -1.0 */
			rect.r = 0x800; 	/* 0x800: -1 */
			rect.i = 0;  
			return rect;
		case -(SCALE11/2):		/* angle: -0.5 */
			rect.r = 0; 
			rect.i = 0x800;		/* 0x800: -1 */
			return rect;
		case 0:             	/* angle:  0.0 */
			rect.r = 0x7FF;		/* 0x7FF:  1 */
			rect.i = 0;
			return rect;
		case (SCALE11/2):		/* angle:  0.5 */
			rect.r = 0; 
			rect.i = 0x7FF;		/* 0x7FF:  1 */
			return rect;
		default:
			break;
	}       

	//init for iRECT_C
	///////////////////////////////////////////////////////////////////////////// z: 1.11 domain (IWL:0)
	x = 1 << 11;					//source operand 1: convert from 12.0 to 1.11 
	x <<= 2;						//convert to 1.13 format
	y = 0;
	z = angle; 						//source operand 2 
									//convert angle p to fixed-point 1.11 format
	z <<= 2;						//convert angle p to 1.13 format
	///////////////////////////////////////////////////////////////////////////// z: 1.13 domain (IWL:0)

	//initial rotation
	preRotate = 0;
	if(z >= (int)(SCALE13/2) || z <= -(int)(SCALE13/2)){
		/* Note: (int)(SCALE11*M_PI/2) is a precomputable integer constant */
		preRotate = cordicInitRotate(&x, &y, &z, iRECT_C);	/* z: 1.13 format (IWL:0) */
	}

	z <<= 1;	/* z: now in 0.14 format (IWL:-1): [-0.5, 0.5] */

	//call to cordicCompute()
	//should be made universal enough for both modes
	cordicCompute(&x, &y, &z, iRECT_C);

	//gain compensation
	//x, y: 14.0 format
	//An: 0.14 format
	//result: 14.14 format -> (shift right by 14 bits) -> 14.0 format
	//x = (int)((double)x*(1/An));
	//y = (int)((double)y*(1/An));
	x = (x * F_1An) >> 14;
	y = (y * F_1An) >> 14; 

	z >>= 1;	/* z: now back in 1.13 format (IWL:0): [-1, 1] */

	//rotate-back for initially-rotated points
	if(preRotate){		
		cordicBackwardRotate(&x, &y, &z, preRotate, iRECT_C);	/* z: 1.13 format (IWL:0) */
	}

	///////////////////////////////////////////////////////////////////////////// z: 1.13 domain
	x >>= 2;	//convert from 14.0 to 12.0
	y >>= 2;	//convert from 14.0 to 12.0
	//z >>= 2;	//convert from 1.13 to 1.11 

	///////////////////////////////////////////////////////////////////////////// z: 1.11 domain
	rect.r = x;
	rect.i = y;

	return rect;
}

/** 
* @brief Run CORDIC in vectoring mode (iPOLAR_C): (x, y) -> theta
* 
* @param x x in integer (12.0 format)
* @param y y in integer (12.0 format)
*
* @return angle Angle in 1.11 format fixed-point [-1, 1)
*/
int runCordicVectoringMode(int x, int y)
{
	int i;
	double p;
	int preRotate;

	int z;
	cplx polar;

	//init for iPOLAR_C
	///////////////////////////////////////////////////////////////////////////// z: 1.11 domain (IWL:0)
	x = x << 2;		//source 1: convert from 12.0 to 14.0 format
	y = y << 2;		//source 2: convert from 12.0 to 14.0 format
	z = 0;
	///////////////////////////////////////////////////////////////////////////// z: 1.13 domain (IWL:0)

	//initial rotation
	preRotate = 0;
	if(x < 0){
		preRotate = cordicInitRotate(&x, &y, &z, iPOLAR_C);
	}

	z <<= 1;	/* z: now in 0.14 format (IWL:-1): [-0.5, 0.5] */

	//call to cordicCompute()
	//should be made universal enough for both modes
	cordicCompute(&x, &y, &z, iPOLAR_C);	/* z: 0.14 format (IWL:-1) */

	z >>= 1;	/* z: now back in 1.13 format (IWL:0): [-1, 1] */

	//gain compensation
	//x, y: 14.0 format
	//An: 0.14 format
	//result: 14.14 format -> (shift right by 14 bits) -> 14.0 format
	//x = (int)((double)x*(1/An));
	//y = (int)((double)y*(1/An));
	x = (x * F_1An) >> 14;
	y = (y * F_1An) >> 14; 

	//rotate-back for initially-rotated points
	if(preRotate){		
		cordicBackwardRotate(&x, &y, &z, preRotate, iPOLAR_C);	/* z: 1.13 format (IWL:0) */
	}

	///////////////////////////////////////////////////////////////////////////// z: 1.13 domain
	//x >>= 2;	//convert from 14.0 to 12.0
	//y >>= 2;	//convert from 14.0 to 12.0
	z >>= 2;	//convert from 1.13 to 1.11 

	///////////////////////////////////////////////////////////////////////////// z: 1.11 domain

	return z;
}
