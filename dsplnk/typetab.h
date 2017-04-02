/*
All Rights Reserved.
*/
/** 
* @file typetab.h
* Header for instruction type definition
* @date 2008-12-23
*/

#ifndef	_TYPETAB_H
#define	_TYPETAB_H

#include "dspdef.h"

/** 
* Instruction type - Each instruction can have different type even 
* with the same opcode.
*/
/*
enum eType {
    t01a = 1, t01b, t01c, 
    t03a, t03c, t03d, t03e, t03f, t03g, t03h, t03i,
    t04a, t04b, t04c, t04d, t04e, t04f, t04g, t04h,
    t06a, t06b, t06c, t06d,
    t08a, t08b, t08c, t08d,
    t09c, t09d, t09e, t09f, t09g, t09h, t09i,
    t10a, t10b, 
    t11a, 
    t12e, t12f, t12g, t12h, t12m, t12n, t12o, t12p,
    t14c, t14d, t14k, t14l, 
    t15a, t15b, t15c, t15d, t15e, t15f,
    t16a, t16b, t16c, t16d, t16e, t16f,
    t17a, t17b, t17c, t17d, t17e, t17f, t17g, t17h,
    t18a, 
    t19a, 
    t20a, 
    t22a, 
    t23a, 
    t25a, t25b, 
    t26a, 
    t29a, t29b, t29c, t29d,
    t30a, 
    t31a, 
    t32a, t32b, t32c, t32d,
    t37a, 
    t40e, t40f, t40g, 
    t41a, t41b, t41c, t41d, 
    t42a, t42b, 
    t43a, 
    t44b, t44d,
    t45b, t45d,
    t46a, t46b,
    eTypeEnd,
};
*/

extern const char *sType[];			/**< Instruction type mnemonics */

int getType(char *s); 

#endif	/* _TYPETAB_H */
