
var G_HD_WEBSOCKET;
var G_HD_CONNECTED = 0;
var LastHabSentences = [];
var G_SPECTRUM_DATA;
var G_SPECTRUM_ZOOM = 0;
var G_DEMOD_DATA;
var G_SENTECES_OK_COUNT = 0;

var GLOBALS =
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
		return;

	var server = document.getElementById("server_address").value;
	console.debug("Connecting to ", server, " ...");
	G_HD_WEBSOCKET = new WebSocket("ws://" + server);
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
	for(var param in GLOBALS)
		SendCommand("get:" + param);

	console.debug("ws_onOpen: init refresh.");
	RefreshPowerSpectrum();
	RefreshDemod();
	RefreshLivePrint();
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
			GLOBALS.frequency = parseFloat(set_match[2]);
		if(set_match[1] == "decimation")
			GLOBALS.decimation = parseInt(set_match[2]);
		else if(set_match[1] == "gain")
			GLOBALS.gain = parseFloat(set_match[2]);
		else if(set_match[1] == "baud")
			GLOBALS.baud = parseInt(set_match[2]);
		else if(set_match[1] == "lowpass_bw")
			GLOBALS.lowpass_bw = parseFloat(set_match[2]);
		else if(set_match[1] == "lowpass_trans")
			GLOBALS.lowpass_trans = parseFloat(set_match[2]);
		else if(set_match[1] == "rtty_bits")
			GLOBALS.rtty_bits = parseFloat(set_match[2]);
		else if(set_match[1] == "rtty_stops")
			GLOBALS.rtty_stops = parseFloat(set_match[2]);
		else if(set_match[1] == "lowPass")
		{
			GLOBALS.lowpass_bw = parseFloat(set_match[2].split(',')[0]);
			GLOBALS.lowpass_trans = parseFloat(set_match[2].split(',')[1]);
		}
		else if(set_match[1] == "biastee")
		{
			GLOBALS.biastee = parseFloat(set_match[2]);
			debug_print("Received Message: ", i_data);
		}
		else if(set_match[1] == "afc")
		{
			GLOBALS.afc = parseFloat(set_match[2]);
			debug_print("Received Message: ", i_data);
		}
		else if(set_match[1] == "dc_remove")
		{
			GLOBALS.dc_remove = parseFloat(set_match[2]);
			debug_print("Received Message: ", i_data);
		}
		else if(set_match[1] == "datasize")
		{
			GLOBALS.datasize = parseFloat(set_match[2]);
			debug_print("Received Message: ", i_data);
		}

		SetGuiToGlobals(GLOBALS);

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
			G_SENTECES_OK_COUNT += 1;
			document.getElementById("cnt_habsentence_count").innerHTML = "OK: " + G_SENTECES_OK_COUNT;

			var sntnc = info_match[2];

			var cnt_habsentence_list = document.getElementById("cnt_habsentence_list")
			cnt_habsentence_list.innerHTML = '<text style=\"color: rgb(0,200,0);\">' + sntnc + '</text>';;

			while(LastHabSentences.length > 12)
				LastHabSentences.pop();
			LastHabSentences.forEach( function(i_snt){
				cnt_habsentence_list.innerHTML += '<br><text>' + i_snt + '</text>';
				}
			);

			LastHabSentences.unshift( info_match[2] );
		}
		else if(info_match[1] == "liveprint")
		{
			var livestream = info_match[2];
			if(livestream.length > 100)
				livestream = livestream.substr(livestream.length-100, livestream.length);
			document.getElementById("cnt_liveprint").innerHTML = livestream;
		}

		return true;
	}

	return false;

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
	var canvas = document.getElementById("powerSpectrumCanvas");

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

	var canvas = document.getElementById("demodCanvas");

	SendCommand("demod:res=" + canvas.width);
	RefreshDemod_lastReq = d.getTime();

	setTimeout(function () {RefreshDemod();}, 1000 / G_DEMOD_FPS);
}


function RefreshLivePrint()
{
	if(!G_HD_CONNECTED)
		return;
	SendCommand("liveprint");
	setTimeout(function () {RefreshLivePrint();}, 1000 / 4);
}


function SetBiasT()
{
	var value = GLOBALS.biastee;
	if(value)
	{
		SendCommand("set:biastee=0");
		var button = document.getElementById("HD_biastee");
		button.style.backgroundColor = "hsl(210, 15%, 34%)";
		button.style.color = "#AAA";
	}
	else
	{
		SendCommand("set:biastee=1");
		var button = document.getElementById("HD_biastee");
		button.style.backgroundColor = "#bb0";
		button.style.color = "#000";
	}
}


function SetAFC()
{
	var value = GLOBALS.afc
	if(value)
	{
		SendCommand("set:afc=0");
		var button = document.getElementById("HD_afc");
		button.style.backgroundColor = "hsl(210, 15%, 34%)";
		button.style.color = "#AAA";
	}
	else
	{
		SendCommand("set:afc=1");
		var button = document.getElementById("HD_afc");
		button.style.backgroundColor = "#bb0";
		button.style.color = "#000";
	}s
}

function SetDCRemove()
{
	var value = GLOBALS.dc_remove;
	if(value)
	{
		SendCommand("set:dc_remove=0");
		var button = document.getElementById("HD_dc_remove");
		button.style.backgroundColor = "hsl(210, 15%, 34%)";
		button.style.color = "#AAA";
	}
	else
	{
		SendCommand("set:dc_remove=1");
		var button = document.getElementById("HD_dc_remove");
		button.style.backgroundColor = "#bb0";
		button.style.color = "#000";
	}
}

function SetPayload(i_payload_id)
{
	SendCommand("set:payload=" + i_payload_id);
}
