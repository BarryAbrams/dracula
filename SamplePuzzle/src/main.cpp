#include <Tlv493d.h>
#include <FastLED.h>
#include "PuzzleCommunication.h" 

#define RESET_PIN 9
#define OVERRIDE_PIN A3
#define UNSOLVED_PIN A0
#define SOLVED_PIN A2

#define SOLVED_TIMEOUT 1000
#define NUM_SENSORS 1

#define NUM_LEDS 1
#define DATA_PIN 17

CRGB leds[NUM_LEDS];

PuzzleCommunication puzzleComm(RESET_PIN, OVERRIDE_PIN, UNSOLVED_PIN, SOLVED_PIN, 255);

Tlv493d Tlv493dMagnetic3DSensor = Tlv493d();
float sensorsValues[] = {0};

enum AnimationState {
  NONE,
  SOLVED_START,
  SOLVED_HOLD
};

AnimationState animationState = NONE;

unsigned long previousMillis = 0;

bool checkIsSolved() {
    for (int i = 0; i < NUM_SENSORS; i++) {
      if (sensorsValues[i] > -2) {
        return false;
      } 
    }
  return true;
}

void solve() {
    animationState = SOLVED_START;
    previousMillis = millis();
}

void unsolve() {
    animationState = NONE;
    previousMillis = millis();
}

int calculateValue(float incomingValue) {
  int zStatus = 0;
   if (incomingValue < - 0.3) {
    zStatus = 1;
  } else if (incomingValue > 0.3) {
    zStatus = 2;
  }

  return incomingValue;
}

void updateSensors() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    Tlv493dMagnetic3DSensor.updateData();
    float zValue = Tlv493dMagnetic3DSensor.getZ();
    int sensorValue = calculateValue(zValue);
    sensorsValues[i] = zValue;
  }
}

void initalizeSensors() {
  Tlv493dMagnetic3DSensor.begin();
  Tlv493dMagnetic3DSensor.setAccessMode(Tlv493dMagnetic3DSensor.ULTRALOWPOWERMODE);
  Tlv493dMagnetic3DSensor.disableTemp();
}

void generalI2CReset() {
  Serial.println("GENERAL RESET START");
  Wire.beginTransmission(0x00);
  Wire.endTransmission();
  initalizeSensors();
  Serial.println("GENERAL RESET DONE");
}

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

  puzzleComm.begin();
  puzzleComm.setCallbacks(solve, unsolve, checkIsSolved);

  initalizeSensors();
}


void loop() {
  updateSensors();

  if (Tlv493dMagnetic3DSensor.getMeasurementDelay() > 100) {
    generalI2CReset();
  }

  Serial.print(millis());
  Serial.print("\t");
  for (int i =0; i<NUM_SENSORS; i++) {
    Serial.print("\t");
    Serial.print(sensorsValues[i]);
    Serial.print("\t");
    Serial.print(Tlv493dMagnetic3DSensor.getMeasurementDelay());
  }

  Serial.print("\t");
  Serial.print(digitalRead(RESET_PIN));
  Serial.print("\t");
  Serial.print(digitalRead(OVERRIDE_PIN));
  Serial.print("\t");
  Serial.print(digitalRead(UNSOLVED_PIN));
  Serial.print("\t");
  Serial.print(digitalRead(SOLVED_PIN));

  Serial.print("\t");
  Serial.print(puzzleComm.getCurrentState());
  Serial.println();


  if (puzzleComm.getCurrentState() == UNSOLVED) {
    int brightness = map(abs(sensorsValues[0]), 0, 15, 0, 255); // Map sensor value to brightness

    if (sensorsValues[0] >= 0) {
      leds[0] = CRGB::Cyan;
    } else {
      leds[0] = CRGB::Magenta;
    }

    FastLED.setBrightness(brightness);
  } else if (puzzleComm.getCurrentState() == SOLVED) {
    leds[0] = CRGB::Green;
    FastLED.setBrightness(50);
  }



  FastLED.show();

  puzzleComm.update();


  delay(Tlv493dMagnetic3DSensor.getMeasurementDelay());
}