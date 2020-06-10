
class Average // rolling avg classs
{
	constructor(max_count, init_val=0)
	{
		this.max_count_ = max_count;
		this.count_ = 0;
		this.sum_ = 0;
		this.add(init_val);
	}

	get_avg()
	{
		if(this.count_ == 0)
			return this.sum_;
		return this.sum_/this.count_;
	}

	add(val)
	{
		if(this.count_ == this.max_count_)
		{
			// console.debug("SATURATE");
			this.sum_ = this.get_avg() * (this.max_count_-1) + val;
		}
		else
		{
			this.count_ += 1;
			this.sum_ += val;
		}
	}

	reset(val)
	{
		this.sum_ = val;
		this.count_ = 1;
	}
};

var AccSpectrumArray = {
	arr_: new Array(),
	count_: 0,
	maxCount_: 3
};

var AvgCnt = 10; // Rolling average length. Smooths out spectrum drawing.
var NoiseFloorAvg = new Average(AvgCnt);
var SpectrumMinAvg = new Average(AvgCnt);
var SpectrumMaxAvg = new Average(AvgCnt);

// rolling average on array
function AccumulateSpectrumArray(i_arr)
{
	if(AccSpectrumArray.arr_.length != i_arr.length)
	{
		AccSpectrumArray.arr_ = i_arr;
		AccSpectrumArray.count_ = 1;
		return AccSpectrumArray.arr_;
	}

	if( AccSpectrumArray.count_ < AccSpectrumArray.maxCount_ )
	{
		for(var i=0 ; i<AccSpectrumArray.arr_.length; ++i)
			AccSpectrumArray.arr_[i] += i_arr[i];
		AccSpectrumArray.count_ += 1;
	}
	else
	{
		for(var i=0 ; i<AccSpectrumArray.arr_.length; ++i)
		{
			AccSpectrumArray.arr_[i] =
				(AccSpectrumArray.arr_[i] / AccSpectrumArray.count_) * (AccSpectrumArray.maxCount_-1) + i_arr[i];
		}
	}

	// return value
	var spectr_result = new Array(AccSpectrumArray.arr_.length);
	for(var i=0 ; i<AccSpectrumArray.arr_.length; ++i)
		spectr_result[i] = AccSpectrumArray.arr_[i] / AccSpectrumArray.count_;
	return spectr_result;
}

function DrawPowerSpectrum(i_canvas, i_spectrum)
{
	if(i_spectrum == undefined)
		return;

	var ctx = i_canvas.getContext("2d");

	// CLEAR THE CANVAS
	//
    ctx.clearRect(0, 0, i_canvas.width, i_canvas.height);

    // BG
    //
    var grd_bg = ctx.createLinearGradient(0, 0, 0, i_canvas.height);
	grd_bg.addColorStop(0, HD_COLOR_SCHEME['CSS']['HD_bg']);
	grd_bg.addColorStop(.5, HD_COLOR_SCHEME['CSS']['HD_fg']);
	grd_bg.addColorStop(1, HD_COLOR_SCHEME['CSS']['HD_bg']);
	ctx.fillStyle = grd_bg;
	ctx.fillRect(0, 0, i_canvas.width, i_canvas.height);

	G_SPECTRUM_ZOOM = Math.max(0, Math.min(1, G_SPECTRUM_ZOOM));

	var zoom = G_SPECTRUM_ZOOM;

    // LOWPASS FILTER DRAW
	//
	var _lowpass_bw_relative = HD_GLOBALS.lowpass_bw / i_spectrum.sampling_rate_ / (1.0 - .999*zoom);
    var _lowpass_trans = HD_GLOBALS.lowpass_trans / (1.0 - .999*zoom);
	var grd_lowpass = ctx.createLinearGradient(0, 0, i_canvas.width-1, 0);
	var _l  = Math.max(0, .5 - .5 * _lowpass_bw_relative);
	var _ll = Math.max(0, .5 - .5 * (_lowpass_bw_relative + _lowpass_trans));
	var _r  = Math.min(1, .5 + .5 * _lowpass_bw_relative);
	var _rr = Math.min(1, .5 + .5 * (_lowpass_bw_relative + _lowpass_trans));

	grd_lowpass.addColorStop(_ll, HD_COLOR_SCHEME['CSS']['HD_bg']);
	grd_lowpass.addColorStop(_l, HD_COLOR_SCHEME["SPECTRUM"]["FILTER"]);
	grd_lowpass.addColorStop(_r, HD_COLOR_SCHEME["SPECTRUM"]["FILTER"]);
	grd_lowpass.addColorStop(_rr, HD_COLOR_SCHEME['CSS']['HD_bg']);
	grd_lowpass.addColorStop(1, HD_COLOR_SCHEME['CSS']['HD_bg']);
	ctx.fillStyle = grd_lowpass;
	ctx.fillRect(0, 0, i_canvas.width, i_canvas.height);

	// SPECTRUM
	//

	// decode spectrum values
	var spectrum_values_arr = new Array(i_canvas.width);
	for(var x=0; x<i_canvas.width; ++x)
	{
		var x_0_1 = x/(i_canvas.width-1);

		var value_encoded = i_spectrum.values_[ Math.round(x_0_1 * i_spectrum.values_.length) ];
		var true_value;
		if(i_spectrum.type_size_ == 1) // 8 bit char
			true_value = i_spectrum.min_ + (value_encoded / 255) * (i_spectrum.max_ - i_spectrum.min_);
		else if(i_spectrum.type_size_ == 2) // uint16_t
			true_value = i_spectrum.min_ + (value_encoded / 65535) * (i_spectrum.max_ - i_spectrum.min_);
		else if(i_spectrum.type_size_ == 4) // 32float
			true_value = value_encoded;

		spectrum_values_arr[x] = true_value;
	}
	// border values are NaN. needs fixing
	spectrum_values_arr[0] = spectrum_values_arr[1];
	spectrum_values_arr[spectrum_values_arr.length-1] = spectrum_values_arr[spectrum_values_arr.length-2];

	// accumulate spectrum and noisefloor for nice rendering
	spectrum_values_arr = AccumulateSpectrumArray(spectrum_values_arr);
	NoiseFloorAvg.add(i_spectrum.noise_floor_);
	SpectrumMinAvg.add(i_spectrum.min_);
	SpectrumMaxAvg.add(i_spectrum.max_);

	var noise_floor_avg = NoiseFloorAvg.get_avg();
	var spectrum_min_avg = SpectrumMinAvg.get_avg();
	var spectrum_max_avg = SpectrumMaxAvg.get_avg();
	// spectrum_min_avg *= 1.3; // drawing pedestal

	// draw
	var power_grd = ctx.createLinearGradient(0, 0, 0, i_canvas.height);
	power_grd.addColorStop(1-Math.abs(noise_floor_avg / spectrum_min_avg), HD_COLOR_SCHEME['SPECTRUM']['HIGH']);
	power_grd.addColorStop(1-Math.abs(spectrum_max_avg / spectrum_min_avg), HD_COLOR_SCHEME['SPECTRUM']['MID']);
	power_grd.addColorStop(1 , HD_COLOR_SCHEME['SPECTRUM']['LOW']);

	ctx.strokeStyle = power_grd;
	ctx.beginPath();
	for(var x=0; x<i_canvas.width; ++x)
	{
		var val_0_1 = 1.0 - Math.abs(spectrum_values_arr[x] / spectrum_min_avg);
		val_0_1 = Math.max(val_0_1, 0);
		var val_pixel = (1-val_0_1) * (i_canvas.height-1);
		ctx.moveTo(x, i_canvas.height - 1);
		ctx.lineTo(x, val_pixel);
	}
	ctx.stroke();

	// CENTER LINE
	//
	ctx.strokeStyle = '#555555';
	ctx.beginPath();
	ctx.moveTo(i_canvas.width/2, i_canvas.height - 1);
	ctx.lineTo(i_canvas.width/2, 0);
	ctx.stroke();

	// NOISE FLOOR
	//
	ctx.strokeStyle = '#333388';
	ctx.beginPath();
	var nf_0_1 = 1.0 - Math.abs(noise_floor_avg / spectrum_min_avg);
	nf_0_1 = Math.max(nf_0_1, 0);
	ctx.moveTo(0, (1-nf_0_1)*i_canvas.height - 1);
	ctx.lineTo(i_canvas.width, (1-nf_0_1)*i_canvas.height - 1);
	ctx.stroke();

	// PEAK LEFT
	//
	if(i_spectrum.peak_left_valid_)
		ctx.strokeStyle = '#FF2200';
	else
		ctx.strokeStyle = '#113322';
	ctx.beginPath();
	var peak_left_0_1 = i_spectrum.peak_left_ / i_spectrum.size_;
	ctx.moveTo(peak_left_0_1 * i_canvas.width, 0);
	ctx.lineTo(peak_left_0_1 * i_canvas.width, i_canvas.height - 1);
	ctx.stroke();

	// PEAK RIGHT
	//
	if(i_spectrum.peak_right_valid_)
		ctx.strokeStyle = '#0088FF';
	else
		ctx.strokeStyle = '#113322';
	ctx.beginPath();
	var peak_right_0_1 = i_spectrum.peak_right_ / i_spectrum.size_;
	ctx.moveTo(peak_right_0_1 * i_canvas.width, 0);
	ctx.lineTo(peak_right_0_1 * i_canvas.width, i_canvas.height - 1);
	ctx.stroke();

}


function DrawDemod(i_canvas, i_demod)
{
	if(i_demod == undefined)
		return;

	var ctx = i_canvas.getContext("2d");

	// Clear the canvas
    ctx.clearRect(0, 0, i_canvas.width, i_canvas.height);

    // BG
    //
    var grd_bg = ctx.createLinearGradient(0, 0, 0, i_canvas.height);
	grd_bg.addColorStop(0, HD_COLOR_SCHEME['CSS']['HD_bg']);
	grd_bg.addColorStop(.5,HD_COLOR_SCHEME['CSS']['HD_fg']);
	grd_bg.addColorStop(1, HD_COLOR_SCHEME['CSS']['HD_bg']);
	ctx.fillStyle = grd_bg;
	ctx.fillRect(0, 0, i_canvas.width, i_canvas.height);

	// CENTER LINE
	//
	ctx.strokeStyle = HD_COLOR_SCHEME['SPECTRUM']['LOW'];
	ctx.beginPath();
	ctx.moveTo(0, i_canvas.height/2);
	ctx.lineTo(i_canvas.width, i_canvas.height/2);
	ctx.stroke();

	// DEMOD
	//
	ctx.strokeStyle = HD_COLOR_SCHEME['SPECTRUM']['MID'];

	ctx.beginPath();

	ctx.moveTo(0, 0);

	for(var x=0; x<i_canvas.width; ++x)
	{
		var x_0_1 = x/(i_canvas.width-1);

		var encoded_value =  i_demod.values_[ Math.round(x_0_1 * i_demod.values_.length) ];
		var true_value =  0;
		if(i_demod.type_size_ == 1) // 8 bit char
			true_value = i_demod.min_ + encoded_value / 255.0 * (i_demod.max_ - i_demod.min_);
		else if(i_demod.type_size_ == 2) // uint16_t
			true_value = i_demod.min_ + encoded_value / 65535.0 * (i_demod.max_ - i_demod.min_);
		else if(i_demod.type_size_ == 4) // 32float
			true_value =  encoded_value;

		true_value *= .75; // scale down a little bit
		var val_0_1 = .5 + .5 * true_value / Math.max( Math.abs( i_demod.min_ ), Math.abs( i_demod.max_ ) );

		var val_pixel = (1.0-val_0_1) * (i_canvas.height-1);
		ctx.lineTo(x, val_pixel);
	}

	ctx.stroke();
}


function ResizeCanvas(canvas_id)
{
	// for some reason canvas size has to be checked periodically
	// at least when demod canvas is zero in size
	// ????
	var canvasNode = document.getElementById(canvas_id);
	var canvasDiv = canvasNode.parentNode;
	var canvasDiv_clientWidth = canvasDiv.clientWidth;
	var canvasDiv_clientHeight = canvasDiv.clientHeight;

	if(canvasDiv_clientWidth && canvasDiv_clientHeight) {
		canvasNode.width = canvasDiv_clientWidth;
		canvasNode.height = canvasDiv_clientHeight;
		setTimeout(() => {
			ResizeCanvas(canvas_id)
		}, 60000);
	} else {
		setTimeout(() => {
			ResizeCanvas(canvas_id)
		}, 1000);
	}
}


var AnimatePowerSpectrum_last = 0;
var G_PowerCanvas;
function AnimatePowerSpectrum(timestamp)
{
	if(G_PowerCanvas == undefined)
		G_PowerCanvas = document.getElementById("HabDec_powerSpectrumCanvas");

	if(!AnimatePowerSpectrum_last)
		AnimatePowerSpectrum_last = timestamp;

	if( (timestamp - AnimatePowerSpectrum_last) > (1000/G_POWER_FPS) )
	{
		AnimatePowerSpectrum_last = timestamp;
		DrawPowerSpectrum(G_PowerCanvas, G_SPECTRUM_DATA);
	}

	window.requestAnimationFrame(AnimatePowerSpectrum);
}

var AnimateDemod_last = 0;
var G_DemodCanvas;
function AnimateDemod(timestamp)
{
	if(G_DemodCanvas == undefined)
		G_DemodCanvas = document.getElementById("HabDec_demodCanvas");

	if(!AnimateDemod_last)
		AnimateDemod_last = timestamp;

	if( (timestamp - AnimateDemod_last) > (1000/G_DEMOD_FPS) )
	{
		AnimateDemod_last = timestamp;
		DrawDemod(G_DemodCanvas, G_DEMOD_DATA);
	}

	window.requestAnimationFrame(AnimateDemod);
}
