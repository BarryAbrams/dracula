# Library for controlling the Adafruit 7-segment display that looks like this
# :88:8'8
# Requires the Adafruit HT16K33 library

import HT16K33

FALLBACK_CHAR = 0b01010011 # '?'
DECIMAL = 1 << 7 # Which bit is used for the decimal point

def constrain(val, lo, hi):
	return max(lo, min(val, hi))

class SevenSeg(HT16K33.HT16K33):
	
	chars = {
		# Bits go in the order: xx cc nw sw ss se ne nn
		' ' : 0b00000000 ,
		'.' : 0b00001000 ,
		'-' : 0b01000000 ,
		"'" : 0b00000010 ,
		'0' : 0b00111111 ,
		'1' : 0b00000110 ,
		'2' : 0b01011011 ,
		'3' : 0b01001111 ,
		'4' : 0b01100110 ,
		'5' : 0b01101101 ,
		'6' : 0b01111101 ,
		'7' : 0b00000111 ,
		'8' : 0b01111111 ,
		'9' : 0b01101111 ,
		'L' : 0b00111000 ,
		'C' : 0b00111001 ,
		'?' : 0b01010011 ,
		'c' : 0b01011000 ,
		'b' : 0b01111100 ,
		'i' : 0b00000100 ,
		'n' : 0b01010100 ,
		'd' : 0b01011110 ,
		'P' : 0b01110011 ,
		'K' : 0b01110101 ,
		'S' : 0b01101101 ,
		'E' : 0b01111001 ,
		'J' : 0b00011110 ,
		'Z' : 0b01011011 ,
		'I' : 0b00000110 ,
		't' : 0b01111000 ,
		'A' : 0b01110111 ,
		'g' : 0b01101111 ,
		'V' : 0b00111110 ,
		'H' : 0b01110110 ,
		'h' : 0b01110100 ,
		'r' : 0b01010000 ,
		'i' : 0b00010000 ,
		'U' : 0b00111110 ,
		# Bits go in the order: xx cc nw sw ss se ne nn
		# Add more here as necessary!
	}
	
	def __init__(self, **kwargs):
		super(SevenSeg, self).__init__(**kwargs)
	
	@classmethod
	def char(cls, c):
		return cls.chars.get(c, FALLBACK_CHAR)
	
	def set(self, pos, bits):
		pos = constrain(pos, 0, 3)
		if pos >= 2: pos += 1 # Offset because position 2 is special
		pos *= 2
		bits &= 0x7F # Chop off the highest bit (the decimal bit)
	#	self.buffer[pos] = bits | (self.buffer[pos] & DECIMAL) # Use this version to make decimal points persist
		self.buffer[pos] = bits
	
	def set2(self, bits): # Set the special position 2, used for any lights that aren't the digits themselves
		self.buffer[4] = bits & 0xFF
	
	def set3(self, pos, on):
		pos = constrain(pos, 0, 3)
		if pos >= 2: pos += 1
		pos *= 2
		if on:
			self.buffer[pos] |= DECIMAL
		else:
			self.buffer[pos] &= ~DECIMAL
	
	def set_string(self, string):
		string = string[:4] # Only four positions on the display
		for pos, c in enumerate(string):
			self.set(pos, self.char(c))
	
	def set_string_with_decimal(self, string, fillchar=' '): # Used when you want to pass a string containing decimal points
		i = 0 # Position in the string
		j = 0 # Position in the display
		string += fillchar*4
		if string.startswith('.'): string = fillchar+string # Can't start with a decimal point
		while j < 4:
			self.set(j, self.char(string[i]))
			i += 1
			j += 1
			if string[i] == '.':
				self.set3(j-1, True)
				i += 1
	
	def set_punct(self, punct): # This sets the punctuation, and also sets the decimals as a fallback
		val = 0
		punct += '   ' # Make sure ranges are good
		if punct[1] == ':': # Bit 2: second colon
			val |= 0x02
		if punct[0] == ':' or punct[0] == "'": # Bit 3: half of the first colon
			val |= 0x04
			self.set3(0, True)
		if punct[0] == ':' or punct[0] == '.': # Bit 4: half of the first colon
			val |= 0x08
			self.set3(3, True)
		if punct[2] == "'": # Bit 5: the fixed decimal point on top
			val |= 0x10
			self.set3(2, True)
		self.set2(val)
	
	def set_decimals(self, decimals):
		decimals += '    '
		for i, char in enumerate(decimals[:4]):
			self.set3(i, char!=' ')
	
	def write(self, string='    ', punct='   ', decimals='    '):
		self.set_string(string)
		self.set_punct(punct)
		self.set_decimals(decimals)
		self.write_display()