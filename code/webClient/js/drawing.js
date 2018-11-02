function DrawPowerSpectrum(i_canvas, i_spectrum)
{
	var ctx = i_canvas.getContext("2d");

	// CLEAR THE CANVAS
	//
    ctx.clearRect(0, 0, i_canvas.width-1, i_canvas.height-1);

    // BG
    //
    var grd_bg = ctx.createLinearGradient(0, 0, 0, i_canvas.height-1);
	grd_bg.addColorStop(0, "hsl(210, 15%, 24%)");
	grd_bg.addColorStop(.5, "hsl(210, 15%, 40%)");
	grd_bg.addColorStop(1, "hsl(210, 15%, 24%)");
	ctx.fillStyle = grd_bg;
	ctx.fillRect(0, 0, i_canvas.width-1, i_canvas.height-1);

	G_SPECTRUM_ZOOM = Math.max(0, Math.min(1, G_SPECTRUM_ZOOM));

	var zoom = G_SPECTRUM_ZOOM;

    // LOWPASS FILTER DRAW
    //
    var _lowpass_bw = GLOBALS.lowpass_bw / (1.0 - .999*zoom);
    var _lowpass_trans = GLOBALS.lowpass_trans / (1.0 - .999*zoom);
	var grd_lowpass = ctx.createLinearGradient(0, 0, i_canvas.width-1, 0);
	var _l  = Math.max(0, .5 - .5 * _lowpass_bw);
	var _ll = Math.max(0, .5 - .5 * (_lowpass_bw + _lowpass_trans));
	var _r  = Math.min(1, .5 + .5 * _lowpass_bw);
	var _rr = Math.min(1, .5 + .5 * (_lowpass_bw + _lowpass_trans));

	grd_lowpass.addColorStop(0, "rgba(15,25,50,0)");
	grd_lowpass.addColorStop(_ll, "rgba(15,25,50,0)");
	grd_lowpass.addColorStop(_l, "#113555");
	grd_lowpass.addColorStop(_r, "#113555");
	grd_lowpass.addColorStop(_rr, "rgba(15,25,50,0)");
	grd_lowpass.addColorStop(1, "rgba(15,25,50,0)");
	ctx.fillStyle = grd_lowpass;
	ctx.fillRect(0, 0, i_canvas.width-1, i_canvas.height-1);

	// SPECTRUM
	//
	var power_grd = ctx.createLinearGradient(0, 0, 0, i_canvas.height-1);
	power_grd.addColorStop(0.2, "yellow");
	power_grd.addColorStop(.6, "#993311");
	power_grd.addColorStop(1, "black");

	ctx.strokeStyle = power_grd;
	ctx.beginPath();
	if(i_spectrum.type_size_ == 1) // 8 bit char
	{
		for(var x=0; x<i_canvas.width; ++x)
		{
			var x_0_1 = x/(i_canvas.width-1);
			var val_0_1 = i_spectrum.values_[ Math.round(x_0_1 * i_spectrum.values_.length) ] / 255;
			var val_pixel = (1-val_0_1) * (i_canvas.height-1);
			ctx.moveTo(x, i_canvas.height - 1);
			ctx.lineTo(x, val_pixel);
		}
	}
	else if(i_spectrum.type_size_ == 2) // uint16_t
	{
		for(var x=0; x<i_canvas.width; ++x)
		{
			var x_0_1 = x/(i_canvas.width-1);
			var val_0_1 = i_spectrum.values_[ Math.round(x_0_1 * i_spectrum.values_.length) ] / 65535;
			var val_pixel = (1-val_0_1) * (i_canvas.height-1);
			ctx.moveTo(x, i_canvas.height - 1);
			ctx.lineTo(x, val_pixel);
		}
	}
	else if(i_spectrum.type_size_ == 4) // 32float
	{
		for(var x=0; x<i_canvas.width; ++x)
		{
			var x_0_1 = x/(i_canvas.width-1);
			var val_0_1 = (i_spectrum.values_[Math.round(x_0_1 * i_spectrum.values_.length)] - i_spectrum.min_) / (i_spectrum.max_ - i_spectrum.min_) ;
			var val_pixel = (1-val_0_1) * (i_canvas.height-1);
			ctx.moveTo(x, i_canvas.height - 1);
			ctx.lineTo(x, val_pixel);
		}
	}
	ctx.stroke();

	// CENTER LINE
	//
	ctx.strokeStyle = '#888888';
	ctx.beginPath();
	ctx.moveTo(i_canvas.width/2, i_canvas.height - 1);
	ctx.lineTo(i_canvas.width/2, 0);
	ctx.stroke();

	// NOISE FLOOR
	//
	ctx.strokeStyle = '#333388';
	ctx.beginPath();
	var nf_0_1 = (i_spectrum.noise_floor_ - i_spectrum.min_) / (i_spectrum.max_ - i_spectrum.min_);
	ctx.moveTo(0, (1-nf_0_1)*i_canvas.height - 1);
	ctx.lineTo(i_canvas.width-1, (1-nf_0_1)*i_canvas.height - 1);
	ctx.stroke();

	// PEAK LEFT
	//
	if(i_spectrum.peak_left_valid_)
		ctx.strokeStyle = '#FF5500';
	else
		ctx.strokeStyle = '#55000';
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
		ctx.strokeStyle = '#000055';
	ctx.beginPath();
	var peak_right_0_1 = i_spectrum.peak_right_ / i_spectrum.size_;
	ctx.moveTo(peak_right_0_1 * i_canvas.width, 0);
	ctx.lineTo(peak_right_0_1 * i_canvas.width, i_canvas.height - 1);
	ctx.stroke();

}


function DrawDemod(i_canvas, i_demod)
{
	var ctx = i_canvas.getContext("2d");

	// Clear the canvas
    ctx.clearRect(0, 0, i_canvas.width, i_canvas.height);

    // BG
    //
    var grd_bg = ctx.createLinearGradient(0, 0, 0, i_canvas.height-1);
	grd_bg.addColorStop(0, "hsl(210, 15%, 24%)");
	grd_bg.addColorStop(.5,"hsl(210, 15%, 40%)");
	grd_bg.addColorStop(1, "hsl(210, 15%, 24%)");
	ctx.fillStyle = grd_bg;
	ctx.fillRect(0, 0, i_canvas.width-1, i_canvas.height-1);

	// CENTER LINE
	//
	ctx.strokeStyle = 'hsl(210, 15%, 30%)';
	ctx.beginPath();
	ctx.moveTo(0, i_canvas.height/2);
	ctx.lineTo(i_canvas.width-1, i_canvas.height/2);
	ctx.stroke();

	// DEMOD
	//
	ctx.strokeStyle = "#aa5500";

	ctx.beginPath();

	ctx.moveTo(0, 0);

	for(var x=0; x<i_canvas.width; ++x)
	{
		var x_0_1 = x/(i_canvas.width-1);

		var raw_value =  i_demod.values_[ Math.round(x_0_1 * i_demod.values_.length) ];
		var full_scale_value =  0;
		if(i_demod.type_size_ == 1) // 8 bit char
			full_scale_value = i_demod.min_ + raw_value / 255.0 * (i_demod.max_ - i_demod.min_);
		else if(i_demod.type_size_ == 2) // uint16_t
			full_scale_value = i_demod.min_ + raw_value / 65535.0 * (i_demod.max_ - i_demod.min_);
		else if(i_demod.type_size_ == 4) // 32float
			full_scale_value =  raw_value;

		full_scale_value *= .75; // scale down a little bit
		var val_0_1 = .5 + .5 * full_scale_value / Math.max( Math.abs( i_demod.min_ ), Math.abs( i_demod.max_ ) );

		var val_pixel = (1.0-val_0_1) * (i_canvas.height-1);
		ctx.lineTo(x, val_pixel);
	}

	ctx.stroke();
}


function ResizeCanvas(canvas_id)
{
	var canvasNode = document.getElementById(canvas_id);
	var canvasDiv = canvasNode.parentNode;

	canvasNode.style.width = '100%';
	canvasNode.style.height = '100%';
	canvasNode.width = canvasDiv.clientWidth;
	canvasNode.height = canvasDiv.clientHeight;
}
