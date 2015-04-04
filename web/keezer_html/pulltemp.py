#pulltemps.py

import json, time
import MySQLdb as mdb
def index(req):
	db = mdb.connect("localhost", "root", "sunshine", "keezer_temp")
	cur = db.cursor()
	cur.execute("select * from readings order by timestamp desc limit 1800;")
	rows = cur.fetchall()
	return json.dumps(rows)
