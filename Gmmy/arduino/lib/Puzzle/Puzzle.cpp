// Puzzle.cpp

#include "Puzzle.h"

Puzzle::Puzzle(const char* name, 
               uint8_t resetPin, 
               uint8_t overridePin, 
               uint8_t unsolvedPin, 
               uint8_t solvedPin, 
               uint8_t altSolvedPin, 
               uint8_t solvablePin,
               MessageSignal signal,
               MessageData current_state,
               MessageSignal dependantPuzzle,
               std::vector<CallbackFunctionList> resetCallbacks,
               std::vector<CallbackFunctionList> solvedCallbacks,
               std::vector<CallbackFunctionList> altSolvedCallbacks) :
    name(name), 
    resetPin(resetPin), 
    overridePin(overridePin), 
    unsolvedPin(unsolvedPin), 
    solvedPin(solvedPin), 
    altSolvedPin(altSolvedPin), 
    solvablePin(solvablePin), 
    signal(signal), 
    current_state(current_state), 
    dependantPuzzle(dependantPuzzle),
    resetCallbacks(resetCallbacks),
    solvedCallbacks(solvedCallbacks),
    altSolvedCallbacks(altSolvedCallbacks) {
    // Initialize previous pin states to a known state, e.g., LOW
    for (int i = 0; i < 5; ++i) {
        prevPinStates[i] = LOW;
    }
}

void Puzzle::setup() {
    pinMode(resetPin, OUTPUT);
    pinMode(overridePin, OUTPUT);
    pinMode(unsolvedPin, INPUT_PULLUP);
    pinMode(solvedPin, INPUT_PULLUP);


    if(solvablePin != 255) {
        pinMode(solvablePin, OUTPUT);
    }

    prevPinStates[0] = digitalRead(unsolvedPin);
    prevPinStates[1] = digitalRead(solvedPin);
    if(altSolvedPin != 255) {
        pinMode(altSolvedPin, INPUT_PULLUP);
        prevPinStates[2] = digitalRead(altSolvedPin);
    }
}

MessageData Puzzle::getCurrentState() const {
    return current_state;
}

MessageSignal Puzzle::getSignal() const {
    return signal;
}

MessageSignal Puzzle::getDependantPuzzle() const {
    return dependantPuzzle;
}

void Puzzle::setState(MessageData newState) {
    digitalWrite(resetPin, (newState & 1) ? HIGH : LOW);
    digitalWrite(overridePin, (newState & 2) ? HIGH : LOW);
}

void Puzzle::endPulse() {
    digitalWrite(resetPin, LOW);
    digitalWrite(overridePin, LOW);
    pulseStartTime = 0;
}

void Puzzle::startPulse(MessageData newState) {
    digitalWrite(resetPin, (newState & 1) ? HIGH : LOW);
    digitalWrite(overridePin, (newState & 2) ? HIGH : LOW);
    pulseStartTime = millis();
}

bool Puzzle::getSolvable() {
    if(solvablePin != 255) {
        return digitalRead(solvablePin);
    } else {
        return true;
    }
}

bool Puzzle::getAltSolved() {
    return digitalRead(altSolvedPin);
}

void Puzzle::setSolvable(bool state) {
    digitalWrite(solvablePin, state);
}

void Puzzle::checkPinChanges() {
    if (pulseStartTime != 0 && millis() - pulseStartTime >= 250) {
        endPulse();
    }

    MessageData computedData = 0;  // Initialize with some default value
    bool changed = false;

    uint8_t currentUnsolvedState = digitalRead(unsolvedPin);
    uint8_t currentSolvedState = digitalRead(solvedPin);
    uint8_t altSolvedState = false;
    if(altSolvedPin != 255) {
        altSolvedState = digitalRead(altSolvedPin);
    }

    // Determine the state based on the combination of unsolved and solved states
    if (currentUnsolvedState == 1 && currentSolvedState == 0) {
        // unsolved
        current_state = 3;
    } else {
        if(altSolvedPin != 255) {
            if (currentUnsolvedState == 0 && currentSolvedState == 1 && altSolvedState == 1) {
                current_state = 4;
            } else if (currentUnsolvedState == 0 && currentSolvedState == 1 && altSolvedState == 0) {
                current_state = 6;
            } else  {
                current_state = 0;
            } 
        } else {
            if (currentUnsolvedState == 0 && currentSolvedState == 1) {
                current_state = 4;
            } else if (currentUnsolvedState == 0 && currentSolvedState == 0) {
                current_state = 0;
            } else if (currentUnsolvedState == 1 && currentSolvedState == 1) {
                current_state = 5;
            } 
        }
    }

   
    
    if (current_state != prev_state) {
        if (current_state == 3) {
            for (auto& callback : resetCallbacks) {
                callback();
            }
        }

        if (current_state == 4) {
            for (auto& callback : solvedCallbacks) {
                callback();
            }
        }

        if (current_state == 6) {
            for (auto& callback : altSolvedCallbacks) {
                callback();
            }
        }

        if (onPinChangeCallback) {
            onPinChangeCallback(signal, current_state);
        }
    }

    prev_state = current_state;
}