// Puzzle.h

#ifndef PUZZLE_H
#define PUZZLE_H

#include <Arduino.h>

typedef uint8_t MessageData;
typedef uint8_t MessageSignal;
typedef void (*CallbackFunction)(MessageSignal, MessageData);

class Puzzle {
private:
    const char* name;
    uint8_t pins_in[3];
    uint8_t pins_out[3];
    MessageSignal signal;
    MessageData current_state;
    uint8_t prevPinStates[3];
    CallbackFunction onPinChangeCallback;

public:
    Puzzle(const char* name, 
           const uint8_t pins_in[3], 
           const uint8_t pins_out[3], 
           MessageSignal signal,
           MessageData current_state);

    void setup();  // Add this line

    MessageData getCurrentState() const;
    void setState(MessageData newState);
    void checkPinChanges();
    void setCallback(CallbackFunction callback) {
        onPinChangeCallback = callback;
    }
};


#endif // PUZZLE_H
