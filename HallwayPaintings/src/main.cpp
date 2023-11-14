#include <Wire.h>
#include <Tlv493d.h>

#include <FastLED.h>
#include "PuzzleCommunication.h"

#define NUM_LEDS 1
#define DATA_PIN 17
#define SOLVED_TIMEOUT 1000

#define TCA9548_ADDRESS 0x70
#define NUM_SENSORS 6

Tlv493d Tlv493dMagnetic3DSensors[NUM_SENSORS];
const uint8_t sensorOrder[NUM_SENSORS] = {0,1,2,3,4,5};  // Adjust based on your setup
int sensorsValues[NUM_SENSORS] = {0};
int currentSensor = 0;


#define RESET_PIN A3
#define OVERRIDE_PIN A4
#define UNSOLVED_PIN A2
#define SOLVED_PIN A1
#define ALT_SOLVED_PIN A0

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

bool checkIsAltSolved() {
  static unsigned long firstSolvedMillis = 0;
  static bool lastReading = false;

  bool currentReading = true;
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (sensorsValues[i] != -1) {
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

void altSolve() {
    animationState = SOLVED_FADE_TO_WHITE;
}

void solve() {
    animationState = SOLVED_FADE_TO_WHITE;
}

void unsolve() {
    animationState = NONE;
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

void setup() {
  Wire.begin();
  Serial.begin(9600);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  puzzleComm.begin();
  puzzleComm.setCallbacks(solve, unsolve, checkIsSolved);
  puzzleComm.setAltSolveCallback(altSolve, checkIsAltSolved);

  Serial.println("TEST");
  for (int i = 0; i < NUM_SENSORS; i++) {
    tcaSelect(i);
    Tlv493dMagnetic3DSensors[i].begin();
    Tlv493dMagnetic3DSensors[i].setAccessMode(Tlv493dMagnetic3DSensors[i].MASTERCONTROLLEDMODE);
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