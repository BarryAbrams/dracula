from DMXEnttecPro import Controller
from DMXEnttecPro.utils import get_port_by_serial_number, get_port_by_product_id
import time
import numpy as np

# Function to convert hex color to RGB
def hex_to_rgb(hex_color):
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

# Function to linearly interpolate between two colors
def lerp_color(color_start, color_end, t):
    return tuple(int(a + (b - a) * t) for a, b in zip(color_start, color_end))

class Fixture:
    def __init__(self, dmx_controller, start_address):
        self.dmx_controller = dmx_controller
        self.start_address = start_address

    def set_color_hex(self, hex_color):
        rgb = hex_to_rgb(hex_color)
        self.set_color_rgb(*rgb)

    def set_color_rgb(self, red, green, blue):
        self.dmx_controller.set_channel(self.start_address, red)
        self.dmx_controller.set_channel(self.start_address + 1, green)
        self.dmx_controller.set_channel(self.start_address + 2, blue)

# Example fixture class for a specific type of light
class RGBFixture(Fixture):
    def set_brightness(self, brightness):
        # Assuming the brightness channel is the first channel
        self.dmx_controller.set_channel(self.start_address, brightness)

    def set_color_rgb(self, red, green, blue):
        self.dmx_controller.set_channel(self.start_address + 1, red)
        self.dmx_controller.set_channel(self.start_address + 2, green)
        self.dmx_controller.set_channel(self.start_address + 3, blue)

# Setup the DMX controller
dmx_port = get_port_by_serial_number('6A2XSC5J')
dmx = Controller(dmx_port)

# Create an instance of the fixture at DMX address 1
my_fixture = RGBFixture(dmx, 1)

# Set the color using a hex value
my_fixture.set_brightness(255) # Full brightness
my_fixture.set_color_hex('#FF00FF') # A gray color

color_start = hex_to_rgb('#00FFFF')  # Cyan
color_end = hex_to_rgb('#FF00FF')    # Magenta

# Animation parameters
duration = 2  # Duration of the animation in seconds
frame_rate = 120  # Frame rate
num_frames = frame_rate * duration

# Animation loop
start_time = time.time()
try:
    for frame in range(num_frames):
        # Calculate the time factor (t)
        t = (frame % num_frames) / num_frames
        # Interpolate the color
        color = lerp_color(color_start, color_end, t)
        # Set the interpolated color
        my_fixture.set_color_rgb(*color)
        dmx.submit()
        # Wait to maintain the frame rate
        time.sleep(max(1.0/frame_rate - (time.time() - start_time - frame/frame_rate), 0))
except KeyboardInterrupt:
    dmx.close()
except Exception as e:
    print("An error occurred:", e)
    dmx.close()

# Start the submit loop
try:
    while True:
        time.sleep(1/frame_rate)
        dmx.submit()
except KeyboardInterrupt:
    dmx.close()
except Exception as e:
    print("An error occurred:", e)
    dmx.close()
