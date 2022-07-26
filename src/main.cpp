/* 
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-web-server-websocket-sliders/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include <ESPmDNS.h>
//#include <Wire.h>

typedef enum {UPDATE_PWM} MessageType;

struct I2CMessage {
  int8_t type;
  int16_t val;
};


// Replace with your network credentials
const char* ssid = "ESP";
const char* password = "bart&stef";
// const char* ssid = "Venmans";
// const char* password = "wkhow81984";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object

AsyncWebSocket ws("/ws");
// Set LED GPIO
const int ledPin1 = 2;
const int ledPin2 = 5;
const int ledPin3 = 14;

String message = "";
String sliderValue1 = "0";
String sliderValue2 = "0";
String sliderValue3 = "0";

int dutyCycle1;
int dutyCycle2;
int dutyCycle3;

// setting PWM properties
const int freq = 5000;
const int ledChannel1 = 0;
const int ledChannel2 = 1;
const int ledChannel3 = 2;

const int resolution = 8;

int potValue = 0;
int batteryVoltage[30];
int batteryVoltageIndex = 0;
int prevTime = 0;
float currentBatteryVoltage = 0;
float prevBatteryVoltage = 0;

int prevTimeButton1 = 0;
int prevTimeButton2 = 0;

float mapfloat(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (float)((x - in_min) * (out_max - out_min)) / (float)((in_max - in_min) + out_min);
}

//Json Variable to Hold Slider Values
JSONVar sliderValues;

//Get Slider Values
String getSliderValues(){
  sliderValues["sliderValue1"] = String(sliderValue1);
  sliderValues["sliderValue2"] = String(sliderValue2);
  sliderValues["sliderValue3"] = String(sliderValue3);

  String jsonString = JSON.stringify(sliderValues);
  return jsonString;
}

// Initialize SPIFFS
void initFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else{
   Serial.println("SPIFFS mounted successfully");
  }
}

// Initialize WiFi
void initWiFi() {
  // WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);
  // Serial.print("Connecting to WiFi ..");
  // while (WiFi.status() != WL_CONNECTED) {
  //   Serial.print('.');
  //   delay(1000);
  // }
  // Serial.println(WiFi.localIP());
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

}

void notifyClients(String sliderValues) {
  ws.textAll(sliderValues);
}

volatile int interruptCounter;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t * timer = NULL;
int totalInterruptCounter;

bool updatingDutyCycle[] = {false, false};
int valueNewDutyCycle[] = {0,0};

void IRAM_ATTR onTimer(){
  if(updatingDutyCycle[0] == true){
    if(dutyCycle1>valueNewDutyCycle[0]) dutyCycle1--;
    else dutyCycle1++;
    if(dutyCycle1 == valueNewDutyCycle[0]){
      updatingDutyCycle[0] = false;
      //timerAlarmDisable(timer);
    }
  }
  if(updatingDutyCycle[1] == true){
    if(dutyCycle2>valueNewDutyCycle[1]) dutyCycle2--;
    else dutyCycle2++;
    if(dutyCycle2 == valueNewDutyCycle[1]){
      updatingDutyCycle[1] = false;
      //timerAlarmDisable(timer);
    }
  }
}

void updateDutyCycle(int channel, int value){
  valueNewDutyCycle[channel] = value;
  updatingDutyCycle[channel] = true;
  //timerAlarmEnable(timer);
}


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*)data;
    if (message.indexOf("1s") >= 0) {
      sliderValue1 = message.substring(2);
      //dutyCycle1 = map(sliderValue1.toInt(), 0, 100, 0, 255);


      I2CMessage mesg;
      mesg.type = 100;
      mesg.val = map(sliderValue1.toInt(), 0, 100, 0, 255);

      Serial.print("Size of : " );
      Serial.println(sizeof(MessageType));

      // Wire.beginTransmission(4); // transmit to device #4
      // Wire.write((byte *)&mesg, sizeof(I2CMessage));        // sends five bytes
      // //Wire.write(101);              // sends one byte  
      // Wire.endTransmission();    // stop transmitting
      updateDutyCycle(0,map(sliderValue1.toInt(), 100, 0, 0, 255));
      
      Serial.println(dutyCycle1);
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
      if(sliderValue1.toInt()>50){
        //ESP.deepSleep(10e6);
      }
    }
    if (message.indexOf("2s") >= 0) {
      sliderValue2 = message.substring(2);
      //dutyCycle2 = map(sliderValue2.toInt(), 0, 100, 0, 255);
      I2CMessage mesg;
      mesg.type = 101;
      mesg.val = map(sliderValue2.toInt(), 0, 100, 0, 255);

      Serial.print("Size of : " );
      Serial.println(sizeof(MessageType));

      // Wire.beginTransmission(4); // transmit to device #4
      // Wire.write((byte *)&mesg, sizeof(I2CMessage));        // sends five bytes
      // //Wire.write(101);              // sends one byte  
      // Wire.endTransmission();    // stop transmitting     

      updateDutyCycle(1,map(sliderValue2.toInt(), 100, 0, 0, 255));
      Serial.println(dutyCycle2);
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
    }    
    if (message.indexOf("3s") >= 0) {
      sliderValue3 = message.substring(2);
      dutyCycle3 = map(sliderValue3.toInt(), 0, 100, 0, 255);
      Serial.println(dutyCycle3);
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
    }
    if (strcmp((char*)data, "getValues") == 0) {
      notifyClients(getSliderValues());
    }

    if(strcmp((char*)data, "getOnTime") == 0){
      String returnString = "method=onTime;";
      returnString += millis();
      notifyClients(returnString);
      
    }

    if(strcmp((char*)data, "getCurrentVoltage") == 0){
      String returnString = "method=currentVoltage;";

      char formattedVoltage[10];
      sprintf(formattedVoltage,"%.2fV",currentBatteryVoltage);
      returnString += String(formattedVoltage);
      
      notifyClients(returnString);
      
    }

    Serial.println((char*)data);

  }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

String getBatteryVoltageValues(){
  String result;
  int timeInstance = 0;
  for(int i=batteryVoltageIndex+1; i<30; i++){
      result = result + timeInstance+"s:  ";
      result = result + batteryVoltage[i];
      result = result + "<br>";
      timeInstance++;
  }
  for(int i=0; i<batteryVoltageIndex+1; i++){
      result = result + timeInstance+"s:  ";
      result = result + batteryVoltage[i];
      result = result + "<br>";
      timeInstance++;
  }
  return result;
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}







void setup() {
  Serial.begin(115200);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);

  pinMode(25, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);

  initFS();
  initWiFi();
  //Wire.begin(); // join i2c bus (address optional for master)
  
  // if(!MDNS.begin("smartven")) {
  //    Serial.println("Error starting mDNS");
  //    return;
  // }

  // configure LED PWM functionalitites
  ledcSetup(ledChannel1, freq, resolution);
  ledcSetup(ledChannel2, freq, resolution);
  ledcSetup(ledChannel3, freq, resolution);

  // attach the channel to the GPIO to be controlled
  ledcAttachPin(ledPin1, ledChannel1);
  ledcAttachPin(ledPin2, ledChannel2);
  ledcAttachPin(ledPin3, ledChannel3);

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 2000, true);
  timerAlarmEnable(timer);

  initWebSocket();
  
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Web Server Root URL
  server.on("/values", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", "101");
  });

  // Web Server Root URL
  server.on("/voltage", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", getBatteryVoltageValues().c_str());
    Serial.println(getBatteryVoltageValues());
  });

  
  
  server.serveStatic("/", SPIFFS, "/");

  // Start server
  server.begin();

}

void loop() {
  ledcWrite(ledChannel1, dutyCycle1);
  ledcWrite(ledChannel2, dutyCycle2);
  ledcWrite(ledChannel3, dutyCycle3);

  ws.cleanupClients();
  //int tempVal = analogRead(A0);
  // if(tempVal < potValue-16 || tempVal > potValue+16){
  //   sliderValue1 = tempVal>>5;
  //   potValue = tempVal;
  //   notifyClients(getSliderValues());
  //   Serial.println("Slider updated because of pot");
  //   delay(100);
  // }


  //Serial.println(digitalRead(25));

  if(digitalRead(25)==0 && millis()-prevTimeButton1>500){
    if(sliderValue1 == "0"){
      sliderValue1 = "100";
    }
    else sliderValue1 = "0";
    
    //dutyCycle1=0;
    updateDutyCycle(0,map(sliderValue1.toInt(), 100, 0, 0, 255));
    Serial.print("light 1 to 0");
    Serial.println(dutyCycle1);
    notifyClients(getSliderValues());
    prevTimeButton1 = millis();
    
  }

  if(digitalRead(26)==0 && millis()-prevTimeButton2>500){
    if(sliderValue2 == "0"){
      sliderValue2 = "100";
    }
    else sliderValue2 = "0";
      updateDutyCycle(1,map(sliderValue2.toInt(), 100, 0, 0, 255));
      //dutyCycle2=0;
      Serial.print("light 2 to ");
    Serial.println(sliderValue2);
      notifyClients(getSliderValues());
      prevTimeButton2 = millis();
      
    }

  if(millis()-prevTime>1000){
    //Serial.println("hello ");
    prevTime = millis();
    batteryVoltage[batteryVoltageIndex]=analogRead(A0);
    potValue = batteryVoltage[batteryVoltageIndex];
    // Serial.print("Analog in: ");
    // Serial.println(potValue);
    // Serial.print("Mapped value: ");
    
    


    currentBatteryVoltage = ((float)((potValue - 0) * (3.3 - 0)) / (float)((4095 - 0) + 0))*11.0;;//*((100+534.615)/(100));
    if(currentBatteryVoltage != prevBatteryVoltage){
      String returnString = "method=currentVoltage;";

      char formattedVoltage[10];
      sprintf(formattedVoltage,"%.2fV",currentBatteryVoltage);
      returnString += String(formattedVoltage);
      
      notifyClients(returnString);
    }
    prevBatteryVoltage = currentBatteryVoltage;
    // Serial.println(mapfloat(potValue,0,4095, 0, 3.3));
    // Serial.println(currentBatteryVoltage);
    batteryVoltageIndex--;
    if(batteryVoltageIndex<0)batteryVoltageIndex = 29;
  }

  //write value to flash

}
