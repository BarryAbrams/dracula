// Puzzle.cpp

#include "Puzzle.h"

Puzzle::Puzzle(const char* name, 
               const uint8_t pins_in[3], 
               const uint8_t pins_out[3],
               MessageSignal signal,
               MessageData current_state) :
    name(name), signal(signal), current_state(current_state) {
    for (int i = 0; i < 3; i++) {
        this->pins_in[i] = pins_in[i];
        this->pins_out[i] = pins_out[i];
    }
}

void Puzzle::setup() {
    for(int i = 0; i < 3; i++) {
        if(pins_in[i] != 255) {
            pinMode(pins_in[i], INPUT);
        }
        if(pins_out[i] != 255) {
            pinMode(pins_out[i], OUTPUT);
        }
    }
}

MessageData Puzzle::getCurrentState() const {
    return current_state;
}

void Puzzle::setState(MessageData newState) {
    for(int i = 0; i < 3; i++) {
        if(pins_out[i] != 255) {
            digitalWrite(pins_out[i], (newState & (1 << i)) ? HIGH : LOW);
        }
    }
}

void Puzzle::checkPinChanges() {
    MessageData computedData = 0;  // Initialize with some default value
    bool changed = false;
    for(int i = 0; i < 3; i++) {
        uint8_t currentState = digitalRead(pins_in[i]);
        if (currentState != prevPinStates[i]) {
            // Serial.print("Pin ");       // Print which pin changed
            // Serial.print(pins_in[i], DEC);
            // Serial.print(" changed to: ");
            // Serial.println(currentState, DEC);  // Print new state
            
            prevPinStates[i] = currentState;
            changed = true;

            computedData |= (currentState << i); // This is just an example. Adjust based on your needs.
        }
    }
    
    if (changed) {
        current_state = computedData;
        // Serial.print("Computed data: ");
        for(int i = 0; i < 3; i++) {
            if(pins_out[i] != 255) {
                digitalWrite(pins_out[i], LOW);
            }
        } 
        // if (computedData == 0) {
        //     Serial.println("NoData");
        // } else if (computedData == 1) {
        //     Serial.println("Override");
        // } else if (computedData == 2) {
        //     Serial.println("Reset");
        // } else if (computedData == 3) {
        //     Serial.println("Unsolved");
        // } else if (computedData == 4) {
        //     Serial.println("Solved");
        // } else if (computedData == 5) {
        //     Serial.println("Extra");
        // } else {
        //     Serial.println("Unknown value");
        // }

        if (onPinChangeCallback) {
            onPinChangeCallback(signal, computedData);
        }
    }
}