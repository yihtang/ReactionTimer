; Author: YihTang Yeo
; Program: Reaction Timer
; Description: Translating C into Assembly, with reference to Patrick Payne's code
; LCD: 5x7 instead of 7 segment display
; Date: 26 August 2012

; NOTE: R16 register is reserved for command and data sending for LCD. Be careful when using this register.
; NOTE: Due to the limitations of Assembly, this program can only work with ATmega328 with 16MHz clock speed

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

	SEI				; set global interrupt

	INF_LOOP:		; loop forever

		JMP PRINTRESULT_LCD				; print result to LCD
		
		SUBI GAME_ACTIVE, 1			; if GAME_ACTIVE - 1 = 0, Z = 1
		BREQ STARTGAME				; Branch if Z = 1 - note: short jump, must be within 64bytes of PC

		SUBI GAME_ACTIVE, 2			; if GAME_ACTIVE - 2 = 0, Z = 1
		BREQ CHECK_BUTTON_PRESSED	; Branch to check if GAME_BUTTON_PRESSED = 1
		
	RJMP INF_LOOP

	; --- game states ---

	STARTGAME:
		LDI GAME_RESULT_H, 0		; reset to 0
		LDI GAME_RESULT_L, 0		; reset to 0
		JMP DELAY_2S				; delay for 2s
		SBI PORTD, 6				; turn on LED on PD6
		LOADIO TCNT1H, 0			; reset TCNT1 to 0 (always write to high first)
		LOADIO TCNT1L, 0			; reset TCNT1 to 0
		LOADIO TIMSK1, (1<<OCIE1A)	; activate timer 1 interrupt
		LOADIO TCCR1B, ((1<<CS10) | (1<<CS12))	; start timer to prescaler of 1024
		LOADIO EIFR, ((1<<INTF1) | (INTF0))		; clear interrupt flag by writing 1 to it
		LOADIO EIMSK, (1<<INT0)		; activate game button interrupt
		LDI GAME_ACTIVE, 2			; set game_state into 2
		RJMP INF_LOOP				; go back to the infinite loop

	CHECK_BUTTON_PRESSED:			; routine to check if button is pressed
		SUBI GAME_BUTTON_PRESSED, 1	; if GAME_BUTTON_PRESSED - 1 = 0, Z = 1
		BREQ STOPGAME				; Branch to STOPGAME if button is pressed
		RJMP INF_LOOP				; go back to the infinite loop

	STOPGAME:
		CBI TIMSK1, OCIE1A			; clear interrupt for Timer1 Output CompA match
		CBI EIMSK, INT0				; clear interrupt for external button interrupt
		LOADIO TCCR1B, 0b00001000	; turn off CS12:10 to stop timer counting
		CBI PORTD, 6				; turn off LED
		; send value of TCNT1 into registers, use IN for from I/O to Register
		IN GAME_RESULT_H, TCNT1H
		IN GAME_RESULT_L, TCNT1L
		; note: TCNT/(MAX_COUNT/1000) = TCNT/15.625.  We can go for divide 2 for 4 times
		; not as accurate because TCNT/16 will give some different results
		; ROR (rotate right) helps to give division by 2. the C flag will set if the LSB is 1
		; note: we are dealing with 16bit value, which needs a little bit of trick
		LDI	R18, 4						; loop for 4 times, divide 2^4 = 16
		LOOP_8_TIMES: 
			CLC							; clear C = 0 so 0 will always enter the MSB of GAME_RESULT_H
			ROR GAME_RESULT_H			; if the LSB of GAME_RESULT_H is 1, C = 1, and ...
			ROR GAME_RESULT_L			; ... upon next ROR, it will go into the the MSB of GAME_RESULT_L
			DEC R18
			BRNE LOOP_8_TIMES			; keep doing it until R18 = 0
		JMP INF_LOOP					; go back to the infinite loop


; --- functions body below ---

EXT_INT0_ISR:
	PUSH R17
	CALL DELAY_1MS				; delay to solve debouncing button issues
	CPI GAME_ACTIVE, 2			; check if GAME_ACTIVE = 2
	BRNE GOBACK_INT0			; if it's not (Z = 0, result not 0), then return immediately
	IN R17, PIND				; read PIND values into Register
	SBRS R17, 2					; skip next line if bit #2 is set
	RJMP GOBACK_INT0			; this line will only be executed when R17 bit #2 is cleared
	LDI GAME_BUTTON_PRESSED, 1	; register user input and set button as pressed
	GOBACK_INT0:
		POP R17
		RETI

EXT_INT1_ISR:
	PUSH R17
	CALL DELAY_1MS				; delay to solve debouncing button issues
	CPI GAME_ACTIVE, 0			; check if GAME_ACTIVE = 0
	BRNE GOBACK_INT1			; if it's not (Z = 0, result not 0), then return immediately
	IN R17, PIND				; read PIND values into Register
	SBRS R17, 3					; skip next line if bit #3 is set
	RJMP GOBACK_INT1			; this line will only be executed when R17 bit #3 is cleared
	LDI GAME_ACTIVE, 1			; set GAME_ACTIVE mode to start LED blinking 
	GOBACK_INT1:
		POP R17
		RETI

TIMER1_COMPA_ISR: 	
	CPI GAME_ACTIVE, 2			; check if GAME_ACTIVE = 2
	BRNE GOBACK_COMPA			; if it's not (Z = 0, result not 0), then return immediately
	LDI GAME_ACTIVE, 0			; game goes back to IDLE mode
	LDI GAME_RESULT_H, HIGH(999)
	LDI GAME_RESULT_L, LOW(999)

	; disable interrupts
	CBI TIMSK1, OCIE1A
	CBI EIMSK, INT0
	LOADIO TCCR1B, 0b00001000	; turn off CS12:10 to stop timer counting
	CBI PORTD, 6				; turn off LED as well
	GOBACK_COMPA: 
		RETI

; --- delay routines for LCD ---
; 16 MHz MCU - each tick is 0.0625 us
; note: these delays aren't exact, there are overhead and stuffs that are not included in calculations
;		but it serves the purpose of waiting for a certain time as a delay

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

DELAY_2S:
	PUSH R17
	PUSH R18
	LDI R17, 100			; outer loop, repeat 100 times
	LOOP2S_OUT:		
		LDI R18, 10			; inner loop, repeat 10 times
		LOOP2S_IN:
			CALL DELAY_2MS
			DEC R18
			BRNE LOOP2S_IN
		DEC R17
		BRNE LOOP2S_OUT
	POP R18					; Note: Stack - Last in first out
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

PRINTRESULT_LCD:

	RET