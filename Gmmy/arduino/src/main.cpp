#include <Arduino.h>
#include <FastLED.h>

#include "Puzzle.h"

//  #define SERIAL_DEBUGGING

enum SFX {
  NONE = -1, // to represent no sound or error
  WAHHAHA = 1, // 1-1
  GUNSHOT, // 1-2
  SCREAM, // 1-3
  CAR_HORN, // 1-4
  MACHINERY,  // 1-5
  SFX_6,  // 1-6
  SFX_7,  // 1-7
  SFX_8,  // 1-8
  SFX_9,  // 1-9
  SFX_10, // 1-10
  SFX_11, // 1-11
  SFX_12, // 1-12
  HEAVY_SWITCH, // 1-13
  SFX_14, // 1-14
  SFX_15, // 1-15
  PRESSURE_PLATE = 16, // 2-1
  SFX_17, // 2-2
  SFX_18, // 2-3
  SFX_19, // 2-4
  SFX_20, // 2-5
  SFX_21, // 2-6
  SFX_22, // 2-7
  SFX_23, // 2-8
  SFX_24, // 2-9
  SFX_25, // 2-10
  SFX_26, // 2-11
  SFX_27, // 2-12
  SFX_28, // 2-13
  SFX_29, // 2-14
  SFX_30  // 2-15
  // more can be added...
};

enum RELAY {
    NO_RELAY = -1,
    TREE_DOOR = 52,
    CEMETARY_DOOR = 50,
    HALLWAY_DOOR = 48,
    ROTATING_DOOR = 46,
    HALLWAY_UV = 44,
    DRACULA_UV = 42
};

void playSound(SFX soundNumber);

// Definitions
#define N_PUZZLES 10
#define LED_PIN 23
#define N_BUTTONS (N_PUZZLES*2+1)
#define STATUS_INDEX (N_BUTTONS-1)
#define DEBOUNCE_TIME 500

typedef uint8_t MessageSignal;
const MessageSignal Puzzle1 = 0, Puzzle2 = 1, Puzzle3 = 2, Puzzle4 = 3, Puzzle5 = 4, Puzzle6 = 5, Puzzle7 = 6, Puzzle8 = 7, Puzzle9 = 8, Puzzle10 = 9, Sound = 19, Time = 20, Shutdown = 21, Startup = 22, Debug = 0x71, Query = 0x72, EndOfMessages = 0x7F;

typedef uint8_t MessageData;
const MessageData NoData = 0, Override = 1, Reset = 2, Unsolved = 3, Solved = 4, Blocked = 5, FirstSolved = 6;

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
    digitalWrite(relay, state);
}

Puzzle puzzles[] = {
    // Puzzle("hallway", 
    //     A1,A0,A2,A3,A4,255, 
    //     Puzzle5, NoData, NONE, 
    //     {resetCallback, [](){ updateRelay(HALLWAY_DOOR, LOW); }}, 
    //     {solvedCallback, [](){ playSound(HEAVY_SWITCH); }, [](){ updateRelay(HALLWAY_DOOR, HIGH); }}, 
    //     {altSolvedCallback, [](){ playSound(PRESSURE_PLATE); }}),
    // Puzzle("cryptex", 
    //     5,4,6,7,255,255, 
    //     Puzzle7, NoData, NONE, 
    //     {resetCallback, [](){ updateRelay(ROTATING_DOOR, LOW); }}, 
    //     {solvedCallback, [](){ playSound(MACHINERY); }, [](){ updateRelay(ROTATING_DOOR, HIGH); }}, 
    //     {altSolvedCallback}
    // ),
    Puzzle("sample", 
        8,9,10,11,255,255, 
        Puzzle1, NoData, NONE, 
        {resetCallback, [](){ updateRelay(ROTATING_DOOR, LOW); }}, 
        {solvedCallback, [](){ playSound(MACHINERY); }, [](){ updateRelay(ROTATING_DOOR, HIGH); }}, 
        {altSolvedCallback}
    ),
    // Puzzle("lights", 
    //     4,5,6,7,255,255, 
    //     Puzzle9, NoData, NONE, 
    //     {resetCallback}, 
    //     {solvedCallback}, 
    //     {altSolvedCallback}
    // ),
    // Puzzle("dracula", 
    //     8,9,10,11,255,12, 
    //     Puzzle10, NoData, Puzzle9, 
    //     {resetCallback}, 
    //     {solvedCallback}, 
    //     {altSolvedCallback}),
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

    if (sig <= 9) {
        if (data == Override) {
            puzzles[sig].startPulse(Override);
        }
         if (data == Reset) {
            puzzles[sig].startPulse(Reset);
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

// Pin definitions
const uint8_t NUMBER_FILE_PINS[4] = {14, 15, 16, 17};
const uint8_t NUMBER_FOLDER_PINS[3] = {18, 19, 20};
const uint8_t SEND_COMMAND_PIN = 21;

// Functions
void setPins(const uint8_t pins[], uint8_t n, uint8_t value) {
    for(uint8_t i = 0; i < n; i++) {
        digitalWrite(pins[i], (value & (1 << i)) ? HIGH : LOW);
    }
}

unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 2000; // 5 seconds in milliseconds
uint8_t currentSoundNumber = 1; // start with the first sound
const uint8_t MAX_SOUNDS = 120; // max number of sounds

void sendCommand() {
    digitalWrite(SEND_COMMAND_PIN, HIGH);
    delay(100);  // Pulse for 20 ms
    digitalWrite(SEND_COMMAND_PIN, LOW);
}

void printPinStates() {
    #ifdef SERIAL_DEBUGGING
    Serial.print("File Pins: ");
    for (uint8_t i = 0; i < 4; i++) {
        Serial.print(digitalRead(NUMBER_FILE_PINS[i]));
        Serial.print(" ");
    }
    Serial.print(" | Folder Pins: ");
    for (uint8_t i = 0; i < 3; i++) {
        Serial.print(digitalRead(NUMBER_FOLDER_PINS[i]));
        Serial.print(" ");
    }
    Serial.println();
    #endif 
}

void setNumbersAndSend(uint8_t file, uint8_t folder) {
    setPins(NUMBER_FILE_PINS, 4, file);
    setPins(NUMBER_FOLDER_PINS, 3, folder);
    printPinStates();
    delay(100);
    sendCommand();
}

uint8_t translateSoundNumber(uint8_t desiredSound) {    
    const uint8_t translationTable[15] = {9, 15, 1, 14, 2, 3, 7, 13, 12, 6, 10, 4, 5, 11, 8};

    #ifdef SERIAL_DEBUGGING

    Serial.print("desired sound: ");
    Serial.print(desiredSound);

    #endif

    if (desiredSound < 1 || desiredSound > 15) {
        #ifdef SERIAL_DEBUGGING
        Serial.println(" OUT OF RANGE TRANSLATE");
        #endif
        return 0;
    }

    // Return the index (plus 1 for 1-based) of the desiredSound in the translation table.
    for (uint8_t i = 0; i < 15; i++) {
        if (translationTable[i] == desiredSound) {
            #ifdef SERIAL_DEBUGGING
            Serial.print(", maps to sound: ");
            Serial.println(i);
            #endif
            return i + 1;
        }
    }
    
    // Error: Sound not found in the table
    return 0;
}


void playSound(SFX soundNumber) {
    if(soundNumber < 1 || soundNumber > 120) {
        return;
    }

    uint8_t folderNumber = (soundNumber - 1) / 15;
    uint8_t fileNumber = (soundNumber) % 15;
    if (fileNumber == 0) {
        fileNumber = 15;
    }
    uint8_t fileNumberTranslated = translateSoundNumber(fileNumber);
    #ifdef SERIAL_DEBUGGING

    Serial.print("Play: ");
    Serial.print(soundNumber);
    Serial.print(":");

    Serial.print(folderNumber);
    Serial.print("-");
    Serial.println(fileNumber);
    #endif

    setNumbersAndSend(fileNumberTranslated, folderNumber);
}

void setup() {
    Serial.begin(9600);
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, N_PUZZLES);
    FastLED.setBrightness(128);  // Correct way to set brightness
    FastLED.show();
    
    for(uint8_t i = 0; i < N_BUTTONS; i++) pinMode(buttons[i], INPUT_PULLUP);

    for(size_t i = 0; i < sizeof(puzzles) / sizeof(Puzzle); i++) {
        puzzles[i].setup();
        // puzzles[i].setState(Reset);
        puzzles[i].setCallback(update_light);
        // puzzles[i].setPlaySoundCallback(playSound);
    }


    for(uint8_t i = 0; i < 4; i++) pinMode(NUMBER_FILE_PINS[i], OUTPUT);
    for(uint8_t i = 0; i < 3; i++) pinMode(NUMBER_FOLDER_PINS[i], OUTPUT);
    pinMode(SEND_COMMAND_PIN, OUTPUT);

    // randomSeed(analogRead(0)); // Use an unconnected analog pin for randomness
}

void loop() {
    for(uint8_t i = 0; i < N_BUTTONS; i++) {
        if(button_pressed(i)) handle_button(i);
    }
    // Serial.println(puzzles[0].getCurrentState());
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


    process_messages();
    delay(50);
}