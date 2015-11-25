#include "mraa.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <math.h>

/**
 * This program is simply set up to drive two different motors, a light, and a laser
 * to be used with the WOU CS490 3D Scanner project. This allows for the ability to
 * drive two different stepper motors by inputting the desired direction, speed in
 * degrees per second, and amount of degrees to move. This also allows for the ability
 * to control the brightness of both an LED and a laser by inputting a percentage for
 * the desired brightness in relation to a 100kHz output.
 *
 * @author Cameron Stanavige
 * @version 11/24/2015
 */

// Sparkfun Motor
#define SPARK				0 // motor ID
#define SPARK_ENABLE_PIN	4
#define SPARK_DIR_PIN		6
#define SPARK_STEP_PIN		7
#define SPARK_RES			0.0625 // amount moved per step = 0.9 * (1/16)

// Kysan Motor
#define KYSAN				1 // motor ID
#define KYSAN_ENABLE_PIN	8
#define KYSAN_DIR_PIN		9
#define KYSAN_STEP_PIN		10
#define KYSAN_RES			0.1125 // amount moved per step = 1.8 * (1/16)

// Laser and LED
#define LED_POWER_PIN		5
#define LASER_POWER_PIN 	2
#define LASER_VMOD_PIN		3

// parameter values
#define ENABLE				0 // motors
#define DISABLE				1
#define CLOCKWISE			0
#define COUNTERCLOCKWISE	1
#define DOWN				0
#define UP					1
// lights
#define OFF					0
#define ON					1

// contexts
mraa_gpio_context spark_enable;
mraa_gpio_context spark_dir;
mraa_gpio_context spark_step;

mraa_gpio_context kysan_enable;
mraa_gpio_context kysan_dir;
mraa_gpio_context kysan_step;

mraa_gpio_context laser_power;
mraa_pwm_context laser_Vmod;
mraa_pwm_context led_power;

// motor offsets (amount motors need to move to make up for inaccuracy the of prior move)
static float volatile sparkOffset;
static float volatile kysanOffset;
// track previous move direction to determine if offset needs to be switched
static int volatile sparkPrevDir;
static int volatile kysanPrevDir;

//#define QUIT_HANDLER // uncomment to allow for exiting from an infinite for loop

// function prototypes
void moveSpark(char, int, float);
void moveKysan(char, int, float);
unsigned int findSteps(int, float, char);
unsigned int getPeriod(int, char);

void setLEDLevel(int);
void setLaserLevel(int);

void danceDemo();
void cleanUp();
#ifdef QUIT_HANDLER
void quitHandler(int);
#endif

int main(int argc, char* argv[]) {

#ifdef QUIT_HANDLER
	signal(SIGINT, quitHandler); // set up quitHandler for safe exit from for loop
#endif

	// Sparkfun motor setup
	spark_enable = mraa_gpio_init(SPARK_ENABLE_PIN);
	spark_dir = mraa_gpio_init(SPARK_DIR_PIN);
	spark_step = mraa_gpio_init(SPARK_STEP_PIN);

	if (mraa_gpio_dir(spark_enable, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	if (mraa_gpio_dir(spark_dir, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	if (mraa_gpio_dir(spark_step, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	// Kysan motor setup
	kysan_enable = mraa_gpio_init(KYSAN_ENABLE_PIN);
	kysan_dir = mraa_gpio_init(KYSAN_DIR_PIN);
	kysan_step = mraa_gpio_init(KYSAN_STEP_PIN);

	if (mraa_gpio_dir(kysan_enable, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	if (mraa_gpio_dir(kysan_dir, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	if (mraa_gpio_dir(kysan_step, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	// LED setup
	led_power = mraa_pwm_init(LED_POWER_PIN);
	mraa_pwm_period_us(led_power, 10); // set to 100kHz

	// Laser setup
	laser_power = mraa_gpio_init(LASER_POWER_PIN);

	if (mraa_gpio_dir(laser_power, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}
	mraa_gpio_write(laser_power, ON);

	laser_Vmod = mraa_pwm_init(LASER_VMOD_PIN);
	mraa_pwm_period_us(laser_Vmod, 10); // set to 100kHz

	// default both motors to disabled
	mraa_gpio_write(spark_enable, DISABLE);
	mraa_gpio_write(kysan_enable, DISABLE);

	// set motor offsets to 0
	sparkOffset = 0.0;
	kysanOffset = 0.0;

	// set initial direction
	sparkPrevDir = CLOCKWISE;
	kysanPrevDir = CLOCKWISE;

	// default both lights to disabled
	mraa_pwm_enable(led_power, OFF);
	mraa_pwm_enable(laser_Vmod, OFF);

	//for () {
	// make it dance here
	danceDemo();
	//}

	// clean up
	cleanUp();

	return 0;
} // end main

/**
 * Moves the Sparkfun motor the desired degrees in the desired direction at the
 * desired speed.
 *
 * @param dir 		The direction to move the motor
 * @param dps 		The speed to move the motor in degrees per second
 * @param degrees 	The number of degrees to move the motor
 */
void moveSpark(char dir, int dps, float degrees) {
	struct timespec t1, t2; // struct for tracking period time
	unsigned int t;

	unsigned int steps = findSteps(dir, degrees, SPARK); // find number of steps to move
	unsigned int period = getPeriod(dps, SPARK); // find length of step period

	mraa_gpio_write(spark_enable, ENABLE); // enable motor
	mraa_gpio_write(spark_dir, dir); // set direction

	int i;
	for (i = 0; i < steps; i++) { // move desired number of steps
		clock_gettime(CLOCK_MONOTONIC, &t1); // get start time

		mraa_gpio_write(spark_step, UP); // write high

		t = 0;
		while (t < period / 2) { // wait half the period
			clock_gettime(CLOCK_MONOTONIC, &t2);
			t = t2.tv_nsec - t1.tv_nsec;
		}

		mraa_gpio_write(spark_step, DOWN); // write low

		while (t < period) { // wait rest of the period
			clock_gettime(CLOCK_MONOTONIC, &t2);
			t = t2.tv_nsec - t1.tv_nsec;
		}
	}
	mraa_gpio_write(spark_enable, DISABLE); // disable motor
} // end moveSpark

/**
 * Moves the Kysan motor the desired degrees in the desired direction at the
 * desired speed.
 *
 * @param dir 		The direction to move the motor
 * @param dps 		The speed to move the motor in degrees per second
 * @param degrees 	The number of degrees to move the motor
 */
void moveKysan(char dir, int dps, float degrees) {
	struct timespec t1, t2; // struct for tracking period time
	unsigned int t;

	unsigned int steps = findSteps(dir, degrees, KYSAN); // find number of steps to move
	unsigned int period = getPeriod(dps, KYSAN); // find length of step period

	mraa_gpio_write(kysan_enable, ENABLE); // enable motor
	mraa_gpio_write(kysan_dir, dir); // set direction

	int i;
	for (i = 0; i < steps; i++) { // moved desired number of steps
		clock_gettime(CLOCK_MONOTONIC, &t1); // get start time

		mraa_gpio_write(kysan_step, UP); // write high

		t = 0;
		while (t < period / 2) { // wait half the period
			clock_gettime(CLOCK_MONOTONIC, &t2);
			t = t2.tv_nsec - t1.tv_nsec;
		}

		mraa_gpio_write(kysan_step, DOWN); // write low

		while (t < period) { // wait rest of the period
			clock_gettime(CLOCK_MONOTONIC, &t2);
			t = t2.tv_nsec - t1.tv_nsec;
		}
	}
	mraa_gpio_write(kysan_enable, DISABLE); // disable motor
} // end moveKysan

/**
 * Determine the number of steps each motor needs to be pulsed in order to move the
 * desired number of degrees. Will accommodate for previous inaccurate moves if the
 * desired degrees did not equate to a whole number of steps.
 *
 * @param dir The direction the motor is moving on this rotation
 * @param degree The desired number of degrees to move the motor
 * @param motor The motor to determine the steps for: SPARK or KYSAN
 *
 * @return The number of steps needed to move the desired amount of degrees
 */
unsigned int findSteps(int dir, float degree, char motor) {
	float steps;
	float rSteps;
	if (motor == SPARK) { // SparkFun motor
		if (dir != sparkPrevDir) // check if direction changed
			sparkOffset *= (-1); // switch offset if direction changed
		degree -= sparkOffset; // make up for previous inaccuracy, if any
		steps = 1 / SPARK_RES * degree; // determine # of steps
		rSteps = roundf(steps); // round to whole number of steps
		sparkOffset = (rSteps - steps) / (1 / SPARK_RES); // calculate inaccuracy caused by roundf
	} else { // Kysan motor
		if (dir != kysanPrevDir) // check if direction changed
			kysanOffset *= (-1); // switch offset if direction changed
		degree -= kysanOffset; // make up for previous inaccuracy, if any
		steps = 1 / KYSAN_RES * degree; // determine # of steps
		rSteps = roundf(steps); // round to whole number of steps
		kysanOffset = (rSteps - steps) / (1 / KYSAN_RES); // calculate inaccuracy caused by roundf
	}
	return (unsigned int) rSteps;
} // end findSteps

/**
 * Determine the length of the step period in nanoseconds needed in order to move
 * the motor at the desired speed.
 *
 * @param dps The desired speed of the motor in degrees per second.
 * @param motor The motor to determine the speed for: SPARK or KYSAN
 *
 * @return The length of the step period in nanoseconds needed to move at the desired speed
 */
unsigned int getPeriod(int dps, char motor) {
	return motor == SPARK ?
			(unsigned int) roundf(1.0 / dps * 1000000000 * SPARK_RES) :
			(unsigned int) roundf(1.0 / dps * 1000000000 * KYSAN_RES);
} // end getPeriod

/**
 * Sets the brightness of the LED to the desired percentage.
 *
 * @param percent The desired brightness of the LED (0-100; set to 0 to turn off)
 */
void setLEDLevel(int percent) {
	if (percent == 0) { // turn off LED
		mraa_pwm_write(led_power, 0.0); // dim all the way
		mraa_pwm_enable(led_power, OFF); // disable LED
	} else {
		mraa_pwm_enable(led_power, ON); // enable light
		mraa_pwm_write(led_power, percent / 100.0); // set to desired level
	}
} // end setLEDLevel

/**
 * Sets the brightness of the Laser to the desired percentage.
 *
 * @param percent The desired brightness of the laser (0-100; set to 0 to turn off)
 */
void setLaserLevel(int percent) {
	if (percent == 0) { // turn off laser
		mraa_pwm_write(laser_Vmod, 0.0); // dim all the way
		mraa_pwm_enable(laser_Vmod, OFF); // disable laser
	} else {
		mraa_pwm_enable(laser_Vmod, ON); // enable laser
		mraa_pwm_write(laser_Vmod, percent / 100.0); // set to desired level
	}
} // end setLaserLevel

/**
 * Demo to make the motors, laser, and LED dance for 1 minute
 */
void danceDemo() {
	moveSpark(CLOCKWISE, 90, 90);
	moveSpark(COUNTERCLOCKWISE, 90, 90);
	moveKysan(CLOCKWISE, 18, 25);
	moveSpark(COUNTERCLOCKWISE, 90, 90);
	moveSpark(CLOCKWISE, 90, 90);
	moveKysan(COUNTERCLOCKWISE, 18, 25);

	setLEDLevel(25);
	moveSpark(CLOCKWISE, 90, 90);
	setLaserLevel(25);

	setLEDLevel(0);
	moveKysan(CLOCKWISE, 90, 90);
	setLaserLevel(0);

	setLEDLevel(15);
	moveSpark(COUNTERCLOCKWISE, 45, 45);
	setLaserLevel(15);

	setLEDLevel(0);
	moveKysan(COUNTERCLOCKWISE, 45, 45);
	setLaserLevel(1);

	setLEDLevel(1);
	int i;
	for (i = 0; i < 2; i++) {
		moveKysan(CLOCKWISE, 90, 90);
		moveKysan(COUNTERCLOCKWISE, 90, 90);
		moveSpark(CLOCKWISE, 18, 25);
		moveKysan(COUNTERCLOCKWISE, 90, 90);
		moveKysan(CLOCKWISE, 90, 90);
		moveSpark(COUNTERCLOCKWISE, 18, 25);

		setLEDLevel(35);
		moveKysan(CLOCKWISE, 90, 90);
		setLaserLevel(35);

		setLEDLevel(10);
		moveSpark(CLOCKWISE, 90, 90);
		setLaserLevel(10);

		setLEDLevel(50);
		moveKysan(COUNTERCLOCKWISE, 45, 45);
		setLaserLevel(50);

		moveSpark(COUNTERCLOCKWISE, 45, 45);
		setLEDLevel(75);
		setLaserLevel(75);

		moveKysan(CLOCKWISE, 20, 20);
		setLaserLevel(1);
		setLEDLevel(90);
		moveKysan(CLOCKWISE, 45, 45);
		setLEDLevel(75);
		moveKysan(CLOCKWISE, 90, 90);
		setLEDLevel(50);
		moveKysan(CLOCKWISE, 120, 120);
		setLEDLevel(25);
		moveKysan(CLOCKWISE, 180, 180);

		moveKysan(COUNTERCLOCKWISE, 180, 180);
		setLEDLevel(1);
		setLaserLevel(25);
		moveKysan(COUNTERCLOCKWISE, 120, 120);
		setLaserLevel(50);
		moveKysan(COUNTERCLOCKWISE, 90, 90);
		setLaserLevel(75);
		moveKysan(COUNTERCLOCKWISE, 45, 45);
		setLaserLevel(90);
		moveKysan(COUNTERCLOCKWISE, 20, 20);

		moveSpark(CLOCKWISE, 20, 20);
		setLEDLevel(1);
		setLaserLevel(90);
		moveSpark(CLOCKWISE, 45, 45);
		setLaserLevel(75);
		moveSpark(CLOCKWISE, 90, 90);
		setLaserLevel(50);
		moveSpark(CLOCKWISE, 120, 120);
		setLaserLevel(25);
		moveSpark(CLOCKWISE, 180, 180);

		moveSpark(COUNTERCLOCKWISE, 180, 180);
		setLaserLevel(1);
		setLEDLevel(25);
		moveSpark(COUNTERCLOCKWISE, 120, 120);
		setLEDLevel(50);
		moveSpark(COUNTERCLOCKWISE, 90, 90);
		setLEDLevel(75);
		moveSpark(COUNTERCLOCKWISE, 45, 45);
		setLEDLevel(90);
		moveSpark(COUNTERCLOCKWISE, 20, 20);

		setLEDLevel(0);
		setLaserLevel(0);
	}
}

/**
 * Cleans up the program after running the code by freeing the memory used for the
 * different contexts.
 */
void cleanUp() {
	mraa_gpio_write(spark_enable, DISABLE);
	mraa_gpio_write(kysan_enable, DISABLE);

	mraa_gpio_close(spark_enable);
	mraa_gpio_close(spark_dir);
	mraa_gpio_close(spark_step);

	mraa_gpio_close(kysan_enable);
	mraa_gpio_close(kysan_dir);
	mraa_gpio_close(kysan_step);

	mraa_pwm_write(led_power, 0.0);
	mraa_pwm_write(laser_Vmod, 0.0);

	mraa_pwm_enable(led_power, OFF);
	mraa_pwm_enable(laser_Vmod, OFF);
	mraa_gpio_write(laser_power, OFF);

	mraa_pwm_close(led_power);
	mraa_pwm_close(laser_Vmod);
	mraa_gpio_close(laser_power);
} // end cleanUp

#ifdef QUIT_HANDLER
/**
 * Function used to safely exit an infinite for loop (if used) watching for the command
 * of CTRL_C. Once called, will write all pins low and free the memory used before exiting.
 *
 * @param sig The signal or command to watch for.
 */
void quitHandler(int sig) {
	if (sig == SIGINT) {
		printf("Exiting");
		mraa_gpio_write(spark_enable, DISABLE);
		mraa_gpio_write(kysan_enable, DISABLE);
		mraa_pwm_write(led_power, 0.0);
		mraa_pwm_write(laser_Vmod, 0.0);

		mraa_pwm_enable(led_power, OFF);
		mraa_pwm_enable(laser_Vmod, OFF);
		mraa_gpio_write(laser_power, OFF);

		mraa_gpio_close(spark_enable);
		mraa_gpio_close(spark_dir);
		mraa_gpio_close(spark_step);
		mraa_gpio_close(kysan_enable);
		mraa_gpio_close(kysan_dir);
		mraa_gpio_close(kysan_step);
		mraa_pwm_close(led_power);
		mraa_pwm_close(laser_Vmod);
		mraa_gpio_close(laser_power);
	}
	exit(EXIT_SUCCESS);
} // end QUIT_HANDLER
#endif
