;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 
; - Ver 0.2 - this is for dspsim ver 1.6+
; 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.DATA
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.CODE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Type 6d
	LD	ACC0, 0xAAAAAA		; ACC0: 0xFFAAAAAA
; Type 6b
	LD	ACC0.H, 0x01		; ACC0: 0x01AAAAAA
; Type 25a
	SATACC	ACC0			; ACC0: 0x007FFFFF
; Type 6d
	LD	ACC1, 0x000FFF		; ACC1: 0x00000FFF
; Type 6b
	LD	ACC1.H, 0xFF		; ACC1: 0xFF000FFF
; Type 25a
	SATACC	ACC1			; ACC1: 0xFF800000
; Type 6d
	LD	ACC2, 0xAAAAAA		; ACC2: 0xFFAAAAAA
	LD	ACC3, 0x000FFF		; ACC3: 0x00000FFF
; Type 6b
	LD	ACC2.H, 0x01		; ACC2: 0x01AAAAAA
	LD	ACC3.H, 0xFF		; ACC3: 0xFF000FFF
; Type 25b
	SATACC.C ACC2			; ACC2: 0x007FFFFF
							; ACC3: 0xFF800000
;
	NOP
