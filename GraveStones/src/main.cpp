#include "PuzzleCommunication.h" 

#define RESET_PIN A2
#define OVERRIDE_PIN A3
#define UNSOLVED_PIN A1
#define SOLVED_PIN A0

PuzzleCommunication puzzleComm(RESET_PIN, OVERRIDE_PIN, UNSOLVED_PIN, SOLVED_PIN, 255);

#define SOLVED_TIMEOUT 1000

#define NUM_BUTTONS 5
int buttons[NUM_BUTTONS] = {7,9,10,11,12};
bool lastButtonState[NUM_BUTTONS] = {HIGH, HIGH, HIGH, HIGH, HIGH};
unsigned long lastDebounceTime[NUM_BUTTONS] = {0, 0, 0, 0, 0};
unsigned long previousMillis = 0;
unsigned long debounceDelay = 50;
bool pulseActive = false;
unsigned long pulseStartTime;
const unsigned long pulseDuration = 250;
bool pulseTriggered[NUM_BUTTONS] = {false, false, false, false, false};

bool checkIsSolved() {
  bool isSolved = true;
  for (int i = 0; i < NUM_BUTTONS; i++) {
      if (digitalRead(buttons[i]) == HIGH) {
        isSolved = false;
      }
  }
  return isSolved;
}

void solve() {

}

void unsolve() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
      pulseTriggered[i] = false;
  }
}


void setup() {
  Serial.begin(9600);

  puzzleComm.begin();
  puzzleComm.setCallbacks(solve, unsolve, checkIsSolved);

  for (int i=0; i<NUM_BUTTONS; i++) {
    pinMode(buttons[i], INPUT_PULLUP);
  }

}


void loop() {


  Serial.print(millis());
  Serial.print("\t");
  Serial.print(digitalRead(RESET_PIN));
  Serial.print("\t");
  Serial.print(digitalRead(OVERRIDE_PIN));
  Serial.print("\t");
  Serial.print(digitalRead(SOLVED_PIN));
  Serial.print("\t");
  Serial.println(digitalRead(UNSOLVED_PIN));
  // Serial.print("\t");
  // Serial.println(puzzleComm.getCurrentState());

  if (puzzleComm.getCurrentState() == UNSOLVED) {
    digitalWrite(13,false);

    for (int i = 0; i < NUM_BUTTONS; i++) {
      int reading = digitalRead(buttons[i]);
      if (reading != lastButtonState[i]) {
        lastDebounceTime[i] = millis();
      }
      
      if ((millis() - lastDebounceTime[i]) > debounceDelay) {
        if (reading == LOW  && !pulseTriggered[i]) {
          pulseActive = true;
          pulseStartTime = millis();
          digitalWrite(UNSOLVED_PIN, HIGH);
          digitalWrite(SOLVED_PIN, HIGH);
          pulseTriggered[i] = true;
        }
      }
      
      lastButtonState[i] = reading;
    }

    if (pulseActive && (millis() - pulseStartTime) >= pulseDuration) {
      pulseActive = false;
      digitalWrite(UNSOLVED_PIN, HIGH);
      digitalWrite(SOLVED_PIN, LOW);
    }

    // Serial.println("UNSOLVED");

  } else if (puzzleComm.getCurrentState() == SOLVED) {
    // Serial.println("SOLVED");
    digitalWrite(13,true);
  }
  
  if (!pulseActive) {
    puzzleComm.update();
  }

  delay(50);
}