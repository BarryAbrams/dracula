import RPi.GPIO as GPIO
import serial
from datetime import datetime, timedelta
import threading
from enum import IntEnum
from sevenseg import SevenSeg
from time import sleep, localtime
import pygame

import os
os.putenv('SDL_VIDEODRIVER', 'dummy')  # Set before pygame init

active_scenes = []
current_music = None
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

puzzles = [
    {
        "name":"tree_door",
        "signal":MessageSignal.Puzzle2,
        "state":MessageData.NoData
    },
    {
        "name":"slide",
        "signal":MessageSignal.Puzzle4,
        "state":MessageData.NoData
    },
    {
        "name":"hallway",
        "signal":MessageSignal.Puzzle5,
        "state":MessageData.NoData
    },
    {
        "name":"cryptex",
        "signal":MessageSignal.Puzzle7,
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
        "id":0,
        "name":"forest_intro",
        "requirements":[],
        "replaces":[],
        "music":"69_Forest_Night.wav"
    },
    {
        "id":1,
        "name":"forest_crypt",
        "requirements": [
            ["all", {"puzzle": "tree_door", "state": MessageData.Solved}]
        ],        
        "replaces":["forest_intro"]
    },
    {
        "id":2,
        "name":"forest_stairs",
        "requirements": [
            ["all", {"puzzle": "tree_door", "state": MessageData.Solved}]
        ],
        "replaces":["forest_crypt"]
    },
    {
        "id":3,
        "name":"hallway_off",
        "requirements":[],
        "replaces":[]
    },
    {
        "id":4,
        "name":"hallway_on",
        "requirements": [
            ["all", {"puzzle": "slide", "state": MessageData.Solved}]
        ],
        "replaces":["hallway_off"],
        "music":"242_Spiders_Den.wav"
    },
    {
        "id":5,
        "name":"parlor_off",
        "requirements":[],
        "replaces":[]
    },
    {
        "id":6,
        "name":"parlor_on",
        "requirements": [
            ["any", 
            {"puzzle": "hallway", "state": MessageData.FirstSolved}, 
            {"puzzle": "hallway", "state": MessageData.Solved}
            ]
        ],
        "replaces":"parlor_off",
        "music":"148_Barovian_Castle.wav"
    },
    {
        "id":7,
        "name":"bedroom_off",
        "requirements":[],
        "replaces":[]
    },
    {
        "id":8,
        "name":"bedroom_on",
        "requirements": [
            ["all", {"puzzle": "cryptex", "state": MessageData.Solved}]
        ],
        "replaces":["bedroom_off"],
        "music":"242_Spiders_Den.wav"
    },
    {
        "id":9,
        "name":"illuminate",
        "requirements": [
            ["all", {"puzzle": "dracula_illuminate", "state": MessageData.Solved}]
        ],
        "replaces":["parlor_on", "bedroom_on", "hallway_on"]
    },
    {
        "id":10,
        "name":"kill_dracula",
        "requirements": [
            ["all", {"puzzle": "dracula_stab", "state": MessageData.Solved}]
        ],
        "replaces":["illuminate"],
        "music":"267_Court_of_the_Count.wav"
    }
]


GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

def time(time): return timedelta(seconds=pytimeparse.timeparse.timeparse(time))
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
        print(f"Trying to load: {sound_filepath}")

        if self.current_track:
            pygame.mixer.music.fadeout(fade_time)
            sleep(fade_time / 1000.0)  # Sleep for the fadeout duration to let it complete
        pygame.mixer.music.load(sound_filepath)
        pygame.mixer.music.play(loops=-1)  # Loop indefinitely with -1
        print("playing?")
        self.current_track = filename

    def stop(self):
        pygame.mixer.music.stop()

class Timer(object):
    def __init__(self, mega, background_sound):
        self.is_running = False
        self.seconds_left = 0
        self.game_started = None
        self.display = None
        self.mega = mega
        self.background_sound = background_sound

        self.timer_thread = threading.Thread(target=self.tick)
        self.timer_thread.start()

    def tick(self):
        while True:
            if self.is_running and self.game_started is not None:
                current_time = datetime.now()
                elapsed_time = current_time - self.game_started
                self.seconds_left = game_length - elapsed_time.total_seconds()
                print("Time left:", int(self.seconds_left), "seconds")

                if self.seconds_left <= 1:
                    self.stop_timer()

            sleep(1)

    def start_timer(self):
        print("Start game")
        if not self.is_running:
            self.display.win_time = None
            self.seconds_left = game_length
            self.game_started = datetime.now()
            self.is_running = True
            # self.background_sound.play_background('69_Forest_Night.wav', fade_time=3000)
            self.mega.send_message(MessageSignal.Sound, 1) # play a specific sfx when timer starts
            if self.display is not None:
                self.display.is_running = True

    def stop_timer(self):
        print("Stop game")
        self.is_running = False
        if self.display is not None:
            # self.background_sound.play_background('148_Barovian_Castle.wav', fade_time=3000)
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
            self.display.win_time = None
        pass

class Mega(object):
    def __init__(self, background_sound):
        self.ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1)

        sleep(2)

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

                # print(f"Received: SIGNAL {signal_name}, DATA {data_name}")

   

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
                puzzle['state'] = state

        self.update_scenes()
        
        pass
        # if puzzle == "Puzzle10" and state == "Solved":
        #     self.background_sound.play_background('148_Barovian_Castle.wav', fade_time=3000)
        # if puzzle == "Puzzle5" and state == "Solved":
        #     self.background_sound.play_background('267_Court_of_the_Count.wav', fade_time=3000)
        # pass


    def meets_requirement(self, puzzle_state, requirement):
        if type(requirement) is dict:
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


    def update_scenes(self):
        global current_music
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
        for active_scene in active_scenes:
            print(active_scene['name'])
            if 'music' in active_scene:
                music = active_scene['music']

        if music and current_music != music:
            current_music = music
            self.background_sound.play_background(current_music, fade_time=3000)


class Buttons(object):
    def __init__(self, timer):
        print("start button thread")
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

        for button in self.buttons.values():
            GPIO.setup(button, GPIO.IN, pull_up_down=GPIO.PUD_UP)

        self.prev_state = None
        self.button_thread = threading.Thread(target=self.run)
        self.button_thread.start()
        
        pass

    def run(self):
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
                sleep(0.1)
        except KeyboardInterrupt:
            pass
        finally:
            GPIO.cleanup()

class Display(object):
    def __init__(self, timer):
        print("start 7 segment thread")
        self.disp = SevenSeg()
        sleep(1)
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
            print('>> 7-segment error! <<')

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
                current_second = localtime().tm_sec  # Get the current second
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

if __name__ == "__main__":
    sound = BackgroundSound()
    mega = Mega(sound) 
    timer = Timer(mega, sound)
    buttons = Buttons(timer)
    disp = Display(timer)
    timer.display = disp
    # sound.play_background('148_Barovian_Castle.wav', fade_time=3000)