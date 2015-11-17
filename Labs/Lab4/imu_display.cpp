#include <iostream>
#include "mraa.hpp"
#include "oled/Edison_OLED.h"
using namespace std;

/*
 * This program is the set up for using the 9-degrees of freedom board on the Intel
 * Edison microcontroller. This will set the address on the I2C, write to the
 * registers, read and assemble the incoming data. It will then output the data to
 * the edOLED screen of the Edison.
 *
 * The Up, Down, and Select buttons will be initialized starting out on a welcome
 * page. The Up and Down buttons will scroll through 5 pages:
 * ~ Page 1: A welcome screen with simple instructions.
 * ~ Page 2: The temperature in C and F.
 * ~ Page 3: Acceleration
 * ~ Page 4: Gyroscope
 * ~ Page 5: Magnetometer
 *
 * *** THESE VALUES HAVE NOT BEEN CALIBRATED ***
 *
 * The Select button will exit the program.
 *
 * @author Cameron Stanavige
 * @version 11/14/2015
 */

// addresses
#define XM_ADDR 0x1D
#define G_ADDR  0x6B

//temperature constants
#define CTRL_REG5_XM  0x24
#define OUT_TEMP_L_XM 0x05
#define OUT_TEMP_H_XM 0x06

// accelerometer constants
#define CTRL_REG1_XM  0x20
#define CTRL_REG2_XM  0x21
#define OUT_X_L_A	  0X28
#define OUT_X_H_A	  0X29
#define OUT_Y_L_A	  0X2A
#define OUT_Y_H_A	  0X2B
#define OUT_Z_L_A	  0X2C
#define OUT_Z_H_A	  0X2D

// gyroscope constants
#define CTRL_REG1_G   0x20
//#define CTRL_REG2_G   0x21
//#define CTRL_REG3_G   0x22
//#define CTRL_REG4_G   0x23
//#define CTRL_REG5_G   0x24
#define OUT_X_L_G	  0x28
#define OUT_X_H_G	  0x29
#define OUT_Y_L_G	  0x2A
#define OUT_Y_H_G	  0x2B
#define OUT_Z_L_G	  0x2C
#define OUT_Z_H_G	  0x2D

// magnetometer constants
#define CTRL_REG1_XM  0x20
#define CTRL_REG5_XM  0x24
#define CTRL_REG6_XM  0x25
#define CTRL_REG7_XM  0x26
#define OUT_X_L_M	  0x08
#define OUT_X_H_M	  0x09
#define OUT_Y_L_M	  0x0A
#define OUT_Y_H_M	  0x0B
#define OUT_Z_L_M	  0x0C
#define OUT_Z_H_M	  0x0D

// pins associated with each button
#define PIN_UP  	47
#define PIN_DOWN	44
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
#define DB_Up 		Buttons.Buttons_bit.Buttons_Up
#define DB_Down		Buttons.Buttons_bit.Buttons_Down
#define DB_Select	Buttons.Buttons_bit.Buttons_Select
#define BIT7		0x80

// debounce thresholds
#define PRESS_THRESHOLD 	0x3F
#define RELEASE_THRESHOLD 	0xFC

// button
mraa::Gpio* button_Up;
mraa::Gpio* button_Down;
mraa::Gpio* button_Select;

// display control
static int volatile page = 1;
static int volatile running = 0;

// function prototypes
int16_t assemble(uint8_t, uint8_t);
int16_t assemble2(uint8_t, uint8_t);

void printTemp(mraa::I2c*, edOLED*);
float getTemp(mraa::I2c*);

void printGyro(mraa::I2c*, edOLED*);

void printAccel(mraa::I2c*, edOLED*);

void printMag(mraa::I2c*, edOLED*);

void printWelcome(edOLED*);

void interrupt_Up(void*);
void interrupt_Down(void*);
void interrupt_Select(void*);

/*
 * Main method to run this program. Sets up the I2Cs, buttons, and interrupts. Then
 * runs a continuous loop that switches through the available pages as they are
 * selected until the exit button is pressed.
 */
int main(int argc, char* argv[]) {
	// temp-accel-mag i2c
	mraa::I2c* i2c = new mraa::I2c(1);
	i2c->address(XM_ADDR);

	// gyro i2c
	mraa::I2c* i2cG = new mraa::I2c(1);
	i2cG->address(G_ADDR);

	// oled display setup
	edOLED* oled = new edOLED();
	oled->begin();
	oled->clear(ALL);
	oled->display();

	//button setups

	Buttons.Buttons = 0xFF;

	// up_button
	button_Up = new mraa::Gpio(PIN_UP, true, true);
	button_Up->dir(mraa::DIR_IN);

	button_Up->isr(mraa::EDGE_BOTH, &interrupt_Up, NULL);

	// down_button
	button_Down = new mraa::Gpio(PIN_DOWN, true, true);
	button_Down->dir(mraa::DIR_IN);

	button_Down->isr(mraa::EDGE_BOTH, &interrupt_Down, NULL);

	// select button
	button_Select = new mraa::Gpio(PIN_SELECT, true, true);
	button_Select->dir(mraa::DIR_IN);

	button_Select->isr(mraa::EDGE_BOTH, &interrupt_Select, NULL);

	while (running == 0) {
		switch (page) {
		case 1:
			printWelcome(oled);
			break;
		case 2:
			printTemp(i2c, oled);
			break;
		case 3:
			printAccel(i2c, oled);
			break;
		case 4:
			printGyro(i2cG, oled);
			break;
		case 5:
			printMag(i2c, oled);
			break;
		default:
			page = 1;
		}

		usleep(4000);
	}

	// clean-up before exiting
	oled->clear(PAGE);
	oled->display();
	delete i2c;
	delete i2cG;
	delete oled;
	button_Up->isrExit();
	delete button_Up;
	button_Down->isrExit();
	delete button_Down;
	button_Select->isrExit();
	delete button_Select;
	return 0;
}

/*
 * Takes two unsigned 8-bit integers (a low and high) and converts them into one
 * 16-bit signed integer.
 *
 * @param low The lower 8 bits
 * @param hi The upper 8 bits
 *
 * @return The combined 16-bit signed integer
 */
int16_t assemble(uint8_t low, uint8_t hi) {
	uint16_t l = (uint16_t) low;
	uint16_t h = (uint16_t) hi;
	return (int16_t) ((h << 8) | l);
}

/*
 * Takes two unsigned 8-bit integers (a low and high) and converts them into one
 * 12-bit signed integer.
 *
 * @param low The lower 8 bits
 * @param hi The upper 4 bits
 *
 * @return The combined 12-bit signed integer
 */
int16_t assemble2(uint8_t low, uint8_t hi) {
	uint8_t temp;
	if (hi >= 0x08) { // negative number
		temp = 0xF0;
		hi |= temp;
	} else { // positive number
		temp = 0x00;
		hi ^= temp;
	}

	return assemble(low, hi);
}

/*
 * Prints the device temperature to the screen.
 *
 * @param i2c The device address to write to/read from.
 * @param oled The OLED screen to print out to.
 */
void printTemp(mraa::I2c* i2c, edOLED* oled) {
	i2c->writeReg(CTRL_REG5_XM, 0x98);

	float temp = getTemp(i2c);

	char* t = new char[6];
	sprintf(t, "%.3f", temp);

	oled->clear(PAGE);
	oled->setCursor(0, 0);
	oled->print("Temp");
	oled->print("\nC: ");
	oled->print(t);

	temp = temp * (9.0/5) + 32;
	sprintf(t, "%.3f", temp);

	oled->print("\nF: ");
	oled->print(t);

	oled->display();

	usleep(500000);
}

/*
 * Gets the temperature from the device.
 *
 * @param i2c The device address to write to/read from.
 *
 * @return The temperature in C
 */
float getTemp(mraa::I2c* i2c) {
	uint8_t lowTemp = i2c->readReg(OUT_TEMP_L_XM);
	uint8_t hiTemp = i2c->readReg(OUT_TEMP_H_XM);
	int16_t both = assemble2(lowTemp, hiTemp);

	return (both / 8.0) + 21.0;
}

/*
 * Prints the x, y, & z gyroscope values to the oled screen.
 *
 * @param i2cG The device address to write to/read from.
 * @param oled The OLED screen to print out to
 */
void printGyro(mraa::I2c* i2cG, edOLED* oled) {
	i2cG->writeReg(CTRL_REG1_G, 0x0F);
//	i2cG->writeReg(CTRL_REG2_G, 0x00);
//	i2cG->writeReg(CTRL_REG3_G, 0x00);
//	i2cG->writeReg(CTRL_REG4_G, 0x00);
//	i2cG->writeReg(CTRL_REG5_G, 0x00);

	uint8_t xLow = i2cG->readReg(OUT_X_L_G);
	uint8_t xHi = i2cG->readReg(OUT_X_H_G);

	uint8_t yLow = i2cG->readReg(OUT_Y_L_G);
	uint8_t yHi = i2cG->readReg(OUT_Y_H_G);

	uint8_t zLow = i2cG->readReg(OUT_Z_L_G);
	uint8_t zHi = i2cG->readReg(OUT_Z_H_G);

	int16_t x = assemble(xLow, xHi);
	int16_t y = assemble(yLow, yHi);
	int16_t z = assemble(zLow, zHi);

	oled->clear(PAGE);
	oled->setCursor(0, 0);
	oled->print("Gyro\n");
	oled->print("X: ");
	oled->print(x);
	oled->print("\nY: ");
	oled->print(y);
	oled->print("\nZ: ");
	oled->print(z);
	oled->display();

	usleep(500000);
}

/*
 * Prints the x, y, & z accelerometer values to the oled screen.
 *
 * @param i2c The device address to write to/read from.
 * @param oled The OLED screen to print out to
 */
void printAccel(mraa::I2c* i2c, edOLED* oled) {
	i2c->writeReg(CTRL_REG1_XM, 0x47);
	i2c->writeReg(CTRL_REG2_XM, 0xC0);

	uint8_t xLow = i2c->readReg(OUT_X_L_A);
	uint8_t xHi = i2c->readReg(OUT_X_H_A);

	uint8_t yLow = i2c->readReg(OUT_Y_L_A);
	uint8_t yHi = i2c->readReg(OUT_Y_H_A);

	uint8_t zLow = i2c->readReg(OUT_Z_L_A);
	uint8_t zHi = i2c->readReg(OUT_Z_H_A);

	int16_t x = assemble(xLow, xHi);
	int16_t y = assemble(yLow, yHi);
	int16_t z = assemble(zLow, zHi);

	oled->clear(PAGE);
	oled->setCursor(0, 0);
	oled->print("Accel\n");
	oled->print("X: ");
	oled->print(x);
	oled->print("\nY: ");
	oled->print(y);
	oled->print("\nZ: ");
	oled->print(z);
	oled->display();

	usleep(500000);
}

/*
 * Prints the x, y, & z magnetometer values to the oled screen.
 *
 * @param i2c The device address to write to/read from.
 * @param oled The OLED screen to print out to
 */
void printMag(mraa::I2c* i2c, edOLED* oled) {
	i2c->writeReg(CTRL_REG1_XM, 0x47);
	i2c->writeReg(CTRL_REG5_XM, 0x18);
	i2c->writeReg(CTRL_REG6_XM, 0x20);
	i2c->writeReg(CTRL_REG7_XM, 0x00);

	uint8_t xLow = i2c->readReg(OUT_X_L_M);
	uint8_t xHi = i2c->readReg(OUT_X_H_M);

	uint8_t yLow = i2c->readReg(OUT_Y_L_M);
	uint8_t yHi = i2c->readReg(OUT_Y_H_M);

	uint8_t zLow = i2c->readReg(OUT_Z_L_M);
	uint8_t zHi = i2c->readReg(OUT_Z_H_M);

	int16_t x = assemble(xLow, xHi);
	int16_t y = assemble(yLow, yHi);
	int16_t z = assemble(zLow, zHi);

	oled->clear(PAGE);
	oled->setCursor(0, 0);
	oled->print("Mag\n");
	oled->print("X: ");
	oled->print(x);
	oled->print("\nY: ");
	oled->print(y);
	oled->print("\nZ: ");
	oled->print(z);
	oled->display();

	usleep(500000);
}

/*
 * Prints the welcome screen to the OLED.
 *
 * @param oled The OLED to print to.
 */
void printWelcome(edOLED* oled) {
	oled->clear(PAGE);
	oled->setCursor(0, 0);
	oled->print("Press Up & Down to  scroll.\nSelect to exit.");
	oled->display();
}

/*
 * ISR for the Up button. When pressed, will increment the page.
 */
void interrupt_Up(void* args) {
	int value_Up;
	while (running == 0) {
		static uint8_t UshiftReg = 0xFF; // shift register

		value_Up = button_Up->read(); // read button value

		UshiftReg >>= 1;

		if (value_Up == 1) { // reset register if not pressed
			UshiftReg |= BIT7;
		}

		if (DB_Up == 0) { // button released
			if (UshiftReg >= RELEASE_THRESHOLD) {
				//cout << "Up released" << endl;
				DB_Up = 1;
			}
		} else { // button pressed
			if (UshiftReg <= PRESS_THRESHOLD) {
				//cout << "Up pressed" << endl;
				if (page == 5)
					page = 1;
				else
					page++;
				DB_Up = 0;
			}
		}
		usleep(1000);
	}
	usleep(500);
} // end Up

/*
 * ISR for the Down button. When pressed, will decrement the current page.
 */
void interrupt_Down(void* args) {
	int value_Down;
	while (running == 0) {
		static uint8_t DshiftReg = 0xFF; // shift register

		value_Down = button_Down->read(); // read button value

		DshiftReg >>= 1;

		if (value_Down == 1) { // reset register if not pressed
			DshiftReg |= BIT7;
		}

		if (DB_Down == 0) { // button released
			if (DshiftReg >= RELEASE_THRESHOLD) {
				//cout << "Down released" << endl;
				DB_Down = 1;
			}
		} else { // button pressed
			if (DshiftReg <= PRESS_THRESHOLD) {
				//cout << "Down pressed" << endl;
				if (page == 1)
					page = 5;
				else
					page--;
				DB_Down = 0;
			}
		}
		usleep(1000);
	}
	usleep(500);
} // end Down

/*
 * ISR for the Select button. When pressed, will exit the program.
 */
void interrupt_Select(void* args) {
	int value_Select;
	while (running == 0) {
		static uint8_t SshiftReg = 0xFF; // shift register

		value_Select = button_Select->read(); // read button value

		SshiftReg >>= 1;

		if (value_Select == 1) { // reset register if not pressed
			SshiftReg |= BIT7;
		}

		if (DB_Select == 0) { // button released
			if (SshiftReg >= RELEASE_THRESHOLD) {
				//cout << "Select released" << endl;
				running = 1;
				DB_Select = 1;
			}
		} else { // button pressed
			if (SshiftReg <= PRESS_THRESHOLD) {
				//cout << "Select pressed" << endl;
				DB_Select = 0;
			}
		}
		usleep(1000);
	}
	usleep(500);
} // end Select
