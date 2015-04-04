import serial, json, random, time, sys
import MySQLdb as mdb

ard = serial.Serial('/dev/ttyUSB0', 9600)
db = mdb.connect("localhost", "root", "sunshine", "keezer_temp")
cur = db.cursor()

while 1:
	line = ard.readline()
	print line
	#line = str(temp) + "," + str(setting) + ",\"Off\"" #\r\n"	
	line = line.rstrip('\r\n ').split(',')
	line[2] = "'" + line[2] + "'"
	line = ','.join(line)
	query = "insert into readings (target, temp, relayState, timestamp) values ("
	query += line + "," + str(time.time()) + ");"
	print query
	cur.execute(query)
	db.commit()
	time.sleep(1)

