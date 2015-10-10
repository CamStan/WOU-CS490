#include "mraa.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * CS 490 Lab-2 Part-2
 *
 * Reads in the values from the x-axis on the accelerometer and
 * prints them out as a hexadecimal, an integer value, and a
 * floating point value normalized to +- 1g.
 *
 * @author Cameron Stanavige & Gene Osborne
 * @version 10/9/2015
 */

/*
 * Constant x-axis values to determine what is level and the range.
 * xZeroG = value that determines when x is level
 * xPosG = value that determines when x is 1g
 * xNegG = value that determines when x is -1g
 */
const float xZeroG = 1449;
const float xPosG = 1722;
const float xNegG = 1171;

// function prototypes
float normalize(unsigned int, float, float, float);

int main(int argc, char* argv[]) {
	// variable declarations
	unsigned int xValue;

	// set the pin to be used
	mraa_aio_context xAxis = mraa_aio_init(0);
	// change the bit accuracy to 12 bits
	mraa_aio_set_bit(xAxis, 12);

	if (xAxis == NULL) {
		fprintf(stderr, "Coulnd't initialize AIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	// loop to run program
	for (;;) {
		xValue = mraa_aio_read(xAxis);
		printf("xAxis: %x, %d, %f\n", xValue, xValue, normalize(xValue, xZeroG, xPosG, xNegG));
		usleep(200000);
	}
	mraa_aio_close(xAxis);
} // end main

/**
 * Function to normalize the value being read in to +- 1g.
 *
 * @param reading The value that was read in from the accelerometer
 * @param middle The value to determine what is zero
 * @param max The value to determine what is 1g
 * @param min The value to determine what is -1g
 *
 * @return Floating point value normalized between -1 and 1
 */
float normalize(unsigned int reading, float middle, float max, float min) {
	return reading > middle ? (reading - middle) / (max - middle) : (reading - middle) / (middle - min);
} // end normlize
