#include <stdio.h>
#include <stdlib.h>
#include <mraa.h>

/*
 * Button is setup just for button A using debouncing to watch and print to the
 * console when button A is pressed and released.
 *
 * Cameron Stanavige
 * CS490
 * Lab3
 */

// pins associated with each button
#define PIN_A  		49
//#define PIN_B  	46
//#define PIN_UP  	47
//#define PIN_DOWN	44
//#define PIN_LEFT 	165
//#define PIN_RIGHT  	45
//#define PIN_SELECT  48

// struct to determine the debounced state of each button
union {
	unsigned char Buttons;
	struct {
		unsigned char Buttons_A :1;
		unsigned char Buttons_B :1;
		unsigned char Buttons_Up :1;
		unsigned char Buttons_Down :1;
		unsigned char Buttons_Left :1;
		unsigned char Buttons_Right :1;
		unsigned char Buttons_Select :1;
		unsigned char Buttons_Bit7 :1;
	} Buttons_bit;
} Buttons;

// simplified reference to each debounced button
#define DB_A 	Buttons.Buttons_bit.Buttons_A
#define BIT7	0x80

// debounce thresholds
#define PRESS_THRESHOLD 	0x3F
#define RELEASE_THRESHOLD 	0xFC

int main() {
	// button A setup
	mraa_gpio_context button_A = mraa_gpio_init_raw(PIN_A);

	if (mraa_gpio_dir(button_A, MRAA_GPIO_IN) != MRAA_SUCCESS) {
		fprintf(stderr, "Coulnd't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	Buttons.Buttons = 0xFF;

	int value_A;

	for (;;) {
		// shift register set to all 1s
		static uint8_t shiftReg = 0xFF;

		// read in of current state of button A
		value_A = mraa_gpio_read(button_A);

		// shift in a 0 to the register
		shiftReg >>= 1;

		// reset register if A is not pressed
		if (value_A == 1) {
			shiftReg |= BIT7;
		}

		// determine if A has been pressed/released and satisfies the threshold
		if (DB_A == 0) {
			if (shiftReg >= RELEASE_THRESHOLD) {
				printf("Button A released\n");
				DB_A = 1;
			}
		} else {
			if (shiftReg <= PRESS_THRESHOLD) {
				printf("Button A pressed\n");
				DB_A = 0;
			}
		}
		usleep(5000);
	} // end for loop

	mraa_gpio_close(button_A);

	return MRAA_SUCCESS;
} // end main

