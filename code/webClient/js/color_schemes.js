
var HD_COLOR_SCHEMES = {};

HD_COLOR_SCHEMES["DEFAULT"] = {
    BG: "hsl(210, 15%, 15%)",
    FG: "hsl(210, 15%, 25%)",
    BUTTON_TEXT: "hsl(0, 0%, 65%)",
    ENABLED: "hsl(50, 100%, 40%)",
    LABEL: "hsl(32, 93%, 45%)",
    FOCUS: "hsl(10, 15%, 64%)",
    SENTENCE: "hsl(210, 50%, 80%)",
    SPECTRUM: {
        HIGH: "hsl(30, 30%, 100%)",
        MID: "hsl(30, 80%, 75%)",
        LOW: "hsl(30, 100%, 30%)",
        FILTER: "hsla(200, 50%, 30%, .5)"
    }
};

HD_COLOR_SCHEMES["RED"] = {
    BG: "hsl(0, 0%, 0%)",
    FG: "hsl(0, 100%, 12%)",
    ENABLED: "hsl(10, 100%, 30%)",
    LABEL: "hsl(0, 100%, 40%)",
    BUTTON_TEXT: "hsl(0, 100%, 30%)",
    FOCUS: "hsl(260, 15%, 64%)",
    SENTENCE: "hsl(0, 100%, 50%)",
    SPECTRUM: {
        HIGH: "hsl(0, 50%, 100%)",
        MID: "hsl(0, 80%, 50%)",
        LOW: "hsl(0, 100%, 10%)",
        FILTER: "hsla(0, 15%, 30%, .5)"
    }
}

HD_COLOR_SCHEMES["GREEN"] = {
    BG: "hsl(120, 0%, 0%)",
    FG: "hsl(120, 100%, 12%)",
    ENABLED: "hsl(150, 100%, 30%)",
    LABEL: "hsl(120, 100%, 40%)",
    BUTTON_TEXT: "hsl(120, 100%, 30%)",
    FOCUS: "hsl(300, 15%, 64%)",
    SENTENCE: "hsl(120, 100%, 50%)",
    SPECTRUM: {
        HIGH: "hsl(120, 50%, 100%)",
        MID: "hsl(120, 80%, 50%)",
        LOW: "hsl(120, 100%, 10%)",
        FILTER: "hsla(120, 15%, 30%, .5)"
    }
}

HD_COLOR_SCHEMES["BLUE"] = {
    BG: "hsl(190, 0%, 0%)",
    FG: "hsl(190, 100%, 16%)",
    ENABLED: "hsl(150, 100%, 40%)",
    LABEL: "hsl(190, 100%, 40%)",
    BUTTON_TEXT: "hsl(190, 100%, 40%)",
    FOCUS: "hsl(300, 15%, 75%)",
    SENTENCE: "hsl(190, 100%, 60%)",
    SPECTRUM: {
        HIGH: "hsl(190, 50%, 100%)",
        MID: "hsl(190, 80%, 50%)",
        LOW: "hsl(190, 100%, 15%)",
        FILTER: "hsla(120, 15%, 40%, .5)"
    }
}

var HD_COLOR_SCHEME = HD_COLOR_SCHEMES["DEFAULT"];

function HD_ApplyColorScheme(i_color_scheme) {
    console.debug("HD_ApplyColorScheme");

    HD_COLOR_SCHEME = i_color_scheme;

    var elems;
    var color_str = "";

    //BG
    color_str = HD_COLOR_SCHEME['BG'];
    document.getElementsByTagName('body')[0].style.backgroundColor = color_str;
    document.getElementsByTagName('html')[0].style.backgroundColor = color_str;

    //FG - mostly buttons
    color_str = HD_COLOR_SCHEME['FG'];
    elems = document.getElementsByTagName('button');
    for(var i in elems) 
        if(elems[i].style)
            elems[i].style.backgroundColor = color_str;
    elems = document.getElementsByClassName('increment_button PayloadsDropButton');
    for(var i in elems) 
        if(elems[i].style)
            elems[i].style.backgroundColor = color_str;
    
    //LABEL
    color_str = HD_COLOR_SCHEME['LABEL'];
    elems = document.getElementsByTagName('label')
    for(var i in elems) 
        if(elems[i].style)
            elems[i].style.color = color_str;
    elems = document.getElementsByClassName('ctrl_box');
    for(var i in elems)
        if(elems[i].style)
            elems[i].style.color = color_str;
        
    //BUTTON_TEXT
    color_str = HD_COLOR_SCHEME['BUTTON_TEXT'];
    elems = document.getElementsByTagName('button');
    for(var i in elems) 
        if(elems[i].style)
            elems[i].style.color = color_str;
    
    //FOCUS
    color_str = HD_COLOR_SCHEME['FOCUS'];
    elems = document.getElementsByTagName('button');
    for(var i in elems) 
        if(elems[i].style)
         {
            elems[i].style.hover = color_str;
            elems[i].style.focus = color_str;
        }
    
    //SENTENCE
    color_str = HD_COLOR_SCHEME['SENTENCE'];
    elems = document.getElementsByClassName('habsentence_text');
    for(var i in elems) 
        if(elems[i].style)
            elems[i].style.color = color_str;
    
    SetGuiToGlobals();
}