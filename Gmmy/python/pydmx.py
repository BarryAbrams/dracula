import serial

OPEN = bytes.fromhex('7e') # Magic number
CLOSE = bytes.fromhex('e7') # Magic number
WRITE = bytes.fromhex('06 01 02') # Packet type $06 (DMX send), length $0201 (513 bytes)
INIT1 = bytes.fromhex('03 02 00 00 00') # Packet type $03 (get config…?), length $0002, all zeroes - why?
INIT2 = bytes.fromhex('10 02 00 00 00') # Packet type $10 (unknown), length $0002, all zeroes - cannot find any documentation on what packet type $10 should do? Why is this here?

debug = False

def constrain(x, low, high): return min(high, max(low, x)) # Simple wrapper to keep a value between a min and a max inclusive

class DMX:
	def __init__(self, port):
		if port:
			self.no_controller = False
			if debug: print('Opening serial port', port)
			self.serial = serial.Serial(port, baudrate=57600)
			self.serial.write(OPEN + INIT1 + CLOSE)
			self.serial.write(OPEN + INIT2 + CLOSE)
			self.data = bytearray(513) # A DMX packet is 513 bytes; the first one is $00 for standard use, the other 512 are the channels. Note, this means channels are 1-indexed!
			self.data[0] = 0
			if debug: print('Ready')
			
		else:
			self.no_controller = True

	
	def set(self, chan, val):
		if not self.no_controller:
			chan = constrain(chan, 1, 512)
			val = constrain(int(val), 0, 255)
			self.data[chan] = val
		
	def blackout(self):
		for i in range(1, 513):
			self.data[i] = 0
	
	def render(self):
		if not self.no_controller:
			self.serial.write(OPEN + WRITE + self.data + CLOSE)
