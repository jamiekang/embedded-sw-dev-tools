;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This is a test assembly input file for DSP simulator
;
; - Ver 0.2 - this is compatible with dspsim ver 1.6+ 
;
; Note: this is to test multi-latency cases
;	1. this code generates different register values according to 
;      "delay slot" enable/disable status.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.DATA
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.CODE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; register initializations
	LD	CNTR, 2
;
	LD	ACC0, 0
	LD	ACC1, 0
	LD	ACC2, 0
	LD	R31, 0
	LD	R30, 0
;
;	DO	lp1	UNTIL CE
;;	JUMP lp1
;	ADD	R30, R30, 1
;lp1:	
;	ADD	R31, R31, 1
;
	LD	R0, 0x000
	JUMP	br1
; comment here
	LD	R1, 0x111		; branch delay slot
;
	LD	R8, 0
	LD	R9, 0
	LD	R10, 0
br1:
	LD	I0, 0x1000
	LD	R2, 0x222
	CALL	ca1
	LD	R3, 0x333		; call delay slot
next_ca1:
	LD 	I0, 0x2000
	ADD R10, R2, R3
;
	LD	I0, 0x3000
	LD	R16, DM(I0)		; data read from 0x2000, NOT 0x3000 - R16 should be 0x222, not 0x333

	LD	R6, 0x666
	JUMP	br2
	LD	R7, 0x777		; branch delay slot
; subroutine here
ca1:
	ST	DM(0x2000), R2
	ST	DM(0x3000), R3
	LD	R4, 0x444	
	RTS					; back to next_ca1
	LD	R5, 0x555		; rts delay slot
;
br2:
;
;	Bit 4 of MSTAT: M_MODE or MM - MAC result mode
;   0 = Fractional Mode
;   1 = Integer Mode
;
	MPY	ACC0, R2, R2 (SS)	; MPY in fractional mode
	LD MSTAT, 0x010			; set bit 4 to 1 - set integer mode
	MPY	ACC1, R2, R2 (SS)	; MPY still in fractional mode - due to MSTAT loading delay
	MPY	ACC2, R2, R2 (SS)	; MPY now in integer mode 
	LD MSTAT, 0x000			; set bit 4 to 0 - set fractional mode
;
	ST.C	DM(0x4000), ACC0 (LO)
; NOTE: 1-cycle stall here - type 3i (ST ACC64 just before any LD/ST)
	LD		R20, DM(0x4000)
;
	NOP
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; PC:0023 Line:78 Opcode:NOP Type:30a
; R00:000 R08:000 R16:222 R24:000 
; R01:000 R09:000 R17:000 R25:000 
; R02:222 R10:555 R18:000 R26:000 
; R03:333 R11:000 R19:000 R27:000 
; R04:444 R12:000 R20:908 R28:000 
; R05:000 R13:000 R21:000 R29:000 
; R06:666 R14:000 R22:000 R30:000 
; R07:000 R15:000 R23:000 R31:000 
; ACC0:00091908 ACC0.H:00 ACC0.M:091 ACC0.L:908 
; ACC1:00091908 ACC1.H:00 ACC1.M:091 ACC1.L:908 
; ACC2:00048C84 ACC2.H:00 ACC2.M:048 ACC2.L:C84 
; ACC3:00000000 ACC3.H:00 ACC3.M:000 ACC3.L:000 
; ACC4:00000000 ACC4.H:00 ACC4.M:000 ACC4.L:000 
; ACC5:00000000 ACC5.H:00 ACC5.M:000 ACC5.L:000 
; ACC6:00000000 ACC6.H:00 ACC6.M:000 ACC6.L:000 
; ACC7:00000000 ACC7.H:00 ACC7.M:000 ACC7.L:000 
; I0:3000 M0:0000 L0:0000 B0:0000 
; I1:0000 M1:0000 L1:0000 B1:0000 
; I2:0000 M2:0000 L2:0000 B2:0000 
; I3:0000 M3:0000 L3:0000 B3:0000 
; I4:0000 M4:0000 L4:0000 B4:0000 
; I5:0000 M5:0000 L5:0000 B5:0000 
; I6:0000 M6:0000 L6:0000 B6:0000 
; I7:0000 M7:0000 L7:0000 B7:0000 
; PCSTACK:0000    LPSTACK:0000
; CNTR   :0002    LPEVER :0000
; SSTAT - PCE:1 PCF:0 PCL:1 LSE:1 LSF:0 SSE:1 SOV:0
; PC:0023 NextPC: FFFF
; Time: 35 cycles
; 
; End of Simulation!!
; 
; ----------------------------------
; ** Simulator Statistics Summary **
; ----------------------------------
; Time: 35 cycles for 1 iteration
; Overflow: 0 times
;

