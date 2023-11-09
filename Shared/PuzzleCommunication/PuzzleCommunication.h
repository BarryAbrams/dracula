// PuzzleCommunication.h

#ifndef PuzzleCommunication_h
#define PuzzleCommunication_h

#include "Arduino.h"

class PuzzleCommunication {
public:
    PuzzleCommunication(int resetPin, int overridePin, int unsolvedPin, int solvedPin, int solvablePin = 255);
    void begin();
    void setPuzzleState(bool solved);
    bool readReset();
    bool readOverride();
    bool readSolvable();
    void updateOutputs();

private:
    int _resetPin;
    int _overridePin;
    int _solvablePin; // Optional solvable pin
    int _unsolvedPin;
    int _solvedPin;
    bool _solvablePinSet = false; // Indicates whether the solvable pin is being used
    bool debounceRead(int pin);
};

#endif
