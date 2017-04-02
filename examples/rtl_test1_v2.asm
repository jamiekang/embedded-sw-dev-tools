;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 
; OFDM DSP RTL Test Program 
; - Ver 0.22 - this is for dspsim ver 1.6+
; 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.DATA
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.CODE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Type 17g
	LD	I0, 0x1357
	CP	M0, I0
	LD	_IVEC0, 0x0080
	LD	_IVEC1, 0x1080
	CP	_IVEC2, _IVEC0
	CP	_IVEC3, _IVEC1
; Type 17h
	LD	R0,	0x678
	LD	R1, 0x878
	CP	ACC0, R0
	CP	ACC1, R1
; Type 37a
	SETINT	15
	SETINT	1
	CLRINT	1
	CLRINT	15
;
	LD	_CNTR, 30	
	LD	R0, 0x234
	LD	R2, 0xCBA
	LD	ACC0, 0
; Type 11a
	DO	loop_end0	UNTIL CE
	MAC	 ACC0, R0, R2 (SS)
loop_end0:	
	NOP
; end of Type 11a testing
	LD	R3, 0xFFF
	LD  R4, 0
	ASHIFT ACC0, R3, R4 (LO)
	LSHIFT ACC0, R3, R4 (LO)
; Type 10b
	CALL	call1
; Type 30a
	NOP
; Type 10b
	JUMP	jump1
call1:
	LD  R5, -4
	LD  ACC2, 0xFFFFFF
; Type 20a
	RTS
jump1:
	ASHIFT ACC0, R3, R5 (LO)
	LSHIFT ACC0, R3, R5 (LO)
	ASHIFT ACC0, ACC2, -4 (NORND)
	LSHIFT ACC0, ACC2, -4 (NORND)
;
	LD			I0, 0x100
	LD			M0, 0x1	
	LD			I4, 0x200
	LD			M4, 0x2
	ST			DM(I0 += M0), 0x00F
	ST			DM(I0 += M0), 0x01F
	ST			DM(I0 += M0), 0x2FC
	ST			DM(I0 += M0), 0x333
	ST			DM(I0 += M0), 0x444
	ST			DM(I0 += M0), 0x555
	ST			DM(I0 += M0), 0x666
	ST			DM(I0 += M0), 0x777
	ST			DM(I0 += M0), 0x00F
	ST			DM(I0 += M0), 0x01F
	ST			DM(I0 += M0), 0x2FC
	ST			DM(I0 += M0), 0x333
	ST			DM(I0 += M0), 0x444
	ST			DM(I0 += M0), 0x555
	ST			DM(I0 += M0), 0x666
	ST			DM(I0 += M0), 0x777
	ST			DM(I4 += M4), 0x000
	ST			DM(I4 += M4), 0x999
	ST			DM(I4 += M4), 0xaaa
	ST			DM(I4 += M4), 0xbbb
	ST			DM(I4 += M4), 0xccc
	ST			DM(I4 += M4), 0xddd
	ST			DM(I4 += M4), 0xeee
	ST			DM(I4 += M4), 0xfff
	ST			DM(I4 += M4), 0x000
	ST			DM(I4 += M4), 0x999
	ST			DM(I4 += M4), 0xaaa
	ST			DM(I4 += M4), 0xbbb
	ST			DM(I4 += M4), 0xccc
	ST			DM(I4 += M4), 0xddd
	ST			DM(I4 += M4), 0xeee
	ST			DM(I4 += M4), 0xfff
	LD			I0, 0x100
	LD			M0, 0x1	
	LD			I4, 0x200
	LD			M4, 0x2
	LD			R0, 0x111
	LD			R2, 0xFFF
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Type 1a
	MPY			ACC0, R0, R2 (SS)	||	LD	 	R0, DM(I0 += M0)	|| LD		R2, DM(I4 += M4)
; Type 1c
	SUB			R6, R0, R2 		||	LD	 	R0, DM(I0 += M0)	|| LD		R2, DM(I4 += M4)	; multifunction
; 
	LD	I0, 0x100
	LD	M0, 2
	NOP
;
; Type 1b
	MAS.C		ACC0, R0, R2 (SS)	||	LD.C 	R0, DM(I0 += M0)	|| LD.C		R2, DM(I4 += M4)
; Type 1a
	LD 			R0, DM(I0 += M0)	||	LD		R2, DM(I4 += M4)	;; multifunction
; Type 1b
	LD.C 		R0, DM(I0 += M0)	||	LD.C	R2, DM(I4 += M4)
; Type 4a
	MAC			ACC0, R0, R2 (SS)	||	LD		R0, DM(I0 += M0)
; Type 4c
	ADD	 		R6, R0, R2 		||	LD		R0, DM(I0 += M0)
; Type 4b
	MPY.C		ACC0, R0, R2 (SS)	||	LD.C	R0, DM(I0 += M0)
; Type 4d
	SUB.C		R6, R0, R2 		||	LD.C	R0, DM(I0 += M0)
; Type 12e
	LSHIFT 		ACC0, R0, 3 (LO)	||	LD		R0, DM(I0 += M0)
; Type 12f
	ASHIFT.C	ACC0, R0, -5 (LO)	||	LD.C	R0, DM(I0 += M0)
; Type 4e
	MAS			ACC0, R0, R2 (SS)	||	ST		DM(I0 += M0), R0
; Type 4g
	SUBB	 		R6, R0, R2 		||	ST		DM(I0 += M0), R0
; Type 4f
	MAC.C		ACC0, R0, R2 (SS)	||	ST.C	DM(I0 += M0), R0
; Type 4h
	ADDC.C		R6, R0, R2 		||	ST.C	DM(I0 += M0), R0
; Type 12g
	LSHIFT	ACC0, R0, 3 (LO)	||	ST		DM(I0 += M0), R2
; Type 12h
	ASHIFT.C	ACC0, R0, -7 (LO)	||	ST.C	DM(I0 += M0), R2
; Type 8a
	MPY			ACC0, R0, R2 (SS)	||	CP		R0, R1
; Type 8c
	SUBC	 		R6, R0, R2 		||	CP		R0, R2
; Type 8b
	MAC.C		ACC0, R0, R2 (SS)	||	CP.C	R0, R6
; Type 8d
	ADD.C		R6, R0, R2 		||	CP.C	R0, R2
; Type 14c
	LSHIFT 		ACC0, R0, -9 (LO)	||	CP		R0, R7
; Type 14d
	ASHIFT.C	ACC0, R0, -3 (LO)	||	CP.C	R0, R2 
; Type 43a
	SUB R0, R2, R1 || MAC ACC0, R0, R1 (SS)
; Type 44b
	ADDC R0, R2, R3 || ASHIFT ACC0, R3, -5 (LORND)
; Type 44d
	ADDC R0, R2, R3 || ASHIFT ACC0, ACC0, -1 (LORND)
; Type 45b
	MAC ACC3, R2, R7 (SS) || LSHIFT ACC2, R2, -1 (HIRND)
; Type 45d
	MAC ACC3, R2, R7 (SS) || LSHIFT ACC2, ACC0, -1 (HIRND)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CORDIC - not implemented yet (09.05.08)
;
; Type 6c
;	LD.C		R0, (30, 40)
; Type 42a
;	POLAR.C		R2, R0
;	RECT.C		R4,	R2
; Type 42b
;	CONJ.C		R6, R4	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Type 6d
	LD	ACC0, 0x1800
; Type 41a
	RNDACC	ACC0
; Type 41c
	CLRACC	ACC0
; Type 25a
	SATACC	ACC0
; Type 6d
	LD	ACC0, 0x1800
	LD	ACC1, 0x1800
; Type 41b
	RNDACC.C	ACC0*
; Type 41d
	CLRACC.C	ACC0
; Type 25b
	SATACC.C	ACC0*
; Type 6b
	LD			R2, 0x0FF
	LD			R0, 0x08
; Type 15a
	ASHIFT	ACC0, R2, 3 (HI)	
	ASHIFT	ACC2, R2, 3 (LO)	
; Type 6d
	LD	ACC4, 0xFFF
	LD	ACC6, 0xFFF000
; Type 15a
	ASHIFTOR	ACC4, R2, 3 (HI)	
	ASHIFTOR	ACC6, R2, 3 (LO)	
; Type 16a
	LSHIFT	ACC0, R2, R0 (HI)	
	LSHIFT	ACC2, R2, R0 (LO)	
; Type 6d
	LD	ACC4, 0xFFF
	LD	ACC6, 0xFFF000
; Type 15a
	LSHIFTOR	ACC4, R2, R0 (HI)	
	LSHIFTOR	ACC6, R2, R0 (LO)
; Type 15b
	ASHIFT.C	ACC0, R2, 3 (HI)	
	ASHIFT.C	ACC2, R2, 3 (LO)	
	ASHIFTOR.C	ACC4, R2, 3 (HI)	
	ASHIFTOR.C	ACC6, R2, 3 (LO)	
; Type 16b
	LSHIFT.C	ACC0, R2, R0 (HI)	
	LSHIFT.C	ACC2, R2, R0 (LO)	
	LSHIFTOR.C	ACC4, R2, R0 (HI)	
	LSHIFTOR.C	ACC6, R2, R0 (LO)
; Type 15c
	ASHIFT	ACC0, ACC6, 23 (RND)
	LSHIFT	ACC1, ACC2, -12 (RND)
; Type 15d
	ASHIFT.C	ACC0, ACC6, 24 (RND)
	LSHIFT.C	ACC0, ACC6, -3 (RND)
; Type 6d
	LD	ACC0, 0xFFF
; Type 15c
	ASHIFT	ACC1, ACC0, 4 (RND)
	ASHIFT	ACC2, ACC0, 8 (RND)
	ASHIFT	ACC3, ACC0, 12 (RND)
	ASHIFT	ACC4, ACC0, 16 (RND)
	ASHIFT	ACC5, ACC0, 20 (RND)
	ASHIFT	ACC6, ACC0, 24 (RND)
; Type 6d
	LD	ACC0, 0xFFF000
; Type 15c
	ASHIFT  ACC1, ACC0, -4 (RND)
	ASHIFT	ACC2, ACC0, -8 (RND)
	ASHIFT	ACC3, ACC0, -12 (RND)
	ASHIFT	ACC4, ACC0, -16 (RND)
	ASHIFT	ACC5, ACC0, -20 (RND)
	ASHIFT	ACC6, ACC0, -24 (RND)
; Type 6d
	LD.C	R4, (12,12)
	LD.C	R2, (0x055,0xEAA)
	LD.C	R0, (0x180,0x355)
	EXPADJ.C R4, R2
	EXPADJ.C R4, R0
; Type 6c
	LD		R2, 0x004
	EXP		R3, R2 (HI)		; R3: 8
	ASHIFT	R0, R2, R3 (LO)	;
; Type 6c
	LD.C	R2, (0x055,0xEAA)
	EXP		R6, R2 (HI)
	EXP		R7, R3 (HI)
	EXP.C	R4, R2 (HI)
; Type 6b
	LD			R3, 12
	LD			R2, 0x080
	LD			R1, 0x055
	LD			R0, 0x180
; Type 16c
	EXPADJ		R3, R2
	EXPADJ		R3, R1
	EXPADJ		R3, R0
; Type 6b
	LD			R2, 0x000
	LD			R1, 0x1FF
	EXP			R3, R2 (HI)		; R3: 0x00B
	EXP			R3, R1 (LO)		; R3: 0x00E
	ASHIFT		ACC0, R2, R3 (HI)	; ACC0: 0x00000000
	LSHIFTOR	ACC0, R1, R3 (LO)	; ACC0: 0x007FC000
; Type 6b
	LD			R0, 0x1FF
	EXP			R1, R0 (HI)
	EXP			R1, R0 (HIX)
	EXP			R1, R0 (LO)
; Type 6b
	LD			R0, 0xFF
	EXP			R1, R0 (HI)
; Type 6b
	LD			R0, 0x7F
	EXP			R1, R0 (HI)
	EXP			R1, R0 (HI)
; Type 6b
	LD		ACC0.L, 0xFFF
	LD		ACC2.L, 0xFFF
	LD		ACC4.L, 0xFFF
	LD		ACC6.L, 0xFFF
	LD		ACC0.M, 0xFFF
	LD		ACC2.M, 0xFFF
; Type 06b
	LD  R0, 4
	LD	R1, 1
	LD	R2, 2
	LD	R3, -1
	LD	R4, -2
; Type 40g
	MPY.C	ACC0, R0, R2 (SS)
	MAC.C	ACC2, R0, R2 (SS)
	MAS.C	ACC4, R0, R2 (SS)
; Type 40f
	MPY.RC	ACC0, R0, R2 (UU)
	MAC.RC	ACC2, R0, R2 (UU)
	MAS.RC	ACC4, R0, R2 (UU)
; Type 40e
	MPY	ACC0, R3, R3 (SS)
	MPY	ACC1, R3, R3 (UU)
	MPY	ACC2, R3, R3 (US)
	MPY	ACC3, R3, R3 (SU)
	MPY	NONE, R3, R4 (RND)
	MPY	ACC0, R3, R4 (RND)
	MPY	ACC1, R3, R4 (SS)
	MPY	ACC2, R3, R4 (SU)
	MPY	ACC3, R3, R4 (US)
	MPY	ACC4, R3, R4 (UU)
	MPY	ACC0, R1, R2 (RND)
	MPY	ACC1, R1, R2 (SS)
	MPY	ACC2, R1, R2 (SU)
	MPY	ACC3, R1, R2 (US)
	MPY	ACC4, R1, R2 (UU)
	MPY	ACC0, R3, R2 (RND)
	MPY	ACC1, R3, R2 (SS)
	MPY	ACC2, R3, R2 (SU)
	MPY	ACC3, R3, R2 (US)
	MPY	ACC4, R3, R2 (UU)
	ADD NONE, R1, R3
	ADD	ACC4.M, R1, R3
	ADD	ACC5.L, R1, R3
	ADD	ACC6.M, R1, R4
	ADD ACC7.L, R2, R3
; Type 06a
	LD	_CNTR, 0x1111
	LD	I0, 0x4000
	LD	M0, 0x0002
	LD	I1, 0x4000
	LD	M1, 0x0002
	LD	ASTAT.R, 0x00F0
	LD	ASTAT.C, 0x000F		; write to read-only register: ignored.
	LD	_SSTAT, 0x000F		; write to read-only register: ignored.
; Type 06b
	LD	R0, 0
	LD	R1, 0x111
	LD	R2, 0x222
	LD	R3, 0x333
	LD	R4, 0x444
	LD	R5, 0x555
	LD	R6, 0x666
	LD	R7, 0x777
; Type 09f
	SUBBC.C R14, R0, (2, 3)
	ADD.C 	R14, R0, (2, 3)
	ADD.C 	R14, R0*, (2, 3)
; Type 09e
	SUBBC 	R16, R12, 2	
	ADD 	R16, R12, 2
; Type 09c
	SUB	R8, R1, R1
	IF NE ADD R9, R1, R7
	IF EQ ADD R9, R1, R7
; Type 09d
	SUBC.C R10, R0, R2
; Type 09i
	AND R10, R7, 0x7
; Type 06d
	LD	ACC0, 0x0000000F
	LD	ACC1, 0x000000FF
	LD	ACC2, 0x00000FFF
	LD	ACC3, 0x0000FFFF
	LD	ACC4, 0x000FFFFF
	LD	ACC5, 0x00FFFFFF
	LD	ACC6, 0x00112233
	LD  ACC7, 0x00445566
; Type 17a
	CP R8, R7
; Type 17b
	CP R9, _CNTR
	CP _CNTR, R6
; Type 17c
	CP ACC0, ACC7
; Type 17d
	CP.C R10, R2
; Type 17e
	CP.C R12, _CNTR
	CP.C _CNTR, R4
; Type 17f
	CP.C ACC2, ACC4
; Type 22a
	ST DM(I0+=M0), 0x123
	ST DM(I0+=M0), 0x456
	LD	I0, 0x4000
; Type 29c
	ST DM(I0+=16), R0	
	ST DM(I0+16), R1	
	ST DM(I0+=16), R2	
	ST DM(I0+=16), R3	
; Type 29d
	ST.C DM(I0+=16), R0	
	ST.C DM(I0+16), R2	
; Type 29a
	LD R8, DM(I1+=16)
	LD R9, DM(I1+16)
; Type 29b
	LD.C R10, DM(I1+=16)
	LD.C R12, DM(I1+16)
; Type 32c
	ST DM(I0+=M0), R0	
	ST DM(I0+M0), R1	
	ST DM(I0+=M0), R2	
	ST DM(I0+=M0), R3	
; Type 32d
	ST.C DM(I0+=M1), R0	
	ST.C DM(I0+M1), R2	
; Type 32a
	LD R8, DM(I1+=M0)
	LD R9, DM(I1+M0)
; Type 32b
	LD.C R10, DM(I1+=M1)
	LD.C R12, DM(I1+M1)
; Type 06c
	LD.C	R0, (2, 3)				
	LD.C	R2, (B#111, -B#1001)	
; Type 03f
	ST DM(0x3000), R0
	ST DM(0x3001), R1
; Type 03g
	ST.C DM(0x3002), R2
; Type 03h
	ST DM(0x3002), ACC6 (LO)
	ST DM(0x3004), ACC7 (HI)
; Type 03i
	ST.C DM(0x3006), ACC6 (LO)
	ST.C DM(0x3008), ACC6 (HI)
; Type 03a
	LD	R0, DM(0x3006)
; Type 03c
	LD.C R2, DM(0x3006)
; Type 03d
	LD ACC0, DM(0x3004) (LO)
	LD ACC2, DM(0x3004) (HI)
; Type 03e
	LD.C ACC0, DM(0x3000) (LO)
	LD.C ACC2, DM(0x3000) (HI)
; Type 23a
	LD			R1, 0		; 12 MSB of dividend
	LD			R0, 0xE		; 12 LSB of dividend
	LD			R2, 0x3		; divisor
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; signed division algorithm with fix for negative division error
; from 2-45 of ADSP-219x DSP Instruction Set Reference
; input:  R1 - 12 MSB of dividend (numerator)	- Op1
; input:  R0 - 12 LSB of dividend (numerator)	- Op0
; input:  R2 - divisor (denominator)			- Op2
; output: R2 - quotient (corrected)				
; intermediate (scratch) registers: R12
;
	CP			R12, R2			; R12: a copy of divisor (denominator)
	ABS			R2, R2			; make the divisor positive
	DIVS		R0, R1, R2
	DIVQ		R0, R1, R2		; #1
	DIVQ		R0, R1, R2		; #2
	DIVQ		R0, R1, R2		; #3
	DIVQ		R0, R1, R2		; #4
	DIVQ		R0, R1, R2		; #5
	DIVQ		R0, R1, R2		; #6
	DIVQ		R0, R1, R2		; #7
	DIVQ		R0, R1, R2		; #8
	DIVQ		R0, R1, R2		; #9
	DIVQ		R0, R1, R2		; #10
	DIVQ		R0, R1, R2		; #11
; now, quotient is in R0
	CP			R2, R0
	ADD			NONE, R12, 0	; test if R12 is positive or negative
	IF LT SUBB	R2, R0, 0		; R2 = 0 - R0; negate R0 if R2 was negative
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
	LD	R0, 2
	LD	R1, 0
	SUB	R2, R0, R1
;
	LD			ACC0, 0xEEDDCC				; ld test
	LD			ACC1, 0x112233
aa:
	ST.C		DM(0x1000), ACC0 (HI)
	ST.C		DM(0x1004), ACC0 (LO)
bb: LD.C		ACC2, DM(0x1000) (HI)
	LD.C		ACC4, DM(0x1000) (LO)
	LD.C		ACC6, DM(0x1004) (HI)
	LD.C		ACC6, DM(0x1004) (LO)
	ADD			ACC2, ACC1, ACC0
	SUB			ACC3, ACC2, ACC1
	SUBB		ACC4, ACC2, ACC1
	CP			ACC5, ACC4
	CP.C		ACC6, ACC4
	LD			I0, 0xFFFF
	CP			R0, I0
	CP			I1, R0
	CP.C		R2, I0
	CP.C		I2, R2
	LD			I0, 0x1111
	CP			R0, I0
	CP			I1, R0
	CP.C		R2, I0
	CP.C		I2, R2
	LD.C		R0, (30, 40)
	POLAR.C		R2, R0
	RECT.C		R4,	R2
	CONJ.C		R6, R4	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	CP			R0, R1
	CP			R0, I0
	CP			I2, R2
	CP.C		R4, R6
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ENA			BR, OL, AS, INT
	DIS			BR, OL, AS, INT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	LD			I4, 0x100
	LD			M5, 5
	LD			L4, 13
	LD			B4, 0x100
	LD			R0, DM(I4+=M5)		; begin circular buffer addressing
	LD			R0, DM(I4+=M5)
	LD			R0, DM(I4+=M5)
	LD			R0, DM(I4+=M5)
	LD			R0, DM(I4+=M5)
	LD			R0, DM(I4+=M5)
	LD			R0, DM(I4+=M5)
	LD			R0, DM(I4+=M5)
	LD			R0, DM(I4+=M5)
	LD			R0, DM(I4+=M5)
	LD			R0, DM(I4+=M5)
	LD			R0, DM(I4+=M5)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	LD.C	R0, (2, 3)				
	LD.C	R2, (B#111, -B#1001)	
	ADDC.C 	R4, R2, R0*
	ADDC R0, R2, 5			
	LD		I2,	0x108				
	LD		M2,	0x2					
	ST.C	DM(I2 += M2), R6		
	LD.C	R4, (0x10, -0x1F)		
	ST.C	DM(0x1000), R4			
	LD.C	R2, DM(0x1000)			
	LD	R1, -1
	LD	R2, 2
	LD	R3, -3
	LD	R4, 4
	IF LE ADD	R0, R2, R4			
	IF NE ADDC R0, R2, R4			
	IF GE ADDC R0, R2, 0			
	IF TRUE SUB R1, R2, R3			
	SUBC R1, R2, R3			
	SUBC R1, R2, 0			
	SUBB R1, R2, R3			
	SUBBC R1, R2, R3		
	AND	R2, R3, R4			
	OR	R2, R3, R4			
	XOR	R2, R3, R4			
	AND	R2, R3, 1			
	OR	R2, R3, 0x8			
	XOR	R2, R3, 0xF			
	NOT	R5, R4				
	ABS	R6, R5				
	ADD	R0, R2, R4			
	IF EQ ADD R8, R10, R12
	IF TRUE ADD R7, R9, 11
	ADDC R0, R2, R4			
	ADDC R0, R2, 0			
	SUB R1, R2, R3			
	SUBC R1, R2, R3			
	SUBC R1, R2, 0			
	SUBB R1, R2, R3			
	SUBBC R1, R2, R3		
	AND	R2, R3, R4			
	OR	R2, R3, R4			
	XOR	R2, R3, R4			
	NOT	R5, R4				
	ABS	R6, R5				
	ADD.C	R0, R2, R4			
	ADDC.C R0, R2, R4			
	ADDC.C R0, R2, (0,0)			
	SUB.C R2, R2, R6			
	SUBC.C R2, R2, R6*
	SUBC.C R2, R2, (3,4)			
	IF NE ADDC.C R2, R2, R6			
	IF NE ADDC.C R2, R2, (0,0)			
	IF NE SUBC.C R2, R2, R6			
	IF NE SUBC.C R2, R2, (0,0)			
	IF NE SUBB.C R2, R2, R6			
	IF NE SUBBC.C R2, R2, R6			
	SUBBC.C R2, R2, R6		
	NOT.C	R6, R4				
	ABS.C	R6, R4				
	LD	I2,	0x108			
	LD	I0,	0x200			
	LD	I1,	0x400			
	LD	M2,	0x2				
	LD	R4, DM(0x100)		
	ST	DM(0x100), R4		
	LD	_CNTR, 3			
	LD	R2, -b#11			
	LD	R6, DM(I2 += M2)	
	LD	R7, DM(I2 + M2)		
	ST	DM(I2+=M2), R16		
	ST	DM(I2+M2), R17		
	LD	R10, DM(I0 += 0x10)	
	LD	R10, DM(I1 += 0x10)	
	CP	R20, R6
	LD	I0,	0x200			
	LD.C	R4, DM(0x100)		
	ST.C	DM(0x100), R4		
	LD.C	R2, (3,4)
	LD.C	R6, DM(I2 += M2)	
	LD.C	R8, DM(I2 + M2)		
	ST.C	DM(I2+=M2), R16		
	ST.C	DM(I2+M2), R18		
	LD.C	R10, DM(I0 += 0x10)	
	LD.C	R10, DM(I1 += 0x10)	
	CP.C	R20, R6
	PUSH	PC, STS
	NOP
	POP		PC
	NOP
	PUSH	LOOP
	NOP
	POP		LOOP, STS
	LD	R11, DM(I5 + 3)	
	ST	DM(I4 += 0x10), R12	
	ST	DM(I5 + 3), R13		
	ST	DM(I5), R13			
	LD	R12, DM(I6)
	ST	DM(I2+=M2), 0x200	
	ST	DM(0xFF), R0
	ST	DM(I2+33), R4
	ST	DM(I2+= -5), R5
	CALL	target1
	LD	R0, 2
	LD	R2, -b#11	
	CLRACC	ACC0	
	LD	_CNTR, 3	
	DO	loop_end1	UNTIL CE
loop_end1:	
	MAC	 ACC0, R0, R2 (SS)
	NOP
target1:
	LD	R3, 3
	RTS
	NOP
	NOP
	NOP
	NOP
