import serial
import time
import os
import serial.tools.list_ports
import threading


class ArduinoComm:

    SAVED_PORT_FILE = "saved_port.txt"

    def __init__(self, port=None, baudrate=9600, timeout=1):
        if not port:
            port = self._get_saved_port() or self._prompt_for_port()
            self._save_port(port)
        self.ser = serial.Serial(port, baudrate, timeout=timeout)
        time.sleep(2)  # Wait for serial connection to initialize
        self.running = False
        self.thread = threading.Thread(target=self._command_loop)

    @staticmethod
    def _list_available_ports():
        ports = list(serial.tools.list_ports.comports())
        return [port.device for port in ports if port.device]

    @classmethod
    def _get_saved_port(cls):
        if os.path.exists(cls.SAVED_PORT_FILE):
            with open(cls.SAVED_PORT_FILE, "r") as file:
                return file.readline().strip()
        return None

    @classmethod
    def _save_port(cls, port):
        with open(cls.SAVED_PORT_FILE, "w") as file:
            file.write(port)

    def _prompt_for_port(self):
        print("Available ports:")
        ports = self._list_available_ports()
        for i, port in enumerate(ports, 1):
            print(f"{i}. {port}")
        choice = int(input("Select a port (Enter the number): "))
        return ports[choice-1]

    def send_message(self, message_type, hex_byte_value, retries=3):
        message = "<{}{}>".format(message_type, hex_byte_value)
        for _ in range(retries):
            self.ser.write(message.encode())
            if self._await_ack():
                return True
        return False

    def _await_ack(self, timeout=1):
        start_time = time.time()
        while (time.time() - start_time) < timeout:
            if self.ser.in_waiting:
                data = self.ser.readline().decode().strip()
                if data == "<ACK>":
                    return True
        return False

    def receive_message(self):
        data = self.ser.readline().decode().strip()
        if not data:
            return None
        
        # If data starts and ends with delimiters, parse it
        if data.startswith("<") and data.endswith(">"):
            message = data[1:-1]
            if message[0] == "R":  # Check if the message type is "Random"
                random_value = int(message[1:])
                self.handle_random_value(random_value)
            return message
        else:  # Otherwise, consider it as debug message and print it
            print(f"[DEBUG from Arduino]: {data}")
            return None
        
    def handle_random_value(self, value):
        # Implement desired behavior in response to the received integer here.
        # For now, let's just print it to the console.
        print(f"Received random value from Arduino: {value}")


    def close(self):
        self.ser.close()

    def _command_loop(self):
        try:
            while self.running:
                if self.ser.in_waiting:
                    self.receive_message()  # this will automatically print out debug messages
                else:
                    command = input("Enter 'toggle' to toggle LED or 'exit' to quit: ").strip().lower()
                    if command == "toggle":
                        if self.send_message("M", "TOGGLE"):
                            print("LED toggled!")
                        else:
                            print("Failed to send toggle command.")
                    elif command == "exit":
                        self.stop()
        finally:
            self.close()

    def start(self):
        self.running = True
        self.thread.start()

    def stop(self):
        self.running = False
        if self.thread.is_alive():
            self.thread.join()


arduino = ArduinoComm()
arduino.start()





