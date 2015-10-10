#include "mraa.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * CS490 Lab-2 Part-1
 *
 * Uses the digital output pins to blink a red and yellow LED in the pattern:
 * Red on
 * Yellow on
 * Both on
 * Both off
 *
 * @author Cameron Stanavige
 * @version 10/7/2015
 *
 */

int main(int argc, char* argv[]) {
	// set the pins to be used
	mraa_gpio_context rPin = mraa_gpio_init(7);
	mraa_gpio_context yPin = mraa_gpio_init(8);

	if (rPin == NULL || yPin == NULL) {
		fprintf(stderr, "Coulnd't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	// set the pins as output
	if (mraa_gpio_dir(rPin, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
		fprintf(stderr, "Cannot set D7 as output, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}
	if (mraa_gpio_dir(yPin, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
		fprintf(stderr, "Cannot set D8 as output, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	// repeating loop
	for (;;) {
		mraa_gpio_write(rPin, 0);
		mraa_gpio_write(yPin, 0);
		sleep(1);
		mraa_gpio_write(rPin, 1);
		sleep(1);
		mraa_gpio_write(rPin, 0);
		mraa_gpio_write(yPin, 1);
		sleep(1);
		mraa_gpio_write(rPin, 1);
		sleep(1);
	}

	return EXIT_SUCCESS;
}
