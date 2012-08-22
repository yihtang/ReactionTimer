/*
 * Author: Patrick Payne
 * Project: Reaction Timer
 * file: Reaction_Timer.c
 * date created: 8/12/2012 1:53:08 PM
 * Purpose: This is a sketch for a reaction timer game. see file for details
 * of gameplay and electronics.
 */

#define F_CPU 16000000
#define MAX_COUNT ((F_CPU)/ 1024 - 1) // the value that TCNT1 that leads to a 1s delay

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
 *    game_active: 1 when the game just started, 0 otherwise, 2 when the LED lights off
 *    game_button_pressed: 1 if the game button has been pressed, 0 otherwise
 */
unsigned char game_active = 0;
unsigned char game_button_pressed = 0;
unsigned short last_game_count = 0;

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

			last_game_count = 0;
			// delay, so the user has to anticipate the beep
			// TODO: introduce a random delay.
		    // FIXED BUG: without the cli() and sei(), the player could press the game button
            // before the delay was done.
       		// cli(); COMMENT: I removed this cli() statement, because I added an IF statement in ISR(INT0)
            _delay_ms(2000);


			PORTD |= 1 << PORTD7;			// turn on LED
            TCNT1 = 0;						// begin TCNT1 calculation from 0 to 15624 which will be one second
            TIMSK1 |= 1 << OCIE1A;			// activate the timer1 interrupt
            TCCR1B |= ((1 << CS10) | (1 << CS12)); // reactivate timer
            EIMSK |= 1 << INT0;				// activate the game button interrupt
			// sei(); COMMENT: I removed this sei() statement, because I added an IF statement in ISR(INT0)
			game_active = 2; // go into state where LED blinks off
			
			
		}
		
		else if (game_active == 2 && game_button_pressed == 1){
			// deactivate the timer1 interrupt and the game button interrupt to prevent further interruptions when displaying on LCD
			TIMSK1 &= ~(1 << OCIE1A);
			EIMSK &= ~(1 << INT0);

            // deactivate timer1: Edit, I completely disable them using 000 in CS12:10
            TCCR1B &= ~((1 << CS10) | (1 << CS11) | (1 <<CS12));

            PORTD &= ~(1 << PORTD7);		// turn off LED

			/* because TCNT1 is interrupt when the game button is pressed, it'll not count to 15624 but somewhere in between.
			* thus, read the current value of TCNT1 and divide it by a constant to get time in milliseconds.
			* 15624 = 1000 ms, thus 1 ms = 15.625.  If decimals are not allowed in this calculation, then go with 16.
			* it'll be a little bit inaccurate, but should not be too big of a deal. Error = 2.3%.*/
			last_game_count = TCNT1 / (MAX_COUNT/1000);

			/* now the game is over, so reset the game state and stop interrupts */
			game_button_pressed = 0;
			game_active = 0;

		}/*if*/

        disp_number(last_game_count, 10); // displays the last reaction time
	} /*for*/
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

	// determined from using the formula [desired_time_in_sec * (clock_speed_in_Hz / prescaler) - 1]
	// in this case it would be
	OCR1A = (unsigned short) MAX_COUNT;

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
	if (game_active == 2)
	{
		game_active = 0; // game goes into reset after 1 second, which is when the OCIE1A flag is set
		last_game_count = 999; // when game ends
		
		// disable interrupts and stop timer after game goes into reset after 1s
		TIMSK1 &= ~(1 << OCIE1A);
		EIMSK &= ~(1 << INT0);
		TCCR1B &= ~((1 << CS10) | (1 << CS11) | (1 <<CS12));
	}	
}

// Reset button
ISR(INT1_vect)
{
	/* comment off this IF statement, if you want to allow RESET from anywhere */
	if (game_active == 0) // only allows restarting the game when game is inactive
		game_active = 1;
}
//the game button
ISR(INT0_vect)
{
	if (game_active == 2) // button press is only valid when game is active
		game_button_pressed = 1;
}
