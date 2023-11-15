import time
import board
import busio
from adafruit_mcp230xx.mcp23017 import MCP23017

# Constants
SEND_INTERVAL = 2.0  # Interval in seconds
MAX_SOUNDS = 120

# Setup I2C and MCP23017
i2c = busio.I2C(board.SCL, board.SDA)
mcp = MCP23017(i2c)

# Initialize pins
pins = [mcp.get_pin(i) for i in range(8)]
for pin in pins:
    pin.switch_to_output()

def set_pins(pin_numbers, value):
    for i, pin_num in enumerate(pin_numbers):
        pins[pin_num].value = (value >> i) & 1

def send_command():
    pins[7].value = True  # Assuming SEND_COMMAND_PIN is the 8th pin
    time.sleep(.25)
    pins[7].value = False

def play_sound(sound_number):
    if sound_number < 1 or sound_number > 120:
        return

    folder_number = (sound_number - 1) // 15
    file_number = sound_number % 15
    file_number = 15 if file_number == 0 else file_number

    set_pins(range(4), file_number)  # Assuming first 4 pins for file number
    set_pins(range(4, 7), folder_number)       # Assuming next 3 pins for folder number

    time.sleep(.1)
    send_command()

def main():
    
    play_sound(3)

if __name__ == "__main__":
    main()
