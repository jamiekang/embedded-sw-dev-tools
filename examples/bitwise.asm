;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This is a test assembly input file for DSP simulator
; - Ver 0.2 - this is compatible with dspsim ver 1.6+ 

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.DATA
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.CODE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; register initializations
;
	LD	R24, 0xAAA
;
	TGLBIT	R00, R24, 0x0		; AN:1, R00: 0xAAB
	TGLBIT	R01, R24, 0x1		; AN:1, R01: 0xAA8
	TGLBIT	R02, R24, 0x2		; AN:1, R02: 0xAAE
	TGLBIT	R03, R24, 0x3		; AN:1, R03: 0xAA2
	TGLBIT	R04, R24, 0x4		; AN:1, R04: 0xABA
	TGLBIT	R05, R24, 0x5		; AN:1, R05: 0xA8A
	TGLBIT	R06, R24, 0x6		; AN:1, R06: 0xAEA
	TGLBIT	R07, R24, 0x7		; AN:1, R07: 0xA2A
	TGLBIT	R08, R24, 0x8		; AN:1, R08: 0xBAA
	TGLBIT	R09, R24, 0x9		; AN:1, R09: 0x8AA
	TGLBIT	R10, R24, 0xA		; AN:1, R10: 0xEAA
	TGLBIT	R11, R24, 0xB		; R11: 0x2AA
;
	LD	R24, 0x000
;
	SETBIT	R00, R24, 0x0		; R00: 0x001
	SETBIT	R01, R24, 0x1		; R01: 0x002
	SETBIT	R02, R24, 0x2		; R02: 0x004
	SETBIT	R03, R24, 0x3		; R03: 0x008
	SETBIT	R04, R24, 0x4		; R04: 0x010
	SETBIT	R05, R24, 0x5		; R05: 0x020
	SETBIT	R06, R24, 0x6		; R06: 0x040
	SETBIT	R07, R24, 0x7		; R07: 0x080
	SETBIT	R08, R24, 0x8		; R08: 0x100
	SETBIT	R09, R24, 0x9		; R09: 0x200
	SETBIT	R10, R24, 0xA		; R10: 0x400
	SETBIT	R11, R24, 0xB		; AN:1, R11: 0x800
;
	LD	R24, 0xAAA
;
	TSTBIT	R00, R24, 0x0		; AZ:1, R00: 0x000
	TSTBIT	R01, R24, 0x1		; AZ:0, R01: 0x002
	TSTBIT	R02, R24, 0x2		; AZ:1, R02: 0x000
	TSTBIT	R03, R24, 0x3		; AZ:0, R03: 0x008
	TSTBIT	R04, R24, 0x4		; AZ:1, R04: 0x000
	TSTBIT	R05, R24, 0x5		; AZ:0, R05: 0x020
	TSTBIT	R06, R24, 0x6		; AZ:1, R06: 0x000
	TSTBIT	R07, R24, 0x7		; AZ:0, R07: 0x080
	TSTBIT	R08, R24, 0x8		; AZ:1, R08: 0x000
	TSTBIT	R09, R24, 0x9		; AZ:0, R09: 0x200
	TSTBIT	R10, R24, 0xA		; AZ:1, R10: 0x000
	TSTBIT	R11, R24, 0xB		; AZ:0, AN:1, R11: 0x800
;
	LD	R24, 0xFFF
	CLRBIT	R00, R24, 0x0		; AN:1, R00: 0xFFE
	CLRBIT	R01, R24, 0x1		; AN:1, R01: 0xFFD
	CLRBIT	R02, R24, 0x2		; AN:1, R02: 0xFFB
	CLRBIT	R03, R24, 0x3		; AN:1, R03: 0xFF7
	CLRBIT	R04, R24, 0x4		; AN:1, R04: 0xFEF
	CLRBIT	R05, R24, 0x5		; AN:1, R05: 0xFDF
	CLRBIT	R06, R24, 0x6		; AN:1, R06: 0xFBF
	CLRBIT	R07, R24, 0x7		; AN:1, R07: 0xF7F
	CLRBIT	R08, R24, 0x8		; AN:1, R08: 0xEFF
	CLRBIT	R09, R24, 0x9		; AN:1, R09: 0xDFF
	CLRBIT	R10, R24, 0xA		; AN:1, R10: 0xBFF
	CLRBIT	R11, R24, 0xB		; AN:0, R11: 0x7FF
	NOP
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; PC:0034 Line:68 Opcode:NOP Type:30a
; R00:FFE R08:EFF R16:000 R24:FFF 
; R01:FFD R09:DFF R17:000 R25:000 
; R02:FFB R10:BFF R18:000 R26:000 
; R03:FF7 R11:7FF R19:000 R27:000 
; R04:FEF R12:000 R20:000 R28:000 
; R05:FDF R13:000 R21:000 R29:000 
; R06:FBF R14:000 R22:000 R30:000 
; R07:F7F R15:000 R23:000 R31:000 
; ACC0:00000000 ACC0.H:00 ACC0.M:000 ACC0.L:000 
; ACC1:00000000 ACC1.H:00 ACC1.M:000 ACC1.L:000 
; ACC2:00000000 ACC2.H:00 ACC2.M:000 ACC2.L:000 
; ACC3:00000000 ACC3.H:00 ACC3.M:000 ACC3.L:000 
; ACC4:00000000 ACC4.H:00 ACC4.M:000 ACC4.L:000 
; ACC5:00000000 ACC5.H:00 ACC5.M:000 ACC5.L:000 
; ACC6:00000000 ACC6.H:00 ACC6.M:000 ACC6.L:000 
; ACC7:00000000 ACC7.H:00 ACC7.M:000 ACC7.L:000 
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
; PC:0034 NextPC: FFFF
; Time: 53 cycles
; 
; End of Simulation!!
; 
; ----------------------------------
; ** Simulator Statistics Summary **
; ----------------------------------
; Time: 53 cycles for 1 iteration
; Overflow: 0 times

