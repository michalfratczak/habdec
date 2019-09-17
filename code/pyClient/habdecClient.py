#!/usr/bin/env python

# BASIC COMMUNICATION WITH HABDEC WEBSOCKET SERVER
#

import sys
import string
from ws4py.client.threadedclient import WebSocketClient # pip install ws4py
import struct
import numpy
import time
import math
import threading
import random
import tkinter as tk

byte_to_str = str
if sys.version_info[0] > 2:
	byte_to_str = lambda x: str(x, 'utf-8')


# power spectrum as received from server
class PowerSpectrum:
	header_size_ = 0		# int32_t
	noise_floor_ = 0		# float32
	noise_variance_ = 0		# float32
	sampling_rate_= 0 		# float32
	shift_ = 0				# float32
	peak_left_ = 0			# int32_t
	peak_right_ = 0			# int32_t
	peak_left_valid_ = 0	# int32_t
	peak_right_valid_ = 0	# int32_t
	min_ = 0				# float32
	max_ = 0				# float32
	type_size_ = 0,			# int32_t. value:s 1/2/4 for uint8 uint16 float32
	size_ = 0				# int32_t
	values_ = [] 			# raw data of type uint8|uint16|float32


# demodulated graph as received from server
class Demod:
	header_size_ = 0		# int32_t
	min_ = 0				# float32
	max_ = 0				# float32
	type_size_ = 0			# int32_t. value:s 1/2/4 for uint8 uint16 float32
	size_ = 0				# int32_t
	values_ = [] 			# raw data of type uint8|uint16|float32


# GLOBAL VARIABLES:
#
WS = None # websocket
WS_THREAD = None
DEMOD = Demod()
SPECTRUM = PowerSpectrum()
# decoder state
STATE = {
	'frequency': 0,
	'sampling_rate': 0,
	'gain': 0,
	'baud': 0,
	'rtty_bits': 0,
	'rtty_stops': 0,
	'lowpass_bw': 0,
	'lowpass_trans': 0,
	'biastee': 0,
	'decimation': 0,
	'afc': 0,
	'dc_remove': 0,
}
SENTENCES = []
RTTY_STREAM = ''


# WEBSOCKET CLIENT
#
class HabDecClient(WebSocketClient):
	set_callback = None
	info_callback = None

	def received_message(self, m):
		# python2: i_data is str
		# python3: i_data is bytes
		i_data = m.data

		if not i_data:
			return

		if i_data.startswith(b'PWR_'): # power spectrum graph data
			HandleResponse_PowerSpectrum(i_data[len('PWR_'):])
		elif i_data.startswith(b'DEM_'): # demodulated graph data
			HandleResponse_Demod(i_data[len('DEM_'):])
		elif i_data.startswith(b'cmd::set:'):
			HandleResponse_Set(i_data)
			if self.set_callback:
				self.set_callback()
		elif i_data.startswith(b'cmd::info:'):
			HandleResponse_Info(i_data)
			if self.info_callback:
				self.info_callback()

	def opened(self):
		print("Opened Connection")

	def closed(self, code, reason=None):
		print("Closed down", code, reason)


def InitConnection(i_srv, set_callback, info_callback):
	global WS
	global WS_THREAD
	WS = HabDecClient( i_srv )
	WS.set_callback = set_callback
	WS.info_callback = info_callback
	WS.connect()
	WS_THREAD = threading.Thread( target = lambda: WS.run_forever() )
	WS_THREAD.start()


def CloseConnection():
	print('Closing connection...')
	WS.close()
	print('Closed connection...')


def SendCommand(i_cmd):
	'''
	available commands:
		set:frequency=434		get:frequency
		set:gain=10				get:gain
		set:baud=50				get:baud
		set:rtty_bits=8			get:rtty_bits
		set:rtty_stops=2		get:rtty_stops
		set:lowpass_bw=1500		get:lowpass_bw
		set:lowpass_trans=.01	get:lowpass_trans
		set:biastee=1			get:biastee
		set:decimation=2		get:decimation
		set:afc=1				get:afc
		set:dc_remove=1			get:dc_remove
		set:payload=9f25cc048d401ebed112479a2ecef4f8
		power:res=512,zoom=0.5
		demod:res=256
		liveprint
		sentence
		stats

	For others, see websocket server implementation

	When sending a "set:' command, server echoes back a value that was actually set.
	Use it to synchronize your GUI
	'''
	if WS:
		WS.send( str('cmd::' + i_cmd) )


def HandleResponse_Set(i_cmd):
	global STATE
	if i_cmd.startswith(b'cmd::set:'):
		value = byte_to_str(i_cmd.split(b'=',1)[-1])
		if i_cmd.startswith(b'cmd::set:frequency'):
			STATE['frequency'] = float(value)
		elif i_cmd.startswith(b'cmd::set:gain'):
			STATE['gain'] = int(float(value))
		elif i_cmd.startswith(b'cmd::set:baud'):
			STATE['baud'] = float(value)
		elif i_cmd.startswith(b'cmd::set:rtty_bits'):
			STATE['rtty_bits'] = int(float(value))
		elif i_cmd.startswith(b'cmd::set:rtty_stops'):
			STATE['rtty_stops'] = int(float(value))
		elif i_cmd.startswith(b'cmd::set:lowpass_bw'):
			STATE['lowpass_bw'] = float(value)
		elif i_cmd.startswith(b'cmd::set:lowpass_trans'):
			STATE['lowpass_trans'] = float(value)
		elif i_cmd.startswith(b'cmd::set:biastee'):
			STATE['biastee'] = bool( float(value) )
		elif i_cmd.startswith(b'cmd::set:decimation'):
			STATE['decimation'] = int(float(value))
		elif i_cmd.startswith(b'cmd::set:afc'):
			STATE['afc'] = bool( float(value) )
		elif i_cmd.startswith(b'cmd::set:dc_remove'):
			STATE['dc_remove'] = bool( float(value) )


def HandleResponse_Info(i_cmd):
	global STATE
	if i_cmd.startswith(b'cmd::info:'):
		value = byte_to_str(i_cmd.split(b'=',1)[-1])
		if i_cmd.startswith(b'cmd::info:sampling_rate'):
			STATE['sampling_rate'] = float(value)
		if i_cmd.startswith(b'cmd::info:liveprint'):
			global RTTY_STREAM
			RTTY_STREAM += str(value)
		if i_cmd.startswith(b'cmd::info:sentence'):
			SENTENCES.append(value)
			SendCommand("stats")
		if i_cmd.startswith(b'cmd::info:stats'):
			print(value)


def HandleResponse_PowerSpectrum(_data):
	global SPECTRUM
	offset = 0
	SPECTRUM.header_size_ = 	struct.unpack('i', _data[offset:offset+4])[0]; 	offset += 4;
	SPECTRUM.noise_floor_ = 	struct.unpack('f', _data[offset:offset+4])[0]; 	offset += 4;
	SPECTRUM.noise_variance_ =	struct.unpack('f', _data[offset:offset+4])[0]; 	offset += 4;
	SPECTRUM.sampling_rate_=	struct.unpack('f', _data[offset:offset+4])[0]; 	offset += 4;
	SPECTRUM.shift_  =			struct.unpack('f', _data[offset:offset+4])[0]; 	offset += 4;
	SPECTRUM.peak_left_  = 		struct.unpack('i', _data[offset:offset+4])[0]; 	offset += 4;
	SPECTRUM.peak_right_  = 	struct.unpack('i', _data[offset:offset+4])[0]; 	offset += 4;
	SPECTRUM.peak_left_valid_=	struct.unpack('i', _data[offset:offset+4])[0]; 	offset += 4;
	SPECTRUM.peak_right_valid_=	struct.unpack('i', _data[offset:offset+4])[0]; 	offset += 4;
	SPECTRUM.min_  = 			struct.unpack('f', _data[offset:offset+4])[0]; 	offset += 4;
	SPECTRUM.max_  = 			struct.unpack('f', _data[offset:offset+4])[0]; 	offset += 4;
	SPECTRUM.type_size_ =		struct.unpack('i', _data[offset:offset+4])[0]; 	offset += 4;
	SPECTRUM.size_ = 			struct.unpack('i', _data[offset:offset+4])[0]; 	offset += 4;

	if(SPECTRUM.type_size_ == 1): # unsigned 8 bit char
		SPECTRUM.values_ = numpy.fromstring( _data[offset:], dtype=numpy.ubyte ) # unsigned char
	elif(SPECTRUM.type_size_ == 2): # uint16_t
		SPECTRUM.values_ = numpy.fromstring( _data[offset:], dtype=numpy.ushort ) # uint16_t
	elif(SPECTRUM.type_size_ == 4): # 32float
		SPECTRUM.values_ = numpy.fromstring( _data[offset:], dtype=numpy.float32 )


def HandleResponse_Demod(_data):
	global DEMOD
	offset = 0
	DEMOD.header_size_ = struct.unpack('i', _data[offset:offset+4])[0]; 	offset += 4;
	DEMOD.min_ = 		 struct.unpack('f', _data[offset:offset+4])[0]; 	offset += 4;
	DEMOD.max_ = 		 struct.unpack('f', _data[offset:offset+4])[0]; 	offset += 4;
	DEMOD.type_size_ =	 struct.unpack('i', _data[offset:offset+4])[0]; 	offset += 4;
	DEMOD.size_ = 		 struct.unpack('i', _data[offset:offset+4])[0]; 	offset += 4;

	if(DEMOD.type_size_ == 1): # unsigned 8 bit char
		DEMOD.values_ = numpy.fromstring( _data[offset:], dtype=numpy.ubyte ) # unsigned char
	elif(DEMOD.type_size_ == 2): # uint16_t
		DEMOD.values_ = numpy.fromstring( _data[offset:], dtype=numpy.ushort ) # uint16_t
	elif(DEMOD.type_size_ == 4): # 32float
		DEMOD.values_ = numpy.fromstring( _data[offset:], dtype=numpy.float32 )


def UpdateState():
	for _key in STATE:
		SendCommand('get:' + _key)


def DecodeValues(i_data):
	'''
	convert values from 8/16/32 bits to 32 float
	'''
	true_values = [None] * len(i_data.values_)
	for i in range(len(i_data.values_)):
		value_encoded = i_data.values_[i]
		true_value = 0
		if(i_data.type_size_ == 1): # 8 bit char
			true_value = i_data.min_ + (float(value_encoded) / 255) * (i_data.max_ - i_data.min_)
		elif(i_data.type_size_ == 2): # uint16_t
			true_value = i_data.min_ + (float(value_encoded) / 65535) * (i_data.max_ - i_data.min_)
		elif(i_data.type_size_ == 4): # 32float
			true_value = value_encoded
		true_values[i] = true_value
	return true_values
