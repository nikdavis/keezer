#test.py

import json, csv, os.path

def index(req):
	temp_file = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'data.csv'), "rb")

	temp_csv = csv.reader(temp_file, delimiter=",");

	temp_output = []
	for line in temp_csv:
		line.reverse()
		temp_output.append( line )

	output = json.dumps(temp_output)
	
	return output
