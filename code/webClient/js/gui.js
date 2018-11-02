
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
						SendCommand,  .0, 1.0, .05, .002, .02);
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
