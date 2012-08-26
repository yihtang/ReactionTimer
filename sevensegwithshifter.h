/* Author: Patrick Payne
 * file: sevenseg-blink.h
 * Purpose: To blink any given numeral on the 7-segment display. This is an
 * upgrade over the older method I used, where 11 digital pins were required.
 * 6 MCU pins are required. Their functions are as follows:
 *  SER, serial input for the bit-shifter
 *  RCLK, if set high writes the register values to the output.
 *  SRCLK, if set high shifts the register.
 *
 *  HUNDREDS, when set LOW turns on the hundreds digit display.
 *  TENS, when set LOW turns on the tens digit display.
 *  ONES, when set LOW turns on the ones digit display.
 * 
 * Terminal Layout on board: |+|-|SER|RCLK|SRCLK|100|10|0|
 */

#ifndef SEVEN_SEG_H
#define SEVEN_SEG_H 
#include <avr/interrupt.h>
#include <avr/io.h>
/******************************MACRO DEFINITIONS*******************************/

#define F_CPU 16000000

// Define the location of the DDR registers used by the display and shifter
#define SER_DDR DDRB
#define RCLK_DDR DDRB
#define SRCLK_DDR DDRB
#define HUNDREDS_DDR DDRB
#define TENS_DDR DDRB
#define ONES_DDR DDRB

// Define which registers contain the output values
#define SER_REG PORTB
#define RCLK_REG PORTB
#define SRCLK_REG PORTB
#define HUNDREDS_REG PORTB
#define TENS_REG PORTB
#define ONES_REG PORTB

// Define which position in the above registers each bit occupies
#define SER_BIT PORTB0
#define RCLK_BIT PORTB1
#define SRCLK_BIT PORTB2
#define HUNDREDS_BIT PORTB3
#define TENS_BIT PORTB4
#define ONES_BIT PORTB5
 
/******************FUNCTION AND GLOBAL VARIABLE DECLARATIONS*******************/
 
/* FUNCTION: setup_sevenseg() -- Sets up the pins needed to drive the display
 *
 * initialises all the digital output pins that are needed for the display, and
 * gives them initial values.
 *
 *
 * Side-effects:
 *   The digital pins connected to the display and shifter are set to output.
 */
void setup_sevenseg();                             

/* FUNCTION: shift_write_byte() -- writes a byte to the shift register
 *
 * returns the mask needed to flash the desired numeral. Assumes that ports
 * D0 to D7 were used, and correspond sequentially to the assigned alphabetical
 * code used for seven segment displays. Dependent on macros defining bit
 * locations.
 *
 * Parameters and preconditions:
 *    shift_byte - the byte to be written to the shift register.
 *
 * Return value:
 *    NONE
 *
 * Side-effects:
 *   The bit is written to the register, and set as its output.
 */
void shift_write_byte(unsigned char shift_byte);

/* FUNCTION: disp_digit() -- displays the digit on the current 7-segment display
 *
 * Displays the input digit in the stated position, with 0 being the ones 
 * column. Depends on the macros that give the bit locations.
 *
 * Parameters and preconditions:
 *    digit - the digit to be displayed
 *    place - The column of the digit (0 in the ones column)
 *
 * Return value:
 *    NONE
 *
 * Side-effects:
 *   The 7-segment display will show the desired digit in the given position,
 *   until a new digit is displayed.
 */
void disp_digit(unsigned char digit, unsigned char place);

/* FUNCTION: disp_number() -- displays the input number on the 7-segment display
 *
 * Displays the input number on the seven segment display. positive integers
 * with up to 3 digits can be displayed. the display is held for the indicated
 * number of cycles, each lasting approximately 0.73 ms.
 *
 * Parameters and preconditions:
 *    0 <= number <= 999: the integer to be displayed
 *    0 <= cycles <= 256: the number of cycles the digit should last
 *
 * Side-effects:
 *   The 7-segment display will show the desired number.
 */
void disp_number(unsigned short number, unsigned short cycles);

#endif