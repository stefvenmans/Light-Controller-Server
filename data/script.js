// Complete project details: https://randomnerdtutorials.com/esp32-web-server-websocket-sliders/

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getValues(){
    websocket.send("getValues");
}

function getOnTime(){
    websocket.send("getOnTime");
}

function getCurrentVoltage(){
    websocket.send("getCurrentVoltage");
}

function getVoltageValues(){
    websocket.send("getVoltageValues");
}



function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    console.log("Gateway:" + gateway)
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getValues();
    getOnTime();
    getCurrentVoltage();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function updateSliderPWM(element) {
    var sliderNumber = element.id.charAt(element.id.length-1);
    var sliderValue = document.getElementById(element.id).value;
    document.getElementById("sliderValue"+sliderNumber).innerHTML = sliderValue;
    console.log(sliderValue);
    websocket.send(sliderNumber+"s"+sliderValue.toString());
}

function onMessage(event) {
    console.log(event.data);
    
    var myArray = event.data.split("method=");
    if(myArray.length>1){
        myArray = myArray[1].split(";");
        
        if(myArray[0] == "onTime"){
            console.log("update time");
            console.log(myArray[1]);
            document.getElementById("onTime").innerHTML = myArray[1]
        }
        else if(myArray[0] == "currentVoltage"){
            console.log("update voltage");
            console.log(myArray[1]);
            document.getElementById("currentVoltage").innerHTML = myArray[1]
        }
    }
    else{
        var myObj = JSON.parse(event.data);
        var keys = Object.keys(myObj);

        for (var i = 0; i < keys.length; i++){
            var key = keys[i];
            document.getElementById(key).innerHTML = myObj[key];
            document.getElementById("slider"+ (i+1).toString()).value = myObj[key];
        }
    }
}
