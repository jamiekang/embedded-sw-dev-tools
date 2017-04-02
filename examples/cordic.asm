;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 
; OFDM DSP CORDIC Test Program 
; - Ver 0.1 - this is for dspsim ver 1.6+
; 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.DATA
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.CODE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; POLAR.C: (x,y) -> angle computation
; RECT.C : angle -> (cos,sin) computation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	LD.C		R0, (0, 40)		; (R0, R1): (    0,0x028) = (      0,     40)
	POLAR.C		R2, R0			; (R2, R3): (0x400,    0) = (    0.5,      0), Note - exact value: (0.5, 0)
	RECT.C		R4,	R2			; (R4, R5): (    0,0x7FF) = (      0, 0.9995), Note – exact value: (0, 1)	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	LD.C		R0, (-1, 0)		; (R0, R1): (0xFFF,    0) = (     -1,      0)
	POLAR.C		R2, R0			; (R2, R3): (0x790,    0) = (0.9453,0), Note - exact value: (-1, 0)	-> ok
	RECT.C		R4,	R2			; (R4, R5): (0x81E,0x15D) = (-0.9854, 0.1704), Note – exact value: (-1, 0)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	LD.C		R0, (3, 0)		; (R0, R1): (3,0)
	POLAR.C		R2, R0			; (R2, R3): (0xFE1,    0) = (-0.0151,      0), Note - exact value: (0, 0)
	RECT.C		R4,	R2			; (R4, R5): (0x7FD,0xF9E) = ( 0.9985,-0.0479), Note – exact value: (1, 0)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	LD.C		R0, (0, -256)	; (R0, R1): (    0,0xF00) = (      0,   -256)
	POLAR.C		R2, R0			; (R2, R3): (0xC00,    0) = (   -0.5,      0), Note - exact value: (-0.5, 0)
	RECT.C		R4,	R2			; (R4, R5): (    0,0x800) = (      0,     -1), Note – exact value: (0, -1) 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	LD.C		R0, (30, 40)	; (R0, R1): (0x01E,0x028) = (     30,     40)
	POLAR.C		R2, R0			; (R2, R3): (0x25C,    0) = ( 0.2949,      0), Note - exact value: (0.2952, 0)
	RECT.C		R4,	R2			; (R4, R5): (0x4CD,0x666) = ( 0.6001, 0.7998), Note – exact value: (0.60, 0.80)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	LD.C		R0, (-40, 40)	; (R0, R1): (0xFD8,0x028) = (    -40,     40)
	POLAR.C		R2, R0			; (R2, R3): (0x600,    0) = (   0.75,      0), Note - exact value: (0.75, 0)
	RECT.C		R4,	R2			; (R4, R5): (0xA58,0x5A8) = (-0.7070, 0.7070), Note – exact value: (-0.7071, 0.7071)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	LD.C		R0, (-40, -40)	; (R0, R1): (0xFD8,0xFD8) = (    -40,    -40)
	POLAR.C		R2, R0			; (R2, R3): (0xA00,    0) = (  -0.75,      0), Note - exact value: (-0.75, 0)
	RECT.C		R4,	R2			; (R4, R5): (0xA58,0xA57) = (-0.7070,-0.7075), Note – exact value: (-0.7071, -0.7071)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	LD.C		R0, (40, -40)	; (R0, R1): (0x028,0xFD8) = (     40,    -40)
	POLAR.C		R2, R0			; (R2, R3): (0xE00,    0) = (  -0.25,      0), Note - exact value: (-0.25, 0)
	RECT.C		R4,	R2			; (R4, R5): (0x5A8,0xA58) = ( 0.7070,-0.7070), Note – exact value: (0.7071, -0.7071)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	NOP