#!/usr/bin/env python

import sys
import string
import traceback
from functools import partial
import tkinter as tk

import habdecClient as hdc

# rolling avg classs
class Average:
	def __init__(self,max_count, init_val=0):
		self.max_count_ = max_count
		self.count_ = 0
		self.sum_ = 0
		self.add(init_val)

	def get_avg(self):
		if self.count_ == 0:
			return self.sum_
		return float(self.sum_)/self.count_

	def add(self, val):
		if self.count_ == self.max_count_:
			self.sum_ = self.get_avg() * (self.max_count_-1) + val
		else:
			self.count_ += 1
			self.sum_ += val

	def reset(self, val):
		self.sum_ = val
		self.count_ = 1


# GLOBAL VARIABLES:
#
WS = None # websocket

AccSpectrumArray = { # accumulated power spectrum values - used for smoothing out spectrum display
	'arr_': [],
	'count_': 0,
	'maxCount_': 3 # bigger values --> smoother but slower display
}
AvgCnt = 10 # Rolling average length. Smooths out spectrum display
NoiseFloorAvg = Average(AvgCnt)
SpectrumMinAvg = Average(AvgCnt)
SpectrumMaxAvg = Average(AvgCnt)


# gui
GUI = tk.Tk() # tkinter
SPECTRUM_CANVAS = None
SPECTRUM_CANVAS_RES_X = 512
SPECTRUM_CANVAS_RES_Y = 128
SPECTRUM_CANVAS_BG = '#002244'
ZOOM = 0
DEMOD_CANVAS = None
DEMOD_CANVAS_RES_X = 512
DEMOD_CANVAS_RES_Y = 64
DEMOD_CANVAS_BG = '#002244'
CONTROLS = {}
RTTY_STREAM_WIDGET = None
RTTY_LAST_SENTENCE = None


def AccumulateSpectrumArray(i_arr):
	if len(AccSpectrumArray['arr_']) != len(i_arr):
		AccSpectrumArray['arr_'] = i_arr
		AccSpectrumArray['count_'] = 1
		return AccSpectrumArray['arr_']

	if AccSpectrumArray['count_'] < AccSpectrumArray['maxCount_']:
		for i in range(len(AccSpectrumArray['arr_'])):
			AccSpectrumArray['arr_'][i] += i_arr[i]
		AccSpectrumArray['count_'] += 1
	else:
		for i in range(len(AccSpectrumArray['arr_'])):
			AccSpectrumArray['arr_'][i] = (AccSpectrumArray['arr_'][i] / AccSpectrumArray['count_']) * (AccSpectrumArray['maxCount_']-1) + i_arr[i]

	# return value
	spectr_result = [None] * len(AccSpectrumArray['arr_'])
	for i in range(len(AccSpectrumArray['arr_'])):
		spectr_result[i] = AccSpectrumArray['arr_'][i] / AccSpectrumArray['count_']
	return spectr_result


def RedrawSpectrum():
	try:
		# hdc.SendCommand( bytearray("power:res=" + str(SPECTRUM_CANVAS_RES_X) + ",zoom=" +  str(ZOOM), 'utf-8') )
		hdc.SendCommand( "power:res=" + str(SPECTRUM_CANVAS_RES_X) + ",zoom=" +  str(ZOOM) )

		if not len(hdc.SPECTRUM.values_):
			GUI.after(500, RedrawSpectrum)
			return

		global SPECTRUM_CANVAS

		SPECTRUM_CANVAS.delete('all')

		zoom = ZOOM # [0-1) values

		# LOWPASS FILTER DRAW
		#
		_lowpass_bw_relative = hdc.STATE['lowpass_bw'] / hdc.SPECTRUM.sampling_rate_ / (1.0 - .999*zoom)
		_lowpass_trans = hdc.STATE['lowpass_trans'] / (1.0 - .999*zoom)
		_l  = max(0, .5 - .5 * _lowpass_bw_relative)
		_r  = min(1, .5 + .5 * _lowpass_bw_relative)
		SPECTRUM_CANVAS.create_rectangle(
						_l*SPECTRUM_CANVAS_RES_X, 0,
						_r*SPECTRUM_CANVAS_RES_X, SPECTRUM_CANVAS_RES_Y-1,
						fill='#3333aa' )

		# POWER SPECTRUM
		#
		spectrum_values_ = hdc.DecodeValues(hdc.SPECTRUM)

		# resize vector to SPECTRUM_CANVAS_RES_X
		spectrum_values_resX = [None] * SPECTRUM_CANVAS_RES_X
		for x in range(SPECTRUM_CANVAS_RES_X):
			x_0_1 = float(x)/(SPECTRUM_CANVAS_RES_X-1)
			spectrum_values_resX[x] = spectrum_values_[ int( round(x_0_1 * (len(spectrum_values_)-1)) ) ]

		# border values are NaN. needs fixing
		#spectrum_values_resX[0] = spectrum_values_resX[1]
		#spectrum_values_resX[-1] = spectrum_values_resX[-2]

		# accumulate spectrum and noisefloor for nice rendering
		spectrum_values_resX = AccumulateSpectrumArray(spectrum_values_resX)
		NoiseFloorAvg.add(hdc.SPECTRUM.noise_floor_)
		SpectrumMinAvg.add(hdc.SPECTRUM.min_)
		SpectrumMaxAvg.add(hdc.SPECTRUM.max_)

		noise_floor_avg = NoiseFloorAvg.get_avg()
		spectrum_min_avg = SpectrumMinAvg.get_avg()
		spectrum_max_avg = SpectrumMaxAvg.get_avg()

		# draw
		for x in range(SPECTRUM_CANVAS_RES_X):
			val_0_1 = 1.0 - abs(spectrum_values_resX[x] / spectrum_min_avg)
			val_0_1 = max(val_0_1, 0)
			val_pixel = (1.0-val_0_1) * (SPECTRUM_CANVAS_RES_Y-1)
			val_pixel = int(round(val_pixel))
			SPECTRUM_CANVAS.create_line(x, SPECTRUM_CANVAS_RES_Y-1, x, val_pixel, fill='#CC0000')

		# CENTER LINE
		#
		SPECTRUM_CANVAS.create_line(SPECTRUM_CANVAS_RES_X/2, SPECTRUM_CANVAS_RES_Y-1, SPECTRUM_CANVAS_RES_X/2, 0, fill='#888888')

		# NOISE FLOOR
		#
		nf_0_1 = 1.0 - abs(noise_floor_avg / spectrum_min_avg)
		nf_0_1 = max(nf_0_1, 0)
		SPECTRUM_CANVAS.create_line( 0, (1.0-nf_0_1)*SPECTRUM_CANVAS_RES_Y - 1,
									 SPECTRUM_CANVAS_RES_X-1, (1.0-nf_0_1)*SPECTRUM_CANVAS_RES_Y - 1,
									 fill='#111111' )

		# PEAK LEFT
		#
		colour = '#FF2200'
		if(hdc.SPECTRUM.peak_left_valid_):
			colour = '#FF2200'
		else:
			colour = '#113322'
		peak_left_0_1 = float(hdc.SPECTRUM.peak_left_) / hdc.SPECTRUM.size_
		SPECTRUM_CANVAS.create_line( peak_left_0_1 * SPECTRUM_CANVAS_RES_X, 0,
									peak_left_0_1 * SPECTRUM_CANVAS_RES_X, SPECTRUM_CANVAS_RES_Y - 1,
									fill=colour )

		# PEAK RIGHT
		#
		if(hdc.SPECTRUM.peak_right_valid_):
			colour = '#0088FF'
		else:
			colour = '#113322'
		peak_right_0_1 = float(hdc.SPECTRUM.peak_right_) / hdc.SPECTRUM.size_
		SPECTRUM_CANVAS.create_line( peak_right_0_1 * SPECTRUM_CANVAS_RES_X, 0,
									peak_right_0_1 * SPECTRUM_CANVAS_RES_X, SPECTRUM_CANVAS_RES_Y - 1,
									fill=colour )
	except:
		print(traceback.format_exc())

	GUI.after(50, RedrawSpectrum)


def RedrawDemod():
	try:
		# hdc.SendCommand( bytearray("demod:res=" + str(DEMOD_CANVAS_RES_X), 'utf-8') )
		hdc.SendCommand( "demod:res=" + str(DEMOD_CANVAS_RES_X) )

		if not len(hdc.DEMOD.values_):
			GUI.after(500, RedrawDemod)
			return

		global DEMOD_CANVAS

		DEMOD_CANVAS.delete('all')

		demod_values_ = hdc.DecodeValues(hdc.DEMOD)

		# resize vector to SPECTRUM_CANVAS_RES_X
		demod_values_resX = [None] * DEMOD_CANVAS_RES_X
		for x in range(DEMOD_CANVAS_RES_X):
			x_0_1 = float(x)/(DEMOD_CANVAS_RES_X-1)
			demod_values_resX[x] = demod_values_[ int( round(x_0_1 * (len(demod_values_)-1)) ) ]

		# draw
		prev_x = 0
		prev_y = 0
		for x in range(DEMOD_CANVAS_RES_X):
			val_ = demod_values_resX[x]
			val_ *= .75 # scale down a little bit
			val_ = .5 + .5 * val_ / max( abs( hdc.DEMOD.min_ ), abs( hdc.DEMOD.max_ ) )
			val_pixel = (1.0-val_) * (DEMOD_CANVAS_RES_Y-1)
			val_pixel = int(round(val_pixel))
			DEMOD_CANVAS.create_line(prev_x, prev_y, x, val_pixel, fill='#CC0000')
			prev_x = x
			prev_y = val_pixel
	except:
		print(traceback.format_exc())

	GUI.after(500, RedrawDemod)


'''
def RefreshRttyStream():
	hdc.SendCommand('liveprint')
	GUI.after( 250, RefreshRttyStream )
'''

def UpdateRtty():
	if RTTY_STREAM_WIDGET:
		new_text = hdc.RTTY_STREAM
		if len(new_text) > 80:
			new_text = new_text[-80:]
		new_text = new_text.replace('\n', ' ')
		RTTY_STREAM_WIDGET.delete('1.0', 'end')
		RTTY_STREAM_WIDGET.insert('end', new_text)

	if RTTY_LAST_SENTENCE and len(hdc.SENTENCES):
		RTTY_LAST_SENTENCE.delete('1.0', 'end')
		RTTY_LAST_SENTENCE.insert('end', hdc.SENTENCES[-1])

	GUI.after( 200, UpdateRtty )


def ControlCallback(param_name, tk_var, i_type, *args):
	value = tk_var.get()
	if i_type == 'Bool':	_command = 'set:%s=%s' % ( param_name, int(value) )
	else:					_command = 'set:%s=%s' % ( param_name, str(value) )
	hdc.SendCommand(_command)


def UpdateControls():
	#sync GUI with server state
	for param in list(CONTROLS.keys()):
		CONTROLS[param][1].set( hdc.STATE[param] )


def BuildMainWindow():
	grid_row = 0

	# ZOOM BUTTON
	#
	def zoom(i_zoom):
		global ZOOM
		ZOOM += i_zoom
		ZOOM = max(0, min(ZOOM, 1))
	tk.Button(GUI, text = '-', command = lambda: zoom(-.1), width = 30).grid(row=grid_row, column=0)
	tk.Button(GUI, text = '+', command = lambda: zoom(.1),  width = 30).grid(row=grid_row, column=1)
	grid_row += 1

	# SPECTRUM CANVAS
	#
	global SPECTRUM_CANVAS
	SPECTRUM_CANVAS = tk.Canvas(GUI, width=SPECTRUM_CANVAS_RES_X, height=SPECTRUM_CANVAS_RES_Y, bg=SPECTRUM_CANVAS_BG)
	SPECTRUM_CANVAS.grid(row = grid_row, column=1)
	grid_row += 1

	# DEMOD CANVAS
	#
	global DEMOD_CANVAS
	DEMOD_CANVAS = tk.Canvas(GUI, width=DEMOD_CANVAS_RES_X, height=DEMOD_CANVAS_RES_Y, bg=DEMOD_CANVAS_BG)
	DEMOD_CANVAS.grid(row = grid_row, column=1)
	grid_row += 1

	# CONTROL WIDGETS
	#
	def MakeWidget(i_type, i_name, default_value, callback):
		var = None
		widget = None
		if i_type == 'Int':
			var = tk.IntVar(GUI, value=int(default_value))
			widget = tk.Entry( GUI, textvariable = var )
			widget.bind( '<Return>', lambda _var=var: callback(var, i_type) )
		elif i_type == 'Float':
			var = tk.DoubleVar(GUI, value=float(default_value))
			widget = tk.Entry( GUI, textvariable = var )
			widget.bind( '<Return>', lambda _var=var: callback(var, i_type) )
		elif i_type == 'Bool':
			var = tk.BooleanVar(GUI, value=bool(default_value))
			widget = tk.Checkbutton( GUI, variable = var, command = lambda _var=var: callback(var, i_type) )
			# var.trace( 'w', lambda name, index, mode, _var=var: callback(var, i_type) )

		return [widget, var, i_type]

	hdc.UpdateState()

	global CONTROLS
	CONTROLS['frequency'] = 	MakeWidget('Float', 'frequency', 	hdc.STATE['frequency'],		partial(ControlCallback, 'frequency') )
	CONTROLS['gain'] = 			MakeWidget('Int', 	'gain', 		hdc.STATE['gain'], 			partial(ControlCallback, 'gain') )
	CONTROLS['decimation'] = 	MakeWidget('Int', 	'decimation', 	hdc.STATE['decimation'], 	partial(ControlCallback, 'decimation') )
	CONTROLS['baud'] = 			MakeWidget('Float', 'baud', 		hdc.STATE['baud'], 			partial(ControlCallback, 'baud') )
	CONTROLS['rtty_bits'] = 	MakeWidget('Int', 	'rtty_bits', 	hdc.STATE['rtty_bits'], 	partial(ControlCallback, 'rtty_bits') )
	CONTROLS['rtty_stops'] = 	MakeWidget('Int', 	'rtty_stops', 	hdc.STATE['rtty_stops'], 	partial(ControlCallback, 'rtty_stops') )
	CONTROLS['lowpass_bw'] = 	MakeWidget('Float', 'lowpass_bw', 	hdc.STATE['lowpass_bw'], 	partial(ControlCallback, 'lowpass_bw') )
	CONTROLS['lowpass_trans'] = MakeWidget('Float', 'lowpass_trans',hdc.STATE['lowpass_trans'],	partial(ControlCallback, 'lowpass_trans') )
	CONTROLS['biastee'] = 	 	MakeWidget('Bool', 	'biastee', 		hdc.STATE['biastee'], 		partial(ControlCallback, 'biastee') )
	CONTROLS['afc'] = 			MakeWidget('Bool', 	'afc', 			hdc.STATE['afc'], 			partial(ControlCallback, 'afc') )
	CONTROLS['dc_remove'] =		MakeWidget('Bool', 	'dc_remove',	hdc.STATE['dc_remove'],		partial(ControlCallback, 'dc_remove') )

	for param in [ 'frequency', 'gain', 'decimation',
					'lowpass_bw', 'lowpass_trans',
					'baud', 'rtty_bits', 'rtty_stops',
					'biastee', 'afc', 'dc_remove']:
		widget = CONTROLS[param]
		tk.Label(text=param).grid(row=grid_row, column=0)
		widget[0].grid(row=grid_row, column=1)
		grid_row += 1

	UpdateControls()

	# RTTY STREAM
	#
	tk.Label(text='RTTY Stream').grid(row=grid_row, column=0)
	global RTTY_STREAM_WIDGET
	RTTY_STREAM_WIDGET = tk.Text(GUI, width=80, height=1)
	RTTY_STREAM_WIDGET.grid(row=grid_row, column=1)
	grid_row += 1

	# LAST SENTENCE
	#
	tk.Label(text='Last Sentence').grid(row=grid_row, column=0)
	global RTTY_LAST_SENTENCE
	RTTY_LAST_SENTENCE = tk.Text(GUI, width=80, height=1)
	RTTY_LAST_SENTENCE.grid(row=grid_row, column=1)
	grid_row += 1


def main():
	hdc.InitConnection("ws://127.0.0.1:5555/", UpdateControls, UpdateRtty)
	hdc.UpdateState()

	BuildMainWindow()
	GUI.after( 500, RedrawSpectrum )
	GUI.after( 500, RedrawDemod )
	# GUI.after( 250, RefreshRttyStream ) # no need to explicitly poll for data


	GUI.mainloop()
	hdc.CloseConnection()

if __name__ == "__main__":
	main()