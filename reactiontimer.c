/* 
 * Author: Patrick Payne
 * Project: Reaction Timer
 * file: Reaction_Timer.c 
 * date created: 8/12/2012 1:53:08 PM
 * Purpose: This is a sketch for a reaction timer game. see file for details
 * of gameplay and electronics.
 */
 
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>


#include "sevensegwithshifter.h"
#include <util/delay.h>


/******************FUNCTION AND GLOBAL VARIABLE DECLARATIONS*******************/

/* setup(): sets up the display, the I/O ports, and the timer.
 *
 * sets up the relevant pins for the 7segment display (see sevensegwithshifter.h)
 * as well as pins 6-8 for input and output. Only activates the reset interrupt.
 * also sets up timer1 to count milliseconds.
 */
void setup();

/* g_timer is the game timer. It is only used during the main game phase */
unsigned short g_timer = 0;

/*
 *    game_active: 1 when the game is being played, 0 otherwise
 *    game_button_pressed: 1 if the game button has been pressed, 0 otherwise
 */
unsigned char game_active = 0;
unsigned char game_button_pressed = 0;

/*******************************MAIN FUNCTIONS*********************************/

int main(void)
{
	// Setup
	cli();
    setup();
	sei();
	
	// TODO: allow a reset from anywhere in the game loop
	// Loop
	for (;;) {
		// if the reset button has been pressed, start the game
		if (game_active == 1) {
			
			// delay, so the user has to anticipate the beep
			// TODO: introduce a random delay.
			_delay_ms(2000);
			
			// beep and flash LED
			// NOTE: It turns out the LED flash is too short to be visible
			PORTD |= 1 << PORTD7;
			TCCR0A |= (1 << COM0B1);
			_delay_ms(50);
			PORTD &= ~(1 << PORTD7);
			TCCR0A &= ~(1 << COM0B1);
			
			// activate timer, set to zero
			/* NOTE: the counting begins after the beep has finished. so
			 * technically, ~50 ms should be added to the count
			 */
			// TODO: figure out an ELEGANT method of starting the count
			// simultaneously with a timed beep.
			cli();
			g_timer = 0;
			// activate the timer1 (milliseconds) interrupt
			TIMSK1 |= 1 << OCIE1A;
			
			// activate the game button interrupt
			EIMSK |= 1 << INT0;
			game_button_pressed = 0;
			
			// counting loop, stop at 999 or at reset.
			// TODO: resolve bug where the count stops at ~15 ms without
			// the game button being pressed
			// TODO: resolve bug where timer stops at 989 or some other
			// number, rather than 999
			while (g_timer < 1000) {
				if (g_game_state & (1 << GAME_BUTTON_BIT)) break;
				
				sei();
				disp_number(g_timer, 10);
				cli();
			} /*while*/
			
			/* now the game is over, so reset the game state and stop interrupts */
			cli();
			game_button_pressed = 0;
			game_active = 0;
			
			// deactivate the timer1 interrupt and the game button interrupt
			TIMSK1 &= ~(1 << OCIE1A);
			EIMSK &= ~(1 << INT0);
		}/*if*/
		
		// While the game is in standby, display the reaction time from the last game
		disp_number(g_timer, 10);
	}
}

/****************************FUNCTION DEFINITIONS******************************/

/* setup(): sets up the display, the I/O ports, and the timer.
*
* sets up the relevant pins for the 7segment display (see sevensegwithshifter.h)
* as well as pins 6-8 for input and output. Specifies the interrupt behavior
* for both switches, but only activates the reset switch one.
*/
void setup(){
	// set up the seven segment display
	setup_sevenseg();
	
	/* set up Timer1 to count milliseconds. */
	TCCR1A &= 0;
	TCCR1B |= (1 << WGM12) | (1 << CS10);
	TCCR1B &= ~((1 << WGM13) | (1 << CS12) | (1 << CS11));
	OCR1A = 16000;
	
	// set up the led
	DDRD |= DDD6;
	PORTD &= ~(1 << PORTD6);
	
	// set up the switch inputs
	DDRD &= ~((1 << DDD2) | (1 << DDD3));
	PORTD &= ~((1 << PORTD2) | (1 << PORTD3));
	
	/* set up input interrupts, but only activate the game reset one. */
	// Set both interrupts to trigger on a rising edge, since both pins
	// are pulled low by default.
	EICRA |= (1 << ISC11) | (1 << ISC10) | (1 << ISC01) | (1 << ISC00);
	EIMSK |= 1 << INT1;
}

/***************************INTERRUPT SUBROUTINES******************************/
ISR(TIMER1_COMPA_vect)
{
	g_timer++;
}

// Reset button
ISR(INT1_vect)
{
	game_active = 1;
}	
//the game button
ISR(INT0_vect)
{
	game_button_pressed = 1;
}