#include <stdio.h>
#include <stdlib.h>
#include <mraa.h>

/*
 * Button_isr is currently set up for just the A and B buttons to be called
 * and to print when they are pressed to the console using debouncing and
 * using their own interrupt service routines.
 *
 * The Select button is currently set up but has been commented out. The other
 * buttons will be implemented in the future.
 *
 * Cameorn Stanavige
 * CS490
 * Lab3
 */

// pins associated with each button
#define PIN_A  		49
#define PIN_B  		46
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
#define DB_A 		Buttons.Buttons_bit.Buttons_A
#define DB_B 		Buttons.Buttons_bit.Buttons_B
//#define DB_Up 		Buttons.Buttons_bit.Buttons_B
//#define DB_Down		Buttons.Buttons_bit.Buttons_B
//#define DB_Left 	Buttons.Buttons_bit.Buttons_B
//#define DB_Right	Buttons.Buttons_bit.Buttons_B
//#define DB_Select	Buttons.Buttons_bit.Buttons_B
#define BIT7		0x80

// debounce thresholds
#define PRESS_THRESHOLD 	0x3F
#define RELEASE_THRESHOLD 	0xFC

// values being read in from each button
static int volatile value_A;
static int volatile value_B;
//static int volatile value_Select;

// function prototypes
void interrupt_A(void*);
void interrupt_B(void*);
//void interrupt_Up(void*);
//void interrupt_Down(void*);
//void interrupt_Left(void*);
//void interrupt_Right(void*);
//void interrupt_Select(void*);

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
//	mraa_gpio_context button_Select = mraa_gpio_init_raw(PIN_SELECT);
//
//	if (mraa_gpio_dir(button_Select, MRAA_GPIO_IN) != MRAA_SUCCESS) {
//		fprintf(stderr, "Couldn't initialize GPIO, exiting");
//		return MRAA_ERROR_UNSPECIFIED;
//	}
//
//	mraa_gpio_isr(button_Select, edge, &interrupt_Select, button_Select);

	for (;;) {
		usleep(4000);
	}

	mraa_gpio_close(button_A);
	mraa_gpio_close(button_B);
	//mraa_gpio_close(button_Select);

	return MRAA_SUCCESS;

} // end main

/*
 * ISR for button A
 */
void interrupt_A(void* args) {
	for (;;) {
		static uint8_t shiftReg = 0xFF;

		value_A = mraa_gpio_read(args);

		shiftReg >>= 1;

		if (value_A == 1) {
			shiftReg |= BIT7;
		}

		if (DB_A == 0) {
			if (shiftReg >= RELEASE_THRESHOLD) {
				printf("A released\n");
				DB_A = 1;
			}
		} else {
			if (shiftReg <= PRESS_THRESHOLD) {
				printf("A pressed\n");
				DB_A = 0;
			}
		}
	}
	usleep(500);
} // end A

/*
 * ISR for button B
 */
void interrupt_B(void* args) {
	for (;;) {
		static uint8_t BshiftReg = 0xFF;

		value_B = mraa_gpio_read(args);

		BshiftReg >>= 1;

		if (value_B == 1) {
			BshiftReg |= BIT7;
		}

		if (DB_B == 0) {
			if (BshiftReg >= RELEASE_THRESHOLD) {
				printf("B released\n");
				DB_B = 1;
			}
		} else {
			if (BshiftReg <= PRESS_THRESHOLD) {
				printf("B pressed\n");
				DB_B = 0;
			}
		}
	}
	usleep(500);
} // end B

//void interrupt_Up(void* args) {
//
//}
//
//void interrupt_Down(void* args) {
//
//}
//
//void interrupt_Left(void* args) {
//
//}
//
//void interrupt_Right(void* args) {
//
//}
//
///*
// * ISR for button Select
// */
//void interrupt_Select(void* args) {
//	for (;;) {
//		static uint8_t SshiftReg = 0xFF;
//
//		value_Select = mraa_gpio_read(args);
//
//		SshiftReg >>= 1;
//
//		if (value_Select == 1) {
//			SshiftReg |= BIT7;
//		}
//
//		if (DB_Select == 0) {
//			if (SshiftReg >= RELEASE_THRESHOLD) {
//				printf("Select released\n");
//				DB_Select = 1;
//			}
//		} else {
//			if (SshiftReg <= PRESS_THRESHOLD) {
//				printf("Select pressed\n");
//				DB_Select = 0;
//			}
//		}
//	}
//	usleep(500);
//} // end select

