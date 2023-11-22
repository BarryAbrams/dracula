#include <Arduino.h>
#include <FastLED.h>

#include "Puzzle.h"

// #define SERIAL_DEBUGGING

enum RELAY {
    NO_RELAY = -5,
    TREE_DOOR = 24,
    CEMETARY_DOOR = 22,
    PARLOR_DOOR = 26,
    ROTATING_DOOR = 28,
    HALLWAY_UV = 30,
    BEDROOM_UV = 32,
    SKELETONS_ENABLE = 52
};

enum TRIGGER {
    NO_TRIGGER = -1,
    SLIDE_DOOR_SENSOR = 50,
    DRACULA_COFFIN_SENSOR = 59,
    DOOR_BELL_SENSOR = 19
};

bool prevSlideDoorSensor = false;
bool prevDoorBellSensor = false;
bool prevDraculaCoffinSensor = false;


// Definitions
#define N_PUZZLES 10
#define LED_PIN 23
#define N_BUTTONS (N_PUZZLES*2+1)
#define STATUS_INDEX (N_BUTTONS-1)
#define DEBOUNCE_TIME 500

typedef uint8_t MessageSignal;
const MessageSignal Puzzle1 = 0, Puzzle2 = 1, Puzzle3 = 2, Puzzle4 = 3, Puzzle5 = 4, Puzzle6 = 5, Puzzle7 = 6, Puzzle8 = 7, Puzzle9 = 8, Puzzle10 = 9, DoorBell = 15, SlideDoor = 16, CoffinDoor = 17, GameReset = 18, Sound = 19, Time = 20, Shutdown = 21, Startup = 22, Debug = 0x71, Query = 0x72, EndOfMessages = 0x7F;

typedef uint8_t MessageData;
const MessageData None = -1, NoData = 0, Override = 1, Reset = 2, Unsolved = 3, Solved = 4, Blocked = 5, FirstSolved = 6, Reboot = 7;

void resetCallback() {
    #ifdef SERIAL_DEBUGGING
    Serial.println("RESET");
    #endif 
}

void solvedCallback() {
    #ifdef SERIAL_DEBUGGING
    Serial.println("SOLVED");
    #endif 
}

void altSolvedCallback() {
    #ifdef SERIAL_DEBUGGING
    Serial.println("ALT SOLVED");
    #endif 
}

void updateRelay(RELAY relay, bool state) {
    #ifdef SERIAL_DEBUGGING
    Serial.println("ALT SOLVED");
    #endif 
    digitalWrite(relay, !state);
}

Puzzle puzzles[] = {

    Puzzle("gravestones", 
        48,46,44,42,255,255, 
        Puzzle1, NoData, None, 
        {resetCallback, [](){ updateRelay(TREE_DOOR, LOW); }}, 
        {solvedCallback, [](){ updateRelay(TREE_DOOR, HIGH); }}, 
        {altSolvedCallback}
    ),
    Puzzle("gemstones", 
        34,36,38,40,255,255, 
        Puzzle3, NoData, None, 
        {resetCallback}, 
        {solvedCallback}, 
        {altSolvedCallback}
    ),
    Puzzle("hallway",
        A1,A0,A2,A3,A4,255, 
        Puzzle5, NoData, None, 
        {resetCallback, [](){ updateRelay(PARLOR_DOOR, LOW); }}, 
        {solvedCallback}, 
        {altSolvedCallback,[](){ updateRelay(PARLOR_DOOR, HIGH); } }),

    Puzzle("magic_words",
        A6,A7,A14,A15,255,255, 
        Puzzle6, NoData, None, 
        {resetCallback,  [](){ updateRelay(SKELETONS_ENABLE, HIGH); }, [](){ updateRelay(ROTATING_DOOR, LOW); }}, 
        {solvedCallback,  [](){ updateRelay(SKELETONS_ENABLE, LOW); }, [](){ updateRelay(ROTATING_DOOR, HIGH); }}, 
        {altSolvedCallback}
    ),

    Puzzle("cryptex", 
        5,4,6,7,255,255, 
        Puzzle7, NoData, None, 
        {resetCallback, [](){ updateRelay(BEDROOM_UV, HIGH); }}, 
        {solvedCallback, [](){ updateRelay(BEDROOM_UV, LOW); }}, 
        {altSolvedCallback}
    ),
    Puzzle("illuminate",
        17,14,15,16,255,255, 
        Puzzle9, NoData, None, 
        {resetCallback}, 
        {solvedCallback}, 
        {altSolvedCallback}
    ),
    Puzzle("dracula", 
        8,9,10,11,255,12, 
        Puzzle10, NoData, Puzzle9, 
        {resetCallback}, 
        {solvedCallback}, 
        {altSolvedCallback}),
};

struct Message {
    MessageSignal sig : 8;
    MessageData data : 8;
};

// Global variables
CRGB leds[N_PUZZLES] = {0};
const uint8_t buttons[N_BUTTONS] = {
    67, 66, 
    65, 64, 
    63, 27, 
    62, 29, 
    31, 33, 
    53, 51, 
    49, 47, 
    45, 43, 
    41, 39, 
    37, 35, 
    25
};
bool button_state[N_BUTTONS] = {0};
unsigned long cooldown_time[N_BUTTONS] = {0};

bool currentButtonStates[N_BUTTONS] = {false};

void send_message(Message pack, HardwareSerial *port) {
    if(pack.sig != EndOfMessages) {
        byte first = pack.sig | 0x80;
        byte second = pack.data & 0x7F;
        port->write(first);
        port->write(second);
    }
}

inline Message pack_message(MessageSignal a, MessageData b) {
    Message m;
    m.sig = static_cast<MessageSignal>(a & 0x7F);
    m.data = b & 0x7F;
    return m;
}

void send_message_all(Message pack) {
    send_message(pack, &Serial);
}

inline void send_message_all(MessageSignal sig, MessageData data) {
    send_message_all(pack_message(sig, data));
}

void handle_message(MessageSignal sig, MessageData data) {
    // Serial.print(sig);
    if (sig == Sound) {
        // Serial.print("PLAY SOUND");
        // Serial.println(data);
        // playSound(data);
    }

    if (sig == GameReset) {
        updateRelay(CEMETARY_DOOR, LOW);
    }

    if (sig <= 9) {
        int index = -1;
        for (size_t i = 0; i < sizeof(puzzles) / sizeof(puzzles[0]); i++) {

            if (puzzles[i].getSignal() == sig) {
                index = i;
            }
        }

        if (index > -1) {
            if (data == Override) {
                puzzles[index].startPulse(Override);
            }
            if (data == Reset) {
                puzzles[index].startPulse(Reset);
            }
            if (data == Reboot) {
                puzzles[index].startResetPulse();
            }
        }
        
    }
}

// Functions
inline uint8_t reroute(uint8_t raw) {
    return (raw >= 5) ? raw - 5 : 9 - raw;
}

inline CRGB choose_color(MessageData data) {
    if(data == Unsolved) return CRGB(0, 128, 0);
    else if(data == Blocked) return CRGB(128, 128, 0);
    else if(data == Solved) return CRGB(128, 0, 0);
    else if(data == FirstSolved) return CRGB(128, 0, 128);
    else return CRGB(0, 0, 0);
}

bool button_pressed(uint8_t which) {
    if (cooldown_time[which] && millis() - cooldown_time[which] <= DEBOUNCE_TIME) return false;
    bool prev = button_state[which];
    bool curr = digitalRead(buttons[which]) == LOW;
    button_state[which] = curr;
    if (curr && !prev) {
        cooldown_time[which] = millis();
        #ifdef SERIAL_DEBUGGING
        Serial.print("Pin "); 
        Serial.println(buttons[which], DEC);
        #endif 
        return true;
    }
    return false;
}



Message check_messages(HardwareSerial *port) {
    if(port->available()) {
        MessageSignal first = (MessageSignal) port->read();
        if(!(first & 0x80)) return {EndOfMessages, NoData};
        MessageData second = (MessageData) port->read();
        if(second & 0x80) return {EndOfMessages, NoData};
        return pack_message(first, second);
    }
    return {EndOfMessages, NoData};
}



void update_light(MessageSignal sig, MessageData data) {
    if(sig <= Puzzle10) {
        leds[reroute(sig)] = choose_color(data);
        FastLED.show();
        send_message_all(sig, data);
    }
}


void status_check() {
    // First, turn on each light sequentially, then turn off the previous one
    for (uint8_t i = 0; i < N_PUZZLES; ++i) {
        update_light(i, Solved); // Update light to Solved
        delay(100);
        update_light(i, NoData); // Turn off the previous light
    }

    // If there are any puzzles, turn off the light for the last puzzle
    if (sizeof(puzzles) / sizeof(Puzzle) > 0) {
        update_light(puzzles[sizeof(puzzles) / sizeof(Puzzle) - 1].getSignal(), NoData);
    }

    // Now, update each light according to the current state of each puzzle
    for (uint8_t i = 0; i < sizeof(puzzles) / sizeof(Puzzle); ++i) {
        MessageData status = puzzles[i].getCurrentState();
        update_light(puzzles[i].getSignal(), status); // Update light to the current status
    }

    // Send a query message at the end of the status check
    send_message_all(Query, NoData);
}


int8_t findPuzzleIndexBySignal(uint8_t puzzleSignal) {
    for (uint8_t i = 0; i < sizeof(puzzles) / sizeof(puzzles[0]); ++i) {
        if (puzzles[i].getSignal() == puzzleSignal) {
            return i; // Found the puzzle with the matching signal
        }
    }
    return -1; // Signal not found, return an invalid index
}

void handle_button(uint8_t which) {
    #ifdef SERIAL_DEBUGGING
    Serial.print("Button index "); 
    Serial.println(which, DEC);
    #endif

    if(which == STATUS_INDEX) {
        status_check();
        return;
    }

    uint8_t puzzleSignal = which / 2;
    int8_t puzzleIndex = findPuzzleIndexBySignal(puzzleSignal);

    // if (puzzleIndex < sizeof(puzzles) / sizeof(puzzles[0])) {
    MessageData newState = (which % 2 == 0) ? Override : Reset;

     if (puzzleIndex != -1) { // A valid puzzle index was found
        MessageData newState = (which % 2 == 0) ? Override : Reset;
        if (newState == Override && puzzles[puzzleIndex].getSolvable()) {
            puzzles[puzzleIndex].startPulse(newState);
        } else if (newState != Override) {
            puzzles[puzzleIndex].startPulse(newState);
        }
        // send_message_all((MessageSignal)puzzleSignal, newState);
    } else {
        // Handle error case where there is no puzzle with the given signal
        #ifdef SERIAL_DEBUGGING
        Serial.print("No puzzle for button index ");
        Serial.println(which, DEC);
        #endif
    }
}



void process_messages() {
    for(Message msg = check_messages(&Serial); msg.sig != EndOfMessages; msg = check_messages(&Serial)) {
        handle_message(msg.sig, msg.data);
    }
}


void setup() {
    Serial.begin(9600);
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, N_PUZZLES);
    FastLED.setBrightness(128);  // Correct way to set brightness
    FastLED.show();
    
    for(uint8_t i = 0; i < N_BUTTONS; i++) pinMode(buttons[i], INPUT_PULLUP);

    for(size_t i = 0; i < sizeof(puzzles) / sizeof(Puzzle); i++) {
        puzzles[i].setup();
        puzzles[i].setCallback(update_light);
    }

    pinMode(TREE_DOOR, OUTPUT);
    pinMode(CEMETARY_DOOR, OUTPUT);
    pinMode(PARLOR_DOOR, OUTPUT);
    pinMode(ROTATING_DOOR, OUTPUT);
    pinMode(HALLWAY_UV, OUTPUT);
    pinMode(BEDROOM_UV, OUTPUT);

    pinMode(SKELETONS_ENABLE, OUTPUT);
    pinMode(SLIDE_DOOR_SENSOR, INPUT_PULLUP);
    pinMode(DRACULA_COFFIN_SENSOR, INPUT_PULLUP);
    pinMode(DOOR_BELL_SENSOR, INPUT_PULLUP);

    updateRelay(CEMETARY_DOOR, LOW);
}

void handleSimultaneousPress(uint8_t puzzleIndex) {
    // Code to handle the simultaneous press of reset and override buttons for the puzzle
    // #ifdef SERIAL_DEBUGGING
    int actualPuzzleIndex = findPuzzleIndexBySignal(puzzleIndex / 2);
    // if (puzzles[actualPuzzleIndex].pulseStartTime == 0) {
    puzzles[actualPuzzleIndex].startResetPulse();
    // }
    // #endif
    // Add your logic here
}

void loop() {
    for(uint8_t i = 0; i < N_BUTTONS; i++) {
        currentButtonStates[i] = digitalRead(buttons[i]) == LOW;
    }

    // Check for simultaneous button presses
    for(uint8_t i = 0; i < N_BUTTONS - 1; i += 2) {
        if(currentButtonStates[i] && currentButtonStates[i + 1]) {
            // Reset and Override buttons for a puzzle are pressed simultaneously
            handleSimultaneousPress(i); // Handle the simultaneous press
        }
        else if (currentButtonStates[i] || currentButtonStates[i + 1]) {
            // Only one of the buttons is pressed
            if (currentButtonStates[i]) handle_button(i); // Reset button
            else handle_button(i + 1); // Override button
        }
    }

    for(uint8_t i = 0; i < N_BUTTONS; i++) {
        if(button_pressed(i)) handle_button(i);
    }
    // Serial.println(puzzles[2].getCurrentState());
    for (size_t i = 0; i < sizeof(puzzles) / sizeof(puzzles[0]); i++) {
        puzzles[i].checkPinChanges();
        
        if (puzzles[i].getCurrentState() == Solved) {
            for (size_t j = 0; j < sizeof(puzzles) / sizeof(puzzles[0]); j++) {
                if (puzzles[j].getDependantPuzzle() == puzzles[i].getSignal()) {
                    if (!puzzles[j].getSolvable()) {
                        puzzles[j].setSolvable(true);
                        puzzles[j].checkPinChanges();
                        delay(50);
                        MessageData status = puzzles[j].getCurrentState();
                        update_light(puzzles[j].getSignal(), status);
                        // Serial.println(status);
                    }
                }
            }
        } else if (puzzles[i].getCurrentState() == Unsolved) {
             for (size_t j = 0; j < sizeof(puzzles) / sizeof(puzzles[0]); j++) {
                if (puzzles[j].getDependantPuzzle() == puzzles[i].getSignal()) {
                    if (puzzles[j].getSolvable()) {
                        puzzles[j].setSolvable(false);
                        puzzles[j].checkPinChanges();
                        delay(50);
                        MessageData status = puzzles[j].getCurrentState();
                        update_light(puzzles[j].getSignal(), status);
                    }
                }
            }
        }
        
    }

    bool currentCoffinDoorSensor = digitalRead(DRACULA_COFFIN_SENSOR);

    if (prevDraculaCoffinSensor != currentCoffinDoorSensor) {
        if (currentCoffinDoorSensor == 0) {
            // Serial.println("TRIGGER");
            // updateRelay(CEMETARY_DOOR, HIGH);
            send_message_all(CoffinDoor, 4);
        } else {
            send_message_all(CoffinDoor, 3);
        }
        prevDraculaCoffinSensor = currentCoffinDoorSensor;
    }


    bool currentDoorbellSensor = digitalRead(DOOR_BELL_SENSOR);
    updateRelay(HALLWAY_UV, !currentDoorbellSensor);
    if (prevDoorBellSensor != currentDoorbellSensor) {
        if (currentDoorbellSensor == 0) {
            send_message_all(DoorBell, 4);
        } else {
            send_message_all(DoorBell, 3);

        }
        prevDoorBellSensor = currentDoorbellSensor;
    }

    bool currentSlideDoorSensor = digitalRead(SLIDE_DOOR_SENSOR);
    // Serial.println(digitalRead(SLIDE_DOOR_SENSOR));
    if (prevSlideDoorSensor != currentSlideDoorSensor) {
        if (currentSlideDoorSensor == 0) {
            // Serial.println("TRIGGER");
            updateRelay(CEMETARY_DOOR, HIGH);
            send_message_all(SlideDoor, 4);
        } else {
            send_message_all(SlideDoor, 3);
        }
        prevSlideDoorSensor = currentSlideDoorSensor;
    }

    process_messages();
    delay(50);
}