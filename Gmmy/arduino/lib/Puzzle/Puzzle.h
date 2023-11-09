#ifndef PUZZLE_H
#define PUZZLE_H

#include <Arduino.h>
#include <ArduinoSTL.h>

typedef uint8_t MessageData;
typedef uint8_t MessageSignal;
typedef void (*CallbackFunctionList)();
typedef void (*CallbackFunction)(MessageSignal, MessageData);
typedef void (*SoundEffectCallbackFunction)(int);

class Puzzle {
private:
    const char* name;
    uint8_t resetPin;
    uint8_t overridePin;
    uint8_t unsolvedPin;
    uint8_t solvedPin;
    uint8_t altSolvedPin;
    uint8_t solvablePin; // Optional pin
    MessageSignal signal;
    MessageData current_state;
    MessageData prev_state;
    MessageSignal dependantPuzzle;
    std::vector<CallbackFunctionList> resetCallbacks;
    std::vector<CallbackFunctionList> solvedCallbacks;
    std::vector<CallbackFunctionList> altSolvedCallbacks;

    uint8_t prevPinStates[5]; // Adjusted for new pins, including optional solvablePin
    CallbackFunction onPinChangeCallback;
    int solvedSoundEffectId;
    int altSolvedSoundEffectId;
    SoundEffectCallbackFunction onSoundEffectCallback;
    unsigned long pulseStartTime = 0;
    void endPulse();

public:
    Puzzle(const char* name,
           uint8_t resetPin,
           uint8_t overridePin,
           uint8_t unsolvedPin,
           uint8_t solvedPin,
           uint8_t altSolvedPin,
           uint8_t solvablePin, // Default value indicating no pin
           MessageSignal signal,
           MessageData current_state,
           MessageSignal dependantPuzzle,
           std::vector<CallbackFunctionList> resetCallbacks,
           std::vector<CallbackFunctionList> solvedCallbacks,
           std::vector<CallbackFunctionList> altSolvedCallbacks);

    void setup();  // Add this line

    MessageData getCurrentState() const;
    void setState(MessageData newState);
    void startPulse(MessageData newState);
    bool getAltSolved();
    bool getSolvable();
    void setSolvable(bool state);
    void checkPinChanges();
    void setCallback(CallbackFunction callback) {
        onPinChangeCallback = callback;
    }
    MessageSignal getSignal() const;
    MessageSignal getDependantPuzzle() const;
    int getSolvedSoundEffectId() const;
    void setSolvedSoundEffectId(int id);
    int getAltSolvedSoundEffectId() const;
    void setAltSolvedSoundEffectId(int id);
    void setPlaySoundCallback(SoundEffectCallbackFunction callback) {
        onSoundEffectCallback = callback;
    }


    // Additional getters/setters for individual pins can be added here if needed.
};

#endif // PUZZLE_H
