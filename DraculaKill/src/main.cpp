#include <FastLED.h>
#include "PuzzleCommunication.h" // Include the PuzzleCommunication library

#define NUM_LEDS 4
#define DATA_PIN 5
#define MAGNET_SWITCH_PIN 9

CRGB leds[NUM_LEDS];

enum State {
  UNSOLVED,
  SOLVED
};

enum AnimationState {
  SOLVED_START,
  SOLVED_ORANGE,
  SOLVED_FADE_TO_RED,
  SOLVED_FLICKER,
  SOLVED_FADE_TO_BLACK,
  SOLVED_HOLD_BLACK
};

State currentState = UNSOLVED;
AnimationState animationState = SOLVED_START;

unsigned long previousMillis = 0;
unsigned long switchClosedTime = 0; // Stores the time when the switch was closed

const int breathingInterval = 6000; // Breathing effect over 6 seconds
const int fadeToRedInterval = 2000; // Fade to red over 2 seconds
const int flickerInterval = 5000;   // Flicker over 5 seconds
const int fadeToBlackInterval = 3000; // Fade to black over 3 seconds
const unsigned long stableDelay = 250;  // Time in ms to wait for the switch to be stable
bool prevReset = false;
bool prevOverride = false;
bool prevSolvable = false;
int currentBrightness;

// Initialize the PuzzleCommunication with the required pins.
// If the solvable pin is not being used, omit it from the constructor call.
PuzzleCommunication puzzleComm(A3,A4,A2,A1,A0);

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

// This function creates a breathing effect by oscillating the brightness of white color
void breatheWhite() {
  unsigned long currentMillis = millis();
  unsigned long timeIntoInterval = (currentMillis - previousMillis) % breathingInterval;
  currentBrightness = map(sin8(timeIntoInterval * 255 / breathingInterval), 0, 255, (255 * 25) / 100, (255 * 75) / 100);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(0, 0, currentBrightness); // Hue 0 (red), Saturation 0 (white), Value as brightness
  }
}

// This function fades the LEDs from orange to red over a set interval
void fadeToRed() {
  unsigned long currentMillis = millis();
  int progress = map(currentMillis - previousMillis, 0, fadeToRedInterval, 0, 255);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].r = 255; // Max red
    leds[i].g = 255 - progress; // Reduce green to transition from orange to red
    leds[i].b = 0; // No blue
  }
}

// This function makes the LEDs flicker between red and black
void flickerRed() {
  static unsigned long lastToggleMillis = 0;
  static bool redState = true;
  static unsigned long nextInterval = 0;

  unsigned long currentMillis = millis();

  // If we have not set the next interval or the current time has passed the next toggle moment
  if (nextInterval == 0 || currentMillis - lastToggleMillis > nextInterval) {
    // Toggle the state
    redState = !redState;
    setAllLeds(redState ? CRGB::Red : CRGB::Black);

    // Record the time of this toggle
    lastToggleMillis = currentMillis;

    // Set the next interval to a random value
    // For example, random between 100ms and 500ms
    nextInterval = random(100, 500);
  }
}

// This function fades the LEDs from red to black over a set interval
void fadeToBlack() {
  unsigned long currentMillis = millis();
  unsigned long timeSinceFadeStart = currentMillis - previousMillis;

  if (timeSinceFadeStart <= fadeToBlackInterval) {
    uint8_t fadeValue = map(timeSinceFadeStart, 0, fadeToBlackInterval, 255, 0);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].r = fadeValue; // Fade red component
      leds[i].g = 0; // No green component
      leds[i].b = 0; // No blue component
    }
  } else {
    setAllLeds(CRGB::Black); // If the interval has passed, set to black
  }
}

void runSolvedAnimation() {
  unsigned long currentMillis = millis();
  switch (animationState) {
    case SOLVED_ORANGE:
      // Stay orange, wait for fade to red
      if (currentMillis - previousMillis >= fadeToRedInterval) {
        animationState = SOLVED_FADE_TO_RED;
        previousMillis = millis();
      }
      break;
    case SOLVED_FADE_TO_RED:
      fadeToRed();
      if (currentMillis - previousMillis >= fadeToRedInterval) {
        animationState = SOLVED_FLICKER;
        previousMillis = millis();
      }
      break;
    case SOLVED_FLICKER:
      flickerRed();
      if (currentMillis - previousMillis >= flickerInterval) {
        animationState = SOLVED_FADE_TO_BLACK;
        previousMillis = millis();
      }
      break;
    case SOLVED_FADE_TO_BLACK:
      fadeToBlack();
      if (currentMillis - previousMillis >= fadeToBlackInterval) {
        animationState = SOLVED_HOLD_BLACK;
      }
      break;
    case SOLVED_HOLD_BLACK:
      // Hold black, do nothing
      break;
    default:
      // Should not get here
      break;
  }
}

bool isSolved() {
  bool reading = !digitalRead(MAGNET_SWITCH_PIN);
  // Serial.println(reading);
  return reading;
}

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  puzzleComm.begin(); // Initialize the communication pins
  pinMode(MAGNET_SWITCH_PIN, INPUT_PULLUP);
}

void loop() {
  // Read the puzzle states from the communication pins
  bool reset = puzzleComm.readReset();
  bool override = puzzleComm.readOverride();
  bool solvable = puzzleComm.readSolvable();

  bool resetAction = false;
  if (prevReset != reset) {
    if (reset) {
      resetAction = true;
    }
  }

  bool overrideAction = false;
  if (prevOverride != override) {
    if (override) {
      overrideAction = true;
    }
  }

  if (prevSolvable != solvable) {
    puzzleComm.setPuzzleState(currentState);
  }
  
  // Check for reset condition
  if (resetAction && currentState != UNSOLVED) {
    currentState = UNSOLVED;
    animationState = SOLVED_START;
    previousMillis = millis();
    puzzleComm.setPuzzleState(false); // Update output state to UNSOLVED
    // Serial.println("Puzzle state reset to UNSOLVED.");
  }

  // Override logic: if override is true, solve the puzzle
  if (overrideAction and currentState != SOLVED && solvable) {
    currentState = SOLVED;
    animationState = SOLVED_ORANGE;
    puzzleComm.setPuzzleState(true); // Update output state to SOLVED
    // Serial.println("Puzzle overridden to SOLVED.");
  }

  bool solved = isSolved();
  if (currentState == UNSOLVED and solved == true and solvable) {
    if (switchClosedTime == 0) {
      switchClosedTime = millis();
    } else if ((millis() - switchClosedTime) >= stableDelay) {
      currentState = SOLVED;
      animationState = SOLVED_ORANGE;
      puzzleComm.setPuzzleState(true); // Update output state to SOLVED
      switchClosedTime = 0; 
      previousMillis = millis();
      // Serial.println("Puzzle solved by magnetic switch.");
    }
  } else if (!solved) {
    switchClosedTime = 0; // Reset the switch closed timer if the magnet is not detected
  }

  // Execute the breathing or solved animation based on the current state
  if (currentState == UNSOLVED) {
    if (solvable) {
      breatheRed();
    } else {
      breatheWhite();
    }
  } else if (currentState == SOLVED) {
    runSolvedAnimation();
  }


  // Update the LEDs
  FastLED.show();

  // Update the output state to reflect the current state
  puzzleComm.updateOutputs();

  prevSolvable = solvable;
  prevOverride = override;
  prevReset = reset;

  // Debug: Add a small delay to make the serial output readable
  delay(50);
}