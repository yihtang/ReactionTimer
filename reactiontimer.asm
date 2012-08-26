; Author: YihTang Yeo
; Program: Reaction Timer
; Description: Translating C into Assembly, with reference to Patrick Payne's code
; LCD: 5x7 instead of 7 segment display
; Date: 26 August 2012

; NOTE: R16 register is reserved for command and data sending for LCD. Be careful when using this register.

; don't know if this exists!
.INCLUDE "M328DEF.INC" 

; --- constant macros ---
; LCD takes up PORTA0-7 for 8bit data
; LCD takes up PORTB0-2 for RS, R/W, EN

.EQU LCD_DataPort = PORTA
.EQU LCD_DataDDR = DDRA
.EQU LCD_DataPIN = PINA
.EQU LCD_CmdPort = PORTB
.EQU LCD_CmdDDR = DDRB
.EQU LCD_CmdPIN = PINB
.EQU LCD_RS = 0		; define shift for Register Select
.EQU LCD_RW = 1		; define shift for Read/Write
.EQU LCD_EN = 2		; define shift for Enable bit

.EQU GAME_ACTIVE = R22
.EQU GAME_BUTTON_PRESSED = R23
.EQU GAME_RESULT_H = R24
.EQU GAME_RESULT_L = R25

.EQU F_CPU = 16000000
.EQU PRESCALE = 1024
.EQU MAX_COUNT = ((F_CPU)/ PRESCALE - 1) // the value that TCNT1 that leads to a 1s delay

; macro: simplifies the command writing to LCD. Delay 2ms after every command.
.MACRO WRITE_CMD
	LDI R16, @0
	CALL LCD_WRITE_CMD
	CALL DELAY_2MS
.ENDMACRO

; macro: simplifies the data writing to LCD.
.MACRO WRITE_DATA
	LDI R16, @0
	CALL LCD_WRITE_DATA
.ENDMACRO

; macro: simplifies to load I/O process, using R18 as temporary register
.MACRO LOADIO
	LDI R18, @1
	OUT @0, R18
.ENDMACRO

; --- Interrupt jump ---

.ORG 0x00
	JMP MAIN
.ORG 0x02
	JMP EXT_INT0_ISR
.ORG 0x04
	JMP EXT_INT1_ISR
.ORG 0x0E
	JMP TIMER1_COMPA_ISR

;--- main program begins here ---

MAIN:
	; initialize stack pointer to be at the bottom of stack
	LDI R21, HIGH(RAMEND)
	OUT SPH, R21
	LDI R21, LOW(RAMEND)
	OUT SPL, R21

	; set command and data ports to output for LCD
	LDI R21, 0xFF
	OUT LCD_DataDDR, R21
	OUT LCD_CmdDDR, R21

	CBI LCD_CmdPort, LCD_EN ; clear Enable pin
	CALL DELAY_2MS			; wait for 2ms for power on

	WRITE_CMD 0x38			; command to specify LCD 5x7	
	WRITE_CMD 0x0E			; command to turn display on, cursor on
	WRITE_CMD 0x01			; command to clear LCD screen	
	WRITE_CMD 0x06			; command to shift cursor right

	CLI						; clear interrupts
	; setup the game
	LDI GAME_ACTIVE, 0
	LDI GAME_BUTTON_PRESSED, 0
	LDI GAME_RESULT_H, HIGH(999)
	LDI GAME_RESULT_L, LOW(999)
	
	; setup timer to count from 0 to 15624 which will take 1 sec
	LOADIO TCCR1A, 0				; WGM13:10 = 0100 CTC mode with OCR1A
	LOADIO TCCR1B, 0b00001000		; don't turn on CS12:10 first
	LOADIO OCR1AH, HIGH(MAX_COUNT)	; load High bit first before Low bit
	LOADIO OCR1AL, LOW(MAX_COUNT)	; load OCR1A with 1 second delay

	; setup LED and switches
	SBI DDRD, 6		; LED on PD6 as output
	CBI PORTD, 6	; make sure LED is off
	CBI DDRD, 2		; switch on PD2 as input
	CBI DDRD, 3		; switch on PD3 as input
	CBI PORTD, 2
	CBI PORTD, 3

	; set up input interrupts, but only activate the game reset one. 
	; Set both interrupts to trigger on a rising edge, since both are pulled low by default.
	LOADIO EICRA, ((1 << ISC11) | (1 << ISC10) | (1 << ISC01) | (1 << ISC00))
	LOADIO EIMSK, (1<<INT1)




; --- functions body below ---

EXT_INT0_ISR:
	RETI

EXT_INT1_ISR:
	RETI

TIMER1_COMPA_ISR: 
	RETI

; --- delay routines for LCD ---
; 16 MHz MCU - each tick is 0.0625 us

DELAY_1US:
	PUSH R17
	LDI R17, 4	
	LOOP1US:	NOP				; waste 1 clock cycle for delay
				DEC R17			; decrement for it to become 0
				BRNE LOOP1US	; skip this statement once R17 becomes 0
	POP R17
	RET

	; calculation: 
	; 3 times will go through NOP, DEC, BRNE which sum up to 4 clock cycles
	; 1 time will go through NOP, DEC which sum up to 3 clock cycles
	; RET takes 4 clock cycles
	; TOTAL: 19 clock cycles - 1.2 us (not that accurate but ok)

DELAY_100US:
	PUSH R17
	LDI R17, 100
	LOOP100US:	CALL DELAY_1US
				DEC R17
				BRNE LOOP100US
	POP R17
	RET

DELAY_1MS:
	PUSH R17
	LDI R17, 10
	LOOP1MS:	CALL DELAY_100US
				DEC R17
				BRNE LOOP1MS
	POP R17
	RET

DELAY_2MS:
	PUSH R17
	LDI R17, 20
	LOOP2MS:	CALL DELAY_100US
				DEC R17
				BRNE LOOP2MS
	POP R17
	RET

; --- LCD routines ---

LCD_WRITE_CMD:
	OUT LCD_DataPort, R16
	CBI LCD_CmdPort, LCD_RS		; RS = 0 for command
	CBI LCD_CmdPort, LCD_RW		; RW = 0 for writing
	SBI LCD_CmdPort, LCD_EN		; EN = 1 to generate high pulse
	CALL DELAY_1US
	CBI LCD_CmdPort, LCD_EN		; EN = 1 to generate low pulse
	CALL DELAY_100US
	RET

LCD_WRITE_DATA:
	OUT LCD_DataPort, R16
	SBI LCD_CmdPort, LCD_RS		; RS = 1 for data
	CBI LCD_CmdPort, LCD_RW		; RW = 0 for writing
	SBI LCD_CmdPort, LCD_EN		; EN = 1 to generate high pulse
	CALL DELAY_1US
	CBI LCD_CmdPort, LCD_EN		; EN = 1 to generate low pulse
	CALL DELAY_100US
	RET