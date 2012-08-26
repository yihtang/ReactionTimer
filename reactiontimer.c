/*
 * Author: Patrick Payne
 * Project: Reaction Timer
 * file: Reaction_Timer.c
 * date created: 8/12/2012 1:53:08 PM
 * Purpose: This is a sketch for a reaction timer game. see file for details
 * of gameplay and electronics.
 */

#define F_CPU 16000000
#define PRESCALE 1024
#define MAX_COUNT ((F_CPU)/ PRESCALE - 1) // the value that TCNT1 that leads to a 1s delay
#define DEBOUNCE_DELAY_MS 1

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
 *    game_active: 0 when the game hasn't started and is in idle, 
 *	               1 when the game just started, 2 when the LED lights are turned on
 *    game_button_pressed: 1 if the game button has been pressed, 0 otherwise
 */
volatile unsigned char game_active = 0;
volatile unsigned char game_button_pressed = 0;
volatile unsigned short last_game_count = 999;

/*******************************MAIN FUNCTIONS*********************************/

int main(void)
{
	// Setup
	cli();
    setup();    
	sei();

	// Loop
	while(1) {
		// if the reset button has been pressed, start the game
		if (game_active == 1) {
			
			// reset game count
			last_game_count = 0;
			
			// delay, so the user has to anticipate the beep
			// TODO: introduce a random delay.
            _delay_ms(2000);


			PORTD |= 1 << PORTD7;					// turn on LED
            TCNT1 = 0;								// begin TCNT1 calculation from 0 to 15624 which will be one second
            TIMSK1 |= 1 << OCIE1A;					// activate the timer1 interrupt
            TCCR1B |= ((1 << CS10) | (1 << CS12));	// reactivate timer
            
            // Clearing the external interrupt flags (by writing 1 to them; see data sheet)
            // The flags get set regardless of whether interrupts are enabled or not, so we must make sure
            // they are off before enabling the interrupts
            EIFR |= (1 << INTF1) | (1 << INTF0);
            EIMSK |= 1 << INT0;					// activate the game button interrupt
			game_active = 2;					// move into the main game stage
			
		}
		
		else if (game_active == 2 && game_button_pressed == 1){
			// deactivate the timer1 interrupt and the game button interrupt to prevent further interruptions when displaying on LCD
			TIMSK1 &= ~(1 << OCIE1A);
			EIMSK &= ~(1 << INT0);

            // deactivate timer1: Edit, I completely disable them using 000 in CS12:10
            TCCR1B &= ~((1 << CS10) | (1 << CS11) | (1 <<CS12));

            PORTD &= ~(1 << PORTD7);		// turn off LED

			// calculate the time passed since the beginning of the game
            // note the 2 ms offset, added due to the 2 ms debouncing wait in the INT0 ISR
			last_game_count = (TCNT1) / (MAX_COUNT/1000)  - DEBOUNCE_DELAY_MS;

			/* now the game is over, so reset the game state and stop interrupts */
			game_button_pressed = 0;
			game_active = 0;

		}/*if*/
        else if (game_active == 0) disp_number(last_game_count, 10); // displays the last reaction time
	} /*while*/
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
	 * after 1 second, the OCIE1A flag will be set, and we can reset the game play in the ISR */

    // WGM13:10 = 0100 - CTC mode, output compare OCR1A
    // both clock select bits off, until the timer is needed.
    TCCR1A &= 0;
    TCCR1B |= (1 << WGM12);
	TCCR1B &= ~((1 << WGM13));
	
	// set TOP as compare match with OCR1A
	OCR1A = (unsigned short) MAX_COUNT;

    // set up the led
	DDRD |= (1<<DDD6);
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
	if (game_active == 2)
	{
		game_active = 0; // game goes into reset after 1 second, which is when the OCIE1A flag is set
		last_game_count = 999; // when game ends
		
		// disable interrupts and stop timer after game goes into reset after 1s
		TIMSK1 &= ~(1 << OCIE1A);
		EIMSK &= ~(1 << INT0);
		TCCR1B &= ~((1 << CS10) | (1 << CS11) | (1 <<CS12));
        PORTD &= ~(1 << PORTD7);		// turn off LED
        
	}	
}

// Reset button
ISR(INT1_vect)
{
   // delay, in order to ignore switch bounce noise
    _delay_ms(DEBOUNCE_DELAY_MS);
	
	// only reset if the game is idle and the game button is still high.
    if (game_active == 0 && (PIND & (1 << PIND3)))
		game_active = 1;
}
//the game button
ISR(INT0_vect)
{
	// delay, in order to ignore switch bounce noise
    _delay_ms(DEBOUNCE_DELAY_MS);
	
    if (game_active == 2 && (PIND & (1 << PIND2))) // button press is only valid when game is active
		game_button_pressed = 1;
}
