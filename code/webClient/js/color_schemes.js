
var HD_COLOR_SCHEME;
var HD_COLOR_SCHEMES = {};

HD_COLOR_SCHEMES["DEFAULT"] = {
    CSS: {
        HD_bg: 	            "hsl(210, 15%,  15%)",
        HD_fg: 	            "hsl(210, 15%,  25%)",
        HD_button: 	        "hsl(210, 15%,  25%)",
        HD_button_border:   "hsl(210, 15%,  15%)",
        HD_button_text: 	"hsl(0,   0%,   55%)",
        HD_label:      	    "hsl(32,  93%,  45%)",
        HD_highlight:    	"hsl(210, 80%,  45%)",
        HD_enabled:    	    "hsl(50, 100%, 40%)",
        HD_sentence:     	"hsl(0,   100%, 33%)"
    },
    SPECTRUM: {
        HIGH: "hsl(30, 30%, 100%)",
        MID: "hsl(30, 80%, 75%)",
        LOW: "hsl(30, 100%, 30%)",
        FILTER: "hsla(200, 50%, 30%, .5)"
    }
};

HD_COLOR_SCHEMES["RED"] = {
    CSS: {
        HD_bg: 	            "hsl(0, 0%,   0%)",
        HD_fg: 	            "hsl(0, 50%,  10%)",
        HD_button: 	        "hsl(0, 100%, 5%)",
        HD_button_border:   "hsl(0, 100%, 20%)",
        HD_button_text: 	"hsl(0, 100%, 60%)",
        HD_label:      	    "hsl(0, 100%, 45%)",
        HD_highlight:    	"hsl(0, 80%,  45%)",
        HD_enabled:    	    "hsl(0, 100%, 30%)",
        HD_sentence:     	"hsl(0, 100%, 33%)"
    },
    SPECTRUM: {
        HIGH: "hsl(0, 30%, 100%)",
        MID: "hsl(0, 80%, 55%)",
        LOW: "hsl(0, 100%, 10%)",
        FILTER: "hsla(0, 50%, 30%, .5)"
    }
};

HD_COLOR_SCHEMES["GREEN"] = {
    CSS: {
        HD_bg: 	            "hsl(120, 0%,   0%)",
        HD_fg: 	            "hsl(120, 50%,  10%)",
        HD_button: 	        "hsl(120, 100%, 2%)",
        HD_button_border:   "hsl(120, 100%, 15%)",
        HD_button_text: 	"hsl(120, 100%, 30%)",
        HD_label:      	    "hsl(120, 100%, 30%)",
        HD_highlight:    	"hsl(120, 80%,  35%)",
        HD_enabled:    	    "hsl(120, 100%, 20%)",
        HD_sentence:     	"hsl(120, 100%, 33%)"
    },
    SPECTRUM: {
        HIGH: "hsl(120, 30%, 100%)",
        MID: "hsl(120, 80%, 55%)",
        LOW: "hsl(120, 100%, 10%)",
        FILTER: "hsla(120, 50%, 30%, .5)"
    }
};

HD_COLOR_SCHEMES["BLUE"] = {
    CSS: {
        HD_bg: 	            "hsl(210, 0%,   0%)",
        HD_fg: 	            "hsl(210, 50%,  20%)",
        HD_button: 	        "hsl(210, 100%, 4%)",
        HD_button_border:   "hsl(210, 100%, 30%)",
        HD_button_text: 	"hsl(210, 100%, 60%)",
        HD_label:      	    "hsl(210, 100%, 60%)",
        HD_highlight:    	"hsl(210, 80%,  30%)",
        HD_enabled:    	    "hsl(210, 100%, 40%)",
        HD_sentence:     	"hsl(210, 100%, 75%)"
    },
    SPECTRUM: {
        HIGH: "hsl(210, 30%, 100%)",
        MID: "hsl(210, 80%, 75%)",
        LOW: "hsl(210, 100%, 20%)",
        FILTER: "hsla(210, 50%, 30%, .5)"
    }
};

HD_COLOR_SCHEMES["GOLD"] = {
    CSS: {
        HD_bg: 	            "hsl(40, 0%,   0%)",
        HD_fg: 	            "hsl(40, 50%,  20%)",
        HD_button: 	        "hsl(40, 100%, 4%)",
        HD_button_border:   "hsl(40, 100%, 30%)",
        HD_button_text: 	"hsl(40, 100%, 60%)",
        HD_label:      	    "hsl(40, 100%, 60%)",
        HD_highlight:    	"hsl(40, 80%,  30%)",
        HD_enabled:    	    "hsl(40, 100%, 40%)",
        HD_sentence:     	"hsl(40, 100%, 75%)"
    },
    SPECTRUM: {
        HIGH: "hsl(40, 30%, 100%)",
        MID: "hsl(40, 80%, 75%)",
        LOW: "hsl(40, 100%, 20%)",
        FILTER: "hsla(40, 50%, 30%, .5)"
    }
};

function HD_ApplyColorScheme(i_color_scheme) {
    HD_COLOR_SCHEME = i_color_scheme;
    let root = document.documentElement;
    for(prop in i_color_scheme['CSS'])
    {
        // document.documentElement.style.setProperty('--' + prop, i_color_scheme['CSS'][prop]);
        root.style.setProperty('--' + prop, i_color_scheme['CSS'][prop]);
    }

    SetGuiToGlobals(HD_GLOBALS);

}