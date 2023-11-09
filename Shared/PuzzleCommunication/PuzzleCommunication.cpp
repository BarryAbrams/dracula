// PuzzleCommunication.cpp

#include "PuzzleCommunication.h"

PuzzleCommunication::PuzzleCommunication(int resetPin, int overridePin, int unsolvedPin, int solvedPin, int solvablePin)
  : _resetPin(resetPin), _overridePin(overridePin),  _unsolvedPin(unsolvedPin), _solvedPin(solvedPin), _solvablePin(solvablePin)
{
}

void PuzzleCommunication::begin() {
  pinMode(_resetPin, INPUT_PULLUP);
  pinMode(_overridePin, INPUT_PULLUP);
  pinMode(_unsolvedPin, OUTPUT);
  pinMode(_solvedPin, OUTPUT);

  if (_solvablePin != 255) {
    pinMode(_solvablePin, INPUT_PULLUP);
    _solvablePinSet = true;

  }
  setPuzzleState(false);
}

bool PuzzleCommunication::debounceRead(int pin) {
  return digitalRead(pin);
  // Implement the debounce logic here, similar to readDebouncedInput() from your sketch
}

void PuzzleCommunication::setPuzzleState(bool solved) {
  if (readSolvable()) {
    digitalWrite(_unsolvedPin, !solved);
    digitalWrite(_solvedPin, solved);
  } else {
    digitalWrite(_unsolvedPin, HIGH);
    digitalWrite(_solvedPin, HIGH);
  }

  // Serial.print(digitalRead(_unsolvedPin));
  // Serial.print("-");
  // Serial.print(digitalRead(_solvedPin));
  // Serial.println();
}

bool PuzzleCommunication::readReset() {
  return debounceRead(_resetPin);
}

bool PuzzleCommunication::readOverride() {
  return debounceRead(_overridePin);
}

bool PuzzleCommunication::readSolvable() {
  if (_solvablePinSet) {
    // Serial.print("Solvable Pin: ");
    // Serial.print(debounceRead(_solvablePin));
    // Serial.println();
    // return true;
    return debounceRead(_solvablePin);
  } else {
    return true;
  }
  return false; // Return a default value if the solvable pin is not set
}

void PuzzleCommunication::updateOutputs() {
  // Your logic to update the outputs based on the current state
}
