;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 
; OFDM DSP RTL Test Program 
; - Mainly focused for Data Move Instructions
;
; - Ver 0.2 - this is for dspsim ver 1.6+ 
; 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.DATA
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.CODE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Type 06a
;	Instruction							; *** Register/Memory Data AFTER Running This Instruction ***
	LD	CNTR, 0xFEDC					; CNTR: 0xFEDC
; Type 06b
	LD	R0, 0x000						; R0: 0x000
	LD	R1, 0x111						; R1: 0x111
	LD	R2, 0x222						; R2: 0x222
	LD	R3, 0x333						; R3: 0x333
	LD	R4, 0x444						; R4: 0x444
	LD	R5, 0x555						; R5: 0x555
	LD	R6, 0x666						; R6: 0x666
	LD	R7, 0x777						; R7: 0x777
	LD	R8, 0x888						; R8: 0x888
; Type 06d
	LD	ACC0, 0x0000000F				; ACC0: 0x0000000F
	LD	ACC1, 0x000000FF				; ACC1: 0x000000FF
	LD	ACC2, 0x00000FFF				; ACC2: 0x00000FFF
	LD	ACC3, 0x0000FFFF				; ACC3: 0x0000FFFF
	LD	ACC4, 0x000FFFFF				; ACC4: 0x000FFFFF
	LD	ACC5, 0x00FFFFFF				; ACC5: 0xFFFFFFFF		; Note: sign extension of ACC5.H
	LD	ACC6, 0x00112233				; ACC6: 0x00112233
	LD  ACC7, 0x00445566				; ACC7: 0x00445566
; Type 17a
	CP R9, R8							; R9: 0x888
; Type 17b
	CP R10, CNTR						; R10: 0xEDC			; Note: MSB truncation
	CP CNTR, R8							; CNTR: 0xF888			; Note: MSB sign extension
; Type 17c
	CP ACC0, ACC7						; ACC0: 0x00445566
; Type 17d
	CP.C R10, R2						; R10: 0x222, R11: 0x333
; Type 17e
	CP.C R12, CNTR						; R12: 0x888, R13: 0xFFF	; Note: MSB sign extension of R13
	CP.C CNTR, R4						; CNTR: 0x5444				; Note: MSB 4 bits from R5, LSB 12 bits from R4
; Type 17f
	CP.C ACC2, ACC4						; ACC2: 0x000FFFFF, ACC3: 0xFFFFFFFF
; Type 06a
	LD	I0, 0x4000						; I0: 0x4000
	LD	M0, 0x0001						; M0: 0x0001
	NOP									; to resolve latency restriction - M0 update w/ immediate does not apply to the next cycle
; Type 22a
	ST DM(I0+=M0), 0x123				; DM(0x4000): 0x123, I0: 0x4001
	ST DM(I0+=M0), 0x456				; DM(0x4001): 0x456, I0: 0x4002
; Type 06a
	LD	I0, 0x4010						; I0: 0x4010
	NOP									; to resolve latency restriction - I0 update w/ immediate does not apply to the next cycle
; Type 29c
	ST DM(I0+=16), R0					; DM(0x4010): 0x000, I0: 0x4020
	ST DM(I0+=16), R1					; DM(0x4020): 0x111, I0: 0x4030
	ST DM(I0+=16), R2					; DM(0x4030): 0x222, I0: 0x4040
	ST DM(I0+=16), R3					; DM(0x4040): 0x333, I0: 0x4050
; Type 29d
	ST.C DM(I0+=16), R0					; DM(0x4050): 0x000, DM(0x4051): 0x111, I0: 0x4060
	ST.C DM(I0+32), R6					; DM(0x4080): 0x666, DM(0x4081): 0x777, I0: 0x4060
; Type 06a
	LD	I1, 0x4000						; I1: 0x4000
	LD	M1, 0x0002						; M1: 0x0002
; Type 29a
	LD R8, DM(I1+=16)					; R8: 0x123, I1: 0x4010
	LD R9, DM(I1+16)					; R9: 0x111, I1: 0x4010
; Type 06a
	LD	I1, 0x4080						; I1: 0x4080
	NOP									; to resolve latency restriction - I1 update w/ immediate does not apply to the next cycle
; Type 29b
	LD.C R10, DM(I1)					; R10: 0x666, R11: 0x777
; Type 32c
	ST DM(I0+=M0), R0					; DM(0x4060): 0x000, I0: 0x4061
	ST DM(I0+=M0), R1					; DM(0x4061): 0x111, I0: 0x4062
	ST DM(I0+=M0), R2					; DM(0x4062): 0x222, I0: 0x4063
	ST DM(I0+=M0), R3					; DM(0x4063): 0x333, I0: 0x4064
; Type 32d
	ST.C DM(I0+=M1), R0					; DM(0x4064): 0x000, DM(0x4065): 0x111, I0: 0x4066
	ST.C DM(I0+M1), R2					; DM(0x4068): 0x222, DM(0x4069): 0x333, I0: 0x4066
; Type 06a
	LD	I1, 0x4082						; I1: 0x4082
	NOP									; to resolve latency restriction - I1 update w/ immediate does not apply to the next cycle
; Type 22a
	ST DM(I1+=M0), 0x888				; DM(0x4082): 0x888, I1: 0x4083
	ST DM(I1+=M0), 0x999				; DM(0x4083): 0x999, I1: 0x4084
; Type 06a
	LD	I2, 0x4080						; I2: 0x4080
	NOP									; to resolve latency restriction - I2 update w/ immediate does not apply to the next cycle
; Type 32a
	LD R16, DM(I2+=M0)					; R16: 0x666, I2: 0x4081
	LD R17, DM(I2+M0)					; R17: 0x888, I2: 0x4081
; Type 06a
	LD	I2, 0x4060						; I2: 0x4060
	NOP									; to resolve latency restriction - I2 update w/ immediate does not apply to the next cycle
; Type 32b
	LD.C R18, DM(I2+M1)					; R18: 0x222, R19: 0x333, I2: 4060
	LD.C R20, DM(I2+=M1)				; R20: 0x000, R21: 0x111, I2: 4062
; Type 06c
	LD.C	R0, (2, 3)					; R0: 0x002, R1: 0x003
	LD.C	R2, (B#111, -B#1001)		; R2: 0x007, R3: 0xFF7
; Type 03f
	ST DM(0x3000), R0					; DM(0x3000): 0x002
	ST DM(0x3001), R1					; DM(0x3001): 0x003
; Type 03g
	ST.C DM(0x3002), R2					; DM(0x3002): 0x007, DM(0x3003): 0xFF7
; Type 03h
	ST DM(0x3010), ACC6 (LO)			; DM(0x3010): 0x233, DM(0x3011): 0x112
	ST DM(0x3012), ACC6 (HI)			; DM(0x3012): 0x122, DM(0x3013): 0x001
; Type 03i
	ST.C DM(0x3020), ACC6 (LO)			; DM(0x3020): 0x233, DM(0x3021): 0x112, DM(0x3022): 0x566, DM(0x3023): 0x445
	ST.C DM(0x3024), ACC6 (HI)			; DM(0x3024): 0x122, DM(0x3025): 0x001, DM(0x3026): 0x455, DM(0x3027): 0x004
; Type 03a
	LD	R0, DM(0x3010)					; R0: 0x233
; Type 03c
	LD.C R2, DM(0x3010)					; R2: 0x233, R3: 0x112
; Type 03d
	LD ACC0, DM(0x3010) (LO)			; ACC0: 0x00112233
	LD ACC1, DM(0x3010) (HI)			; ACC1: 0x11223300
; Type 03e
	LD.C ACC4, DM(0x3020) (LO)			; ACC4: 0x00112233, ACC5: 0x00445566
	LD.C ACC6, DM(0x3020) (HI)			; ACC6: 0x11223300, ACC7: 0x44556600
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Test circular addressing 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	LD			I4, 0x1000				; I4: 0x1000
	LD			M5, 5					; M5: 0x0005
	LD			L4, 13					; L4: 0x000D - Loop size of 13
	LD			B4, 0x1000				; B4: 0x1000
	LD			I0, 0x1000				; I0: 0x1000
	LD			M0, 0x1					; M0: 0x0001
	NOP									; to resolve latency restriction - M0 update w/ immediate does not apply to the next cycle
	ST			DM(I0+=M0), 0x0			; DM(0x1000): 0x000, I0: 0x1001
	ST			DM(I0+=M0), 0x1			; DM(0x1001): 0x001, I0: 0x1002
	ST			DM(I0+=M0), 0x2			; DM(0x1002): 0x002, I0: 0x1003
	ST			DM(I0+=M0), 0x3			; DM(0x1003): 0x003, I0: 0x1004
	ST			DM(I0+=M0), 0x4			; DM(0x1004): 0x004, I0: 0x1005
	ST			DM(I0+=M0), 0x5			; DM(0x1005): 0x005, I0: 0x1006
	ST			DM(I0+=M0), 0x6			; DM(0x1006): 0x006, I0: 0x1007
	ST			DM(I0+=M0), 0x7			; DM(0x1007): 0x007, I0: 0x1008
	ST			DM(I0+=M0), 0x8			; DM(0x1008): 0x008, I0: 0x1009
	ST			DM(I0+=M0), 0x9			; DM(0x1009): 0x009, I0: 0x100A
	ST			DM(I0+=M0), 0xA			; DM(0x100A): 0x00A, I0: 0x100B
	ST			DM(I0+=M0), 0xB			; DM(0x100B): 0x00B, I0: 0x100C
	ST			DM(I0+=M0), 0xC			; DM(0x100C): 0x00C, I0: 0x100D
	ST			DM(I0+=M0), 0xD			; DM(0x100D): 0x00D, I0: 0x100E
	ST			DM(I0+=M0), 0xE			; DM(0x100E): 0x00E, I0: 0x100F
	ST			DM(I0+=M0), 0xF			; DM(0x100F): 0x00F, I0: 0x1010
; begin circular buffer addressing here
	LD			R00, DM(I4+=M5)			; R00: 0x000, I4: 0x1005
	LD			R01, DM(I4+=M5)			; R01: 0x005, I4: 0x100A
	LD			R02, DM(I4+=M5)			; R02: 0x00A, I4: 0x1002
	LD			R03, DM(I4+=M5)			; R03: 0x002, I4: 0x1007
	LD			R04, DM(I4+=M5)			; R04: 0x007, I4: 0x100C
	LD			R05, DM(I4+=M5)			; R05: 0x00C, I4: 0x1004
	LD			R06, DM(I4+=M5)			; R06: 0x004, I4: 0x1009
	LD			R07, DM(I4+=M5)			; R07: 0x009, I4: 0x1001
	LD			R08, DM(I4+=M5)			; R08: 0x001, I4: 0x1006
	LD			R09, DM(I4+=M5)			; R09: 0x006, I4: 0x100B
	LD			R10, DM(I4+=M5)			; R10: 0x00B, I4: 0x1003
	LD			R11, DM(I4+=M5)			; R11: 0x003, I4: 0x1008
	LD			R12, DM(I4+=M5)			; R12: 0x008, I4: 0x1000
	LD			R13, DM(I4+=M5)			; R13: 0x000, I4: 0x1005
	LD			R14, DM(I4+=M5)			; R14: 0x005, I4: 0x100A
	LD			R15, DM(I4+=M5)			; R15: 0x00A, I4: 0x1002
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	NOP
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; At the end
;---------------------------------------------------------------------------
; PC:0076 Line:174 Opcode:NOP Type:30a
; R00:000 R08:001 R16:666 R24:000 
; R01:005 R09:006 R17:888 R25:000 
; R02:00A R10:00B R18:222 R26:000 
; R03:002 R11:003 R19:333 R27:000 
; R04:007 R12:008 R20:000 R28:000 
; R05:00C R13:000 R21:111 R29:000 
; R06:004 R14:005 R22:000 R30:000 
; R07:009 R15:00A R23:000 R31:000 
; ACC0:00112233 ACC0.H:00 ACC0.M:112 ACC0.L:233 
; ACC1:11223300 ACC1.H:11 ACC1.M:223 ACC1.L:300 
; ACC2:000FFFFF ACC2.H:00 ACC2.M:0FF ACC2.L:FFF 
; ACC3:FFFFFFFF ACC3.H:FF ACC3.M:FFF ACC3.L:FFF 
; ACC4:00112233 ACC4.H:00 ACC4.M:112 ACC4.L:233 
; ACC5:00445566 ACC5.H:00 ACC5.M:445 ACC5.L:566 
; ACC6:11223300 ACC6.H:11 ACC6.M:223 ACC6.L:300 
; ACC7:44556600 ACC7.H:44 ACC7.M:556 ACC7.L:600 
; I0:1010 M0:0001 L0:0000 B0:0000 
; I1:4084 M1:0002 L1:0000 B1:0000 
; I2:4062 M2:0000 L2:0000 B2:0000 
; I3:0000 M3:0000 L3:0000 B3:0000 
; I4:1002 M4:0000 L4:000D B4:1000 
; I5:0000 M5:0005 L5:0000 B5:0000 
; I6:0000 M6:0000 L6:0000 B6:0000 
; I7:0000 M7:0000 L7:0000 B7:0000 
; PCSTACK:0000    LPSTACK:0000
; CNTR   :5444    LPEVER :0000
; SSTAT - PCE:1 PCF:0 PCL:1 LSE:1 LSF:0 SSE:1 SOV:0
; PC:0076 NextPC: FFFF
; Time: 121 cycles
; 
; End of Simulation!!


