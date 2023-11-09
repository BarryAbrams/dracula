#include <FastLED.h>
#include <Wire.h>
#include "PuzzleCommunication.h" 

#define NEOPIXEL_PIN 5
#define NUM_LEDS 20
#define DATA_PIN 5

int currentLed = 0; // Track which LED we're animating
bool prevSolvable = false;

CRGB leds[NUM_LEDS];

enum State {
  UNSOLVED,
  SOLVED
};

enum AnimationState {
  NONE,
  SOLVED_START,
  SOLVED_HOLD
};

State currentState = UNSOLVED;
AnimationState animationState = NONE;

unsigned long previousMillis = 0;
unsigned long solvedTime = 0;
unsigned long stableDelay = 2000;
const int breathingInterval = 6000; // Breathing effect over 6 seconds
int currentBrightness;

//resetPin, overridePin, unsolvedPin, solvedPin, solvablePin
PuzzleCommunication puzzleComm(A3,A4,A2,A1,A0);
int fadeToWhiteInterval = 250; // Duration of the fade to white

void setAllLeds(CRGB color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
}

void breatheRed() {
  unsigned long currentMillis = millis();
  unsigned long timeIntoInterval = (currentMillis - previousMillis) % breathingInterval;
  currentBrightness = map(sin8(timeIntoInterval * 255 / breathingInterval), 0, 255, (255 * 25) / 100, (255 * 75) / 100);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(0, 255, currentBrightness); // Hue 0 (red), Saturation 0 (white), Value as brightness
  }
}

void breatheWhite() {
  unsigned long currentMillis = millis();
  unsigned long timeIntoInterval = (currentMillis - previousMillis) % breathingInterval;
  currentBrightness = map(sin8(timeIntoInterval * 255 / breathingInterval), 0, 255, (255 * 25) / 100, (255 * 75) / 100);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(0, 0, currentBrightness); // Hue 0 (red), Saturation 0 (white), Value as brightness
  }
}

void staggerToGreen() {
  unsigned long currentMillis = millis();
  // Only proceed if the current LED is within the range and the stagger time has passed
  if (currentLed < NUM_LEDS && currentMillis - previousMillis > fadeToWhiteInterval / NUM_LEDS) {
    leds[currentLed++] = CRGB::Green; // Fade the current LED to green
    previousMillis = currentMillis; // Reset the timer for the next LED
    if (currentLed >= NUM_LEDS) { // If all LEDs are green, move to the HOLD state
      animationState = SOLVED_HOLD;
    }
  }
}

void holdOnGreen() {
  // Keep all LEDs on green, nothing changes in this state
}

void runSolvedAnimation() {
  switch (animationState) {
    case SOLVED_START:
      staggerToGreen();
      break;
    case SOLVED_HOLD:
      holdOnGreen();
      break;
    default:
      // If for some reason the state is NONE or any undefined state,
      // you might want to reset to a known state
      break;
  }
}


void setup() {
  Wire.begin();
  Serial.begin(9600);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  puzzleComm.begin();

}

bool isSolved() {
   return false;
}


void loop() {
  // Read the puzzle states from the communication pins
  bool reset = puzzleComm.readReset();
  bool override = puzzleComm.readOverride();
  bool solvable = puzzleComm.readSolvable();
  
  if (prevSolvable != solvable) {
    puzzleComm.setPuzzleState(currentState);
  }

  prevSolvable = solvable;

  // Check for reset condition
  if (reset and currentState != UNSOLVED) {
    currentState = UNSOLVED;
    animationState = NONE;
    previousMillis = millis();
    currentLed = 0;
    puzzleComm.setPuzzleState(false); // Update output state to UNSOLVED
    Serial.println("Puzzle state reset to UNSOLVED.");
  }

  // Override logic: if override is true, solve the puzzle
  if (override and currentState != SOLVED && solvable) {
    currentState = SOLVED;
    animationState = SOLVED_START;
    previousMillis = millis();
    puzzleComm.setPuzzleState(true); // Update output state to SOLVED
    Serial.println("Puzzle overridden to SOLVED.");
  }

  // Main puzzle logic: magnetic switch checking
  bool solved = isSolved();
  if (currentState == UNSOLVED and solved == true and solvable) {
    if (solvedTime == 0) {
      solvedTime = millis();
    } else if ((millis() - solvedTime) >= stableDelay) {
      currentState = SOLVED;
      animationState = SOLVED_START;
      puzzleComm.setPuzzleState(true); // Update output state to SOLVED
      solvedTime = 0; 
      previousMillis = millis();
      Serial.println("Puzzle solved by correct combo.");
    }
  } else if (solved == HIGH) {
    solvedTime = 0; // Reset the switch closed timer if the magnet is not detected
  }

  // Execute the breathing or solved animation based on the current state
  if (currentState == UNSOLVED) {
    if (solvable) {
      breatheWhite();
    } else {
      breatheRed();
    }
  } else if (currentState == SOLVED) {
    runSolvedAnimation();
  }

  // Update the LEDs
  FastLED.show();

  // Update the output state to reflect the current state
  puzzleComm.updateOutputs();

  delay(10);
}