; Author: YihTang Yeo
; Program: Reaction Timer
; Description: Translating C into Assembly, with reference to Patrick Payne's code
; LCD: 5x7 instead of 7 segment display
; Date: 26 August 2012

; --- constant macros ---

.EQU LCD_DataPort = PORTA
.EQU LCD_DataDDR = DDRA
.EQU LCD_DataPIN = PINA
.EQU LCD_CmdPort = PORTB
.EQU LCD_CmdDDR = DDRB
.EQU LCD_CmdPIN = PINB
.EQU LCD_RS = 0		; define shift for Register Select
.EQU LCD_RW = 1		; define shift for Read/Write
.EQU LCD_EN = 2		; define shift for Enable bit

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

	; set command and data ports to output
	LDI R21, 0xFF
	OUT LCD_DataDDR, R21
	OUT LCD_CmdDDR, R21

	CBI LCD_CmdPort, LCD_EN ; clear Enable pin
	CALL DELAY_2MS			; wait for 2ms for power on

	WRITE_CMD 0x38			; command to specify LCD 5x7	
	WRITE_CMD 0x0E			; command to turn display on, cursor on
	WRITE_CMD 0x01			; command to clear LCD screen	
	WRITE_CMD 0x06			; command to shift cursor right

; --- functions body below ---

EXT_INT0_ISR:
	RETI

EXT_INT1_ISR:
	RETI

TIMER1_COMPA_ISR: 
	RETI

DELAY_2MS:
	RET

LCD_WRITE_CMD:
	RET

LCD_WRITE_DATA:
	RET