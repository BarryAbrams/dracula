// PuzzleCommunication.h

#ifndef PuzzleCommunication_h
#define PuzzleCommunication_h

#include "Arduino.h"

enum State {
  UNSOLVED,
  SOLVED,
  FIRSTSOLVED,
  SECONDSOLVED
};


class PuzzleCommunication {
public:
    PuzzleCommunication(int resetPin, int overridePin, int unsolvedPin, int solvedPin, int solvablePin = 255, int altSolvedPin = 255);
    void setCallbacks(void (*solveCallback)(), void (*unsolveCallback)(), bool (*isSolvedCallback)());
    void setAltSolveCallback(void (*altSolveCallback)(), bool (*_isAltSolvedCallback)());
    void begin();
    void update();
    State getCurrentState();
    bool getSolvable();
    bool haltForReset = false;
    void setPuzzleState(bool solved);
    State currentState = UNSOLVED;
    bool wasOverriden = false;

private:
    bool debounceRead(int pin);
    int _resetPin;
    int _overridePin;
    int _solvablePin; // Optional solvable pin
    int _unsolvedPin;
    int _solvedPin;
    int _altSolvedPin;
    bool _solvablePinSet = false; // Indicates whether the solvable pin is being used
    bool _altCallbackSet = false;
    bool prevReset = false;
    bool prevOverride = false;
    bool prevSolvable = true;

    void (*_solveCallback)();
    void (*_altSolveCallback)();
    void (*_unsolveCallback)();
    bool (*_isSolvedCallback)();
    bool (*_isAltSolvedCallback)();
};

#endif
