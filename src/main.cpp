/**
 * @name Frozen Eyes that aren't actually frozen but I like this name better
 * @author Max Miller
 * @version 1.1.2
 * @date Febuary 13th, 2026
 * @details I think my father isn't going to read this, but if he does, "hi dad..."
 */

// ------------------  INCLUDES  ------------------
#include <Arduino.h>
#include <Servo.h>


// ----------------  DEFINITIONS  ----------------
// ------ THIS IS WHERE YOU CHANGE SETTINGS ------

// PINS
#define LEFT_EYE_PIN DD6
#define RIGHT_EYE_PIN DD5
#define ENABLE_BTN_PIN DD2
#define SPEED_POT_PIN A0

// Change how much of an effect the potenitometer has
#define ANALOGUE_MOD 5

// Minimum time between blinks
#define MIN_BLINK_TIME 500

// CHANGE HOW FAR THE SERVO WILL MOVE
#define CLOSED_POS 0
#define OPEN_POS 100
#define LEFT_SERVO_REVERSE
// #define RIGHT_SERVO_REVERSE

// CHANGE TIMING ON EACH BLINK
#define OPENING_TIME 500
#define CLOSED_TIME 200
#define CLOSING_TIME 500
#define EYE_DELTA (OPEN_POS - CLOSED_POS)

// DO NOT MODIFY UNLESS YOU'RE ABSOLUTLY SURE
// Changes the time between the loops, 
// if you make it too fast or slow it will cause problems.
#define STEP_TIME 10


// DO NOT MODIFY
#define OPEN_STEPS (OPENING_TIME / STEP_TIME)
// DO NOT MODIFY
#define OPEN_STEP_VALUE (EYE_DELTA / OPEN_STEPS)

// DO NOT MODIFY
#define CLOSE_STEPS (CLOSING_TIME / STEP_TIME)
// DO NOT MODIFY
#define CLOSE_STEP_VALUE (EYE_DELTA / CLOSE_STEPS)


// Should the btn be pulled up or down to enable
// If it's connected to GND, the state should be 
#define EN_BTN_PUSHED_STATE LOW

// Enable button debounce time
#define DEBOUNCE_TIME 50

// UNCOMMENT if button is momentary
// COMMENT if button is toggle momentary
#define EN_BTN_MOMENTARY

// --------------  END DEFINITIONS  --------------


Servo leftEye;
Servo rightEye;

int pos = 0;
unsigned long next_blink_ms = 0;
int next_blink_time = 0;

// State vars
int i;
int potVal;

// For enable button
volatile unsigned long last_interrupt_time = 0;
volatile bool enable = true;


// Prototypes
bool isEnabled(void);
bool timeToBlink(void);
int getPotVal(void);

bool closeEye();
bool openEye();

void ISR_en_btn_pressed(void);

/**
 * Arduino Setup Function
 */
void setup() {
	leftEye.attach(LEFT_EYE_PIN);
	rightEye.attach(RIGHT_EYE_PIN);

	leftEye.write(OPEN_POS);
	rightEye.write(OPEN_POS);

	pinMode(ENABLE_BTN_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(ENABLE_BTN_PIN), ISR_en_btn_pressed, FALLING);

	pinMode(SPEED_POT_PIN, INPUT);

	// Set random seed using a unconected analogue pin's value
	randomSeed(analogRead(A1));


}


/**
 * Arduino Loop Function
 */
void loop() {
	if (!isEnabled()) { return; }
	// If it is time to blink, then blink
	if (timeToBlink()) {

		// Close Eye Lid
		closeEye();

		// Stay closed for CLOSED_TIME
		delay(CLOSED_TIME);


		openEye();

		potVal = getPotVal();

		// This line creates a random amount of time (in milliseconds) until the next blink
		// The smallest number is MIN_BLINK_TIME + potVal
		// The largest number is (MIN_BLINK_TIME + 1) + (ANALOGUE_MOD * potVal)
		int rand_delta = random(MIN_BLINK_TIME + potVal, (MIN_BLINK_TIME + 1) + (ANALOGUE_MOD * potVal));
		
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
	return analogRead(SPEED_POT_PIN);
}

/**
 * Open Eye Sequence
 */
bool openEye() {
	for(i = 0; i < OPEN_STEPS - 1; i++) {
		// Write the next step to the servos
		#ifdef LEFT_SERVO_REVERSE
			leftEye.write(OPEN_STEPS - i * OPEN_STEP_VALUE);
		#else
			leftEye.write(CLOSED_POS + i * OPEN_STEP_VALUE);
		#endif

		#ifdef RIGHT_SERVO_REVERSE
			rightEye.write(OPEN_STEPS - i * OPEN_STEP_VALUE);
		#else
			rightEye.write(CLOSED_POS + i * OPEN_STEP_VALUE);
		#endif
		
		delay(STEP_TIME);
	}

	// Fully open
	#ifdef LEFT_SERVO_REVERSE
		leftEye.write(CLOSED_POS);
	#else
		leftEye.write(OPEN_POS);
	#endif

	#ifdef RIGHT_SERVO_REVERSE
		rightEye.write(CLOSED_POS);
	#else
		rightEye.write(OPEN_POS);
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
			leftEye.write(CLOSED_POS + i * CLOSE_STEP_VALUE);
		#else
			leftEye.write(OPEN_POS - i * CLOSE_STEP_VALUE);
		#endif

		#ifdef RIGHT_SERVO_REVERSE
			rightEye.write(CLOSED_POS + i * CLOSE_STEP_VALUE);
		#else
			rightEye.write(OPEN_POS - i * CLOSE_STEP_VALUE);
		#endif

		delay(STEP_TIME);
	}

	// Fully closed
	#ifdef LEFT_SERVO_REVERSE
		rightEye.write(OPEN_POS);
	#else
		rightEye.write(CLOSE_STEPS);
	#endif

	#ifdef RIGHT_SERVO_REVERSE
		leftEye.write(CLOSED_POS);
	#else
		leftEye.write(OPEN_POS);
	#endif

	return true;
}


/**
 * Enable Button interupt
 */
void ISR_en_btn_pressed(void) {
	unsigned long interrupt_time = millis();

	// If the time since the last interrupt is greater than the debounce time
	if (interrupt_time - last_interrupt_time > DEBOUNCE_TIME) {
		#ifdef EN_BTN_MOMENTARY
		enable = !enable;
		#else
		enable = digitalRead(ENABLE_BTN_PIN) == EN_BTN_PUSHED_STATE
		#endif
		last_interrupt_time = interrupt_time;
	}

	// All other interrupts during the debounce period are ignored
}