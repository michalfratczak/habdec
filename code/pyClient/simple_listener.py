#!/usr/bin/env python


import threading
import time
from ws4py.client.threadedclient import WebSocketClient # pip install ws4py


WS = None # websocket
EXIT = False


class HabDecClient(WebSocketClient):
	def received_message(self, m):
		print(m.data)
	def opened(self):
		print("Opened Connection")
	def closed(self, code, reason=None):
		print("Closed down", code, reason)


def RequestRttyStream():
	global WS
	while WS and not EXIT:
		WS.send("cmd::liveprint")
		WS.send("cmd::sentence")
		# WS.send("*") # or just send anything to be notified with cmd::info:sentence
		time.sleep(.5)


def main(i_srv = 'ws://127.0.0.1:5555'):
	global WS
	WS = HabDecClient( i_srv )
	WS.connect()

	threading.Thread( target = lambda: WS.run_forever() ).start()

	# you don't need to explicitly poll for data - server will push it to you
	# threading.Thread( target = RequestRttyStream ).start()

	# exit on Ctrl-C
	while 1:
		try:
			time.sleep(1)
		except KeyboardInterrupt:
			global EXIT
			EXIT = True
			WS.close()
			return


if __name__ == '__main__':
	main()
