try:
    import RPi.GPIO as GPIO
    GPIO_AVAILABLE = True
    GPIO.setmode(GPIO.BCM)
except ImportError:
    GPIO_AVAILABLE = False

class Button:
    def __init__(self, name, gpio_pin):
        self.name = name
        self.gpio_pin = gpio_pin
        self.state = False
        if GPIO_AVAILABLE:
            GPIO.setup(self.gpio_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)
        
    def read_state(self):
        if GPIO_AVAILABLE:
            self.state = not GPIO.input(self.gpio_pin)
        else:
            # Return always False (or 0) if GPIO is not available
            self.state = False
        return self.state

class Buttons:
    def __init__(self, button_definitions):
        self.buttons = {}
        for name, gpio_pin in button_definitions.items():
            self.buttons[name] = Button(name, gpio_pin)
            
    def get_state(self, button_name):
        if button_name in self.buttons:
            return self.buttons[button_name].read_state()
        else:
            raise ValueError(f"Button '{button_name}' not defined.")
            
    def get_all_states(self):
        states = {}
        for name, button in self.buttons.items():
            states[name] = button.read_state()
        return states

# Define the buttons
BUTTON_DEFINITIONS = {
	'quit' : 27,
	'lose' : 22,
	'win' : 23,
	'start' : 19,
	'time_up' : 16,
	'time_down' : 17,
	'shutdown' : 25,
	'reset' : 24,
}

buttons = Buttons(BUTTON_DEFINITIONS)

# Example usage:
print(buttons.get_state('quit'))
print(buttons.get_all_states())