#include <Wire.h>
#include "PuzzleCommunication.h" 

#define RESET_PIN A4
#define OVERRIDE_PIN A5
#define UNSOLVED_PIN A3
#define SOLVED_PIN A2

PuzzleCommunication puzzleComm(RESET_PIN, OVERRIDE_PIN, UNSOLVED_PIN, SOLVED_PIN, 255);

enum AnimationState {
  NONE,
  SOLVED_START,
  SOLVED_HOLD
};

AnimationState animationState = NONE;

unsigned long previousMillis = 0;

bool checkIsSolved() {
   return false;
}

void solve() {
    animationState = SOLVED_START;
    previousMillis = millis();
}

void unsolve() {
    animationState = NONE;
    previousMillis = millis();
}

void setup() {
  Wire.begin();
  Serial.begin(9600);

  pinMode(13, OUTPUT);
  puzzleComm.begin();
  puzzleComm.setCallbacks(solve, unsolve, checkIsSolved);
}

void loop() {
  puzzleComm.update();
  Serial.println(puzzleComm.getCurrentState());

  if (puzzleComm.getCurrentState() == UNSOLVED) {
    digitalWrite(13,false);
  } else if (puzzleComm.getCurrentState() == SOLVED) {
    digitalWrite(13,true);
  }


  delay(10);
}