import serial
ard = serial.Serial('/dev/ttyUSB0', 9600)

while 1:
	line = ard.readline()
	print line

