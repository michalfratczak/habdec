
var G_HD_WEBSOCKET;
var G_HD_CONNECTED = 0;
var G_HD_LastHabSentences = [];
var G_SPECTRUM_DATA;
var G_SPECTRUM_ZOOM = 0;
var G_DEMOD_DATA;
var G_POWER_FPS = 40;
var G_DEMOD_FPS = 1;

var HD_GLOBALS =
{
	frequency: 0,
	sampling_rate: 0,
	gain: 0,
	baud: 0,
	rtty_bits: 0,
	rtty_stops: 0,
	lowpass_bw: 0,
	lowpass_trans: 0,
	biastee: 0,
	decimation: 0,
	afc: 0,
	dc_remove: 0,
	datasize: 1
};


function debug_print()
{
	// console.debug(arguments);
	/*
	document.getElementById("cnt_debug").innerHTML = "";
	for (i = 0; i < arguments.length; i++)
		document.getElementById("cnt_debug").innerHTML += arguments[i] + " ";
	*/
}


function OpenConnection()
{
	if(G_HD_CONNECTED)
		G_HD_WEBSOCKET.close();

	var server = document.getElementById("HabDec_server_address").value;
	if( server.toLowerCase().startsWith('ws://') )
		server = server.substr(5, server.length);
	server = 'ws://' + server;
	console.debug("Connecting to ", server, " ...");

	G_HD_WEBSOCKET = new WebSocket(server);
	G_HD_WEBSOCKET.binaryType = 'arraybuffer'; // or 'blob'

	G_HD_WEBSOCKET.onopen =    function(evt) { ws_onOpen(evt) };
	G_HD_WEBSOCKET.onclose =   function(evt) { ws_onClose(evt) };
	G_HD_WEBSOCKET.onmessage = function(evt) { ws_onMessage(evt) };
	G_HD_WEBSOCKET.onerror =   function(evt) { ws_onError(evt) };
}


function ws_onClose(evt)
{
	G_HD_CONNECTED = 0;
	debug_print("DISCONNECTED");
	setTimeout(function () { OpenConnection(); }, 5000);
}


function ws_onError(evt)
{
	debug_print("ws_onError: ", evt.data);
}


function ws_onOpen(evt)
{
	G_HD_CONNECTED = 1;
	debug_print("ws_onOpen: ", "Connected.");

	G_HD_WEBSOCKET.send("hi");
	for(var param in HD_GLOBALS)
		SendCommand("get:" + param);

	console.debug("ws_onOpen: init refresh.");
	RefreshPowerSpectrum();
	RefreshDemod();
	RefreshStats();
}


function ws_onMessage(evt)
{
	if(!G_HD_CONNECTED)
	{
		debug_print("ws_onMessage: not connected.");
		return;
	}

	// console.debug("ON_MSG", evt);

	if(typeof evt.data === "string")
	{
		HandleMessage(evt.data)
	}
	else if(evt.data instanceof ArrayBuffer)
	{
		var what = String.fromCharCode.apply( null, new Uint8Array(evt.data,0,4) );
		if(what == "PWR_")
		{
			G_SPECTRUM_DATA = DecodeSpectrum(evt.data, 4);
			var widget = document.getElementById("frequency");
			widget.setAttribute("step_big", G_SPECTRUM_DATA.sampling_rate_ / 1e6 / 30);
			widget.setAttribute("step_small", G_SPECTRUM_DATA.sampling_rate_ / 1e6 / 500);
			RefreshPowerSpectrum_lastReq = 0;
		}
		else if(what == "DEM_")
		{
			G_DEMOD_DATA = DecodeDemod(evt.data, 4);
			RefreshDemod_lastReq = 0;
		}
		else if(what == "SDV_") // SSDV jpeg
		{
			[callsing_str, image_id, jpeg_en64] = DecodeJpegBase64(evt.data, 4);
			console.debug("SSDV", callsing_str, image_id);
			var img = document.getElementById("HabDec_SSDV_Image");
			img.setAttribute('src', 'data:image/jpeg;base64,' + jpeg_en64);
			var tex= document.getElementById("HabDec_SSDV_Info");
			tex.innerHTML = callsing_str + " / " + image_id;
		}
	}
	else
	{
		debug_print("ws_onMessage: unknown data type.");
		return;
	}
}


function SendCommand(i_cmd)
{
	if(!G_HD_CONNECTED)
	{
		debug_print("SendCommand: not connected.");
		return;
	}
	var msg = "cmd::" + i_cmd;
	// debug_print("SendCommand: ", msg);
	G_HD_WEBSOCKET.send(msg);
}


function HandleMessage(i_data)
{
	if(!G_HD_CONNECTED)
	{
		debug_print("HandleMessage: not connected.");
		return;
	}

	if( !i_data.startsWith("cmd::info:liveprint") )
		debug_print("Received Message: ", i_data);

	// cmd::set
	//
	var set_rex = String.raw`cmd\:\:set\:(\D+)=(.+)`;
	var set_re = new RegExp(set_rex);
	var set_match = set_re.exec(i_data);
	if(set_match != null && set_match.length == 3)
	{
		if(set_match[1] == "frequency")
			HD_GLOBALS.frequency = parseFloat(set_match[2]);
		if(set_match[1] == "decimation")
			HD_GLOBALS.decimation = parseInt(set_match[2]);
		else if(set_match[1] == "gain")
			HD_GLOBALS.gain = parseFloat(set_match[2]);
		else if(set_match[1] == "baud")
			HD_GLOBALS.baud = parseInt(set_match[2]);
		else if(set_match[1] == "lowpass_bw")
			HD_GLOBALS.lowpass_bw = parseFloat(set_match[2]);
		else if(set_match[1] == "lowpass_trans")
			HD_GLOBALS.lowpass_trans = parseFloat(set_match[2]);
		else if(set_match[1] == "rtty_bits")
			HD_GLOBALS.rtty_bits = parseFloat(set_match[2]);
		else if(set_match[1] == "rtty_stops")
			HD_GLOBALS.rtty_stops = parseFloat(set_match[2]);
		else if(set_match[1] == "lowPass")
		{
			HD_GLOBALS.lowpass_bw = parseFloat(set_match[2].split(',')[0]);
			HD_GLOBALS.lowpass_trans = parseFloat(set_match[2].split(',')[1]);
		}
		else if(set_match[1] == "biastee")
		{
			HD_GLOBALS.biastee = parseFloat(set_match[2]);
			debug_print("Received Message: ", i_data);
		}
		else if(set_match[1] == "afc")
		{
			HD_GLOBALS.afc = parseFloat(set_match[2]);
			debug_print("Received Message: ", i_data);
		}
		else if(set_match[1] == "dc_remove")
		{
			HD_GLOBALS.dc_remove = parseFloat(set_match[2]);
			debug_print("Received Message: ", i_data);
		}
		else if(set_match[1] == "datasize")
		{
			HD_GLOBALS.datasize = parseFloat(set_match[2]);
			debug_print("Received Message: ", i_data);
		}

		SetGuiToGlobals(HD_GLOBALS);

		return true;
	}

	// cmd::info
	//
	var data_without_new_lines = i_data.replace(/(\r\n|\n|\r)/gm," ");
	var info_rex = String.raw`cmd\:\:info\:(\D+)=(.+)`;
	var info_re = new RegExp(info_rex);
	var info_match = info_re.exec(data_without_new_lines);
	// if (info_match && info_match.length == 3)
	if (info_match)
	{
		if(info_match[1] == "sentence")
		{
			var sntnc = info_match[2];

			var cnt_habsentence_list = document.getElementById("cnt_habsentence_list")
			cnt_habsentence_list.innerHTML = '<text style=\"color: rgb(0,200,0);\">' + sntnc + '</text>';;

			while(G_HD_LastHabSentences.length > 12)
				G_HD_LastHabSentences.pop();
			G_HD_LastHabSentences.forEach( function(i_snt){
				cnt_habsentence_list.innerHTML += '<br><text>' + i_snt + '</text>';
				}
			);

			G_HD_LastHabSentences.unshift( info_match[2] );

			SendCommand("stats");
		}
		else if(info_match[1] == "liveprint")
		{
			var _new = info_match[2];
			var _current = document.getElementById("cnt_liveprint").innerHTML;
			if(_current.length > 200)
				document.getElementById("cnt_liveprint").innerHTML = _new;
			else
				document.getElementById("cnt_liveprint").innerHTML = _current + _new;
		}
		else if(info_match[1] == "stats")
		{
			DisplayStats(info_match[2]);
		}

		return true;
	}

	return false;

}


function DisplayStats(i_str)
{
	var stats = {
		'ok': 0,
		'dist_line': 0,
		'dist_circ': 0,
		'max_dist': 0,
		'min_elev': 90,
		'age': -1
	};

	var stats_arr = i_str.split(",");

	for(i in stats_arr)
	{
		var tokens = stats_arr[i].split(":");
		var k = tokens[0];
		var v = tokens[1];

		if(k == 'ok')
			stats.ok = parseInt(v);
		if(k == 'dist_line')
			stats.dist_line = parseFloat(v);
		if(k == 'dist_circ')
			stats.dist_circ = parseFloat(v);
		if(k == 'max_dist')
			stats.max_dist = parseFloat(v);
		if(k == 'min_elev')
			stats.min_elev = parseFloat(v);
		if(k == 'age')
			stats.age = parseInt(v);
	}

	var stats_string = "Ok: " + stats.ok
		+ " | Dist-Line: " + (stats.dist_line / 1000).toFixed(1) + "km "
		+ "(" + (stats.max_dist / 1000).toFixed(1) + "km)"
		+ " | Dist-Circle: " + (stats.dist_circ / 1000).toFixed(1) + "km "
		+ " | MinElev: " + (stats.min_elev).toFixed(1)
		+ " | Age: " + stats.age;
	if(stats.age > 30)
		document.getElementById("cnt_stats").innerHTML = "<font color=red>" + stats_string + "</font>";
	else
		document.getElementById("cnt_stats").innerHTML = "<font color=yellow>" + stats_string + "</font>";

}


var RefreshPowerSpectrum_lastReq = 0; // limits number of requests to server
function RefreshPowerSpectrum()
{
	if(!G_HD_CONNECTED)
		return;

	// wait 250ms for last request to be realized
	var d = new Date();
	var now = d.getTime();
	if( 	RefreshPowerSpectrum_lastReq 				 /* last request not realized */
		&& ((now - RefreshPowerSpectrum_lastReq) < 250) /* under 250ms since last request */
		)
	{
		// console.debug("waiting ... ", (now - RefreshPowerSpectrum_lastReq), now);
		setTimeout(function () {RefreshPowerSpectrum();}, 1000 / G_POWER_FPS);
		return;
	}

	G_SPECTRUM_ZOOM = Math.max(0, Math.min(1, G_SPECTRUM_ZOOM));
	var zoom = Math.max(0, Math.min(1, G_SPECTRUM_ZOOM));
	var canvas = document.getElementById("HabDec_powerSpectrumCanvas");

	// resize if canvas iz zero-size
	if( 	!(canvas.offsetParent === null) /*not visible*/
		&& 	(!canvas.width || !canvas.height) ) /*zero size*/
	{
		canvas.width = canvas.parentElement.clientWidth;
		canvas.height = canvas.parentElement.clientHeight;
	}

	SendCommand("power:res=" + canvas.width + ",zoom=" + zoom);
	RefreshPowerSpectrum_lastReq = d.getTime();

	setTimeout(function () {RefreshPowerSpectrum();}, 1000 / G_POWER_FPS);
}


var RefreshDemod_lastReq = 0; // limits number of requests to server
function RefreshDemod()
{
	if(!G_HD_CONNECTED)
		return;

	// wait 250ms for last request to be realized
	var d = new Date();
	var now = d.getTime();
	if( 	RefreshDemod_lastReq 				 /* last request not realized */
		&& ((now - RefreshDemod_lastReq) < 250) /* under 250ms since last request */
		)
	{
		// console.debug("waiting ... ", (now - RefreshDemod_lastReq), now);
		setTimeout(function () {RefreshDemod();}, 1000 / G_DEMOD_FPS);
		return;
	}

	var canvas = document.getElementById("HabDec_demodCanvas");

	SendCommand("demod:res=" + canvas.width);
	RefreshDemod_lastReq = d.getTime();

	setTimeout(function () {RefreshDemod();}, 1000 / G_DEMOD_FPS);
}


// stats are send from server on every success sentence
// but update every 2 seconds to display sentence age
function RefreshStats()
{
	if(!G_HD_CONNECTED)
		return;
	SendCommand("stats");
	setTimeout(function () {RefreshStats();}, 2000);
}


function SetBiasT()
{
	var value = HD_GLOBALS.biastee;
	if(value)
	{
		SendCommand("set:biastee=0");
		var button = document.getElementById("HabDecD_biastee");
		button.style.backgroundColor = "hsl(210, 15%, 34%)";
		button.style.color = "#AAA";
	}
	else
	{
		SendCommand("set:biastee=1");
		var button = document.getElementById("HabDecD_biastee");
		button.style.backgroundColor = "#bb0";
		button.style.color = "#000";
	}
}


function SetAFC()
{
	var value = HD_GLOBALS.afc
	if(value)
	{
		SendCommand("set:afc=0");
		var button = document.getElementById("HabDec_afc");
		button.style.backgroundColor = "hsl(210, 15%, 34%)";
		button.style.color = "#AAA";
	}
	else
	{
		SendCommand("set:afc=1");
		var button = document.getElementById("HabDec_afc");
		button.style.backgroundColor = "#bb0";
		button.style.color = "#000";
	}
}

function SetDCRemove()
{
	var value = HD_GLOBALS.dc_remove;
	if(value)
	{
		SendCommand("set:dc_remove=0");
		var button = document.getElementById("HabDec_dc_remove");
		button.style.backgroundColor = "hsl(210, 15%, 34%)";
		button.style.color = "#AAA";
	}
	else
	{
		SendCommand("set:dc_remove=1");
		var button = document.getElementById("HabDec_dc_remove");
		button.style.backgroundColor = "#bb0";
		button.style.color = "#000";
	}
}
