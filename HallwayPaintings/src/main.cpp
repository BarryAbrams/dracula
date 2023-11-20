#include <Wire.h>
#include <Tlv493d.h>
#include <avr/wdt.h>

#include "PuzzleCommunication.h"

#define SOLVED_TIMEOUT 1000

#define TCA9548_ADDRESS 0x70
#define NUM_SENSORS 6

Tlv493d Tlv493dMagnetic3DSensors[NUM_SENSORS];
const uint8_t sensorOrder[NUM_SENSORS] = {0,1,2,3,4,5};  // Adjust based on your setup
int sensorsValues[NUM_SENSORS] = {0};
float rawSensorsValues[NUM_SENSORS] = {0};
int currentSensor = 0;
float lastSensorValues[NUM_SENSORS] = {0};  // To store the last value of each sensor
int stableCount[NUM_SENSORS] = {0};       // To count the stability of each value


#define RESET_PIN A3
#define OVERRIDE_PIN A4
#define UNSOLVED_PIN A2
#define SOLVED_PIN A1
#define ALT_SOLVED_PIN A0

PuzzleCommunication puzzleComm(RESET_PIN, OVERRIDE_PIN, UNSOLVED_PIN, SOLVED_PIN, 255, ALT_SOLVED_PIN);

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
        if (sensorsValues[i] != 2) {
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
}

void solve() {
}

void unsolve() {
}

int calculateValue(float incomingValue) {
  int zStatus = 0;
   if (incomingValue < - 0.3) {
    zStatus = 1;
  } else if (incomingValue > 0.3) {
    zStatus = 2;
  }

  return zStatus;
}

void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548_ADDRESS);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

void blinkLed() {
  digitalWrite(10, HIGH);
  delay(250);
  digitalWrite(10, LOW);
  delay(250);
}


int ledBrightness = 0;
bool increasing = true;

void pulseLed() {
  if (increasing) {
    ledBrightness += 10;
    if (ledBrightness >= 255) {
      ledBrightness = 255;
      increasing = false;
    }
  } else {
    ledBrightness -= 10;
    if (ledBrightness <= 0) {
      ledBrightness = 0;
      increasing = true;
    }
  }
  analogWrite(10, ledBrightness);
}

float roundFloat(float value, int places) {
    float scaling = pow(10, places);
    return round(value * scaling) / scaling;
}


void generalI2CReset() {
  Serial.println("GENERAL RESET START");
  digitalWrite(10, HIGH);
  delay(250);
  digitalWrite(10, LOW);
  delay(250);
  for (int i = 0; i < 8; i++) {
    tcaSelect(i);
    Wire.beginTransmission(0x00);
    Wire.endTransmission();
  }

  for (int i = 0; i < NUM_SENSORS; i++) {
    tcaSelect(i);
    Tlv493dMagnetic3DSensors[i].begin();
    Tlv493dMagnetic3DSensors[i].setAccessMode(Tlv493dMagnetic3DSensors[i].MASTERCONTROLLEDMODE);
    Tlv493dMagnetic3DSensors[i].disableTemp();
  }
    Serial.println("GENERAL RESET DONE");

}

void updateSensors() {
  bool isStable = false;
  float epsilon = 0.001;  // Small value for float comparison

  for (int i = 0; i < NUM_SENSORS; i++) {
    tcaSelect(sensorOrder[i]);
    Tlv493dMagnetic3DSensors[i].updateData();
    float zValue = Tlv493dMagnetic3DSensors[i].getZ();
    int sensorValue = calculateValue(zValue);

    // Debugging: Print current and last values
    // Serial.print("Sensor ");
    // Serial.print(i);
    // Serial.print(": Current=");
    // Serial.print(zValue);
    // Serial.print(", Last=");
    // Serial.print(lastSensorValues[i]);
    // Serial.print(", Trigger=");
    // Serial.print(zValue == lastSensorValues[i]);
    // Serial.print(", Count=");
    // Serial.println(stableCount[i]);

    // Check if the value is within a small range of the last value
    if (zValue == lastSensorValues[i]) {
      stableCount[i]++;
      if (stableCount[i] > 50) {
        isStable = true;
        generalI2CReset();
        stableCount[i] = 0;
      }
    } else {
      stableCount[i] = 0;
    }

    sensorsValues[i] = sensorValue;
    rawSensorsValues[i] = zValue;
    lastSensorValues[i] = rawSensorsValues[i];  // Update the last value
  }

  if (isStable) {
    blinkLed();
  } else {
    pulseLed();
  }
}



void setup() {
  Wire.begin();
  Serial.begin(9600);
  puzzleComm.begin();
  puzzleComm.setCallbacks(solve, unsolve, checkIsSolved);
  puzzleComm.setAltSolveCallback(altSolve, checkIsAltSolved);

  for (int i = 0; i < NUM_SENSORS; i++) {
    tcaSelect(i);
    Tlv493dMagnetic3DSensors[i].begin();
    Tlv493dMagnetic3DSensors[i].setAccessMode(Tlv493dMagnetic3DSensors[i].ULTRALOWPOWERMODE);
    // Tlv493dMagnetic3DSensors[i].setAccessMode(Tlv493dMagnetic3DSensors[i].MASTERCONTROLLEDMODE);
    Tlv493dMagnetic3DSensors[i].disableTemp();
  }

  pinMode(10, OUTPUT);
  pinMode(12, OUTPUT);
  digitalWrite(10, HIGH);
  digitalWrite(12, HIGH);
  delay(250);
  digitalWrite(10, LOW);
  digitalWrite(12, LOW);
  delay(250);
  digitalWrite(10, HIGH);
  digitalWrite(12, HIGH);
  delay(250);
  digitalWrite(10, LOW);
  digitalWrite(12, LOW);
  delay(250);
  Serial.println("REBOOT");
  wdt_enable(WDTO_2S);
}


void loop() {
  if (!puzzleComm.haltForReset) {
    wdt_reset(); // Reset the watchdog timer regularly
  }

  // static unsigned long lastResetTime = 0;
  // static unsigned long lastBlinkTime = 0;

  // // Fading LED on pin 10
  // if (increasing) {
  //   ledBrightness += 10;
  //   if (ledBrightness >= 255) {
  //     ledBrightness = 255;
  //     increasing = false;
  //   }
  // } else {
  //   ledBrightness -= 10;
  //   if (ledBrightness <= 0) {
  //     ledBrightness = 0;
  //     increasing = true;
  //   }
  // }
  // analogWrite(10, ledBrightness);

  // // General I2C Reset every 10 seconds
  // if (millis() - lastResetTime >= 1000 * 60) {
  //   generalI2CReset();
  //   lastResetTime = millis();
  // }

  // // LED on pin 12 behavior
  // if (checkIsSolved()) {
  //   digitalWrite(12, HIGH);
  // } else if (checkIsAltSolved()) {
  //   if (millis() - lastBlinkTime >= 1000) {
  //     digitalWrite(12, !digitalRead(12));  // Toggle LED state
  //     lastBlinkTime = millis();
  //   }
  // } else {
  //   digitalWrite(12, LOW);
  // }


  updateSensors();

  Serial.print(millis());
  Serial.print("\t");
  for (int i =0; i<NUM_SENSORS; i++) {
    Serial.print("\t");
    Serial.print(rawSensorsValues[i]);
    // Serial.print("(");
    // Serial.print(stableCount[i]);
    // Serial.print(")");
  }

  Serial.println();

  puzzleComm.update();

  if (puzzleComm.getCurrentState() == UNSOLVED) {

  } else if (puzzleComm.getCurrentState() == FIRSTSOLVED) {
   
  } else if (puzzleComm.getCurrentState() == SECONDSOLVED) {

  }
  
  delay(100);
}

