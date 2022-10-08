import os, sys

from flask import Flask, render_template, json
from flask_sock import Sock

from datetime import datetime
import random
import socket

FAKE_SSIDS = [
"echoAP",
"wonka_land",
"BT-XMG8",
"sophieworld",
"lost_monkey",
"WhOaTeAlLtHeChoc",
"Sky-1234_FG",
"TheBadgersDen",
"DottisKennel"
]

index_html = os.environ.get('WORDCLOCK_INDEX_HTML', os.environ['PWD']+"/html/index.html")
index_html = os.path.abspath(index_html)
index_html_dir  = os.path.dirname(index_html)
index_html_file = os.path.basename(index_html)
print(f" * Serving : {index_html}")

app = Flask(__name__,template_folder=index_html_dir)
sock = Sock(app)

STATE={}
STATE['tz'] = 'Europe/London'
STATE['tz_enc']  = None
STATE['time']    = None
STATE['ssid']    = 'helloworldAP'
STATE['conn']    = 'Connected'
STATE['ip']      = None

def updateState():
    STATE['time'] = datetime.now().strftime("%H:%M:%S")
    STATE['ip']   = str(socket.gethostbyname(socket.gethostname()))

def sendStatus(ws):
    updateState()
    ws.send(json.dumps({'type':'stat', 'payld':STATE}))

@app.route('/')
def index():
    return render_template(index_html_file)

@sock.route('/ws')
def ws(ws):
    while True:
        data = ws.receive(timeout=1)
        if data is not None:
            req = json.loads(data)
            if req.get('type',None) == 'scan':
                ws.send(json.dumps({'type': 'scan', 'payld': list(set(random.choices(FAKE_SSIDS,k=5)))}))
            elif req.get('type',None) == 'update':
                if req['payld'].get('type',None) == 'tz':
                    STATE['tz'] = req['payld']['values'][0]
                    STATE['tz_enc'] = req['payld']['values'][1]
                elif req['payld'].get('type',None) == 'wifi':
                    STATE['ssid'] = req['payld']['values'][0]
                    STATE['passkey'] = req['payld']['values'][1]
        sendStatus(ws)

