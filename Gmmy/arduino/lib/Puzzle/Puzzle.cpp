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
               int solvedSoundEffectId,
               int altSolvedSoundEffectId) :
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
    solvedSoundEffectId(solvedSoundEffectId),
    altSolvedSoundEffectId(altSolvedSoundEffectId) {
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

bool Puzzle::getSolvable() {
    if(solvablePin != 255) {
        return digitalRead(solvablePin);
    } else {
        return true;
    }
}

void Puzzle::setSolvable(bool state) {
    digitalWrite(solvablePin, state);
}

void Puzzle::checkPinChanges() {
    MessageData computedData = 0;  // Initialize with some default value
    bool changed = false;

    uint8_t currentUnsolvedState = digitalRead(unsolvedPin);
    uint8_t currentSolvedState = digitalRead(solvedPin);

    // Determine the state based on the combination of unsolved and solved states
    if (currentUnsolvedState == 1 && currentSolvedState == 0) {
        // unsolved
        current_state = 3;
    } else if (currentUnsolvedState == 0 && currentSolvedState == 1) {
        // solved
        current_state = 4;
    } else if (currentUnsolvedState == 0 && currentSolvedState == 0) {
        // unknown
        current_state = 0;
    } else if (currentUnsolvedState == 1 && currentSolvedState == 1) {
        // extra
        current_state = 5;
    }
    
    if (currentUnsolvedState != prevPinStates[0] || currentSolvedState != prevPinStates[1]) {
        changed = true;
    }

    // if(altSolvedPin != 255) {
    //     uint8_t currentAltSolvedState = digitalRead(altSolvedPin);
    //     if (currentAltSolvedState != prevPinStates[2]) {
    //         prevPinStates[2] = currentAltSolvedState;
    //         changed = true;
    //         computedData |= (currentAltSolvedState << 2);  // Adjust the bit position if necessary
    //     }
    // }

    if (changed) {
        prevPinStates[0] = currentUnsolvedState;
        prevPinStates[1] = currentSolvedState;
        if (current_state == 4 /* Adjust based on your condition */) {
            if (onSoundEffectCallback && solvedSoundEffectId >= 0) {
                onSoundEffectCallback(solvedSoundEffectId);
            }
        }

        if (current_state == 6 /* Adjust based on your condition */) {
            if (onSoundEffectCallback && altSolvedSoundEffectId >= 0) {
                onSoundEffectCallback(altSolvedSoundEffectId);
            }
        }

        if (onPinChangeCallback) {
            onPinChangeCallback(signal, current_state);
        }
    }
}