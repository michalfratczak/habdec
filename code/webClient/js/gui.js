

function CreateFloatBoxWithArrows(	i_cnt, i_parameter, i_callback,
									i_min, i_max, i_default, step_small = 0, step_big = 0, step_box = 0)
{
	var widget = document.createElement("INPUT");
	widget.classList.add('ctrl_box');

	widget.setAttribute("id", i_parameter);
	widget.setAttribute("type", "number");
	widget.setAttribute("name", i_parameter);
	widget.setAttribute("min", i_min);
	widget.setAttribute("max", i_max);
	widget.setAttribute("defaultValue", i_default);
	widget.value = i_default;

	widget.onchange = function() {
		i_callback("set:" + i_parameter + "=" + widget.value)	};

	// label
	var lab = document.createElement("LABEL");
	lab.appendChild( document.createTextNode(i_parameter) );

	// determine step size. big and small
	if(!step_small)
	{
		step_small = (i_max - i_min) / 50.0;
		step_big = (i_max - i_min) / 5.0;
	}
	else if(!step_big)
	{
		step_big = (i_max - i_min) / 5.0;
	}
	if(!step_box)
		step_box = step_small;

	widget.setAttribute("step_big", step_big);
	widget.setAttribute("step_small", step_small);
	widget.setAttribute("step", step_box);

	// arrows
	var decr_big   = document.createElement("button");
	decr_big.classList.add("increment_button");
	decr_big.appendChild(document.createTextNode("<<<"));

	var decr_small = document.createElement("button");
	decr_small.classList.add("increment_button");
	decr_small.appendChild(document.createTextNode("<"));

	var incr_small = document.createElement("button");
	incr_small.classList.add("increment_button");
	incr_small.appendChild(document.createTextNode(">"));

	var incr_big   = document.createElement("button");
	incr_big.classList.add("increment_button");
	incr_big.appendChild(document.createTextNode(">>>"));

	// arrows callbacks
	decr_big.onclick = 		function() {
				widget.value = parseFloat(widget.value) - parseFloat(widget.getAttribute("step_big"));
				i_callback("set:" + i_parameter + "=" + widget.value);
			};
	decr_small.onclick = 	function() {
				widget.value = parseFloat(widget.value) - parseFloat(widget.getAttribute("step_small"));
				i_callback("set:" + i_parameter + "=" + widget.value);
			};
	incr_small.onclick = 	function() {
				widget.value = parseFloat(widget.value) + parseFloat(widget.getAttribute("step_small"));
				i_callback("set:" + i_parameter + "=" + widget.value);
			};
	incr_big.onclick = 		function() {
				widget.value = parseFloat(widget.value) + parseFloat(widget.getAttribute("step_big"));
				i_callback("set:" + i_parameter + "=" + widget.value);
			};

	document.getElementById(i_cnt).style.textAlign = "right";

	document.getElementById(i_cnt).appendChild(lab);

	document.getElementById(i_cnt).appendChild(widget);

	document.getElementById(i_cnt).appendChild(decr_big);
	document.getElementById(i_cnt).appendChild(decr_small);
	document.getElementById(i_cnt).appendChild(incr_small);
	document.getElementById(i_cnt).appendChild(incr_big);

	return widget;
}


function SetGuiToGlobals(i_globals)
{
	for(var param in i_globals)
	{
		var value = i_globals[param];
		// console.debug("setting ", param, value);
		try {document.getElementById(param).value = value; }			catch(err) {};
		try {document.getElementById(param + "_box").value = value; }	catch(err) {};
		try {document.getElementById(param).checked = value; }			catch(err) {};
	}

	// buttons

	let root = document.documentElement;
	var rootStyles = getComputedStyle(root);
	var HD_bg = rootStyles.getPropertyValue('--HD_bg');
	var HD_button = rootStyles.getPropertyValue('--HD_button');
	var HD_button_text = rootStyles.getPropertyValue('--HD_button_text');
	var HD_enabled = rootStyles.getPropertyValue('--HD_enabled');

	var biastee_value = HD_GLOBALS.biastee;
	if(biastee_value)
	{
		var button = document.getElementById("HabDecD_biastee");
		button.style.backgroundColor = HD_enabled;
		button.style.color = HD_bg;
	}
	else
	{
		var button = document.getElementById("HabDecD_biastee");
		button.style.backgroundColor = HD_button;
		button.style.color = HD_button_text;
	}


	var afc_value = HD_GLOBALS.afc
	if(afc_value)
	{
		var button = document.getElementById("HabDec_afc");
		button.style.backgroundColor = HD_enabled;
		button.style.color = HD_bg;
	}
	else
	{
		var button = document.getElementById("HabDec_afc");
		button.style.backgroundColor = HD_button;
		button.style.color = HD_button_text;
	}

	var dcremove_value = HD_GLOBALS.dc_remove;
	if(dcremove_value)
	{
		var button = document.getElementById("HabDec_dc_remove");
		button.style.backgroundColor = HD_enabled;
		button.style.color = HD_bg;
	}
	else
	{
		var button = document.getElementById("HabDec_dc_remove");
		button.style.backgroundColor = HD_button;
		button.style.color = HD_button_text;
	}
}


function CreateControls()
{
	var freq_widget = CreateFloatBoxWithArrows("cnt_frequency", "frequency",
						SendCommand,  25, 1200, 434.355, 20.0/1e6, 1000.0/1e6, 1e-6);
	freq_widget.style.width = "100px";
	CreateFloatBoxWithArrows("cnt_decimation", "decimation",
						SendCommand,  0, 8, 8, 1, 1);
	CreateFloatBoxWithArrows("cnt_gain", "gain",
						SendCommand,  0, 49, 21, 1, 5);
	CreateFloatBoxWithArrows("cnt_lowpass_bw", "lowpass_bw",
						SendCommand,  100, 4000, 1500, 50, 200, 10);
	CreateFloatBoxWithArrows("cnt_lowpass_trans", "lowpass_trans",
						SendCommand,  .0025, 0.1, .05, .0025, .01);
	CreateFloatBoxWithArrows("cnt_baud", "baud",
						SendCommand,  50, 1200, 300, 25, 100);
	CreateFloatBoxWithArrows("cnt_rtty_bits", "rtty_bits",
						SendCommand,  7, 8, 8, 1, 1);
	CreateFloatBoxWithArrows("cnt_rtty_stops", "rtty_stops",
						SendCommand,  1, 2, 2, 1, 1);
	CreateFloatBoxWithArrows("cnt_datasize", "datasize",
						SendCommand,  1, 4, 1, 1, 1);
}


function toggleFullscreen(elem)
{
	elem = elem || document.documentElement;
	if (!document.fullscreenElement && !document.mozFullScreenElement &&
		!document.webkitFullscreenElement && !document.msFullscreenElement)
	{
		if (elem.requestFullscreen) {
			elem.requestFullscreen();
		} else if (elem.msRequestFullscreen) {
			elem.msRequestFullscreen();
		} else if (elem.mozRequestFullScreen) {
			elem.mozRequestFullScreen();
		} else if (elem.webkitRequestFullscreen) {
			elem.webkitRequestFullscreen(Element.ALLOW_KEYBOARD_INPUT);
		}
	}
	else
	{
		if (document.exitFullscreen) {
			document.exitFullscreen();
		} else if (document.msExitFullscreen) {
			document.msExitFullscreen();
		} else if (document.mozCancelFullScreen) {
			document.mozCancelFullScreen();
		} else if (document.webkitExitFullscreen) {
			document.webkitExitFullscreen();
		}
	}
}


////////////////////////////////////////
///////////// Color Schemes ////////////
////////////////////////////////////////

function CreateColorSchemesButton()
{
	var ColorSchemesWrapperDiv = document.getElementById("ColorSchemesWrapperDiv");
	ColorSchemesWrapperDiv.classList.add("MenuDropdownWrapper")

	var ColorSchemesButton = document.createElement("button");
	ColorSchemesButton.classList.add("MenuDropButton");
	ColorSchemesButton.id = "HabDec_ColorSchemesButton";
	ColorSchemesButton.onclick = function() { document.getElementById("ColorSchemesDropMenuDiv").classList.toggle("show") };
	ColorSchemesButton.innerHTML = "Colors";
	ColorSchemesWrapperDiv.appendChild(ColorSchemesButton);

	var DropMenuDiv = document.createElement("div");
	DropMenuDiv.id = "ColorSchemesDropMenuDiv";
	DropMenuDiv.classList.add("MenuDropMenu");
	ColorSchemesWrapperDiv.appendChild(DropMenuDiv);
	UpdateColorSchemesButton(HD_COLOR_SCHEMES);
}

function UpdateColorSchemesButton(i_color_schemes)
{
	var DropMenuDiv = document.getElementById("ColorSchemesDropMenuDiv");

	for(color in i_color_schemes)
	(
		function(i_color)
		{
			var pl_button = document.createElement("button");
			pl_button.innerHTML = i_color;
			pl_button.onclick = function(){
				HD_ApplyColorScheme(i_color_schemes[i_color]);
				document.getElementById("HabDec_ColorSchemesButton").click();
			};

			DropMenuDiv.appendChild(pl_button);
		}
	)(color)
}


// ============================================================================
// ============================================================================
// ============================================================================


function HABDEC_BUILD_UI_PowerSpectrum(HABDEC_POWER_SPECTRUM_DIV)
{
	// <!-- Power Spectrum -->

	// <div style="display: grid; grid-template-columns: auto 80px; width: 100%; height: 150px;" >
	var div_top = document.createElement("div");
	div_top.style.display = "grid";
	div_top.style.gridTemplateColumns = "auto 80px";
	div_top.style.width = "100%";
	div_top.style.height = "150px";

	// <div id="cnt_powerSpectrumCanvas" style="display: flex;">
	var div_cnt_powerSpectrumCanvas = document.createElement("div");
	div_cnt_powerSpectrumCanvas.id = "cnt_powerSpectrumCanvas";
	div_cnt_powerSpectrumCanvas.style.display = "flex";

	// spectrum canvas
	var spectrum_canvas = document.createElement("canvas");
	spectrum_canvas.id = "HabDec_powerSpectrumCanvas";
	div_cnt_powerSpectrumCanvas.appendChild(spectrum_canvas);

	// +/- buttons
	// <button type="button" style="height: 50%; width: 100%; overflow: auto; font-size: 30px;" onclick="(function(){G_SPECTRUM_ZOOM += .1})()" > + </button><br>

	var b_plus = document.createElement("button");
	b_plus.innerHTML = "+";
	b_plus.style.height = "50%";
	b_plus.style.width = "100%";
	b_plus.style.overflow = "auto";
	b_plus.style.fontSize = "30px";
	b_plus.onclick = function(){ G_SPECTRUM_ZOOM += .1 };

	var b_minus = document.createElement("button");
	b_minus.innerHTML = "-";
	b_minus.style.height = "50%";
	b_minus.style.width = "100%";
	b_minus.style.overflow = "auto";
	b_minus.style.fontSize = "30px";
	b_minus.onclick = function(){ G_SPECTRUM_ZOOM -= .1 };

	var buttons_div = document.createElement("div");

	div_cnt_powerSpectrumCanvas.appendChild(spectrum_canvas);
	buttons_div.appendChild(b_plus);
	buttons_div.appendChild(b_minus);
	div_top.appendChild(div_cnt_powerSpectrumCanvas);
	div_top.appendChild(buttons_div);

	return div_top;
}


function HABDEC_BUILD_UI_DemodAndInfo()
{
	var div_top = document.createElement("div");

	// <div id="cnt_demodCanvas" style="display: flex; height: 128px;">
	var div_cnt_demodCanvas = document.createElement("div");
	div_cnt_demodCanvas.id = "cnt_demodCanvas";
	div_cnt_demodCanvas.style.height= "128px";

	// demod canvas
	var demod_canvas = document.createElement("canvas");
	demod_canvas.id = "HabDec_demodCanvas";
	div_cnt_demodCanvas.appendChild(demod_canvas);

	// info printout div
	var div_info = document.createElement("div");

	// <div id="cnt_debug" style="font-size: 12px;"></div>
	var div_debug = document.createElement("div");
	div_debug.id = "cnt_debug";

	// <div id="cnt_liveprint" class="habsentence_text" style="color: cadetblue; word-wrap: break-word;"></div>
	var div_cnt_liveprint = document.createElement("div");
	div_cnt_liveprint.id = "cnt_liveprint";
	div_cnt_liveprint.classList.add("habsentence_text");
	// div_cnt_liveprint.style.color = "cadetblue";
	div_cnt_liveprint.style.wordWrap = "break-word";

	//<div id="cnt_stats" class="habsentence_text"  style="color: rgb(250, 0, 0)"></div>
	var div_cnt_stats = document.createElement("div");
	div_cnt_stats.id = "cnt_stats";
	// div_cnt_stats.classList.add("habsentence_text");
	div_cnt_stats.style.color = "rgb(250, 0, 0)";

	// <div id="cnt_habsentence_list" class="habsentence_text"></div>
	var divcnt_habsentence_list = document.createElement("div");
	divcnt_habsentence_list.id = "cnt_habsentence_list";
	divcnt_habsentence_list.classList.add("habsentence_text");

	// SSDV
	var ssdv_div = document.createElement("div");
	var ssdv_info = document.createElement("text");
	ssdv_info.id = "HabDec_SSDV_Info";
	ssdv_info.style.color = "var(--HD_label)"
	var ssdv_img = document.createElement("img");
	ssdv_img.id = "HabDec_SSDV_Image";
	ssdv_img.style.height = "100%";
	var ssdv_img_div = document.createElement("div");
	ssdv_div.appendChild(ssdv_info);
	ssdv_img_div.appendChild(ssdv_img);
	ssdv_div.appendChild(ssdv_img_div);

	// SSDV fullscreen - when clicked
	var ssdv_FS_div = document.createElement("div");
	ssdv_FS_div.classList.add("HD_ssdv_modal");
	ssdv_img.onclick = function(){
		ssdv_FS_div.appendChild(ssdv_img_div);
		ssdv_FS_div.style.display = "block";
	}
	ssdv_FS_div.onclick = function(){
		ssdv_div.appendChild(ssdv_img_div);
		ssdv_FS_div.style.display = "None";
	}
	ssdv_div.appendChild(ssdv_FS_div);


	div_info.appendChild(div_debug);
	div_info.appendChild( document.createElement("br") );
	div_info.appendChild(div_cnt_liveprint);
	div_info.appendChild(div_cnt_stats);
	div_info.appendChild(divcnt_habsentence_list);

	div_top.appendChild(div_cnt_demodCanvas);
	div_top.appendChild(div_info);
	div_top.appendChild(ssdv_div);

	return div_top;

}


function HABDEC_BUILD_UI_Controls()
{
	var buttons = [
		"cnt_spectrum_zoom",
		"cnt_frequency",
		"cnt_decimation",
		"cnt_gain",
		"cnt_lowpass_bw",
		"cnt_lowpass_trans",
		"cnt_baud",
		"cnt_rtty_bits",
		"cnt_rtty_stops",
		"cnt_datasize" ];

	var div_top = document.createElement("div");

	for(i in buttons)
	{
		// <div id="cnt_spectrum_zoom"	class="ctrl_container"> </div>
		var div_buton = document.createElement("div");
		div_buton.classList.add( "ctrl_container" );
		div_buton.id = buttons[i];
		div_top.appendChild(div_buton);
	}

	return div_top;
}


function HABDEC_BUILD_UI_DemodAndControls()
{
	var div_demodAndInfo = 	HABDEC_BUILD_UI_DemodAndInfo();
	var div_controls = 		HABDEC_BUILD_UI_Controls();

	// <div style="display: grid; grid-template-columns: minmax(0, 1fr) 540px;">
	var div_top = document.createElement("div");
	div_top.style.display = "grid";
	div_top.style.gridTemplateColumns = "minmax(0, 1fr) 540px";
	div_top.appendChild(div_demodAndInfo);
	div_top.appendChild(div_controls);

	return div_top;

}


function HABDEC_BUILD_UI_ExtraRadioButtons()
{
	var div_top = document.createElement("div");

	// <button id="HabDecD_biastee" 	onclick="SetBiasT()"    > BiasTee </button>
	var b_biasTee = document.createElement("button");
	b_biasTee.id = "HabDecD_biastee";
	b_biasTee.innerHTML = "BiasTee";
	b_biasTee.onclick = SetBiasT;

	// <button id="HabDec_afc" 		onclick="SetAFC()"      > AFC </button>
	var b_afc = document.createElement("button");
	b_afc.id = "HabDec_afc";
	b_afc.innerHTML = "AFC";
	b_afc.onclick = SetAFC;

	// <button id="HabDec_dc_remove" 	onclick="SetDCRemove()" > DC Remove </button>
	var b_dc_remove = document.createElement("button");
	b_dc_remove.id = "HabDec_dc_remove";
	b_dc_remove.innerHTML = "DC Remove";
	b_dc_remove.onclick = SetDCRemove;

	var div_three_buttons = document.createElement("div");
	// div_three_buttons.display.style = "inline-block";
	div_three_buttons.appendChild(b_biasTee);
	div_three_buttons.appendChild(b_afc);
	div_three_buttons.appendChild(b_dc_remove);

	div_top.appendChild(div_three_buttons);

	return div_top;
}


function HABDEC_BUILD_UI_Server()
{
	var div_top = document.createElement("div");

	// <div><label>Server Address</label></div>
	var server_label = document.createElement("label");
	server_label.innerHTML = "Server Address";
	var div_label = document.createElement("div").appendChild(server_label);

	// <input id="HabDec_server_address" value="localhost:5555" onchange="localStorage.setItem('habdec_server_address', document.getElementById("HabDec_server_address").value);">
	var input_srv = document.createElement("input");
	input_srv.id = "HabDec_server_address";
	input_srv.value = "localhost:5555";
	input_srv.onchange = () => {
		localStorage.setItem('habdec_server_address', document.getElementById("HabDec_server_address").value);
	};

	// <button  onclick="OpenConnection()" >Connect</button>
	var b = document.createElement("button");
	b.innerHTML = "Connect";
	b.onclick = OpenConnection;


	div_top.appendChild(div_label);
	div_top.appendChild(input_srv);
	div_top.appendChild(b);

	return div_top;
}

function HB_WinMsgHandler(i_msg)
{
	// console.debug('HB_WinMsgHandler', i_msg);

	var i_data = i_msg.data;

	// color scheme - with config provided
	//
	var set_rex = String.raw`cmd\:\:setColorScheme\:name=(.+)\:config=(.+)`;
	var set_re = new RegExp(set_rex);
	var set_match = set_re.exec(i_data);
	if(set_match != null)
	{
		var color_scheme = set_match[1];
		var color_config = JSON.parse( set_match[2] );
		HD_COLOR_SCHEMES[color_scheme] = color_config;
		HD_ApplyColorScheme( HD_COLOR_SCHEMES[color_scheme] );
		console.debug("HABDEC Config ColorScheme ", color_scheme);
		// console.debug(HD_COLOR_SCHEMES);
	}
	else
	{
		// color scheme - predefined
		//
		set_rex = String.raw`cmd\:\:setColorScheme\:name=(.+)`;
		set_re = new RegExp(set_rex);
		set_match = set_re.exec(i_data);
		if(set_match != null)
		{
			var color_scheme = set_match[1];
			if( color_scheme in HD_COLOR_SCHEMES)
			{
				console.debug("HABDEC Predefined ColorScheme ", color_scheme);
				HD_ApplyColorScheme( HD_COLOR_SCHEMES[color_scheme] );
			}
		}
	}

	// server address
	//
	set_rex = String.raw`cmd\:\:setServer\:addr=(.+)`;
	set_re = new RegExp(set_rex);
	set_match = set_re.exec(i_data);
	if(set_match != null)
	{
		console.debug('HB_WinMsgHandler ', set_match);
		var addr = set_match[1];
		document.getElementById("HabDec_server_address").value = addr;
	}
}

function HABDEC_BUILD_UI(parent_div)
{
	var div_power = HABDEC_BUILD_UI_PowerSpectrum();
	var div_demod_and_ctrls = HABDEC_BUILD_UI_DemodAndControls();
	var div_extra_radio_buttons = HABDEC_BUILD_UI_ExtraRadioButtons();
	var div_server = HABDEC_BUILD_UI_Server();
	//<!-- <div id="PayloadsWrapperDiv"></div> -->

	// color schemes list
	var div_colors_wrapper = document.createElement("div");
	div_colors_wrapper.id = "ColorSchemesWrapperDiv";

	// fullscreen button
	var div_but_fs = document.createElement("div");
	var btnFullscreen = document.createElement("button");
	btnFullscreen.innerHTML = "Fullscreen";
	btnFullscreen.onclick = () => { toggleFullscreen() };
	div_but_fs.appendChild(btnFullscreen);

	// div for [colors, fillscreen] - in row
	var extra_options = document.createElement("div");
	extra_options.style.display = 'flex';
	extra_options.appendChild(div_colors_wrapper);
	extra_options.appendChild(div_but_fs);

	// parent_div.display.height = "1000px";
	parent_div.appendChild(div_power);
	parent_div.appendChild(div_demod_and_ctrls);
	parent_div.appendChild(div_extra_radio_buttons);
	parent_div.appendChild(div_server);
	// parent_div.appendChild(div_payloads_wrapper);
	// parent_div.appendChild(div_colors_wrapper);
	parent_div.appendChild(extra_options);

	CreateControls();
	CreateColorSchemesButton();

	window.addEventListener('message', HB_WinMsgHandler);

	// HD_ApplyeColorScheme( HD_COLOR_SCHEMES["DEFAULT"] );
}


function habdec_init(habdec_div, habdec_srv_url)
{
	HABDEC_BUILD_UI( document.getElementById(habdec_div) );

	if(habdec_srv_url)
		document.getElementById("HabDec_server_address").value = habdec_srv_url;

	document.getElementById("HabDec_server_address").value = localStorage.getItem('habdec_server_address');
	if(document.getElementById("HabDec_server_address").value == "")
		document.getElementById("HabDec_server_address").value = "localhost:5555"
	ResizeCanvas("HabDec_powerSpectrumCanvas");
	ResizeCanvas("HabDec_demodCanvas");

	HD_ApplyColorScheme(HD_COLOR_SCHEMES["DEFAULT"]);

	AnimatePowerSpectrum(0);
	AnimateDemod(0);

	setTimeout(	OpenConnection, 500);
}
