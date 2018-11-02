#!/usr/bin/env python

import string
import os
import sys
import time
import datetime
import argparse
import json
import re
import traceback
from base64 import b64encode
import httplib
from hashlib import sha256


C_BLACK = 		"\033[1;30m"
C_RED = 		"\033[1;31m"
C_GREEN = 		"\033[1;32m"
C_BROWN = 		"\033[1;33m"
C_BLUE = 		"\033[1;34m"
C_MAGENTA = 	"\033[1;35m"
C_CYAN = 		"\033[1;36m"
C_LIGHTGREY = 	"\033[1;37m"
C_OFF =   		"\033[0m"

# logname = time.strftime("%Y-%m-%d_%H.%M.%S", time.gmtime()) + ".txt"
logname = time.strftime("%Y-%m-%d_%H", time.gmtime()) + ".txt"
with open('./' + logname, 'a') as f:
	f.write(logname + "\n")


def prog_opts():
	parser = argparse.ArgumentParser(description='arg parser')
	parser.add_argument('--addr', 	dest='port', 	action='store', help='port')
	parser.add_argument('--station',dest='station', action='store', help='listener callsign')
	args = parser.parse_args()
	return args


def get_config():
	opts = prog_opts()
	CFG = {}

	CFG['addr'] = ""
	CFG['station'] = ""

	if opts.port:		CFG['addr'] = opts.port
	if opts.station:	CFG['station'] = opts.station
	return CFG


def crc(i_str):
	def _hex(Character):
		_hexTable = '0123456789ABCDEF'
		return _hexTable[Character]

	CRC = 0xffff
	xPolynomial = 0x1021

	for i in xrange(len(i_str)):
		CRC ^= (ord(i_str[i]) << 8)
		for j in xrange(8):
			if CRC & 0x8000:
				CRC = (CRC << 1) ^ 0x1021
			else:
				CRC <<= 1

	result = ''
	result += _hex((CRC >> 12) & 15)
	result += _hex((CRC >> 8) & 15)
	result += _hex((CRC >> 4) & 15)
	result += _hex(CRC & 15)

	return result


def AppendToLogFile(msg):
	with open('./' + logname, 'a') as f:
		msg = string.strip(msg)
		f.write(msg + '\n\r')


def sendSentence(sntc, listener_callsign):
	# print ("PYTHON ", sntc, listener_callsign)
	# return

	sentence = sntc

	if sentence[-1] != '\n':		sentence += '\n'
	if sentence[0] != '$':			sentence = '$' + sentence
	if sentence[1] != '$':			sentence = '$' + sentence

	print "PY SENTENCE ", sentence

	sentence = b64encode(sentence)

	retries = 5

	while retries:
		date = datetime.datetime.utcnow().isoformat("T") + "Z"

		data = {
			"type": "payload_telemetry",
			"data": {
				"_raw": sentence
				},
			"receivers": {
				listener_callsign: {
					"time_created": date,
					"time_uploaded": date,
					},
				},
		}

		c = httplib.HTTPConnection("habitat.habhub.org")
		c.request(
			"PUT",
			"/habitat/_design/payload_telemetry/_update/add_listener/%s" % sha256(sentence).hexdigest(),
			json.dumps(data),  # BODY
			{"Content-Type": "application/json"}  # HEADERS
			)

		response = c.getresponse()
		print response.status, response.reason, response.read()

		if response.status == 201 or response.status == '201':
			retries == 0
			break

		retries = retries - 1
		time.sleep(1)


def main():
	caller = "habdec"

	if len(sys.argv) > 2:
		caller = sys.argv[2]

	if len(sys.argv) > 1:
		sendSentence(sys.argv[1], caller)

if __name__ == '__main__':
	try:
		main()
	except:
		print traceback.format_exc()

