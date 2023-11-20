import curses
import threading

class CLIControl(threading.Thread):
    def __init__(self, buttons_instance, puzzles, mega):
        super().__init__()
        self.buttons = buttons_instance
        self.running = True
        self.actions = ["start", "time_up", "time_down", "reset", "win", "lose", "quit"]
        self.selected = -1
        self.log_win = None
        self.header_win = None
        self.scenes_win = None
        self.puzzles_menu_win = None
        self.log_height = 10
        self.puzzles_selected = 0
        self.active_menu = "puzzles"  # Can be "main" or "puzzles"
        self.log_messages = []
        self.puzzles = puzzles
        self.mega = mega

    def run(self):
        curses.wrapper(self.curses_main)


    def curses_main(self, stdscr):
        curses.curs_set(0)
        stdscr.nodelay(0)
        stdscr.clear()

        curses.start_color()
        curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLACK)  # NoData, Override, Reset
        curses.init_pair(2, curses.COLOR_RED, curses.COLOR_BLACK)    # Unsolved
        curses.init_pair(3, curses.COLOR_GREEN, curses.COLOR_BLACK)  # Solved
        curses.init_pair(4, curses.COLOR_YELLOW, curses.COLOR_BLACK) # Blocked
        curses.init_pair(5, curses.COLOR_CYAN, curses.COLOR_BLACK)   # FirstSolved

        height, width = stdscr.getmaxyx()
        header_height = 1
        scenes_height = 2
        menu_height = 1
        puzzles_menu_width = 30
        log_width = width - puzzles_menu_width

        self.header_win = curses.newwin(header_height, width, 0, 0)
        self.scenes_win = curses.newwin(scenes_height, width, 1, 0)
        self.puzzles_menu_win = curses.newwin(height - header_height - menu_height - scenes_height, puzzles_menu_width, header_height + scenes_height, 0)
        self.log_win = curses.newwin(height - header_height - menu_height - scenes_height, log_width, header_height + scenes_height, puzzles_menu_width)
        menu_win = curses.newwin(menu_height, width, height - menu_height, 0)

        self.puzzles_menu_win.keypad(True)
        menu_win.keypad(True)
        self.draw_header("00:00")


        while self.running:
            self.draw_menu(menu_win)
            self.draw_puzzles_menu()
            self.update_log()

            key = menu_win.getch() if self.active_menu == "main" else self.puzzles_menu_win.getch()


            if key == 9:
                self.active_menu = "puzzles" if self.active_menu == "main" else "main"
                continue
            
            num_puzzles = len(self.puzzles)

            if self.active_menu == "main":

                if key in [curses.KEY_LEFT, curses.KEY_UP]:
                    if self.selected > 0:
                        self.selected -= 1
                    else:
                        self.selected = -1
                        self.active_menu = "puzzles"
                        self.puzzles_selected = (num_puzzles * 2) - 2
                elif key in [curses.KEY_RIGHT, curses.KEY_DOWN]:
                    if self.selected < len(self.actions) - 1:
                        self.selected += 1
                    else:
                        self.active_menu = "puzzles"
                        self.puzzles_selected = 0
                        self.selected = -1

                elif key in [curses.KEY_ENTER, ord('\n'), ord(' ')]:
                    self.process_command(self.actions[self.selected])
                    if self.actions[self.selected] == "quit":
                        break

            elif self.active_menu == "puzzles":

                # Handling UP and DOWN keys
                if key == curses.KEY_UP:
                    if self.puzzles_selected >= 3:  # Change from 2 to 3
                        self.puzzles_selected -= 3
                    else:
                        self.puzzles_selected -= 3
                        self.active_menu = "main"
                        self.selected = len(self.actions) - 1
                elif key == curses.KEY_DOWN:
                    if self.puzzles_selected < (num_puzzles * 3 - 3):  # Change from 2 to 3
                        self.puzzles_selected += 3
                    else:
                        self.puzzles_selected += 3
                        self.active_menu = "main"
                        self.selected = 0

                # Handling RIGHT and LEFT keys
                elif key == curses.KEY_RIGHT:
                    if self.puzzles_selected % 3 < 2:  # Change from 2 to 3
                        self.puzzles_selected += 1
                elif key == curses.KEY_LEFT:
                    if self.puzzles_selected % 3 > 0:  # Change from 2 to 3
                        self.puzzles_selected -= 1

                elif key in [curses.KEY_ENTER, ord('\n'), ord(' ')]:
                    selected_puzzle_index = self.puzzles_selected // 3
                    selected_puzzle = self.puzzles[selected_puzzle_index]['name']
                    selected_action = 'R' if self.puzzles_selected % 3 == 0 else ('O' if self.puzzles_selected % 3 == 1 else 'B')

                    if selected_action == 'R':
                        self.resetPuzzle(self.puzzles[selected_puzzle_index]['signal'])
                    elif selected_action == 'O':
                        self.overridePuzzle(self.puzzles[selected_puzzle_index]['signal'])
                    elif selected_action == 'B':
                        self.rebootPuzzle(self.puzzles[selected_puzzle_index]['signal'])

        curses.endwin()

    def draw_header(self, right_text):
        height, width = self.header_win.getmaxyx()
        game_name = "DRACULA"
        self.header_win.clear()
        self.header_win.addstr(0, 0, game_name)

        self.header_win.addstr(0, max(len(game_name) + 1, width - len(right_text) - 1), right_text)

        self.header_win.refresh()

    def draw_scenes(self, scenes_text):
        if self.scenes_win is not None:
            height, width = self.scenes_win.getmaxyx()
            self.scenes_win.clear()
            self.scenes_win.addstr(0, 0, "active scenes: " + scenes_text)

            horizontal_rule = "-" * (width - 1)
            self.scenes_win.addstr(1, 0, horizontal_rule)

            self.scenes_win.refresh()

    def draw_menu(self, menu_win):
        menu_win.clear()
        height, width = menu_win.getmaxyx()

        x_position = 0

        for idx, action in enumerate(self.actions):
            mode = curses.A_REVERSE if idx == self.selected else curses.A_NORMAL
            
            display_text = action[:width - x_position - 1]

            menu_win.addstr(0, x_position, display_text, mode)

            x_position += len(display_text) + 3
            if x_position >= width:
                break

        menu_win.refresh()

    def draw_puzzles_menu(self):
        self.puzzles_menu_win.clear()
        height, width = self.puzzles_menu_win.getmaxyx()
        color_pair = curses.color_pair(1)
        
        menu_items = []
        for puzzle in self.puzzles:
            combined_item = f"{puzzle['name']} [R][O][B]"
            menu_items.append(combined_item)

        for idx, combined_item in enumerate(menu_items):
            if idx >= height - 1:
                break

            state = self.puzzles[idx]['state']
            if state in [0, 1, 2]:
                color_pair = curses.color_pair(1)
            elif state == 3:
                color_pair = curses.color_pair(2)
            elif state == 4:
                color_pair = curses.color_pair(3)
            elif state == 5:
                color_pair = curses.color_pair(4)
            elif state == 6:
                color_pair = curses.color_pair(5)

            puzzle_label_len = len(self.puzzles[idx]['name'])
            if self.puzzles_selected // 3 == idx:  # Updated division to 3 for [R][O][B]
                puzzle_label_len = len(self.puzzles[idx]['name'])+1  # Length of puzzle name
                button_length = 3  # Length of each button including brackets

                selected_button_index = self.puzzles_selected % 3
                before_length = puzzle_label_len + selected_button_index * button_length
                highlight_length = button_length  # Length of the button text including brackets

                before = combined_item[:before_length]
                highlight = combined_item[before_length:before_length + highlight_length]
                after = combined_item[before_length + highlight_length:]

                # Display the text with appropriate highlighting
                self.puzzles_menu_win.addstr(idx, 0, before, color_pair)
                self.puzzles_menu_win.addstr(idx, len(before), highlight, curses.A_REVERSE | color_pair)
                self.puzzles_menu_win.addstr(idx, len(before) + len(highlight), after, color_pair)
            else:
                self.puzzles_menu_win.addstr(idx, 0, combined_item[:width-1], color_pair)  # Truncate to fit

        self.puzzles_menu_win.refresh()

    def update_log(self):
        if self.log_win:
            self.log_win.clear()

            for idx, log in enumerate(reversed(self.log_messages[-self.log_height:])):
                self.log_win.addstr(idx, 0, log)
            
            self.log_win.refresh()

    def process_command(self, cmd):
        if cmd == "start":
            self.buttons.timer.start_timer()
        elif cmd == "time_up":
            self.buttons.timer.add_time()
        elif cmd == "time_down":
            self.buttons.timer.remove_time()
        elif cmd == "reset":
            self.buttons.reset()
            self.buttons.timer.kill_timer()
        elif cmd == "win":
            self.buttons.win_game()
            self.buttons.timer.stop_timer()
        elif cmd == "lose":
            self.buttons.lose_game()
            self.buttons.timer.stop_timer()
        elif cmd == "quit":
            self.buttons.timer.stop_timer()

        elif cmd == "exit":
            self.running = False

    def resetPuzzle(self, signal):
        # selected_puzzle = self.puzzles[selected_puzzle_index]['name']
        # self.log("Reset: " + str(signal))
        self.mega.send_message(signal, 2)
        pass

    def overridePuzzle(self, signal):
        # selected_puzzle = self.puzzles[selected_puzzle_index]['name']
        # self.log("Override: " + str(signal))
        self.mega.send_message(signal, 1)
        pass

    def rebootPuzzle(self, signal):
        # self.log("Reboot: " + str(signal))
        self.mega.send_message(signal, 7)
        # Add logic to send the reboot signal
        pass

    def log(self, message):
        self.log_messages.append(message)
        self.update_log()


    def stop(self):
        self.running = False