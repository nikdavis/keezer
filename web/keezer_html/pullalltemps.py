#pulltemps.py

import json, time
import MySQLdb as mdb
def index(req):

	req.add_common_vars()
	env_vars = req.subprocess_env
	getReqStr = env_vars['QUERY_STRING']    #the url after the ?
	getReqArr = getReqStr.split('&')        #split to array of k-v pairs
	getReqDict = {}
	for item in getReqArr:                  #loop thru array, fill dictionary
	   tempArr = item.split('=')            #with keys and get values
	   getReqDict[tempArr[0]] = tempArr[1]

	limit = getReqDict['limit']

	db = mdb.connect("localhost", "root", "sunshine", "keezer_temp")
	cur = db.cursor()
	cur.execute("select * from readings order by timestamp desc limit " + limit + ";")
	rows = cur.fetchall()
	return json.dumps(rows)
