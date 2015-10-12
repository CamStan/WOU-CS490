#include "mraa.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*
 * CS 490 Lab-2 Part-3
 *
 * A level that uses the accelerometer to read in x and y values
 * use them to determine if the accelerometer is level by lighting
 * a yellow LED when within 10% of being level and lighting a red LED
 * when within 1% of being level.
 *
 * Code that is commented out in this file can be used for printing out
 * values for testing purposes.
 *
 * @author Cameron Stanavige & Gene Osborne
 * @version 10/9/2015
 *
 */

/*
 * Constant x-axis values to determine what is level and the range.
 * xZeroG = value that determines when x is level
 * xPosG = value that determines when x is 1g
 * xNegG = value that determines when x is -1g
 * yZeroG = value that determines when y is level
 * yPosG = value that determines when y is 1g
 * yNegG = value that determines when y is -1g
 */
const float xZeroG = 1449;
const float xPosG = 1722;
const float xNegG = 1171;
const float yZeroG = 1437;
const float yPosG = 1720;
const float yNegG = 1152;

const float pi = 3.1415926535897;

// function prototypes
float normalize(unsigned int, float, float, float);
int isLevel(float, int);

int main(int argc, char* argv[]) {
	// set the pins to be used for the LEDs
	mraa_gpio_context rPin = mraa_gpio_init(7);
	mraa_gpio_context yPin = mraa_gpio_init(8);

	if (rPin == NULL || yPin == NULL) {
		fprintf(stderr, "Coulnd't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	// set the LED pins as output
	if (mraa_gpio_dir(rPin, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
		fprintf(stderr, "Cannot set D7 as output, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}
	if (mraa_gpio_dir(yPin, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
		fprintf(stderr, "Cannot set D8 as output, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	// set pins to be used by accelerometer
	mraa_aio_context xAxis = mraa_aio_init(0);
	mraa_aio_context yAxis = mraa_aio_init(1);
	// change the bit accuracy to 12 bits
	mraa_aio_set_bit(xAxis, 12);
	mraa_aio_set_bit(yAxis, 12);

	if (xAxis == NULL || yAxis == NULL) {
		fprintf(stderr, "Coulnd't initialize AIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	// variable declarations
	unsigned int xValue, yValue;
	float xNorm, yNorm;
	//int xLevel, yLevel;

	// loop to run program
	for (;;) {
		xValue = mraa_aio_read(xAxis);
		yValue = mraa_aio_read(yAxis);

		xNorm = normalize(xValue, xZeroG, xPosG, xNegG);
		yNorm = normalize(yValue, yZeroG, yPosG, yNegG);

		//xLevel = isLevel(xNorm, 10);
		//yLevel = isLevel(yNorm, 10);

		// light yellow LED when both x and y are within 10%
		if (isLevel(xNorm, 10) == 0 && isLevel(yNorm, 10) == 0)
			mraa_gpio_write(yPin, 1);
		else
			mraa_gpio_write(yPin, 0);
		// light red LED when both x and y are within 1%
		if (isLevel(xNorm, 1) == 0 && isLevel(yNorm, 1) == 0)
			mraa_gpio_write(rPin, 1);
		else
			mraa_gpio_write(rPin, 0);

		//printf("xAxis: %d, %f, %d yAxis: %d, %f, %d\n", xValue, xNorm, xLevel, yValue, yNorm, yLevel);
		//usleep(200000);
	}
	mraa_aio_close(xAxis);
	mraa_aio_close(yAxis);

	return 0;
} // end main

/*
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
	return reading > middle ? (reading - middle) / (max - middle) :	(reading - middle) / (middle - min);
} // end normalize

/*
 * Function to determine if the normalized value is within the specified
 * degree
 *
 * @param norm The normalized floating point value to be determined if it
 * 				if it is within the specified degree of tolerance
 * @param degree The degree of tolerance for norm to be within (1-90 degrees)
 *
 * @return 0 if is level; 1 if not level
 */
int isLevel(float norm, int degree) {
	return fabsf(asin(norm)) * (180/pi) < degree ? 0 : 1;
} //end isLevel
