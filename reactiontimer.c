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

/*
 *    game_active: 1 when the game is being played, 0 otherwise
 *    game_button_pressed: 1 if the game button has been pressed, 0 otherwise
 */
unsigned char game_active = 0;
unsigned char game_button_pressed = 0;

/*******************************MAIN FUNCTIONS*********************************/

int main(void)
{
	unsigned int g_timer;
	
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
			
			PORTD |= 1 << PORTD7;			// turn on LED
			_delay_ms(100);					// change LED flash to be 0.1s to be visible
			TCNT1 = 0;						// begin TCNT1 calculation from 0 to 15624 which will be one second
			TIMSK1 |= 1 << OCIE1A;			// activate the timer1 (milliseconds) interrupt
			EIMSK |= 1 << INT0;				// activate the game button interrupt
			game_button_pressed = 0;
			PORTD &= ~(1 << PORTD7);		// turn off LED			
			
			// stays in the loop forever until button is pressed, or reset is played
			while (1){
				
				if (game_button_pressed == 1){
					disp_number(TCNT1 / 15.625, 10);
					break;
				}
				else if (game_active == 0){
					disp_number(1000, 0); // Note: because of 16bit data issue, I choose to show the maximum time instead of the last game time
					break;
				}
				
			}
			
			g_timer = TCNT1 / 15.625; // not sure if this operation is allowed
			
				
			
			/* now the game is over, so reset the game state and stop interrupts */\
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
	
	/* set up Timer1 to count TCNT1 from 0 to 15624, which will take 1 second. 
	 * after 1 second, the OCF1A flag will be set, and we can reset the game play in the ISR */
	TCCR1A &= 0;
	TCCR1B |= (1 << WGM12) | (1 << CS10) | (1 << CS12); // CS12:10 = 101 - prescaler 1024
	TCCR1B &= ~((1 << WGM13) | (1 << CS11)); // WGM13:10 = 0100 - CTC mode, output compare OCR1A
	
	// determined from using the formula [desired_time_in_sec * (clock_speed_in_Hz / prescaler) - 1]
	// in this case it would be 1s * 16000000 / 1024 - 1 = 15624
	OCR1A = 15624; 
	
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
	game_active = 0; // game goes into reset after 1 second, which is when the OCIE1A flag is set
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