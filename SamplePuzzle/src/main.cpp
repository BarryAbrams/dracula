#include <FastLED.h>
#include <Wire.h>
#include "PuzzleCommunication.h" 

#define NEOPIXEL_PIN 5
#define NUM_LEDS 20
#define DATA_PIN 5

#define RESET_PIN A3
#define OVERRIDE_PIN A4
#define UNSOLVED_PIN A2
#define SOLVED_PIN A1
#define SOLVABLE_PIN A0

PuzzleCommunication puzzleComm(RESET_PIN, OVERRIDE_PIN, UNSOLVED_PIN, SOLVED_PIN, SOLVABLE_PIN);

int currentLed = 0;

CRGB leds[NUM_LEDS];

enum AnimationState {
  NONE,
  SOLVED_START,
  SOLVED_HOLD
};

AnimationState animationState = NONE;

unsigned long previousMillis = 0;
const int breathingInterval = 6000; // Breathing effect over 6 seconds
int currentBrightness;
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
    leds[i] = CHSV(0, 255, currentBrightness);
  }
}

void breatheWhite() {
  unsigned long currentMillis = millis();
  unsigned long timeIntoInterval = (currentMillis - previousMillis) % breathingInterval;
  currentBrightness = map(sin8(timeIntoInterval * 255 / breathingInterval), 0, 255, (255 * 25) / 100, (255 * 75) / 100);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(0, 0, currentBrightness);
  }
}

void staggerToGreen() {
  unsigned long currentMillis = millis();
  if (currentLed < NUM_LEDS && currentMillis - previousMillis > fadeToWhiteInterval / NUM_LEDS) {
    leds[currentLed++] = CRGB::Green; 
    previousMillis = currentMillis;
    if (currentLed >= NUM_LEDS) {
      animationState = SOLVED_HOLD;
    }
  }
}

void holdOnGreen() {

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
      break;
  }
}

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
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  puzzleComm.begin();
  puzzleComm.setCallbacks(solve, unsolve, checkIsSolved);
}

void loop() {
  puzzleComm.update();

  if (puzzleComm.getCurrentState() == UNSOLVED) {
    if (puzzleComm.getSolvable()) {
      breatheWhite();
    } else {
      breatheRed();
    }
  } else if (puzzleComm.getCurrentState() == SOLVED) {
    runSolvedAnimation();
  }

  FastLED.show();

  delay(10);
}