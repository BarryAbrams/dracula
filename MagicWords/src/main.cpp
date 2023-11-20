#include "PuzzleCommunication.h" 

#define RESET_PIN A1
#define OVERRIDE_PIN A0
#define UNSOLVED_PIN A2
#define SOLVED_PIN A3

#define SOLVED_TIMEOUT 1000
#define NUM_SENSORS 1

PuzzleCommunication puzzleComm(RESET_PIN, OVERRIDE_PIN, UNSOLVED_PIN, SOLVED_PIN, 255);

bool checkIsSolved() {
  return false;
}

void solve() {
 
}

void unsolve() {

}

void setup() {
  Serial.begin(9600);
  puzzleComm.begin();
  puzzleComm.setCallbacks(solve, unsolve, checkIsSolved);
}

void loop() {
  puzzleComm.update();
}