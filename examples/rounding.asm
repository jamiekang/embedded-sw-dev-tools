;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This is a test assembly input file for DSP simulator
;
; - Ver 0.2 - this is compatible with dspsim ver 1.6+ 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.DATA
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.CODE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Unbiased rounding (default)
;
	LD	ACC0, 0x000800
	RNDACC	ACC0				; ACC0: 0x00000000
;
	LD	ACC1, 0x001800			
	RNDACC	ACC1				; ACC1: 0x00002000
;
	LD	ACC2, 0x000801			
	RNDACC	ACC2				; ACC2: 0x00001001
;
	LD	ACC3, 0x001801
	RNDACC	ACC3				; ACC3: 0x00002001
; 
	LD	ACC4, 0x0007FF
	RNDACC	ACC4				; ACC4: 0x00000FFF
;
	LD	ACC5, 0x0017FF
	RNDACC	ACC5				; ACC5: 0x00001FFF
;
; Example
	LD	ACC6, 0x1800
	LD	ACC7, 0x1800
	RNDACC.C	ACC6*			; ACC6: 0x00002000, ACC7: 0xFFFFE000
;
; Change M_BIAS(bit 7) of MSTAT 
;
	LD	MSTAT, 0x080			; set bit 7 to 1
;
; Biased rounding
;
	LD	ACC0, 0x000800
	RNDACC	ACC0				; ACC0: 0x00001000	(differs only here)
;
	LD	ACC1, 0x001800			
	RNDACC	ACC1				; ACC1: 0x00002000
;
	LD	ACC2, 0x000801			
	RNDACC	ACC2				; ACC2: 0x00001001
;
	LD	ACC3, 0x001801
	RNDACC	ACC3				; ACC3: 0x00002001
; 
	LD	ACC4, 0x0007FF
	RNDACC	ACC4				; ACC4: 0x00000FFF
;
	LD	ACC5, 0x0017FF
	RNDACC	ACC5				; ACC5: 0x00001FFF
;
; Example
	LD	ACC6, 0x1800
	LD	ACC7, 0x1800
	RNDACC.C	ACC6*			; ACC6: 0x00002000, ACC7: 0xFFFFF000
;
	NOP
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; PC:001F Line:67 Opcode:NOP Type:30a
; R00:000 R08:000 R16:000 R24:000 
; R01:000 R09:000 R17:000 R25:000 
; R02:000 R10:000 R18:000 R26:000 
; R03:000 R11:000 R19:000 R27:000 
; R04:000 R12:000 R20:000 R28:000 
; R05:000 R13:000 R21:000 R29:000 
; R06:000 R14:000 R22:000 R30:000 
; R07:000 R15:000 R23:000 R31:000 
; ACC0:00001000 ACC0.H:00 ACC0.M:001 ACC0.L:000 
; ACC1:00002000 ACC1.H:00 ACC1.M:002 ACC1.L:000 
; ACC2:00001001 ACC2.H:00 ACC2.M:001 ACC2.L:001 
; ACC3:00002001 ACC3.H:00 ACC3.M:002 ACC3.L:001 
; ACC4:00000FFF ACC4.H:00 ACC4.M:000 ACC4.L:FFF 
; ACC5:00001FFF ACC5.H:00 ACC5.M:001 ACC5.L:FFF 
; ACC6:00002000 ACC6.H:00 ACC6.M:002 ACC6.L:000 
; ACC7:FFFFF000 ACC7.H:FF ACC7.M:FFF ACC7.L:000 
; I0:0000 M0:0000 L0:0000 B0:0000 
; I1:0000 M1:0000 L1:0000 B1:0000 
; I2:0000 M2:0000 L2:0000 B2:0000 
; I3:0000 M3:0000 L3:0000 B3:0000 
; I4:0000 M4:0000 L4:0000 B4:0000 
; I5:0000 M5:0000 L5:0000 B5:0000 
; I6:0000 M6:0000 L6:0000 B6:0000 
; I7:0000 M7:0000 L7:0000 B7:0000 
; PCSTACK:0000    LPSTACK:0000
; CNTR   :0000    LPEVER :0000
; SSTAT - PCE:1 PCF:0 PCL:1 LSE:1 LSF:0 SSE:1 SOV:0
; MSTAT - SR:0 BR:0 OL:0 AS:0 MM:0 TI:0 SD:0 MB:1
; PC:001F NextPC: FFFF
; Time: 32 cycles
; 
; End of Simulation!!
; 
; ----------------------------------
; ** Simulator Statistics Summary **
; ----------------------------------
; Time: 32 cycles for 1 iteration
; Overflow: 0 times


