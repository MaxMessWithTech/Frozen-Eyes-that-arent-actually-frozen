/**
 * @name Frozen Eyes that aren't actually frozen but I like this name better
 * @author Max Miller
 * @version 1.1.0
 * @date Febuary 12th, 2026
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

// CHANGE TIMING ON EACH BLINK
#define OPENING_TIME 200
#define CLOSED_TIME 50
#define CLOSING_TIME 200

// DO NOT MODIFY UNLESS YOU'RE ABSOLUTLY SURE
// Changes the time between the loops, 
// if you make it too fast or slow it will cause problems.
#define STEP_TIME 10


// DO NOT MODIFY
#define OPEN_STEPS (OPENING_TIME / STEP_TIME)
// DO NOT MODIFY
#define OPEN_STEP_VALUE ((OPEN_POS - CLOSED_POS) / OPEN_STEPS)

// DO NOT MODIFY
#define CLOSE_STEPS (CLOSING_TIME / STEP_TIME)
// DO NOT MODIFY
#define CLOSE_STEP_VALUE ((OPEN_POS - CLOSED_POS) / CLOSE_STEPS)


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

bool closeEye(int closed_pos, int open_steps, int open_step_value, int step_time);
bool openEye(int open_pos, int close_steps, int close_step_value, int step_time);

void ISR_en_btn_pressed(void);

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

void loop() {
	if (!isEnabled()) { return; }
	// If it is time to blink, then blink
	if (timeToBlink()) {

		// Close Eye Lid
		closeEye(CLOSED_POS, OPEN_STEPS, OPEN_STEP_VALUE, STEP_TIME);

		// Stay closed for CLOSED_TIME
		delay(CLOSED_TIME);


		openEye(OPEN_POS, CLOSE_STEPS, CLOSE_STEP_VALUE, STEP_TIME);

		potVal = getPotVal();

		// This line creates a random amount of time (in milliseconds) until the next blink
		// The smallest number is MIN_BLINK_TIME + potVal
		// The largest number is (MIN_BLINK_TIME + 1) + (ANALOGUE_MOD * potVal)
		int rand_delta = random(MIN_BLINK_TIME + potVal, (MIN_BLINK_TIME + 1) + (ANALOGUE_MOD * potVal));
		
		next_blink_ms = millis() + rand_delta;
	}
}

bool isEnabled(void) {
	return enable;
}

bool timeToBlink(void) {
	return millis() > next_blink_ms;
}

int getPotVal(void) {
	return analogRead(SPEED_POT_PIN);
}

bool closeEye(int closed_pos, int open_steps, int open_step_value, int step_time) {
	for(i = 0; i < OPEN_STEPS - 1; i++) {
		// Write the next step to the servos
		leftEye.write(CLOSED_POS + i * OPEN_STEP_VALUE);
		rightEye.write(CLOSED_POS + i * OPEN_STEP_VALUE);
		
		delay(STEP_TIME);
	}

	// Fully closed
	leftEye.write(CLOSED_POS);
	rightEye.write(CLOSED_POS);

	return true;
}

bool openEye(int open_pos, int close_steps, int close_step_value, int step_time) {
	for(i = 0; i < close_steps - 1; i++) {
		leftEye.write(open_pos + i * close_step_value);
		rightEye.write(open_pos + i * close_step_value);

		delay(step_time);
	}

	// Fully open
	leftEye.write(open_pos);
	rightEye.write(open_pos);

	return true;
}

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