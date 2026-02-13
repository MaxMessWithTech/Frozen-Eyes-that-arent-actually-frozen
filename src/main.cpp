/**
 * @name Frozen Eyes that aren't actually frozen but I like this name better
 * @author Max Miller
 * @version 1.0.2
 * @date Febuary 12th, 2026
 * @details I think my father isn't going to read this, but if he does, "hi dad..."
 */

// ------------------  INCLUDES  ------------------
#include <Arduino.h>
#include <Servo.h>


// ----------------  DEFINITIONS  ----------------

#define LEFT_EYE_PIN DD6
#define RIGHT_EYE_PIN DD5
#define ENABLE_BTN_PIN DD2
#define SPEED_POT_PIN A0

// Change how much of an effect the potenitometer has
#define ANALOGUE_MOD 3

#define CLOSED_POS 0
#define OPEN_POS 100

#define OPENING_TIME 200
#define CLOSED_TIME 50
#define CLOSING_TIME 200

// Changes the time between the loops, 
// if you make it too fast or slow it will cause problems.
#define STEP_TIME 10


// --------------  END DEFINITIONS  --------------

Servo leftEye;
Servo rightEye;

int pos = 0;
unsigned long next_blink_ms = 0;
int next_blink_time = 0;
int i;

void setup() {
  leftEye.attach(LEFT_EYE_PIN);
  rightEye.attach(RIGHT_EYE_PIN);

  leftEye.write(OPEN_POS);
  rightEye.write(OPEN_POS);

  pinMode(ENABLE_BTN_PIN, INPUT_PULLUP);
  pinMode(SPEED_POT_PIN, INPUT);

  // Set random seed using a unconected analogue pin's value
  randomSeed(analogRead(A1));
}

void loop() {
  if (millis() > next_blink_ms && digitalRead(ENABLE_BTN_PIN) == HIGH) {
    const int open_steps = OPENING_TIME / STEP_TIME;
    const int open_step_value = (OPEN_POS - CLOSED_POS) / open_steps;

    for(i = 0; i < open_steps; i++) {
      leftEye.write(CLOSED_POS + i * open_step_value);
      rightEye.write(CLOSED_POS + i * open_step_value);

      delay(STEP_TIME);
    }

    delay(CLOSED_TIME - STEP_TIME);

    const int close_steps = CLOSED_TIME / STEP_TIME;
    const int close_step_value = (CLOSED_POS - OPEN_POS) / open_steps;

    for(i = 0; i < close_steps; i++) {
      leftEye.write(OPEN_POS + i * close_step_value);
      rightEye.write(OPEN_POS + i * close_step_value);

      delay(STEP_TIME);
    }

    next_blink_ms = millis() + random(500, ANALOGUE_MOD * analogRead(SPEED_POT_PIN));
  }
}
