#include <Wire.h>
#include "PuzzleCommunication.h" 

#define RESET_PIN A2
#define OVERRIDE_PIN A3
#define UNSOLVED_PIN A1
#define SOLVED_PIN A0

PuzzleCommunication puzzleComm(RESET_PIN, OVERRIDE_PIN, UNSOLVED_PIN, SOLVED_PIN, 255);

#define BUTTON_LENGTH 4
#define DEBOUNCE_TIME 50
#define LED_ON_TIME 250 // 250ms LED on time
#define ANIMATION_INTERVAL 500 // 500ms between animation cycles
#define SOLUTION_DISPLAY_TIME 4000 // 4 seconds to display solution animation

int leds[BUTTON_LENGTH] = {5,7,10,12};
int buttons[BUTTON_LENGTH] = {4,6,9,11};
unsigned long lastDebounceTimes[BUTTON_LENGTH] = {0, 0, 0, 0};
int buttonStates[BUTTON_LENGTH] = {HIGH, HIGH, HIGH, HIGH};
int lastButtonStates[BUTTON_LENGTH] = {HIGH, HIGH, HIGH, HIGH};
unsigned long code = 0;
unsigned long solutionMatchTime = 0;


// blue green red yellow
// red, blue, yellow, green, blue, red
const unsigned long solution = 314213;

enum AnimationState {
  NONE,
  SOLVED_START,
  SOLVED_HOLD
};

AnimationState animationState = NONE;

unsigned long previousMillis = 0;

bool checkIsSolved() {
  bool solutionMatched = false;

  if (puzzleComm.getCurrentState() == UNSOLVED) {

    for (int i = 0; i < BUTTON_LENGTH; i++) {
      int reading = digitalRead(buttons[i]);

      if (reading != lastButtonStates[i]) {
        lastDebounceTimes[i] = millis();
      }

      if ((millis() - lastDebounceTimes[i]) > DEBOUNCE_TIME) {
        if (reading != buttonStates[i]) {
          buttonStates[i] = reading;

          if (buttonStates[i] == HIGH) {
          
              code = (code * 10 + i + 1) % 1000000; 
              digitalWrite(leds[i], HIGH); // Turn on the LED
              delay(LED_ON_TIME); // Keep the LED on for 250ms
              digitalWrite(leds[i], LOW); // Turn off the LED         
          }
        }
      }
      lastButtonStates[i] = reading;
    }

    if (code == solution) {
        solutionMatched = true;
    }
  }
  
  return solutionMatched;
}




void solve() {
    animationState = SOLVED_START;
    previousMillis = millis();
    solutionMatchTime = millis();
    code = 0;
}

void unsolve() {
    animationState = NONE;
    previousMillis = millis();
    code = 0;
    for (int i = 0; i < BUTTON_LENGTH; i++) {
        digitalWrite(leds[i], LOW); // Turn off all LEDs
      }
}

void setup() {
  Wire.begin();
  Serial.begin(9600);

  for (int i = 0; i<BUTTON_LENGTH; i++) {
    pinMode(leds[i], OUTPUT);
    pinMode(buttons[i], INPUT_PULLUP);

  }

  for (int i = 0; i<BUTTON_LENGTH; i++) {
    digitalWrite(leds[i], LOW);
  }

  pinMode(13, OUTPUT);
  puzzleComm.begin();
  puzzleComm.setCallbacks(solve, unsolve, checkIsSolved);
}

void loop() {
  puzzleComm.update();
  // Serial.print(digitalRead(UNSOLVED_PIN));
  // Serial.print(digitalRead(SOLVED_PIN));
  // Serial.println();




  if (puzzleComm.getCurrentState() == UNSOLVED) {
    digitalWrite(13,false);
  } else if (puzzleComm.getCurrentState() == SOLVED) {
    digitalWrite(13,true);
    unsigned long currentTime = millis();
    if (currentTime - solutionMatchTime < SOLUTION_DISPLAY_TIME) {
      for (int i = 0; i < BUTTON_LENGTH; i++) {
        digitalWrite(leds[i], HIGH); // Turn on all LEDs
      }
      delay(ANIMATION_INTERVAL);
      for (int i = 0; i < BUTTON_LENGTH; i++) {
        digitalWrite(leds[i], LOW); // Turn off all LEDs
      }
      delay(ANIMATION_INTERVAL);
    } else if (currentTime - solutionMatchTime >= SOLUTION_DISPLAY_TIME) {
      for (int i = 0; i < BUTTON_LENGTH; i++) {
        digitalWrite(leds[i], HIGH); // Turn off all LEDs
      }
    }

  }

  delay(10);
}