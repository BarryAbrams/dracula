import pathlib

def find_port(pattern, source='/dev/'):
	while True:
		options = list(pathlib.Path(source).glob(pattern))
		print(options)
		if options: break
		print(f'ERROR: could not find any device matching the pattern "{pattern}". Make sure all the USB cables are plugged in, then press ENTER to continue.')
		input()
	if len(options) > 1:
		print(f'WARNING: multiple ports found! The pattern "{pattern}" could match any of {", ".join(str(o) for o in options)}. Going with the first one, {str(options[0])}.')
	return str(options[0])
