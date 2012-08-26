/* Author: Patrick Payne
 * file: sevensegwithshifter.c
 * Purpose: To implement the functions in sevensegshifter.h. see that file for
 * documentation.
 */
 #include <util/delay.h>
 #include "sevensegwithshifter.h"
 /****************************FUNCTION DEFINITIONS******************************/

void setup_sevenseg()
{
    // set the necessary pins to digital output mode.
    SER_DDR |= 1 << SER_BIT;
    RCLK_DDR |= 1 << RCLK_BIT;
    SRCLK_DDR |= 1 << SRCLK_BIT;
    HUNDREDS_DDR |= 1 << HUNDREDS_BIT;
    TENS_DDR |= 1 << TENS_BIT;
    ONES_DDR |= 1 << ONES_BIT;
    
    // set all the digits to off initially (I.E. HIGH initial output).
    HUNDREDS_REG |= (1 << HUNDREDS_BIT);
    TENS_REG |= (1 << TENS_BIT);
    ONES_REG |= (1 << ONES_BIT);    
}


void shift_write_byte(unsigned char shift_byte)
{
    /* mask_table[i] contains the byte to be written to the shift register 
     * to display the numeral i. The numeral must be from 0-9. This mask
     * assumes that segment "a" to "h" correspond to the LSB to MSB.
     */
    unsigned char mask_table[] = {0x3F, 0x06, 0x5B, 0x4f, 0x66, 
                                 0x6D, 0x7D, 0x07, 0x7F, 0x67};
    
    // Set the register clock low
    RCLK_REG &= ~(1 << RCLK_BIT);
    
    for (int i = 0; i < 8; i++) {
        // Set the serial clock pin low
        SRCLK_REG &= ~(1 << SRCLK_BIT);
        
        // Write the serial input
        SER_REG |= ((mask_table[shift_byte] & (1 << i)) >> i) << SER_BIT;   
        
        // shift the register
        SRCLK_REG |= (1 << SRCLK_BIT);

        // Clear the serial input
        SER_REG &= ~(1 << SER_BIT); 
    }
    
    // Set the serial clock pin low
    SRCLK_REG &= ~(1 << SRCLK_BIT);
    
    // Set the register clock high
    RCLK_REG |= (1 << RCLK_BIT);
}


void disp_digit(unsigned char digit, unsigned char place)
{
    // These arrays contain the locational information for the digit bits
    static volatile uint8_t *place_reg[] = {&ONES_REG, &TENS_REG, &HUNDREDS_REG};
    static unsigned char place_bit[] = {ONES_BIT, TENS_BIT, HUNDREDS_BIT};
    
    // clear all digits (I.E. set them HIGH)
    HUNDREDS_REG |= (1 << HUNDREDS_BIT);
    TENS_REG |= (1 << TENS_BIT);
    ONES_REG |= (1 << ONES_BIT);
    
    // write the digit to the register
    shift_write_byte(digit);
    
    // activate given digit
    *(place_reg[place]) &= ~(1 << place_bit[place]);
}

/* FUNCTION: disp_number() -- displays the input number on the 7-segment display
 *
 * Displays the input number on the seven segment display. positive integers
 * with up to 3 digits can be displayed. the display is held for the indicated
 * number of cycles, each lasting approximately 6 ms.
 *
 * Parameters and preconditions:
 *    0 <= number <= 999: the integer to be displayed
 *    0 <= cycles <= 256: the number of cycles the digit should last
 *
 * Side-effects:
 *   The 7-segment display will show the desired number.
 */
void disp_number(unsigned short number, unsigned short cycles)
{
    static unsigned char digits[3];
    static unsigned short j;
    
    // Separating the 3 digit number into its constituent digits
    digits[0] = number % 10;
    digits[1] = (number / 10) % 10;
    digits[2] = number / 100;

    // Now we display the number
    for(j = 0; j < cycles; j++) {
        for (int k = 0; k < 3; k++) {
            disp_digit(digits[k], k);
            _delay_ms(3);
        }
    }
}