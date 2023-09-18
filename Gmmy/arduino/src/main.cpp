#include <Arduino.h>
#include <FastLED.h>

#include "Puzzle.h"

// #define SERIAL_DEBUGGING

// Definitions
#define N_PUZZLES 10
#define LED_PIN 23
#define N_BUTTONS (N_PUZZLES*2+1)
#define STATUS_INDEX (N_BUTTONS-1)
#define DEBOUNCE_TIME 500


typedef uint8_t MessageSignal;
const MessageSignal Puzzle1 = 0, Puzzle2 = 1, Puzzle3 = 2, Puzzle4 = 3, Puzzle5 = 4, Puzzle6 = 5, Puzzle7 = 6, Puzzle8 = 7, Puzzle9 = 8, Puzzle10 = 9, Time = 20, Shutdown = 21, Startup = 22, Debug = 0x71, Query = 0x72, EndOfMessages = 0x7F;

typedef uint8_t MessageData;
const MessageData NoData = 0, Override = 1, Reset = 2, Unsolved = 3, Solved = 4, Extra = 5;

Puzzle puzzles[] = {
    Puzzle("puzzle 1", (uint8_t[]){A2,A3,A4}, (uint8_t[]){A0,A1,255}, Puzzle1, NoData),
    Puzzle("puzzle 2", (uint8_t[]){11,12,14}, (uint8_t[]){2,3,255}, Puzzle2, NoData),
};


struct Message {
    MessageSignal sig : 8;
    MessageData data : 8;
};

// Global variables
CRGB leds[N_PUZZLES] = {0};
const uint8_t buttons[N_BUTTONS] = {
    67, 66, 65, 64, 63, 27, 62, 29, 31, 33, 53, 51, 49, 47, 45, 43, 41, 39, 37, 35, 25
};
bool button_state[N_BUTTONS] = {0};
unsigned long cooldown_time[N_BUTTONS] = {0};

void handle_message(MessageSignal sig, MessageData data) {
    // Serial.print(sig);
}

// Functions
inline uint8_t reroute(uint8_t raw) {
    return (raw >= 5) ? raw - 5 : 9 - raw;
}

inline CRGB choose_color(MessageData data) {
    if(data == Unsolved) return CRGB(0, 128, 0);
    else if(data == Extra) return CRGB(0, 0, 255);
    else if(data == Solved) return CRGB(128, 0, 0);
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

inline Message pack_message(MessageSignal a, MessageData b) {
    Message m;
    m.sig = static_cast<MessageSignal>(a & 0x7F);
    m.data = b & 0x7F;
    return m;
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

void send_message(Message pack, HardwareSerial *port) {
    if(pack.sig != EndOfMessages) {
        byte first = pack.sig | 0x80;
        byte second = pack.data & 0x7F;
        port->write(first);
        port->write(second);
    }
}

void send_message_all(Message pack) {
    send_message(pack, &Serial);
}

inline void send_message_all(MessageSignal sig, MessageData data) {
    send_message_all(pack_message(sig, data));
}

void update_light(MessageSignal sig, MessageData data) {
    if(sig <= Puzzle10) {
        leds[reroute(sig)] = choose_color(data);
        FastLED.show();
    }
}

void status_check() {
    for(uint8_t i = 0; i <= N_PUZZLES; i++) {
        if(i < N_PUZZLES) update_light((MessageSignal)i, Solved);
        delay(100);
        if(i > 0) update_light((MessageSignal)(i-1), NoData);
    }
    for(uint8_t i = 0; i < N_PUZZLES; i++) {
        if(i < sizeof(puzzles) / sizeof(Puzzle)) {
            MessageData status = puzzles[i].getCurrentState();
            Serial.println(status);
            update_light((MessageSignal)i, status);
        } else {
            update_light((MessageSignal)i, NoData);
        }
    }
    send_message_all(Query, NoData);
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

    MessageData newState;
    uint8_t puzzleIndex = which / 2;

    if (puzzleIndex < sizeof(puzzles) / sizeof(puzzles[0])) {
        if (which % 2 == 0) {
            newState = Override;
        } else {
            newState = Reset;
        }

        puzzles[puzzleIndex].setState(newState);
        send_message_all((MessageSignal)puzzleIndex, newState);
    } else {
        // Optionally handle error case where there is no puzzle at this index
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

    for(int i = 0; i < sizeof(puzzles) / sizeof(Puzzle); i++) {
        puzzles[i].setup();
        puzzles[i].setCallback(update_light);
    }


}

void loop() {
    for(uint8_t i = 0; i < N_BUTTONS; i++) {
        if(button_pressed(i)) handle_button(i);
    }
    for (int i = 0; i < sizeof(puzzles) / sizeof(puzzles[0]); i++) {
        puzzles[i].checkPinChanges();
    }
    process_messages();
    delay(10);
}