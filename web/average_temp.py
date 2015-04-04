import json, urllib2, numpy

# NUMBER OF DATA POINTS
LIMIT = 720
url = "http://50.53.105.161/keezer/pullalltemps.py?limit="


data = json.load(urllib2.urlopen(url + str(LIMIT)))
data.reverse()


state = "Off"
start_on = 0.0
time_on = 0.0
ontime = 0.0
cycles = 0.0
hours = 0.0
min = 0.0
x = []
y = []
for item in data:
	if state == "Off" and item[3] == "On":
		cycles += 1
		start_on = item[2]
	if state == "On" and item[3] == "Off":
		time_on += item[2] - start_on
	state = item[3]
	x.append(item[2])
	y.append(item[0])
	#count on/off cycles
	
# If we never shut off then add the time
# we have still been currently on
if data[len(data)-1][3] == "On":
	time_on += data[len(data)-1][2] - start_on
	
	

duration = float( x[len(x)-1] - x[0] )

integral = numpy.trapz(y, x)

avg = integral / duration

hours = int( duration / 3600.0 )
min = int( ( ( duration / 3600.0 ) - hours ) * 60)

avg_cycle = duration / cycles
cyc_hours = int( avg_cycle / 3600.0 )
cyc_min = int( ( ( avg_cycle / 3600.0 ) - cyc_hours ) * 60)


on_hours = int( time_on / 3600.0 )
on_min = int( ( ( time_on / 3600.0 ) - on_hours) * 60)

print "\n\n"
print "DATA SPANS " + str(hours) + " HRS, " + str(min) + " MIN"
print "AVERAGE TEMP: %.2f" % avg
print "CYCLED ON " + str(cycles) + " TIMES\nEVERY " + str(cyc_hours) + " HRS, " + str(cyc_min) + " MIN"
print "ON OVERALL " + str(on_hours) + " HRS, " + str(on_min) + " MIN"
print "AVERAGE ON-TIME: %.1f" % (time_on / cycles / 60) + " MIN"
print "%.1f" % (float(time_on) / duration * 100) + "% DUTY CYCLE"
print "\n\n"