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
        header_height = 2
        menu_height = 3
        puzzles_menu_width = 30
        log_width = width - puzzles_menu_width

        self.header_win = curses.newwin(header_height, width, 0, 0)
        self.puzzles_menu_win = curses.newwin(height - header_height - menu_height, puzzles_menu_width, header_height, 0)
        self.log_win = curses.newwin(height - header_height - menu_height, log_width, header_height, puzzles_menu_width)
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

                if key == curses.KEY_UP:
                    if self.puzzles_selected >= 2:
                        self.puzzles_selected -= 2
                    else:
                        self.puzzles_selected -= 2
                        self.active_menu = "main"
                        self.selected = len(self.actions) - 1
                elif key == curses.KEY_DOWN:
                    if self.puzzles_selected < (num_puzzles * 2 - 2):
                        self.puzzles_selected += 2
                    else:
                        self.puzzles_selected += 2
                        self.active_menu = "main"
                        self.selected = 0
                        
                elif key == curses.KEY_RIGHT:
                    if self.puzzles_selected % 2 == 0:
                        self.puzzles_selected += 1
                elif key == curses.KEY_LEFT:
                    if self.puzzles_selected % 2 == 1:
                        self.puzzles_selected -= 1

                elif key in [curses.KEY_ENTER, ord('\n'), ord(' ')]:
                    selected_puzzle_index = self.puzzles_selected // 2
                    selected_puzzle = self.puzzles[selected_puzzle_index]['name']
                    selected_action = 'R' if self.puzzles_selected % 2 == 0 else 'O'

                    if selected_action == 'R':
                        self.resetPuzzle(selected_puzzle_index)
                    elif selected_action == 'O':
                        self.overridePuzzle(selected_puzzle_index)

        curses.endwin()

    def draw_header(self, right_text):
        height, width = self.header_win.getmaxyx()
        game_name = "DRACULA"
        self.header_win.clear()
        self.header_win.addstr(0, 0, game_name)

        self.header_win.addstr(0, max(len(game_name) + 1, width - len(right_text) - 1), right_text)

        horizontal_rule = "-" * (width - 1)
        self.header_win.addstr(1, 0, horizontal_rule)

        self.header_win.refresh()

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
            combined_item = f"{puzzle['name']} [R][O]"
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
            if self.puzzles_selected // 2 == idx:
                before = combined_item[:puzzle_label_len + 1 + 3 * (self.puzzles_selected % 2)]
                highlight = combined_item[puzzle_label_len + 1 + 3 * (self.puzzles_selected % 2):puzzle_label_len + 4 + 3 * (self.puzzles_selected % 2)]
                after = combined_item[puzzle_label_len + 4 + 3 * (self.puzzles_selected % 2):]
                
                self.puzzles_menu_win.addstr(idx, 0, before, color_pair)
                if len(before) + len(highlight) < width:
                    self.puzzles_menu_win.addstr(idx, len(before), highlight, curses.A_REVERSE | color_pair)
                if len(before) + len(highlight) + len(after) < width:
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
            self.buttons.timer.kill_timer()
        elif cmd in ["win", "lose", "quit"]:
            self.buttons.timer.stop_timer()
        elif cmd == "exit":
            self.running = False

    def resetPuzzle(self, selected_puzzle_index):
        selected_puzzle = self.puzzles[selected_puzzle_index]['name']
        # self.log("Reset: " + selected_puzzle)
        self.mega.send_message(selected_puzzle_index, 2)
        pass

    def overridePuzzle(self, selected_puzzle_index):
        selected_puzzle = self.puzzles[selected_puzzle_index]['name']
        # self.log("Override: " + selected_puzzle)
        self.mega.send_message(selected_puzzle_index, 1)
        pass

    def log(self, message):
        self.log_messages.append(message)
        self.update_log()


    def stop(self):
        self.running = False