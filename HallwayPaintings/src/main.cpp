#include <FastLED.h>
#include "PuzzleCommunication.h"


#define NUM_LEDS 1
#define DATA_PIN 17
#define SOLVED_1_PIN 5
#define SOLVED_2_PIN 7
#define SOLVED_TIMEOUT 250

#define RESET_PIN A0
#define OVERRIDE_PIN 9
#define UNSOLVED_PIN A1
#define SOLVED_PIN A2
#define ALT_SOLVED_PIN A3

PuzzleCommunication puzzleComm(RESET_PIN, OVERRIDE_PIN, UNSOLVED_PIN, SOLVED_PIN, 255, ALT_SOLVED_PIN);

CRGB leds[NUM_LEDS];

enum AnimationState {
  NONE,
  SOLVED_FADE_TO_WHITE,
  SOLVED_HOLD_WHITE
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

  bool currentReading = !digitalRead(SOLVED_1_PIN);

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

bool checkIsAltSolved() {
  static unsigned long firstSolvedMillis = 0;
  static bool lastReading = false;

  bool currentReading = !digitalRead(SOLVED_2_PIN);

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

void altSolve() {
    animationState = SOLVED_FADE_TO_WHITE;
}

void solve() {
    animationState = SOLVED_FADE_TO_WHITE;
}

void unsolve() {
    animationState = NONE;
}

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  puzzleComm.begin();
  puzzleComm.setCallbacks(solve, unsolve, checkIsSolved);
  puzzleComm.setAltSolveCallback(altSolve, checkIsAltSolved);
  pinMode(SOLVED_1_PIN, INPUT_PULLUP);
  pinMode(SOLVED_2_PIN, INPUT_PULLUP);
}

void loop() {

  puzzleComm.update();

  if (puzzleComm.getCurrentState() == UNSOLVED) {
      setAllLeds(CRGB::Black);
  } else if (puzzleComm.getCurrentState() == FIRSTSOLVED) {
    switch (animationState) {
      case SOLVED_FADE_TO_WHITE:
        fadeFromTo(CRGB::Black, CRGB::White, 2000, SOLVED_HOLD_WHITE);
        break;
      case SOLVED_HOLD_WHITE:
        setAllLeds(CRGB::White);
        break;
      default:
        break;
    }
  } else if (puzzleComm.getCurrentState() == SECONDSOLVED) {
        setAllLeds(CRGB::Cyan);
  }

  FastLED.show();
  
  delay(50);
}