/**
 * @name Frozen Eyes that aren't actually frozen but I like this name better
 * @author Max Miller
 * @version 1.2.3
 * @date Febuary 14th, 2026
 * @details I think my father isn't going to read this, but if he does, "hi dad..."
 */

// ------------------  INCLUDES  ------------------
#include <Arduino.h>
#include <Servo.h>
#include <EEPROM.h>


// ----------------  DEFINITIONS  ----------------
// ------ THIS IS WHERE YOU CHANGE SETTINGS ------

// PINS
#define LEFT_EYE_PIN 	DD6				// Left Eye Servo
#define RIGHT_EYE_PIN 	DD5				// Right Eye Servo
#define MODE_BTN_PIN 	DD3				// Mode Button
#define ENABLE_BTN_PIN 	DD2				// Enable Button
#define POT_PIN 		A0				// Potentiometer
#define SPEED_LED_PIN	8				// Edit Speed LED
#define HOME_LED_PIN	DD7				// Edit Home LED
#define ENABLE_LED_PIN	LED_BUILTIN		// Enable LED


#define POT_MOD 		5				// Change how much of an effect the potenitometer has
#define MIN_BLINK_TIME 	500				// Minimum time between blinks

#define CLOSED_POS 		0				// Default Closed Position in degrees
#define OPEN_POS 		150				// Default Open Position in degrees

// UNCOMMENT if the left/right servos need to be reversed 
#define LEFT_SERVO_REVERSE
// #define RIGHT_SERVO_REVERSE

#define OPENING_TIME 	100				// Time in miliseconds to open the eye
#define CLOSED_TIME 	200				// Time in miliseconds for the eye to stay closed
#define CLOSING_TIME 	100				// Time in miliseconds to close the eye

// DO NOT MODIFY ANYTHING BELOW
// DO NOT MODIFY ANYTHING BELOW
// DO NOT MODIFY ANYTHING BELOW

#define STEP_TIME 		10				// time between the loops
#define BTN_PUSH_ST 	LOW				// Enable Button Pushed State
#define DEBOUNCE_TIME 	100				// Button Debounce Time

// UNCOMMENT if button is momentary
// COMMENT if button is toggle momentary
#define EN_BTN_MOMENTARY

#define EYE_DELTA (OPEN_POS - CLOSED_POS)
#define OPEN_STEPS (OPENING_TIME / STEP_TIME)
#define OPEN_STEP_VALUE (EYE_DELTA / OPEN_STEPS)
#define CLOSE_STEPS (CLOSING_TIME / STEP_TIME)
#define CLOSE_STEP_VALUE (EYE_DELTA / CLOSE_STEPS)

#define HOME_L_ADR		0
#define HOME_R_ADR		HOME_L_ADR + sizeof(int)
#define SPEED_ADR		HOME_R_ADR + sizeof(int)

// --------------  END DEFINITIONS  --------------

enum Mode {
	LOCK,
	CHANGE_SPEED,
	CHANGE_L_HOME,
	CHANGE_R_HOME
};


Servo leftEye;
Servo rightEye;

int pos = 0;
unsigned long next_blink_ms = 0;
int next_blink_time = 0;


// State vars
int i;
int potVal;
int left_offset 	= 0;
int right_offset 	= 0;
int speed_offset 	= 0;

// For enable button
volatile unsigned long last_interrupt_time = 0;
volatile bool enable = true;
volatile Mode mode = LOCK;


// Prototypes
bool isEnabled(void);
bool timeToBlink(void);
int getPotVal(void);

bool closeEye();
bool openEye();

void displayLEDs(void);

void ISR_en_btn_pressed(void);
void ISR_mode_btn_pressed(void);

/**
 * Arduino Setup Function
 */
void setup() {
	// Load Previous Trims
	EEPROM.get(HOME_L_ADR, left_offset);
	EEPROM.get(HOME_R_ADR, right_offset);
	EEPROM.get(SPEED_ADR, speed_offset);

	leftEye.attach(LEFT_EYE_PIN);
	rightEye.attach(RIGHT_EYE_PIN);

	leftEye.write(OPEN_POS);
	rightEye.write(OPEN_POS);

	pinMode(ENABLE_BTN_PIN, INPUT_PULLUP);
	pinMode(MODE_BTN_PIN, INPUT_PULLUP);

	attachInterrupt(digitalPinToInterrupt(ENABLE_BTN_PIN), ISR_en_btn_pressed, FALLING);
	attachInterrupt(digitalPinToInterrupt(MODE_BTN_PIN), ISR_mode_btn_pressed, FALLING);

	pinMode(POT_PIN, INPUT);

	pinMode(SPEED_LED_PIN, OUTPUT);
	pinMode(HOME_LED_PIN, OUTPUT);
	pinMode(ENABLE_LED_PIN, OUTPUT);

	// Set random seed using a unconected analogue pin's value
	randomSeed(analogRead(A1));


}


/**
 * Arduino Loop Function
 */
void loop() {
	if (mode == LOCK) {
		EEPROM.put(HOME_L_ADR, left_offset);
		EEPROM.put(HOME_R_ADR, right_offset);
		EEPROM.put(SPEED_ADR, speed_offset);
	}

	displayLEDs();

	if (!isEnabled()) { 
		#ifdef LEFT_SERVO_REVERSE
			rightEye.write(OPEN_POS);
		#else
			rightEye.write(CLOSED_POS);
		#endif

		#ifdef RIGHT_SERVO_REVERSE
			leftEye.write(OPEN_POS);
		#else
			leftEye.write(CLOSED_POS);
		#endif
		return; 
	}

	// If it is time to blink, then blink
	if (timeToBlink()) {

		potVal = getPotVal();

		if (mode == CHANGE_SPEED) {
			speed_offset = potVal;
		}
		if (mode == CHANGE_L_HOME) {
			left_offset = (potVal - 512) / 32;
		}
		if (mode == CHANGE_R_HOME) {
			right_offset = (potVal - 512) / 32;
		}

		// Close Eye Lid
		closeEye();

		// Stay closed for CLOSED_TIME
		delay(CLOSED_TIME);

		// Open Eye Lid
		openEye();

		// This line creates a random amount of time (in milliseconds) until the next blink
		// The smallest number is MIN_BLINK_TIME + potVal
		// The largest number is (MIN_BLINK_TIME + 1) + (POT_MOD * potVal)
		int rand_delta = random(MIN_BLINK_TIME + speed_offset, (MIN_BLINK_TIME + 1) + (POT_MOD * speed_offset));
		
		next_blink_ms = millis() + rand_delta;
	}
}


/**
 * Is Enables
 * @returns if the system is enabled
 */
bool isEnabled(void) {
	return enable;
}


/**
 * Is is time to blink?
 */
bool timeToBlink(void) {
	return millis() > next_blink_ms;
}

/**
 * Get Potentiometer Value
 */
int getPotVal(void) {
	return analogRead(POT_PIN);
}

/**
 * Open Eye Sequence
 */
bool openEye() {
	for(i = 0; i < OPEN_STEPS - 1; i++) {
		// Write the next step to the servos
		#ifdef LEFT_SERVO_REVERSE
			leftEye.write(OPEN_STEPS - i * OPEN_STEP_VALUE + left_offset);
		#else
			leftEye.write(CLOSED_POS + i * OPEN_STEP_VALUE + left_offset);
		#endif

		#ifdef RIGHT_SERVO_REVERSE
			rightEye.write(OPEN_STEPS - i * OPEN_STEP_VALUE + right_offset);
		#else
			rightEye.write(CLOSED_POS + i * OPEN_STEP_VALUE + right_offset);
		#endif
		
		delay(STEP_TIME);
	}

	// Fully open
	#ifdef LEFT_SERVO_REVERSE
		leftEye.write(CLOSED_POS + left_offset);
	#else
		leftEye.write(OPEN_POS + left_offset);
	#endif

	#ifdef RIGHT_SERVO_REVERSE
		rightEye.write(CLOSED_POS + right_offset);
	#else
		rightEye.write(OPEN_POS + right_offset);
	#endif

	return true;
}


/**
 * Close Eye sequence
 */
bool closeEye() {
	for(i = 0; i < CLOSE_STEPS - 1; i++) {
		// Write the next step to the servos
		#ifdef LEFT_SERVO_REVERSE
			leftEye.write(CLOSED_POS + i * CLOSE_STEP_VALUE + left_offset);
		#else
			leftEye.write(OPEN_POS - i * CLOSE_STEP_VALUE + left_offset);
		#endif

		#ifdef RIGHT_SERVO_REVERSE
			rightEye.write(CLOSED_POS + i * CLOSE_STEP_VALUE + right_offset);
		#else
			rightEye.write(OPEN_POS - i * CLOSE_STEP_VALUE + right_offset);
		#endif

		delay(STEP_TIME);
	}

	// Fully closed
	#ifdef LEFT_SERVO_REVERSE
		rightEye.write(OPEN_POS + left_offset);
	#else
		rightEye.write(CLOSE_STEPS + left_offset);
	#endif

	#ifdef RIGHT_SERVO_REVERSE
		leftEye.write(CLOSED_POS + right_offset);
	#else
		leftEye.write(OPEN_POS + right_offset);
	#endif

	return true;
}


void displayLEDs(void) {
	if (mode == CHANGE_SPEED) {
		digitalWrite(SPEED_LED_PIN, HIGH);
	} else {
		digitalWrite(SPEED_LED_PIN, LOW);
	}

	if (mode == CHANGE_L_HOME || mode == CHANGE_R_HOME) {
		digitalWrite(HOME_LED_PIN, HIGH);
	} else {
		digitalWrite(HOME_LED_PIN, LOW);
	}

	if (isEnabled()) {
		digitalWrite(ENABLE_LED_PIN, HIGH);
	} else {
		digitalWrite(ENABLE_LED_PIN, LOW);
	}
}


/**
 * Enable Button interupt
 */
void ISR_en_btn_pressed(void) {
	unsigned long interrupt_time = millis();

	if (digitalRead(ENABLE_BTN_PIN) != BTN_PUSH_ST) {
		return;
	}

	// If the time since the last interrupt is greater than the debounce time
	if (interrupt_time - last_interrupt_time > DEBOUNCE_TIME) {
		enable = !enable;
		
		last_interrupt_time = interrupt_time;
	}

	// All other interrupts during the debounce period are ignored
}


/**
 * Mode Button Interupt
 */
void ISR_mode_btn_pressed(void) {
	unsigned long interrupt_time = millis();

	// If the time since the last interrupt is greater than the debounce time
	if (interrupt_time - last_interrupt_time > DEBOUNCE_TIME) {
		
		if (mode == LOCK) {
			mode = CHANGE_SPEED;
		}
		else if (mode == CHANGE_SPEED) {
			mode = CHANGE_L_HOME;
		}
		else if (mode == CHANGE_L_HOME) {
			mode = CHANGE_R_HOME;
		}
		else if (mode == CHANGE_R_HOME) {
			mode = LOCK;
		}

		last_interrupt_time = interrupt_time;
	}

	// All other interrupts during the debounce period are ignored
}