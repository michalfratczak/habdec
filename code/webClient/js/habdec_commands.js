
var websocket;
var connected = 0;
var LastHabSentences = [];
var G_SPECTRUM_ZOOM = 0;
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
	dc_remove: 0
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
	if(connected)
		return;

	var server = document.getElementById("server_address").value;
	debug_print("Connecting to ", server, " ...");
	websocket = new WebSocket("ws://" + server);
	websocket.binaryType = 'arraybuffer'; // or 'blob'

	websocket.onopen =    function(evt) { ws_onOpen(evt) };
	websocket.onclose =   function(evt) { ws_onClose(evt) };
	websocket.onmessage = function(evt) { ws_onMessage(evt) };
	websocket.onerror =   function(evt) { ws_onError(evt) };
}


function ws_onClose(evt)
{
	connected = 0;
	debug_print("DISCONNECTED");
	setTimeout(function () { OpenConnection(); }, 5000);
}


function ws_onError(evt)
{
	debug_print("ws_onError: ", evt.data);
}


function ws_onOpen(evt)
{
	connected = 1;
	debug_print("ws_onOpen: ", "Connected.");

	websocket.send("hi");
	for(var param in GLOBALS)
		SendCommand("get:" + param);

	console.debug("ws_onOpen: init refresh.");
	RefreshPowerSpectrum();
	RefreshDemod();
	RefreshLivePrint();
}


function ws_onMessage(evt)
{
	if(!connected)
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
			var spectrum = DecodeSpectrum(evt.data, 4);
			if(GLOBALS.sampling_rate != spectrum.sampling_rate_)
			{
				GLOBALS.sampling_rate = spectrum.sampling_rate_;
				var widget = document.getElementById("frequency");
				widget.setAttribute("step_big", GLOBALS.sampling_rate / 1e6 / 30);
				widget.setAttribute("step_small", GLOBALS.sampling_rate / 1e6 / 500);
			}
			DrawPowerSpectrum(document.getElementById("powerSpectrumCanvas"), spectrum);
			RefreshPowerSpectrum_lastReq = 0;

		}
		else if(what == "DEM_")
		{
			var demod = DecodeDemod(evt.data, 4);
			DrawDemod(document.getElementById("demodCanvas"), demod);
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
	if(!connected)
	{
		debug_print("SendCommand: not connected.");
		return;
	}
	var msg = "cmd::" + i_cmd;
	// debug_print("SendCommand: ", msg);
	websocket.send(msg);
}


function HandleMessage(i_data)
{
	if(!connected)
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
	if(!connected)
		return;

	var FPS = 40; // frames per second refresh

	// wait 250ms for last request to be realized
	var d = new Date();
	var now = d.getTime();
	if( 	RefreshPowerSpectrum_lastReq 				 /* last request not realized */
		&& ((now - RefreshPowerSpectrum_lastReq) < 250) /* under 250ms since last request */
		)
	{
		// console.debug("waiting ... ", (now - RefreshPowerSpectrum_lastReq), now);
		setTimeout(function () {RefreshPowerSpectrum();}, 1000 / FPS);
		return;
	}

	G_SPECTRUM_ZOOM = Math.max(0, Math.min(1, G_SPECTRUM_ZOOM));
	var zoom = Math.max(0, Math.min(1, G_SPECTRUM_ZOOM));
	var canvas = document.getElementById("powerSpectrumCanvas");

	SendCommand("power:res=" + canvas.width + ",zoom=" + zoom);
	RefreshPowerSpectrum_lastReq = d.getTime();

	setTimeout(function () {RefreshPowerSpectrum();}, 1000 / FPS);
}


var RefreshDemod_lastReq = 0; // limits number of requests to server
function RefreshDemod()
{
	if(!connected)
		return;

	var FPS = 4; // frames per second refresh

	// wait 250ms for last request to be realized
	var d = new Date();
	var now = d.getTime();
	if( 	RefreshDemod_lastReq 				 /* last request not realized */
		&& ((now - RefreshDemod_lastReq) < 250) /* under 250ms since last request */
		)
	{
		// console.debug("waiting ... ", (now - RefreshDemod_lastReq), now);
		setTimeout(function () {RefreshDemod();}, 1000 / FPS);
		return;
	}

	var canvas = document.getElementById("demodCanvas");

	SendCommand("demod:res=" + canvas.width);
	RefreshDemod_lastReq = d.getTime();

	setTimeout(function () {RefreshDemod();}, 1000 / FPS);
}


function RefreshLivePrint()
{
	if(!connected)
		return;
	SendCommand("liveprint");
	setTimeout(function () {RefreshLivePrint();}, 1000 / 4);
}


function SetBiasT()
{
	var value = document.getElementById("biastee").checked;
	if(value)
		SendCommand("set:biastee=1");
	else
		SendCommand("set:biastee=0");
}


function SetAFC()
{
	var value = document.getElementById("afc").checked;
	if(value)
		SendCommand("set:afc=1");
	else
		SendCommand("set:afc=0");
}

function SetDCRemove()
{
	var value = document.getElementById("dc_remove").checked;
	if(value)
		SendCommand("set:dc_remove=1");
	else
		SendCommand("set:dc_remove=0");
}
