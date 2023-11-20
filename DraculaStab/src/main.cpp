#include <FastLED.h>
#include "PuzzleCommunication.h"


#define NUM_LEDS 4
#define DATA_PIN 5
#define MAGNET_SWITCH_PIN 9
#define SOLVED_TIMEOUT 250

#define RESET_PIN A3
#define OVERRIDE_PIN A4
#define UNSOLVED_PIN A2
#define SOLVED_PIN A1
#define SOLVABLE_PIN A0

PuzzleCommunication puzzleComm(RESET_PIN, OVERRIDE_PIN, UNSOLVED_PIN, SOLVED_PIN, SOLVABLE_PIN, 255);

CRGB leds[NUM_LEDS];

enum AnimationState {
  NONE,
  SOLVED_START,
  SOLVED_FADE_TO_RED,
  SOLVED_FLICKER,
  SOLVED_FADE_TO_BLACK,
  SOLVED_HOLD_BLACK
};

AnimationState animationState = NONE;

void setAllLeds(CRGB color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
}

void breathe(CRGB color, unsigned long interval) {
  unsigned long currentMillis = millis();
  unsigned long timeIntoInterval = currentMillis % interval;
  int brightness = map(sin8(timeIntoInterval * 255 / interval), 0, 255, (255 * 25) / 100, (255 * 75) / 100);

  CRGB newColor = color;
  newColor.nscale8_video(brightness);
  
  setAllLeds(newColor);
}

void fadeFromTo(CRGB colorStart, CRGB colorEnd, unsigned long fadeDuration, AnimationState onEnd) {
    static unsigned long startFadeMillis = 0;
    static AnimationState lastOnEnd = NONE; // Add a static variable to track the last onEnd state

    unsigned long currentMillis = millis();
    
    if (lastOnEnd != onEnd) {
        startFadeMillis = currentMillis;
        lastOnEnd = onEnd;
    }

    unsigned long timeSinceFadeStart = currentMillis - startFadeMillis;

    if (timeSinceFadeStart <= fadeDuration) {
        float progress = (float)timeSinceFadeStart / (float)fadeDuration;
        CRGB currentColor = blend(colorStart, colorEnd, progress * 255);
        setAllLeds(currentColor);
    } else {
        setAllLeds(colorEnd);
        animationState = onEnd;
    }
}

void flicker(CRGB color, unsigned long totalDuration, unsigned long minInterval, unsigned long maxInterval, AnimationState onEnd) {
  static unsigned long lastToggleMillis = 0;
  static unsigned long startMillis = millis();
  static bool isOn = true;
  static unsigned long nextInterval = 0;

  unsigned long currentMillis = millis();

  if (currentMillis - startMillis > totalDuration) {
    animationState = onEnd;
    return;
  }

  if (nextInterval == 0 || currentMillis - lastToggleMillis > nextInterval) {
    isOn = !isOn;
    setAllLeds(isOn ? color : CRGB::Black);

    lastToggleMillis = currentMillis;
    nextInterval = random(minInterval, maxInterval);
  }
}

bool checkIsSolved() {
  static unsigned long firstSolvedMillis = 0;
  static bool lastReading = false;

  bool currentReading = !digitalRead(MAGNET_SWITCH_PIN);

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
    animationState = SOLVED_FADE_TO_RED;
}

void unsolve() {
    animationState = NONE;
}

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  puzzleComm.begin();
  puzzleComm.setCallbacks(solve, unsolve, checkIsSolved);
  pinMode(MAGNET_SWITCH_PIN, INPUT_PULLUP);
}

void loop() {
  puzzleComm.update();

  if (puzzleComm.getCurrentState() == UNSOLVED) {
    if (puzzleComm.getSolvable()) {
      breathe(CRGB::White, 6000);
    } else {
      breathe(CRGB::Red, 6000);
    }
  } else if (puzzleComm.getCurrentState() == SOLVED) {
    switch (animationState) {
      case SOLVED_FADE_TO_RED:
        fadeFromTo(CRGB::White, CRGB::Red, 2000, SOLVED_FLICKER);
        break;
      case SOLVED_FLICKER:
        flicker(CRGB::Red, 4000, 100, 500, SOLVED_FADE_TO_BLACK);
        break;
      case SOLVED_FADE_TO_BLACK:
        fadeFromTo(CRGB::Red, CRGB::Black, 2000, SOLVED_HOLD_BLACK);
        break;
      case SOLVED_HOLD_BLACK:
        setAllLeds(CRGB::Black);
        break;
      default:
        break;
    }
  }

  FastLED.show();
  
  delay(50);
}