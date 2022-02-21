
import datetime
import requests
import sqlite3
import re
import os.path
import os

DB_NAME = 'states.db'

def getStateLabels():
	gh_token = os.environ['GITHUB_TOKEN']
	r = requests.get("https://api.github.com/repos/jpd002/Play-compatibility/labels", headers={'Authorization': f'token {gh_token}'})

	assert(r.status_code == 200)
	assert(r.headers['content-type'] == "application/json; charset=utf-8")

	res = []
	for entry in r.json():
		d = {}
		d["name"] = entry["name"]
		d["color"] = entry["color"]
		res.append(d)
	return res

def requestGamesStateUpdates(url, res):
	gh_token = os.environ['GITHUB_TOKEN']
	r = requests.get(url, headers={'Authorization': f'token {gh_token}'})

	assert(r.status_code == 200)
	assert(r.headers['content-type'] == "application/json; charset=utf-8")

	if(not r.json()):
		return False

	pattern = re.compile("\w{4}-\d{5}")
	for entry in r.json():
		d = {}
		d["discId"] = entry["title"].split(" ")[0][1:-1]
		d["labels"] = [state["name"] for state in entry["labels"]]
		if(pattern.search(d["discId"])):
			res.append(d)
		else:
			print(f"invalid entry: {entry['title']}")
	return True

def getLatestGamesStateUpdates():
	URL = "https://api.github.com/repos/jpd002/Play-compatibility/issues?state=open&per_page=100&since={}&page={}"

	today = datetime.date.today()
	yesterday = today - datetime.timedelta(days=2)

	i = 1
	res = []
	hasNext = True
	while(hasNext):
		hasNext = requestGamesStateUpdates(URL.format(yesterday.strftime('%Y-%m-%d'), i), res)
		i += 1

	return res

def getAllGamesStateUpdates():
	URL = "https://api.github.com/repos/jpd002/Play-compatibility/issues?state=open&per_page=100&page={}"

	i = 1
	res = []
	hasNext = True
	while(hasNext):
		hasNext = requestGamesStateUpdates(URL.format(i), res)
		i += 1

	return res

def setupDatabase():
	con = sqlite3.connect(DB_NAME)

	cur = con.cursor()
	cur.execute('''CREATE TABLE IF NOT EXISTS games(id INTEGER PRIMARY KEY, discId TEXT, state TEXT, UNIQUE(state, discId))''')
	cur.execute('''CREATE TABLE IF NOT EXISTS labels(id INTEGER PRIMARY KEY, name TEXT, color TEXT, UNIQUE(name))''')
	con.commit()
	return con

def updateDatabase(con, games, labels):
	con.execute('BEGIN')
	cur = con.cursor()

	cur.execute("DELETE FROM labels;")
	for label in labels:
			cur.execute("INSERT OR IGNORE INTO labels(name, color) values(?, ?);", (label['name'], label['color'],))

	for game in games:
		cur.execute("DELETE FROM games WHERE discId=?;", (game['discId'],))
		for label in game['labels']:
			cur.execute("INSERT OR IGNORE INTO games(discId, state) values(?, ?);", (game['discId'], label,))
	con.commit()

if __name__ == '__main__':
	if(os.path.isfile(DB_NAME)):
		print("DB Found, Partial Update")
		games = getLatestGamesStateUpdates()
	else:
		print("DB Not Found, Full Update")
		games = getAllGamesStateUpdates()

	print(f"Updating {len(games)} Game Entries")
	labels = getStateLabels()
	con = setupDatabase()
	updateDatabase(con, games, labels)
	con.close()
