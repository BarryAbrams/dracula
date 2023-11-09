#include <FastLED.h>
#include <Wire.h>
#include <Tlv493d.h>
#include "PuzzleCommunication.h" 

#define TCA9548_ADDRESS 0x70
#define NUM_SENSORS 5
#define NEOPIXEL_PIN 5

#define NUM_LEDS 5
#define DATA_PIN 5

Tlv493d Tlv493dMagnetic3DSensors[NUM_SENSORS];
const uint8_t sensorOrder[NUM_SENSORS] = {0, 1, 2, 3, 4};  // Adjust based on your setup
int currentLed = 0; // Track which LED we're animating

CRGB leds[NUM_LEDS];

enum State {
  UNSOLVED,
  SOLVED
};

enum AnimationState {
  NONE,
  SOLVED_START, // fade from white at current currentBrightness to white at 100%
  SOLVED_STAGGER_TO_GREEN, // stagger each green one on one at a time
  SOLVED_HOLD // stay on green
};

State currentState = UNSOLVED;
AnimationState animationState = NONE;

unsigned long previousMillis = 0;
unsigned long solvedTime = 0;
unsigned long stableDelay = 2000;
const int breathingInterval = 6000; // Breathing effect over 6 seconds
const int fadeToGreenInterval = 2000;
int currentBrightness;
// Initialize the PuzzleCommunication with the required pins.
PuzzleCommunication puzzleComm(A2,A3,A0,A1);
int fadeToWhiteInterval = 250; // Duration of the fade to white

void setAllLeds(CRGB color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
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

void fadeAllToFullWhite() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis <= fadeToWhiteInterval) {
    // Calculate the brightness for the fade
    int brightness = map(currentMillis - previousMillis, 0, fadeToWhiteInterval, currentBrightness, 255);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(0, 0, brightness); // Set to white with the current brightness
    }
  } else {
    animationState = SOLVED_STAGGER_TO_GREEN; // Move to the next state
    previousMillis = currentMillis; // Reset the timer
    currentLed = 0; // Start with the first LED
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
      fadeAllToFullWhite();
      break;
    case SOLVED_STAGGER_TO_GREEN:
      staggerToGreen();
      break;
    case SOLVED_HOLD:
      holdOnGreen(); // Nothing needs to be done in this case
      break;
    default:
      // If for some reason the state is NONE or any undefined state,
      // you might want to reset to a known state
      break;
  }
}

int calculateValue(float incomingValue) {
  int zStatus = 0;
  if (incomingValue < -0.3) {
    zStatus = -1;
  } else if (incomingValue > 0.3) {
    zStatus = 1;
  }

  return -(incomingValue * 10);
}

void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548_ADDRESS);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  puzzleComm.begin();

  for (int i = 0; i < NUM_SENSORS; i++) {
    tcaSelect(i);
    Tlv493dMagnetic3DSensors[i].begin();
  }
}

bool isSolved() {
   bool allSensorsNegative = true; // Assume all sensors are negative until proven otherwise
   for (int i = 0; i < NUM_SENSORS; i++) {
    tcaSelect(sensorOrder[i]);
    Tlv493dMagnetic3DSensors[i].updateData();

    float zValue = Tlv493dMagnetic3DSensors[i].getZ();
    int sensorValue = calculateValue(zValue);
    
    if (sensorValue >= 0) {
      allSensorsNegative = false;
    }
   }

   return allSensorsNegative;
}

void loop() {
  // Read the puzzle states from the communication pins
  bool reset = puzzleComm.readReset();
  bool override = puzzleComm.readOverride();
  
  // Check for reset condition
  if (reset) {
    currentState = UNSOLVED;
    animationState = NONE;
    puzzleComm.setPuzzleState(false); // Update output state to UNSOLVED
    Serial.println("Puzzle state reset to UNSOLVED.");
  }

  // Override logic: if override is true, solve the puzzle
  if (override) {
    currentState = SOLVED;
    animationState = SOLVED_START;
    previousMillis = millis();
    puzzleComm.setPuzzleState(true); // Update output state to SOLVED
    Serial.println("Puzzle overridden to SOLVED.");
  }

  // Main puzzle logic: magnetic switch checking
  bool reading = isSolved();
  if (currentState == UNSOLVED and reading == true) {
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
  } else if (reading == HIGH) {
    solvedTime = 0; // Reset the switch closed timer if the magnet is not detected
  }

  // Execute the breathing or solved animation based on the current state
  if (currentState == UNSOLVED) {
    breatheWhite();
  } else if (currentState == SOLVED) {
    runSolvedAnimation();

  }

  // Update the LEDs
  FastLED.show();

  // Update the output state to reflect the current state
  puzzleComm.updateOutputs();

  delay(10);
}