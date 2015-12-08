/**
 * @file
 * @brief Initialize a button on an Intel Edison SparkFun Block to call the desired
 * function when pressed. Individual pthreads are created to watch each individual
 * button. Uses the MRAA library and implements button debouncing to prevent multiple
 * funciton calls from a single button press. This library assumes the use of an
 * Intel Edison SOC with a Linux OS as it utilizes pthreads.
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <mraa.h>


/**
 * Raw pins values used by the Intel Edison for each button
 */
#define PIN_A  		49
#define PIN_B  		46
#define PIN_UP  	47
#define PIN_DOWN	44
#define PIN_LEFT 	165
#define PIN_RIGHT  	45
#define PIN_SELECT  48

/**
 * 8-bit binary value of 1000 0000: Used in the button debouncing as a reset for the
 * button's shift register
 */
#define BIT_MASK	0x80

/**
 * 8-bit binary values used as thresholds for the button debouncing to determine when
 * the button has actually been pressed or released:
 * 		PRESS_THRESHOLD = 0011 1111
 *   	RELEASE_THRESHOLD = 1111 1100
 */
#define PRESS_THRESHOLD 	0x3F
#define RELEASE_THRESHOLD 	0xFC

/**
 * Struct containing pertinent information to be passed to the thread that will
 * be watching this button. Includes:
 * 		GPIO context from the MRAA library
 * 		Pointer to the funciton this button will call
 * 		Bit used for this button's debouncing
 * 		Pointer to the thread that will be watching this button
 */
struct button_context {
	mraa_gpio_context gpio_button_context;
	void (*action)();
	unsigned char bits;
	uint8_t endFlag;
	pthread_t * thread;
};

/**
 * Initialize a Button  Call this to create a thread to watch the desired button
 * 						that will call the desired fucntion when pressed.
 * 						e.g.: If you want the A Button pressed to excute the function *move_mtr(),
 * 		      				write:		button_init(PIN_A, buttonA, move_mtr);
 * @param int   The pin for the desired button to initialize.
 * @param void* The address to the function for this button to execute
 *
 * @return      A pointer to the button_context struct initialized to operate the
 *               desired button. The button_close() function frees this and other
 *               associated pointers.
 */
struct button_context * button_init(int, void (*)());

/**
 * Function to be watched by the thread that is responsible for debouncing this button
 * and calling the funciton that has been assigned to this button.
 *
 * @param void* Pointer to the function to be called by this button
 *
 * @return      Pointer to this function to allow the passing of this function to the
 *              pthread that will be watching this button.
 */
void * buttonThread(void*);

/**
 * Change/Update the callback function to be called by the desired button_context.
 *
 * @param button_context Pointer to the button context to be updated
 * @param void*          Pointer to the new function for this button to inact
 */
void button_update_function(struct button_context*, void*);

/**
 * Deallocate memory for a button_context and its members.
 *
 * @param button_context Pointer to the button to close.
 */
void button_close(struct button_context*);

#ifdef __cplusplus
}
#endif
#endif /* BUTTON_H_ */
