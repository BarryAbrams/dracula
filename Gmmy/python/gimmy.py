import pygame, argparse, time, threading, serial, json
from datetime import datetime, timedelta
from enum import IntEnum
import clicontrol, globals_module
from sevenseg import SevenSeg
from pydmx import DMX
from portfinder import find_port
import RPi.GPIO as GPIO
import bootunes
import os
os.putenv('SDL_VIDEODRIVER', 'dummy')  # Set before pygame init
cli_control = None

active_scenes = []
current_music = None
timer_play = False
class MessageSignal(IntEnum):
    Puzzle1 = 0
    Puzzle2 = 1
    Puzzle3 = 2
    Puzzle4 = 3
    Puzzle5 = 4
    Puzzle6 = 5
    Puzzle7 = 6
    Puzzle8 = 7
    Puzzle9 = 8
    Puzzle10 = 9
    Sound = 19
    Time = 20
    Shutdown = 21
    Startup = 22
    Debug = 0x71
    Query = 0x72
    EndOfMessages = 0x7F

class MessageData(IntEnum):
    NoData = 0
    Override = 1
    Reset = 2
    Unsolved = 3
    Solved = 4
    Blocked = 5
    FirstSolved = 6

class SoundFX(IntEnum):
    NoData = 0
    BatSwarm = 1
    PressurePlate = 2
    Gong = 11

puzzles = [
    {
        "name":"gravestones",
        "signal":MessageSignal.Puzzle1,
        "state":MessageData.NoData
    },
    {
        "name":"slide",
        "signal":MessageSignal.Puzzle3,
        "state":MessageData.NoData
    },
    {
        "name":"hallway",
        "signal":MessageSignal.Puzzle5,
        "state":MessageData.NoData,
        "first_solve_sound":SoundFX.BatSwarm,
        "solve_sound":SoundFX.Gong,
        "unsolve_sound":SoundFX.PressurePlate,
    },
  
    {
        "name":"cryptex",
        "signal":MessageSignal.Puzzle7,
        "state":MessageData.NoData,
        "solve_sound":SoundFX.BatSwarm,
        "unsolve_sound":SoundFX.PressurePlate,
    },
       {
        "name":"seance",
        "signal":MessageSignal.Puzzle8,
        "state":MessageData.NoData
    },
    {
        "name":"dracula_illuminate",
        "signal":MessageSignal.Puzzle9,
        "state":MessageData.NoData
    },
    {
        "name":"dracula_stab",
        "signal":MessageSignal.Puzzle10,
        "state":MessageData.NoData
    }
]

scenes = [
    {
        "name":"preshow",
        "requirements": [
            ["all", {"global_check": "timer_playing", "value":False}]
        ], 
        "replaces":[],
        "music":"69_Forest_Night.mp3",
        "light_animation":"hallway_chandelier"
    },
    {
        "name":"forest_intro",
        "requirements": [
            ["all", {"global_check": "timer_playing", "value":True}]
        ], 
        "replaces":[],
        "music":"69_Forest_Night.mp3",
        "light_animation":"castle_wall_ambient"
    },
    {
        "name":"crypt",
        "requirements": [
            ["all", {"puzzle": "tree_door", "state": MessageData.Solved}]
        ],        
        "replaces":["forest_intro"],
        "light_animation":"crypt_ambient"
    },
    {
        "name":"forest_stairs",
        "requirements": [
            ["all", {"puzzle": "gravestones", "state": MessageData.Solved}]
        ],
        "light_animation":"forest_stairs_ambient"
    },
    {
        "name":"hallway",
        "requirements": [
            ["all", {"puzzle": "slide", "state": MessageData.Solved}]
        ],
        "replaces":[],
        "music":"242_Spiders_Den.mp3"
    },
    {
        "name":"parlor",
        "requirements": [
            ["any", 
            {"puzzle": "hallway", "state": MessageData.FirstSolved}, 
            {"puzzle": "hallway", "state": MessageData.Solved}
            ]
        ],
        "replaces":[],
        "music":"148_Barovian_Castle.mp3"
    },
    {
        "name":"bedroom",
        "requirements": [
            ["all", {"puzzle": "cryptex", "state": MessageData.Solved}]
        ],
        "replaces":[],
        "music":"242_Spiders_Den.mp3"
    },
    {
        "name":"illuminate",
        "requirements": [
            ["all", {"puzzle": "dracula_illuminate", "state": MessageData.Solved}]
        ],
        "replaces":["parlor", "bedroom", "hallway"]
    },
    {
        "name":"kill_dracula",
        "requirements": [
            ["all", {"puzzle": "dracula_stab", "state": MessageData.Solved}]
        ],
        "replaces":["illuminate"],
        "music":"267_Court_of_the_Count.mp3",
        "light_animation":"kill_dracula"
    }
]


# def time(time): return timedelta(seconds=pytimeparse.timeparse.timeparse(time))
now = datetime.now
zero = timedelta(0) 
startup_time = now()

game_length = 60*60

class BackgroundSound:
    def __init__(self):
        pygame.init()
        pygame.mixer.init()
        pygame.mixer.music.set_volume(1.0)  # Set to maximum
        # devices = tuple(sdl2_audio.get_audio_device_names(capture_devices))
        self.current_track = None
        
    def play_background(self, filename, fade_time=0):
        current_directory = os.getcwd()
        sound_filepath = os.path.join(current_directory, "ambient", filename)
        
        # For debugging purposes
        log(f"Play ambient: {filename}")

        if self.current_track:
            pygame.mixer.music.fadeout(fade_time)
            time.sleep(fade_time / 1000.0)  # Sleep for the fadeout duration to let it complete
        pygame.mixer.music.load(sound_filepath)
        pygame.mixer.music.play(loops=-1)  # Loop indefinitely with -1
        self.current_track = filename

    def stop(self):
        pygame.mixer.music.stop()

class Timer(object):
    def __init__(self, mega, background_sound, scene_manager):
        self.is_running = False
        self.seconds_left = 0
        self.game_started = None
        self.display = None
        self.mega = mega
        self.background_sound = background_sound
        self.scene_manager = scene_manager

        self.timer_thread = threading.Thread(target=self.tick)
        self.timer_thread.start()

    def tick(self):
        while True:
            if self.is_running and self.game_started is not None:
                current_time = datetime.now()
                elapsed_time = current_time - self.game_started
                self.seconds_left = game_length - elapsed_time.total_seconds()
                time_left = int(self.seconds_left)
                time_log(time_left)

                # log(str(puzzles[0]['state']))

                if self.seconds_left <= 1:
                    self.stop_timer()

            time.sleep(1)

    def start_timer(self):
        log("Start game")
        if not self.is_running:
            self.seconds_left = game_length
            self.game_started = datetime.now()
            self.is_running = True
            # self.background_sound.play_background('69_Forest_Night.wav', fade_time=3000)
            # self.mega.send_message(MessageSignal.Sound, 1)
            globals_module.timer_playing = True
            self.scene_manager.update_scenes()
            if self.display is not None:
                self.display.win_time = None
                self.display.is_running = True

    def stop_timer(self):
        log("Stop game")
        self.is_running = False
        globals_module.timer_playing = False
        self.scene_manager.update_scenes()
        if self.display is not None:
            # self.background_sound.play_background('148_Barovian_Castle.mp3', fade_time=3000)
            self.display.win_time = self.seconds_left
            self.display.is_running = False

    def add_time(self):
        if self.is_running:
            self.game_started += timedelta(minutes=1)
            self.seconds_left += 60

    def remove_time(self):
        if self.is_running:
            self.game_started -= timedelta(minutes=1)
            self.seconds_left -= 60

    def kill_timer(self):
        if self.is_running == False:
            self.is_running = False
            if self.display is not None:
                self.display.win_time = None
        pass

class Mega(object):
    def __init__(self, background_sound, scene_manager):
        self.ser = serial.Serial('/dev/ttyACM1', 9600, timeout=1)
        # self.ser = serial.Serial('/dev/tty.usbmodem21401', 9600, timeout=1)
        self.scene_manager = scene_manager
        time.sleep(2)

        read_thread = threading.Thread(target=self.read_messages)
        read_thread.start()
        self.background_sound = background_sound

        pass

    def read_messages(self):
        """Continuously read messages from the Arduino."""
        while True:
            while self.ser.inWaiting():
                first_byte = self.ser.read(1)
                second_byte = self.ser.read(1)
                
                message_signal = ord(first_byte) & 0x7F
                message_data = ord(second_byte) & 0x7F
                
                # try:
                signal_name = MessageSignal(message_signal).name
                data_name = MessageData(message_data).name
                
                if message_signal <= 9:
                    self.perform_logic(message_signal, message_data)

                    if cli_control:
                        cli_control.draw_puzzles_menu()
                        found_puzzle = None
                        for puzzle in puzzles:
                            if puzzle['signal'] == message_signal:
                                found_puzzle = puzzle['name']

                        
                        log(found_puzzle + " " + MessageData(message_data).name)

                # except:
                #     print(f"Couldn't find Data {message_signal}")

    def send_message(self, signal, data):
        """Send a message to the Arduino."""
        signal = signal & 0x7F
        data = data & 0x7F
        
        self.ser.write(bytes([signal | 0x80, data]))


    def perform_logic(self, puzzle_id, state):
        global active_scenes
        for puzzle in puzzles:
            if puzzle['signal'] == puzzle_id:
                if puzzle['state'] != state:
                    if state == 4:
                        if "solve_sound" in puzzle:
                            bootunes.play_sound(puzzle['solve_sound'])
                    if state == 6:
                        if "first_solve_sound" in puzzle:
                            bootunes.play_sound(puzzle['first_solve_sound'])
                puzzle['state'] = state

        self.scene_manager.update_scenes()
        pass

class Animation:
    def __init__(self, data):
        self.name = data['name']
        self.lights = data['lights']
        self.duration = data['duration']
        self.priority = data['priority']
        self.delay = data['delay']
        self.loop = data['loop']
        self.steps = data['steps']
        self.start_time = None

class LightType:
    def __init__(self, id, channels):
        self.id = id
        self.channels = channels

class Light:
    def __init__(self, id, light_type, address):
        self.id = id
        self.type = light_type
        self.address = address
        self.channel_values = {channel: 0 for channel in self.type.channels}

    def setColor(self, hex_color):
        hex_color = hex_color.lstrip('#')

        if len(hex_color) == 6 and all(ch in self.channel_values for ch in ['R', 'G', 'B']):
            # Standard 6-digit RGB hex code
            rgb = tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
            self.channel_values['BRIGHTNESS'] = 255
            self.channel_values['R'], self.channel_values['G'], self.channel_values['B'] = rgb

            if "MACRO" in self.channel_values:
                self.channel_values['MACRO'] = 0
                self.channel_values['MACRO_SPEED'] = 0

        elif len(hex_color) == 8 and all(ch in self.channel_values for ch in ['R', 'G', 'B', 'BRIGHTNESS']):
            # 8-digit hex code with brightness
            brightness = int(hex_color[:2], 16)
            rgb = tuple(int(hex_color[i:i+2], 16) for i in (2, 4, 6))
            self.channel_values['BRIGHTNESS'] = brightness
            self.channel_values['R'], self.channel_values['G'], self.channel_values['B'] = rgb

        else:
            raise ValueError("Invalid hex color format or the light does not have the required channels")

    def setBrightness(self, brightness_percent):
        if 'BRIGHTNESS' not in self.channel_values:
            raise ValueError("This light does not have a BRIGHTNESS channel")

        if not 0 <= brightness_percent <= 100:
            raise ValueError("Brightness percent must be between 0 and 100")

        brightness_value = int((brightness_percent / 100) * 255)
        self.channel_values['BRIGHTNESS'] = brightness_value


    def get_channel_values_array(self):
        return [self.channel_values[channel] for channel in self.type.channels]


class SceneManager(object):
    def __init__(self, background_sound):
        port = find_port('ttyACM0*')
        # port = find_port('tty.usbmodem00*')
        self.dmx = DMX(port)
        self.sound = background_sound
        self.current_music = None
        self.active_scenes = []
        self.frames_per_second = 30
        self.light_types = self.load_light_types()
        self.lights = self.load_lights()
        self.active_animations = []

        loop_thread = threading.Thread(target=self.loop)
        loop_thread.start()

        self.play_animation("forest_ambient")
        self.update_scenes()

        self.update_dmx_data()
        self.dmx.render()

    def load_light_types(self):
        light_types = []
        data = self.load_json_data()
        for lt in data[0]['types']:
            light_types.append(LightType(lt['id'], lt['channels']))
        return light_types

    def load_lights(self):
        lights = []
        used_channels = set()

        data = self.load_json_data()
        for light_data in data[1]['lights']:
            light_type = next((lt for lt in self.light_types if lt.id == light_data['type']), None)
            if not light_type:
                raise ValueError(f"Light type {light_data['type']} not found")

            start_address = light_data['address']
            end_address = start_address + len(light_type.channels) - 1

            if any(ch in used_channels for ch in range(start_address, end_address + 1)):
                raise ValueError(f"DMX channel overlap detected for light {light_data['id']}")

            used_channels.update(range(start_address, end_address + 1))

            if end_address > 512:
                raise ValueError(f"DMX channel range for light {light_data['id']} exceeds the universe limit")

            lights.append(Light(light_data['id'], light_type, start_address))

        return lights

    def get_full_dmx_array(self):
        dmx_universe = [0] * 512
        for light in self.lights:
            start_address = light.address - 1
            channel_values = light.get_channel_values_array()

            for i, value in enumerate(channel_values):
                address = start_address + i
                if address < 512:
                    dmx_universe[address] = value
                else:
                    break

        return dmx_universe

    def update_dmx_data(self):
        dmx_universe = self.get_full_dmx_array()
        for i, value in enumerate(dmx_universe):
            self.dmx.set(i + 1, value)  # +1 because DMX channels are 1-indexed


    def load_json_data(self):
        # Load the JSON data from the file
        with open('lights.json', 'r') as file:
            return json.load(file)

    def play_animation(self, animation_name):
        with open(f'light_animations/{animation_name}.json', 'r') as file:
            animation_data = json.load(file)

        for anim in animation_data:
            animation = Animation(anim)
            animation.start_time = time.time()
            animation.initial_start_time = animation.start_time  # Set the initial start time
            self.active_animations.append(animation)

    def update_lights_for_animation(self, animation, current_time):
        elapsed = (current_time - animation.start_time) * 1000  # Convert to ms
        if elapsed < animation.delay:
            return  # Delay not yet passed, do not start the animation

        # Adjust elapsed time to account for the delay
        adjusted_elapsed = elapsed - animation.delay

        pos = (adjusted_elapsed % animation.duration) / animation.duration

        current_step = None
        next_step = None

        # Find the current and next steps
        for i in range(len(animation.steps) - 1):
            if pos >= animation.steps[i]['pos'] and pos < animation.steps[i + 1]['pos']:
                current_step = animation.steps[i]
                next_step = animation.steps[i + 1]
                break

        # Handle wrap-around for looping animations
        if pos >= animation.steps[-1]['pos']:
            current_step = animation.steps[-1]
            next_step = animation.steps[0]

        if current_step is None or next_step is None:
            return  # No valid steps found

        # Interpolate the color
        color = self.interpolate_color(current_step['color'], next_step['color'], pos, current_step['pos'], next_step['pos'])

        for light_id in animation.lights:
            light = next((l for l in self.lights if l.id == light_id), None)
            if light:
                light.setColor(color)

        self.render_scene()

    def interpolate_color(self, start_color, end_color, pos, start_pos, end_pos):
        """
        Interpolate between two colors, ensuring that the RGB values are within the 0-255 range.
        """
        if start_pos == end_pos:
            return start_color  # Avoid division by zero

        # Convert hex colors to RGB
        start_rgb = tuple(int(start_color.lstrip('#')[i:i+2], 16) for i in (0, 2, 4))
        end_rgb = tuple(int(end_color.lstrip('#')[i:i+2], 16) for i in (0, 2, 4))

        # Calculate the interpolation factor
        factor = (pos - start_pos) / (end_pos - start_pos)

        # Interpolate each color channel and clamp values between 0 and 255
        interpolated_rgb = tuple(min(max(int(start + (end - start) * factor), 0), 255) for start, end in zip(start_rgb, end_rgb))
        interpolated_color = f"#{''.join(f'{c:02x}' for c in interpolated_rgb)}"

        return interpolated_color


    def render_scene(self):
        """
        Renders the current scene by sending the DMX data to the ultraDMX Micro device.
        """
        self.update_dmx_data()
        self.dmx.render()

    def loop(self):
        while True:
            current_time = time.time()

            for animation in self.active_animations[:]:
                elapsed = (current_time - animation.start_time) * 1000

                if animation.start_time == animation.initial_start_time and elapsed < animation.delay:
                    continue

                if elapsed - (animation.delay if animation.start_time == animation.initial_start_time else 0) > animation.duration:
                    if animation.loop:
                        animation.start_time = current_time - ((elapsed - animation.delay) % animation.duration) / 1000
                    else:
                        self.active_animations.remove(animation)
                        continue

                self.update_lights_for_animation(animation, current_time)

            self.render_scene()
            time.sleep(1 / self.frames_per_second)


    def active_light_animations(self):
        light_animations = []
        for active_scene in self.active_scenes:
            if 'light_animation' in active_scene:
                light_animations.append(active_scene['light_animation'])

        return light_animations

    def update_scenes(self):
        active_scenes = []
        puzzles_dict = {puzzle['name']: puzzle['state'] for puzzle in puzzles}

        for scene in scenes:
            enable_scene = not scene['requirements'] or all(self.meets_requirement(puzzles_dict, req) for req in scene['requirements'])

            if enable_scene:
                if 'replaces' in scene:
                    active_scenes = [s for s in active_scenes if s['name'] not in scene['replaces']]

                if scene not in active_scenes:
                    active_scenes.append(scene)

        music = None
        active_scene_names = []
        for active_scene in active_scenes:
            active_scene_names.append(active_scene['name'])
            if 'music' in active_scene:
                music = active_scene['music']

        log(", ".join(active_scene_names))

        self.active_scenes = active_scenes


        if music and self.current_music != music:
            self.current_music = music
            print("music play", self.current_music)
            self.sound.play_background(self.current_music, fade_time=3000)

    def meets_requirement(self, puzzle_state, requirement):
        if type(requirement) is dict:
            if 'global_check' in requirement:
                global_variable = requirement['global_check']
                return self.check_global_variable(global_variable, requirement['value'])
            else:
                return puzzle_state.get(requirement['puzzle']) == requirement['state']
        elif type(requirement) is list:
            if requirement[0] == 'any':
                return any(self.meets_requirement(puzzle_state, req) for req in requirement[1:])
            elif requirement[0] == 'all':
                return all(self.meets_requirement(puzzle_state, req) for req in requirement[1:])
            elif requirement[0] == 'not_any':
                return not any(self.meets_requirement(puzzle_state, req) for req in requirement[1:])
            elif requirement[0] == 'not_all':
                return not all(self.meets_requirement(puzzle_state, req) for req in requirement[1:])
        return False

    def check_global_variable(self, variable_name, expected_value):
        global_vars = get_global_variables()  # You need to implement this function
        return global_vars.get(variable_name) == expected_value

   

class Display(object):
    def __init__(self, timer):
        print("start 7 segment thread")
        self.disp = SevenSeg()
        time.sleep(1)
        self.disp.begin()
        self.clear_disp()
        self.is_running = False
        self.win_time = None
        self.timer = timer
        self.flash_flag = False
        self.last_second = -1  # Initialize to -1 to force an initial update
        run_thread = threading.Thread(target=self.run)
        run_thread.start()

    def run(self):
        while True:
            self.draw_disp()

    def clear_disp(self):
            try:
               self.disp.write('    ', '   ')
            except OSError:
                self.fail_disp()

    def fail_disp(self): # Called when the 7-segment display acts up, which it shouldn't do, but...
        pass
            # print('>> 7-segment error! <<')

    def draw_disp(self):
        first, second = self.text_for_disp()
        try:
            self.disp.write(first, second)
        except OSError: # Something went wrong on the I2C bus, and we can't use the seven-seg this tick
            self.fail_disp()

    def text_for_disp(self):
        if self.is_running or self.win_time is not None:
            if self.win_time is not None:
                relevant_time = self.win_time
            else:
                relevant_time = self.timer.seconds_left

            secs = relevant_time
            if secs < 0:
                negative = True
                secs = -secs
            else:
                negative = False
            mins, secs = divmod(secs, 60)
            secs = int(secs)
            mins = int(mins)
            if mins > 99:
                mins %= 100
                overflow = True
            elif mins > 9 and negative:
                mins %= 10
                overflow = True
            else:
                overflow = False

            if negative:
                first = '-{:01d}{:02d}'.format(mins, secs)
            else:
                first = '{:02d}{:02d}'.format(mins, secs)

            if overflow:
                second = "': "
            else:
                second = ' : '

            if self.win_time is not None:
                current_second = time.localtime().tm_sec  # Get the current second
                self.flash_flag = current_second % 2 == 0  # Toggle every second

                if self.flash_flag:
                    return first, second
                else:
                    # Return empty display to hide the text
                    return '    ', '   '
            else:
                return first, second
        else:
            t = int(self.background_timer().total_seconds() * 2)
            name = "drACULA   "
            i = t % len(name)
            first = (name + name)[i:i + 4]
            if t % 4:
                second = "::'"
            else:
                second = '   '
            return first, second

    def background_timer(self):
        return datetime.now() - startup_time

class Buttons(object):
    def __init__(self, args, timer):
        log("start button thread")
        self.buttons = {
            'quit': 27,
            'lose': 22,
            'win': 23,
            'start': 19,
            'time_up': 16,
            'time_down': 17,
            'shutdown': 25,
            'reset': 24
        }

        self.timer = timer
        self.args = args

        if self.args.env == "prod":
            for button in self.buttons.values():
                GPIO.setup(button, GPIO.IN, pull_up_down=GPIO.PUD_UP)

        self.prev_state = None
        self.button_thread = threading.Thread(target=self.run)
        self.button_thread.start()
        
        pass

    def run(self):
        if self.args.env == "prod":
            self.prev_state = {button: GPIO.input(gpio_num) for button, gpio_num in self.buttons.items()}
        
            try:
                while True:
                    for button, gpio_num in self.buttons.items():
                        current_state = GPIO.input(gpio_num)
                        if current_state != self.prev_state[button]:
                            if current_state == 0:
                                print(f"Button '{button}' pressed!")
                                if button == "start":
                                    self.timer.start_timer()
                                elif button == "time_up":
                                    self.timer.add_time()
                                elif button == "time_down":
                                    self.timer.remove_time()
                                elif button == "reset":
                                    self.timer.kill_timer()
                                elif button == "win" or button == "lose" or button == "quit":
                                    self.timer.stop_timer()

                            self.prev_state[button] = current_state
                    time.sleep(0.1)
            except KeyboardInterrupt:
                pass
            finally:
                GPIO.cleanup()

def log(message):
    if cli_control is not None:
        cli_control.log_messages.append(message)
        cli_control.update_log()
    else:
        print("\r"+message)

def time_log(seconds):
    hours = seconds // 3600
    minutes = (seconds % 3600) // 60
    seconds = seconds % 60

    time = f"{minutes}:{seconds:02d}"
    if hours > 0:
        time = f"{hours}:{minutes:02d}:{seconds:02d}"
        

    if cli_control is not None:
        cli_control.draw_header(time)
    else:
        print("\r Time Remaining:"+time)

def get_global_variables():
    # Access the global variables from globals_module
    return {
        "timer_playing": globals_module.timer_playing,
        "some_other_global": globals_module.some_other_global,
        # Add other global variables as needed
    }

mega = None

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", action="store_true", help="Start with CLI control")
    parser.add_argument("--env", choices=['dev', 'prod'], help="Start with CLI control", default="prod")

    args = parser.parse_args()

    if args.env == "prod":
        GPIO.setmode(GPIO.BCM)
        GPIO.setwarnings(False)

    sound = BackgroundSound() # before continuing, make sure this is fully initalized
    if sound:
        sceneManager = SceneManager(sound)
        mega = Mega(sound, sceneManager)
        timer = Timer(mega, sound, sceneManager)
        buttons = Buttons(args, timer)

    if args.env == "prod":
        disp = Display(timer)
        timer.display = disp

    if args.cli:
        cli_control = clicontrol.CLIControl(buttons, puzzles, mega)
        cli_control.start()
