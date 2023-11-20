import pathlib
import serial.tools.list_ports
import sys

def find_port(product_id):
    ports = list(serial.tools.list_ports.comports())
    for port in ports:
        if port.pid == product_id:
            return port.device
    return None

