#include <stdio.h>
#include <stdlib.h>
#include <mraa.h>

/*
 * Button_isr is set up for all buttons and the 4-way directional joystick to be called
 * and to print when they are pressed to the console using debouncing and
 * using their own interrupt service routines.
 *
 * The joystick is set up to register presses and releases but also to continue firing
 * multiple events when they are held in a given direction.
 *
 * Cameorn Stanavige
 * CS490
 * Lab3
 */

// pins associated with each button
#define PIN_A  		49
#define PIN_B  		46
#define PIN_UP  	47
#define PIN_DOWN	44
#define PIN_LEFT 	165
#define PIN_RIGHT  	45
#define PIN_SELECT  48

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
#define DB_A 		Buttons.Buttons_bit.Buttons_A
#define DB_B 		Buttons.Buttons_bit.Buttons_B
#define DB_Up 		Buttons.Buttons_bit.Buttons_Up
#define DB_Down		Buttons.Buttons_bit.Buttons_Down
#define DB_Left 	Buttons.Buttons_bit.Buttons_Left
#define DB_Right	Buttons.Buttons_bit.Buttons_Right
#define DB_Select	Buttons.Buttons_bit.Buttons_Select
#define BIT7		0x80

// debounce thresholds
#define PRESS_THRESHOLD 	0x3F
#define RELEASE_THRESHOLD 	0xFC

// values being read in from each button
static int volatile value_A;
static int volatile value_B;
static int volatile value_Up;
static int volatile value_Down;
static int volatile value_Left;
static int volatile value_Right;
static int volatile value_Select;

// function prototypes
void interrupt_A(void*);
void interrupt_B(void*);
void interrupt_Up(void*);
void interrupt_Down(void*);
void interrupt_Left(void*);
void interrupt_Right(void*);
void interrupt_Select(void*);

int main(int argc, char* argv[]) {
	// type of edge to be used to determine when to interrupt
	mraa_gpio_edge_t edge = MRAA_GPIO_EDGE_BOTH;

	Buttons.Buttons = 0xFF;

	// button A setup
	mraa_gpio_context button_A = mraa_gpio_init_raw(PIN_A);

	if (mraa_gpio_dir(button_A, MRAA_GPIO_IN) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	mraa_gpio_isr(button_A, edge, &interrupt_A, button_A);

	// button B setup
	mraa_gpio_context button_B = mraa_gpio_init_raw(PIN_B);

	if (mraa_gpio_dir(button_B, MRAA_GPIO_IN) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	mraa_gpio_isr(button_B, edge, &interrupt_B, button_B);

	// button Select setup
	mraa_gpio_context button_Select = mraa_gpio_init_raw(PIN_SELECT);

	if (mraa_gpio_dir(button_Select, MRAA_GPIO_IN) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	mraa_gpio_isr(button_Select, edge, &interrupt_Select, button_Select);

	// button Up setup
	mraa_gpio_context button_Up = mraa_gpio_init_raw(PIN_UP);

	if (mraa_gpio_dir(button_Up, MRAA_GPIO_IN) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	mraa_gpio_isr(button_Up, edge, &interrupt_Up, button_Up);

	// button Down setup
	mraa_gpio_context button_Down = mraa_gpio_init_raw(PIN_DOWN);

	if (mraa_gpio_dir(button_Down, MRAA_GPIO_IN) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	mraa_gpio_isr(button_Down, edge, &interrupt_Down, button_Down);

	// button Left setup
	mraa_gpio_context button_Left = mraa_gpio_init_raw(PIN_LEFT);

	if (mraa_gpio_dir(button_Left, MRAA_GPIO_IN) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	mraa_gpio_isr(button_Left, edge, &interrupt_Left, button_Left);

	// button Right setup
	mraa_gpio_context button_Right = mraa_gpio_init_raw(PIN_RIGHT);

	if (mraa_gpio_dir(button_Right, MRAA_GPIO_IN) != MRAA_SUCCESS) {
		fprintf(stderr, "Couldn't initialize GPIO, exiting");
		return MRAA_ERROR_UNSPECIFIED;
	}

	mraa_gpio_isr(button_Right, edge, &interrupt_Right, button_Right);

	for (;;) {
		usleep(4000);
	}

	mraa_gpio_close(button_A);
	mraa_gpio_close(button_B);
	mraa_gpio_close(button_Up);
	mraa_gpio_close(button_Down);
	mraa_gpio_close(button_Left);
	mraa_gpio_close(button_Right);
	mraa_gpio_close(button_Select);

	return MRAA_SUCCESS;

} // end main

/*
 * ISR for button A
 */
void interrupt_A(void* args) {
	for (;;) {
		static uint8_t shiftReg = 0xFF; // shift register

		value_A = mraa_gpio_read(args); // read button value

		shiftReg >>= 1;

		if (value_A == 1) { // reset register if not pressed
			shiftReg |= BIT7;
		}

		if (DB_A == 0) { // button released
			if (shiftReg >= RELEASE_THRESHOLD) {
				printf("A released\n");
				DB_A = 1;
			}
		} else { // button pressed
			if (shiftReg <= PRESS_THRESHOLD) {
				printf("A pressed\n");
				DB_A = 0;
			}
		}
		usleep(1000);
	}
	usleep(500);
} // end A

/*
 * ISR for button B
 */
void interrupt_B(void* args) {
	for (;;) {
		static uint8_t BshiftReg = 0xFF; // shift register

		value_B = mraa_gpio_read(args); // read button value

		BshiftReg >>= 1;

		if (value_B == 1) { // reset register if not pressed
			BshiftReg |= BIT7;
		}

		if (DB_B == 0) { // button released
			if (BshiftReg >= RELEASE_THRESHOLD) {
				printf("B released\n");
				DB_B = 1;
			}
		} else { // button pressed
			if (BshiftReg <= PRESS_THRESHOLD) {
				printf("B pressed\n");
				DB_B = 0;
			}
		}
		usleep(1000);
	}
	usleep(500);
} // end B

/*
 * ISR for Up direction
 */
void interrupt_Up(void* args) {
	int c = 0; // count to track event re-firing
	char stillUp = 0; // flag for holding direction
	for (;;) {
		static uint8_t UshiftReg = 0xFF; // shift register

		value_Up = mraa_gpio_read(args); // read button value

		UshiftReg >>= 1;

		if (value_Up == 1) { // reset register if not pressed
			UshiftReg |= BIT7;
		}

		/*
		 * Check to see if joystick is still being held in direction
		 * and if so, fire multiple events.
		 * c % #### is the value used to control how often to fire event
		 */
		if (stillUp == 1 && UshiftReg == 0x00 && c % 1000 == 0) {
			printf("Up still pressed\n");
			usleep(250000);
		}

		if (DB_Up == 0) { // button released
			if (UshiftReg >= RELEASE_THRESHOLD) {
				printf("Up released\n");
				stillUp = 0; // reset flag
				c = 0; // reset counter
				DB_Up = 1;
			}
		} else {
			if (UshiftReg <= PRESS_THRESHOLD) {
				printf("Up pressed\n");
				stillUp = 1; // set still pressed flag
				DB_Up = 0;
			}

		}
		c++; // increment event re-firing counter
		usleep(1000);
	}
	usleep(500);
} // end up

/*
 * ISR for button Down
 */
void interrupt_Down(void* args) {
	int c = 0; // count to track event re-firing
	char stillDown = 0; // flag for holding direction
	for (;;) {
		static uint8_t DshiftReg = 0xFF; // shift register

		value_Down = mraa_gpio_read(args); // read button value

		DshiftReg >>= 1;

		if (value_Down == 1) { // reset register if not pressed
			DshiftReg |= BIT7;
		}

		/*
		 * Check to see if joystick is still being held in direction
		 * and if so, fire multiple events.
		 * c % #### is the value used to control how often to fire event
		 */
		if (stillDown == 1 && DshiftReg == 0x00 && c % 1000 == 0) {
			printf("Down still pressed\n");
			usleep(250000);
		}

		if (DB_Down == 0) { // button released
			if (DshiftReg >= RELEASE_THRESHOLD) {
				printf("Down released\n");
				stillDown = 0; // reset flag
				c = 0; // reset counter
				DB_Down = 1;
			}
		} else {
			if (DshiftReg <= PRESS_THRESHOLD) {
				printf("Down pressed\n");
				stillDown = 1; // set still pressed flag
				DB_Down = 0;
			}

		}
		c++; // increment event re-firing counter
		usleep(1000);
	}
	usleep(500);
} // end down

/*
 * ISR for button Left
 */
void interrupt_Left(void* args) {
	int c = 0; // count to track event re-firing
	char stillLeft = 0; // flag for holding direction
	for (;;) {
		static uint8_t LshiftReg = 0xFF; // shift register

		value_Left = mraa_gpio_read(args); // read button value

		LshiftReg >>= 1;

		if (value_Left == 1) { // reset register if not pressed
			LshiftReg |= BIT7;
		}

		/*
		 * Check to see if joystick is still being held in direction
		 * and if so, fire multiple events.
		 * c % #### is the value used to control how often to fire event
		 */
		if (stillLeft == 1 && LshiftReg == 0x00 && c % 1000 == 0) {
			printf("Left still pressed\n");
			usleep(250000);
		}

		if (DB_Left == 0) { // button released
			if (LshiftReg >= RELEASE_THRESHOLD) {
				printf("Left released\n");
				stillLeft = 0; // reset flag
				c = 0; // reset counter
				DB_Left = 1;
			}
		} else {
			if (LshiftReg <= PRESS_THRESHOLD) {
				printf("Left pressed\n");
				stillLeft = 1; // set still pressed flag
				DB_Left = 0;
			}

		}
		c++; // increment event re-firing counter
		usleep(1000);
	}
	usleep(500);
} // end left

/*
 * ISR for button Right
 */
void interrupt_Right(void* args) {
	int c = 0; // count to track event re-firing
	char stillRight = 0; // flag for holding direction
	for (;;) {
		static uint8_t RshiftReg = 0xFF; // shift register

		value_Right = mraa_gpio_read(args); // read button value

		RshiftReg >>= 1;

		if (value_Right == 1) { // reset register if not pressed
			RshiftReg |= BIT7;
		}

		/*
		 * Check to see if joystick is still being held in direction
		 * and if so, fire multiple events.
		 * c % #### is the value used to control how often to fire event
		 */
		if (stillRight == 1 && RshiftReg == 0x00 && c % 1000 == 0) {
			printf("Right still pressed\n");
			usleep(250000);
		}

		if (DB_Right == 0) { // button released
			if (RshiftReg >= RELEASE_THRESHOLD) {
				printf("Right released\n");
				stillRight = 0; // reset flag
				c = 0; // reset counter
				DB_Right = 1;
			}
		} else {
			if (RshiftReg <= PRESS_THRESHOLD) {
				printf("Right pressed\n");
				stillRight = 1; // set still pressed flag
				DB_Right = 0;
			}

		}
		c++; // increment event re-firing counter
		usleep(1000);
	}
	usleep(500);
} // end right

/*
 * ISR for button Select
 */
void interrupt_Select(void* args) {
	for (;;) {
		static uint8_t SshiftReg = 0xFF; // shift register

		value_Select = mraa_gpio_read(args); // read button value

		SshiftReg >>= 1;

		if (value_Select == 1) { // reset register if not pressed
			SshiftReg |= BIT7;
		}

		if (DB_Select == 0) { // button released
			if (SshiftReg >= RELEASE_THRESHOLD) {
				printf("Select released\n");
				DB_Select = 1;
			}
		} else { // button pressed
			if (SshiftReg <= PRESS_THRESHOLD) {
				printf("Select pressed\n");
				DB_Select = 0;
			}
		}
		usleep(1000);
	}
	usleep(500);
} // end select

