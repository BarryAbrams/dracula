#include <Arduino.h>
// Define the LED pins and the switch pin
const int ledPins[] = {5, 7, 9, 10, 11, 12};
const int numLeds = 6;
const int switchPin = A0;

unsigned long previousMillis = 0;   // will store last time LED was updated
const long interval = 1500;         // interval at which to blink (milliseconds)
const long pauseInterval = 1000;    // interval for pause between patterns

int currentLed = 0;                 // to track the current LED index
int toggleCount = 0;                // to count the number of toggles
bool isPausing = true;              // start with a pause
unsigned long pauseStarted = 0;     // time when pause started

void setup() {
  Serial.begin(9600);
  // Initialize the LED pins as outputs
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  // Initialize the switch pin as an input
  pinMode(switchPin, INPUT_PULLUP);
}

void loop() {
  unsigned long currentMillis = millis();

  if (digitalRead(switchPin) == HIGH) {
    if (isPausing) {
      if (currentMillis - pauseStarted >= pauseInterval) {
        // End the pause
        isPausing = false;
        pauseStarted = currentMillis;
        Serial.println("End Pause");
      }
    } else {
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        if (currentLed < numLeds) {
          // Turn off all LEDs and then turn on the current LED
          for (int i = 0; i < numLeds; i++) {
            digitalWrite(ledPins[i], LOW);
          }
          digitalWrite(ledPins[currentLed], HIGH);
          Serial.print("Light Pin");
          Serial.println(currentLed);
          currentLed++;
        } else if (toggleCount < 4) {
          // Toggle all LEDs
          bool state = toggleCount % 2 == 0;
          for (int i = 0; i < numLeds; i++) {
            digitalWrite(ledPins[i], state ? HIGH : LOW);
          }
          Serial.print("Toogle All Leds: ");
          Serial.println(state);
          toggleCount++;
        } else {
          // Reset for next cycle
          currentLed = 0;
          toggleCount = 0;
          isPausing = true;
          pauseStarted = currentMillis;
        }
      }
    }
  } else {
    // If the switch is LOW, reset the pattern and turn all LEDs off
    isPausing = true;
    currentLed = 0;
    toggleCount = 0;
    for (int i = 0; i < numLeds; i++) {
      digitalWrite(ledPins[i], LOW);
    }
  }
}