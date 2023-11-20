// PuzzleCommunication.cpp

#include "PuzzleCommunication.h"
// #ifdef __AVR__
//  #include <avr/wdt.h>
// #endif
PuzzleCommunication::PuzzleCommunication(int resetPin, int overridePin, int unsolvedPin, int solvedPin, int solvablePin, int altSolvedPin)
  : _resetPin(resetPin), _overridePin(overridePin),  _unsolvedPin(unsolvedPin), _solvedPin(solvedPin), _solvablePin(solvablePin), _altSolvedPin(altSolvedPin)
{
}

void PuzzleCommunication::begin() {
  pinMode(_resetPin, INPUT_PULLUP);
  pinMode(_overridePin, INPUT_PULLUP);
  pinMode(_unsolvedPin, OUTPUT);
  pinMode(_solvedPin, OUTPUT);
  digitalWrite(_solvedPin, HIGH);
  digitalWrite(_unsolvedPin, HIGH);

  if (_altSolvedPin != 255) {
    pinMode(_altSolvedPin, OUTPUT);
    digitalWrite(_altSolvedPin, LOW);
  }

  if (_solvablePin != 255) {
    pinMode(_solvablePin, INPUT_PULLUP);
    _solvablePinSet = true;

  }
  setPuzzleState(false);
}

void PuzzleCommunication::setCallbacks(void (*solveCallback)(), void (*unsolveCallback)(),  bool (*isSolvedCallback)()) {
    _solveCallback = solveCallback;
    _unsolveCallback = unsolveCallback;
    _isSolvedCallback = isSolvedCallback;
}

void PuzzleCommunication::setAltSolveCallback(void (*altSolveCallback)(), bool (*isAltSolvedCallback)()) {
  _altSolveCallback = altSolveCallback;
  _isAltSolvedCallback = isAltSolvedCallback;
  _altCallbackSet = true;
}

void PuzzleCommunication::update() {
  bool reset = debounceRead(_resetPin);
  bool override = debounceRead(_overridePin);
  bool solvable = getSolvable();

  // Check if the reset button is pressed
  if (reset && override) {
    digitalWrite(_unsolvedPin, LOW);
    digitalWrite(_solvedPin, LOW);
    delay(500);

    #ifdef __AVR__
      // wdt_enable(WDTO_15MS);
      // while (true);
    #else
      watchdog_reboot(0, 0, 0);
    #endif
  }

  bool resetAction = (prevReset != reset) && reset;
  bool overrideAction = (prevOverride != override) && override;

  if (resetAction && currentState != UNSOLVED) {
      Serial.println("RESET");
      wasOverriden = false;
      if (_altSolvedPin != 255) {
        if (digitalRead(_altSolvedPin) == HIGH) {
          digitalWrite(_altSolvedPin, LOW);
          currentState = FIRSTSOLVED;
        } else {
          currentState = UNSOLVED;
          setPuzzleState(false);
        }
      } else {
        currentState = UNSOLVED;
        setPuzzleState(false);
        _unsolveCallback();
      }
  }

  if (overrideAction && currentState != SOLVED && solvable) {
      Serial.println("OVERRIDE");
      wasOverriden = true;

      if (_altSolvedPin != 255) {
        if (currentState == UNSOLVED) {
          currentState = FIRSTSOLVED;
          _solveCallback();
          setPuzzleState(true);
        } else if (currentState == FIRSTSOLVED) {
          digitalWrite(_altSolvedPin, HIGH);
          currentState = SECONDSOLVED;
        }
      } else {
        currentState = SOLVED;
        _solveCallback();
        setPuzzleState(true);
      }

  }

  if (prevSolvable != solvable) {
      setPuzzleState(currentState);
  }


  bool solved = _isSolvedCallback();
  if (currentState == UNSOLVED and solved == true and solvable) {
      if (_altSolvedPin != 255) {
        currentState = FIRSTSOLVED;
      } else {
        currentState = SOLVED;
      }
      setPuzzleState(true);
      _solveCallback();
  }

  if (_altCallbackSet) {
  bool secondSolved = _isAltSolvedCallback();
  if (currentState == FIRSTSOLVED and secondSolved == true and solvable) {
      if (_altSolvedPin != 255) {
          currentState = SECONDSOLVED;
          digitalWrite(_altSolvedPin, HIGH);
      } 
  }
  }


  prevSolvable = solvable;
  prevOverride = override;
  prevReset = reset;
}

State PuzzleCommunication::getCurrentState() {
  return currentState;
}

bool PuzzleCommunication::debounceRead(int pin) {
  return digitalRead(pin);
}

void PuzzleCommunication::setPuzzleState(bool solved) {
  if (getSolvable()) {
    digitalWrite(_unsolvedPin, !solved);
    digitalWrite(_solvedPin, solved);
  } else {
    digitalWrite(_unsolvedPin, HIGH);
    digitalWrite(_solvedPin, HIGH);
  }
}

bool PuzzleCommunication::getSolvable() {
  if (_solvablePin != 255) {
    return debounceRead(_solvablePin);
  } else {
    return true;
  }
}