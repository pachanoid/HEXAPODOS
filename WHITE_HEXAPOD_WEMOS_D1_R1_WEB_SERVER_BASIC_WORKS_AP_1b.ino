/* WHITE HEXAPOD WEMOS D1 R1 WEB SERVER
 *  
 *  https://arduino.esp8266.com/stable/package_esp8266com_index.json
 * ESP8266 as Web Server using WiFi Access Point (STA) mode
 * 
 * Open browser, visit 192.168.1.178:88
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Hash.h>

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>  
//#include "SSD1306.h"
#include <SoftwareSerial.h>

//SSID and Password to your ESP Access Point
const char* ssid = "WHITE HEXAPOD";
const char* password = "w1234567890";


SoftwareSerial BlueTooth(D5,D6);  // connect bluetooth module Tx=D5=Yellow wire Rx=D6=naranjo
 
#define REC_FRAMEMILLIS 100     // time between data frames when recording/playing 
 
char CurCmd;
char CurSubCmd;
char CurDpad;
unsigned int BeepFreq = 0;   // frequency of next beep command, 0 means no beep, should be range 50 to 2000 otherwise
unsigned int BeepDur = 0;    // duration of next beep command, 0 means no beep, should be in range 1 to 30000 otherwise
                             // although if you go too short, like less than 30, you'll hardly hear anything

unsigned long NextTransmitTime = 0;  // next time to send a command to the robot
char PlayLoopMode = 0;


void setBeep(int f, int d) {
  // schedule a beep to go out with the next transmission
  // this is not quite right as there can only be one beep per transmission
  // right now so if two different subsystems wanted to beep at the same time
  // whichever one is scheduled last would win. 
  // But because 10 transmits go out per second this seems sufficient, and it's simple
  BeepFreq = f;
  BeepDur = d;
}
int sendbeep(int noheader) {

    unsigned int beepfreqhigh = highByte(BeepFreq);
    unsigned int beepfreqlow = lowByte(BeepFreq);
    if (!noheader) {
      BlueTooth.print("B");
    }
    BlueTooth.write(beepfreqhigh);
    BlueTooth.write(beepfreqlow);

    unsigned int beepdurhigh = highByte(BeepDur);
    unsigned int beepdurlow = lowByte(BeepDur);
    BlueTooth.write(beepdurhigh);
    BlueTooth.write(beepdurlow);

    // return checksum info
    if (noheader) {
      return beepfreqhigh+beepfreqlow+beepdurhigh+beepdurlow;
    } else {
      return 'B'+beepfreqhigh+beepfreqlow+beepdurhigh+beepdurlow;
    }
}



char Data;

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>Hexapod Robot</title>
<style>
"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }"
#JD {
  text-align: center;
}
#JD {
  text-align: center;
  font-family: "Lucida Sans Unicode", "Lucida Grande", sans-serif;
  font-size: 24px;
}
.foot {
  text-align: center;
  font-family: "Comic Sans MS", cursive;
  font-size: 30px;
  color: #F00;
}
.button {
    border: none;
    color: white;
    padding: 17px;
    text-align: center;
    text-decoration: none;
    display: inline-block;
    font-size: 16px;
    margin: 4px 2px;
    cursor: pointer;
    border-radius: 12px;
  width: 100%;
}
.red {background-color: #F00;}
.green {background-color: #090;}
.yellow {background-color:#F90;}
.blue {background-color:#03C;}
</style>
<script>
var websock;
function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
  websock.onopen = function(evt) { console.log('websock open'); };
  websock.onclose = function(evt) { console.log('websock close'); };
  websock.onerror = function(evt) { console.log(evt); };
  websock.onmessage = function(evt) {
    console.log(evt);
    var e = document.getElementById('ledstatus');
    if (evt.data === 'ledon') {
      e.style.color = 'red';
    }
    else if (evt.data === 'ledoff') {
      e.style.color = 'black';
    }
    else {
      console.log('unknown event');
    }
  };
}
function buttonclick(e) {
  websock.send(e.id);
}
</script>
</head>
<body onload="javascript:start();">
&nbsp;
<table width="100%" border="1">
  <tr>
    <td bgcolor="#FFFF33" id="JD">Hexapod Robot Controller</td>
  </tr>

</table>
<table width="100" height="249" border="0" align="center">
<tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> 
<tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> 
            
    <td>&nbsp;</td>
    <td align="center" valign="middle"><form name="form1" method="post" action="">
      <label>
        <button id="F"  type="button" onclick="buttonclick(this);" class="button green">Forward</button> 
      </label>
    </form></td>
    <td>&nbsp;</td>
  </tr>
<tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> 
   
    <td align="center" valign="middle"><form name="form1" method="post" action="">
      <label>
        <button id="L"  type="button" onclick="buttonclick(this);" class="button green">Turn_Left</button> 
      </label>
    </form></td>
    <td align="center" valign="middle"><form name="form1" method="post" action="">
      <label>
        <button id="S"  type="button" onclick="buttonclick(this);" class="button red">Stop</button> 
      </label>
    </form></td>
    <td align="center" valign="middle"><form name="form1" method="post" action="">
      <label>
        <button id="R"  type="button" onclick="buttonclick(this);" class="button green">Turn_Right</button> 
      </label>
    </form></td>
  </tr>
<tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> 
  
    <td>&nbsp;</td>
    <td align="center" valign="middle"><form name="form1" method="post" action="">
      <label>
        <button id="B"  type="button" onclick="buttonclick(this);" class="button green">Backward</button> 
      </label>
    </form></td>     
    <td>&nbsp;</td>
  </tr>
<tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr>
   
        <td align="center" valign="middle"><form name="form1" method="post" action="">
      <label>
        <button id="a"  type="button" onclick="buttonclick(this);" class="button blue">Walk</button> 
      </label>
    </form></td>
        <td align="center" valign="middle"><form name="form1" method="post" action="">
      <label>
        <button id="b"  type="button" onclick="buttonclick(this);" class="button blue">Wave</button> 
      </label>
    </form></td>
        <td align="center" valign="middle"><form name="form1" method="post" action="">
      <label>
        <button id="c"  type="button" onclick="buttonclick(this);" class="button blue">Run</button> 
      </label>
    </form></td>
</tr>
<tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr> <tr>


  
</table>
<p class="foot">Powered by Pachanoid</p>
</body>
</html>
)rawliteral";

WebSocketsServer webSocket = WebSocketsServer(81);
ESP8266WebServer server(80);

void setup() {
 Serial.begin(115200);
 BlueTooth.begin(38400);

 
  delay(500);

  //***************************************************************
 WiFi.mode(WIFI_AP);           //Only Access point
  WiFi.softAP(ssid, password);  //Start HOTspot removing password will disable security
 
  IPAddress myIP = WiFi.softAPIP(); //Get IP address
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");   
  Serial.println(myIP);

  server.on("/", [](){
  server.send(200, "text/html", INDEX_HTML);
  });
  
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  //**********************************************


}

void loop() {

     if(Data=='a')Walk();
else if(Data=='b')Wave();
else if(Data=='c')Run();
else if(Data=='F')forward();
else if(Data=='B')backward();
else if(Data=='R')turnRight();
else if(Data=='L')turnLeft();
else if(Data=='S')Stop();

      
   webSocket.loop();
   server.handleClient();
}



void webSocketEvent(uint8_t num, WStype_t type,uint8_t * Payload, size_t length){
  switch(type) {
    case WStype_DISCONNECTED:
      //Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {IPAddress ip = webSocket.remoteIP(num);}
      break;
    case WStype_TEXT:
      //Serial.printf("%s\r\n", Payload);
      Data = Payload[0];
      Serial.println(Data);
      // send data to all connected clients
      webSocket.broadcastTXT(Payload, length);
     
      break;
    case WStype_BIN:
      hexdump(Payload, length);
      // echo data back to browser
      webSocket.sendBIN(num, Payload, length);
      break;default:break;
  }
}

void forward(){

    BlueTooth.print("V1"); // Vorpal hexapod radio protocol header version 1 (Byte 0 and 1)
    //forzando para que avanze W 1 f
    int eight =8;
    CurCmd =87;         // W char
    CurSubCmd;        // 4 en char for running 
    CurDpad=102;              // f in ascii
    
    BlueTooth.write(eight);             //Byte 2
    BlueTooth.write(CurCmd);            //Byte 3
    BlueTooth.write(CurSubCmd);         //
    BlueTooth.write(CurDpad);
 //BlueTooth.write(850);

    unsigned int checksum = sendbeep(0);

    checksum += eight+CurCmd+CurSubCmd+CurDpad;
    checksum = (checksum % 256);
    BlueTooth.write(checksum);
    //BlueTooth.flush();
    
    setBeep(0,0); // clear the current beep because it's been sent now
    
      NextTransmitTime = millis() + REC_FRAMEMILLIS;
      delay(10);  //menos milisegundos taldea
}


void backward(){

    BlueTooth.print("V1"); // Vorpal hexapod radio protocol header version 1 (Byte 0 and 1)
    //forzando para que avanze w 1 b
    int eight =8;
    CurCmd =87;
    CurSubCmd;
    CurDpad=98;                                 // b in ascii              
    
    BlueTooth.write(eight);             //Byte 2
    BlueTooth.write(CurCmd);            //Byte 3
    BlueTooth.write(CurSubCmd);         //
    BlueTooth.write(CurDpad);
 //BlueTooth.write(850);

    unsigned int checksum = sendbeep(0);

    checksum += eight+CurCmd+CurSubCmd+CurDpad;
    checksum = (checksum % 256);
    BlueTooth.write(checksum);
    //BlueTooth.flush();
    
    setBeep(0,0); // clear the current beep because it's been sent now
    
      NextTransmitTime = millis() + REC_FRAMEMILLIS;
      delay(10);  //menos milisegundos taldea
}

void turnLeft(){

    BlueTooth.print("V1"); // Vorpal hexapod radio protocol header version 1 (Byte 0 and 1)
    //forzando para que avanze w 1 l
    int eight =8;
    CurCmd =87;
    CurSubCmd;
    CurDpad=108;              // l in ascii
    
    BlueTooth.write(eight);             //Byte 2
    BlueTooth.write(CurCmd);            //Byte 3
    BlueTooth.write(CurSubCmd);         //
    BlueTooth.write(CurDpad);
 //BlueTooth.write(850);

    unsigned int checksum = sendbeep(0);

    checksum += eight+CurCmd+CurSubCmd+CurDpad;
    checksum = (checksum % 256);
    BlueTooth.write(checksum);
    //BlueTooth.flush();
    
    setBeep(0,0); // clear the current beep because it's been sent now
    
      NextTransmitTime = millis() + REC_FRAMEMILLIS;
      delay(10);  //menos milisegundos taldea
}

void turnRight(){

    BlueTooth.print("V1"); // Vorpal hexapod radio protocol header version 1 (Byte 0 and 1)
    //forzando para que avanze w 1 r
    int eight =8;
    CurCmd =87;
    CurSubCmd;
    CurDpad=114;                  //r in ascii
    
    BlueTooth.write(eight);             //Byte 2
    BlueTooth.write(CurCmd);            //Byte 3
    BlueTooth.write(CurSubCmd);         //
    BlueTooth.write(CurDpad);
 //BlueTooth.write(850);

    unsigned int checksum = sendbeep(0);

    checksum += eight+CurCmd+CurSubCmd+CurDpad;
    checksum = (checksum % 256);
    BlueTooth.write(checksum);
    //BlueTooth.flush();
    
    setBeep(0,0); // clear the current beep because it's been sent now
    
      NextTransmitTime = millis() + REC_FRAMEMILLIS;
      delay(10);  //menos milisegundos taldea
}
void Stop(){

    BlueTooth.print("V1"); // Vorpal hexapod radio protocol header version 1 (Byte 0 and 1)
    //forzando para que avanze w 1 s
    int eight =8;
    CurCmd =87;
    CurSubCmd;
    CurDpad=115;                  //s in ascii
    
    BlueTooth.write(eight);             //Byte 2
    BlueTooth.write(CurCmd);            //Byte 3
    BlueTooth.write(CurSubCmd);         //
    BlueTooth.write(CurDpad);
 //BlueTooth.write(850);

    unsigned int checksum = sendbeep(0);

    checksum += eight+CurCmd+CurSubCmd+CurDpad;
    checksum = (checksum % 256);
    BlueTooth.write(checksum);
    //BlueTooth.flush();
    
    setBeep(0,0); // clear the current beep because it's been sent now
    
      NextTransmitTime = millis() + REC_FRAMEMILLIS;
      delay(10);  //menos milisegundos taldea
}



  void Walk(){

CurSubCmd=49;

    }

    
   void Run(){
CurSubCmd=52;   //4 asii = 52

    }

    
    void Wave(){
BlueTooth.print("V1"); // Vorpal hexapod radio protocol header version 1 (Byte 0 and 1)
    //forzando para que avanze w 1 f
    int eight =8;
    CurCmd =70;
    CurSubCmd=1;
    CurDpad=102;              // f in ascii
    
    BlueTooth.write(eight);             //Byte 2
    BlueTooth.write(CurCmd);            //Byte 3
    BlueTooth.write(CurSubCmd);         //
    BlueTooth.write(CurDpad);
 //BlueTooth.write(850);

    unsigned int checksum = sendbeep(0);

    checksum += eight+CurCmd+CurSubCmd+CurDpad;
    checksum = (checksum % 256);
    BlueTooth.write(checksum);
    //BlueTooth.flush();
    
    setBeep(0,0); // clear the current beep because it's been sent now
    
      NextTransmitTime = millis() + REC_FRAMEMILLIS;
      delay(10);  //menos de 10 milisegundos taldea
    }
