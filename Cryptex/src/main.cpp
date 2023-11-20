#include <FastLED.h>
#include <Wire.h>
#include <Tlv493d.h>
#include "PuzzleCommunication.h" 

#define TCA9548_ADDRESS 0x70
#define NUM_SENSORS 5
#define NEOPIXEL_PIN 5
#define SOLVED_TIMEOUT 1000

#define NUM_LEDS 5
#define DATA_PIN 5

Tlv493d Tlv493dMagnetic3DSensors[NUM_SENSORS];
const uint8_t sensorOrder[NUM_SENSORS] = {0, 1, 2, 3, 4};  // Adjust based on your setup
int sensorsValues[NUM_SENSORS] = {0};

CRGB leds[NUM_LEDS];

enum AnimationState {
  NONE,
  SOLVED_START, // fade from white at current currentBrightness to white at 100%
  SOLVED_STAGGER_TO_GREEN, // stagger each green one on one at a time
  SOLVED_HOLD // stay on green
};
int currentLed = 0; // Track which LED we're animating
AnimationState animationState = NONE;

unsigned long previousMillis = 0;
unsigned long solvedTime = 0;
unsigned long stableDelay = 2000;
const int breathingInterval = 6000; // Breathing effect over 6 seconds
const int fadeToGreenInterval = 2000;
int currentBrightness;
// Initialize the PuzzleCommunication with the required pins.

#define RESET_PIN A3
#define OVERRIDE_PIN A2
#define UNSOLVED_PIN A1
#define SOLVED_PIN A0

PuzzleCommunication puzzleComm(RESET_PIN, OVERRIDE_PIN, UNSOLVED_PIN, SOLVED_PIN, 255, 255);


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


int calculateValue(float incomingValue) {
  int zStatus = 0;
   if (incomingValue < - 0.3) {
    zStatus = -1;
  } else if (incomingValue > 0.3) {
    zStatus = 1;
  }

  return zStatus;
}

void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548_ADDRESS);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

void updateSensors() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    tcaSelect(sensorOrder[i]);
    Tlv493dMagnetic3DSensors[i].updateData();
    float zValue = Tlv493dMagnetic3DSensors[i].getZ();
    int sensorValue = calculateValue(zValue);
    sensorsValues[i] = sensorValue;
  }
}


bool checkIsSolved() {
  static unsigned long firstSolvedMillis = 0;
  static bool lastReading = false;

  bool currentReading = true;
  for (int i = 0; i < NUM_SENSORS; i++) {
      if (sensorsValues[i] != 1) {
          currentReading = false;
          break;
      }
  }

  if (currentReading) {
    if (!lastReading) {
      firstSolvedMillis = millis();
    } else if (millis() - firstSolvedMillis >= SOLVED_TIMEOUT) {
      return true;
    }
  } else {
    firstSolvedMillis = 0;
  }

  lastReading = currentReading;
  return false;
}


void solve() {
    animationState = SOLVED_START;
}

void unsolve() {
    animationState = NONE;
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  puzzleComm.begin();
  puzzleComm.setCallbacks(solve, unsolve, checkIsSolved);

  for (int i = 0; i < NUM_SENSORS; i++) {
    tcaSelect(i);
    Tlv493dMagnetic3DSensors[i].begin();
    Tlv493dMagnetic3DSensors[i].setAccessMode(Tlv493dMagnetic3DSensors[i].ULTRALOWPOWERMODE);
    Tlv493dMagnetic3DSensors[i].disableTemp();
  }
}

void loop() {

  updateSensors();

  for (int i =0; i<NUM_SENSORS; i++) {
    Serial.print(sensorsValues[i]);
  }

  Serial.println();

  puzzleComm.update();
  Serial.println(puzzleComm.getCurrentState());

  if (puzzleComm.getCurrentState() == UNSOLVED) {
        // setAllLeds(CRGB::Black);
        breatheWhite();
    } else if (puzzleComm.getCurrentState() == SOLVED) {
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

  // Update the LEDs
  FastLED.show();

  delay(10);
}