#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include <Stepper.h>

const char* ssid = "MakeMesse";
const char* password = "DiDumDiDei";
String openWeatherMapApiKey = "cd05c487990402da9347ad01272faae3";
String city = "Hannover";
String countryCode = "DE";
String IPEigenstation = "192.168.10.74";

String serverPath;
const int motor = D4;
const int stepsPerRevolution = 2048;
String jsonBuffer;
unsigned long lastTime = 0;
unsigned long timerDelay = 60000;
int WindDir = 0;
int Position = 0;
int StartRPM = 150;
int MinRPM = 64;
int MaxRPM = 1023;
int PWMFreq = 50;
int pwm = 0;
int Motorpin1 = D5;
int Motorpin2 = D7;
int Motorpin3 = D6;
int Motorpin4 = D8;
int Nullpin = D3;
int MillDir = 0;
int LastMillDir = 0;
int WS = 0;
double Temperatur = 0;
double WindSpeed = 0;
int LuftFeuchte = 0;
int LuftDruck = 0;
char* directions[]={ "N", "NO", "O", "SO", "S","SW", "W", "NW"};

SSD1306Wire display(0x3C, D1, D2, GEOMETRY_64_32);
Stepper myStepper(stepsPerRevolution, Motorpin1, Motorpin2, Motorpin3, Motorpin4);

void setup() {
  analogWriteRange(1023);
  analogWriteFreq(PWMFreq);
  analogWriteMode(motor, 0, OUTPUT);
  analogWrite(motor,0);
  delay(1000);
  myStepper.setSpeed(3);
  pinMode(Nullpin, INPUT);

  Serial.begin(115200);
  display.init();
  display.setContrast(255);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.flipScreenVertically();
  display.display();

  Serial.println("Nullposition finden");
  while (analogRead(Nullpin) <500) {
    movestepper(-1);
  }
  movestepper(15);
  stepperaus();
  
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.println(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();


}

void loop() {
  ArduinoOTA.handle();
  
  // Send an HTTP GET request
  if ((millis() - lastTime) > timerDelay) {
    // Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      if (IPEigenstation == "") {
        serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&units=metric&APPID=" + openWeatherMapApiKey;
        timerDelay = 60000;
      }
      else {
        serverPath = "http://" + IPEigenstation + "/weather";
        timerDelay = 1000;
      }
      jsonBuffer = httpGETRequest(serverPath.c_str());
      
      //Serial.println(jsonBuffer);
      //JSONVar myObject = JSON.parse(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }

      Temperatur = double(myObject["main"]["temp"]);
      WindSpeed = double(myObject["wind"]["speed"]);
      WindDir = int(myObject["wind"]["deg"]);
      LuftFeuchte = int(myObject["main"]["humidity"]);
      LuftDruck = int(myObject["main"]["pressure"]);
     /* WindSpeed = WindSpeed+0.5;
      if (WindSpeed > 35) {
        WindSpeed = 0;
        
      }*/
      //WindSpeed = 33;
      if (WindSpeed <=0.2) {
        pwm=0;
        WS = 0;
        Serial.println("Windstärke: 0");
      }
      else if ((WindSpeed > 0.2) && (WindSpeed <= 1.5)) {
        pwm =MinRPM+1;
        WS = 1;        
        Serial.println("Windstärke: 1");
      }
      else if ((WindSpeed > 1.5) && (WindSpeed <= 3.3)) {
        pwm =MinRPM+8;
        WS = 2;        
        Serial.println("Windstärke: 2");
      }
      else if ((WindSpeed > 3.3) && (WindSpeed <= 5.4)) {
        pwm =MinRPM+16;
        WS = 3;
        Serial.println("Windstärke: 3");
      }      
      else if ((WindSpeed > 5.4) && (WindSpeed <= 7.9)) {
        pwm =MinRPM+25;
        WS = 4;
        Serial.println("Windstärke: 4");
      }
      else if ((WindSpeed > 7.9) && (WindSpeed <= 10.7)) {
        pwm =MinRPM+40;
        WS = 5;
        Serial.println("Windstärke: 5");
      }            
      else if ((WindSpeed > 10.7) && (WindSpeed <= 13.8)) {
        pwm =MinRPM+60;
        WS = 6;
        Serial.println("Windstärke: 6");
      }
      else if ((WindSpeed > 13.8) && (WindSpeed <= 17.1)) {
        pwm =MinRPM+85;
        WS = 7;
        Serial.println("Windstärke: 7");
      }      
      else if ((WindSpeed > 17.1) && (WindSpeed <= 20.7)) {
        pwm =MinRPM+120;
        WS = 8;
        Serial.println("Windstärke: 8");
      }
      else if ((WindSpeed > 20.7) && (WindSpeed <= 24.4)) {
        pwm =MinRPM+160;
        WS = 9;
        Serial.println("Windstärke: 9");
      }
      else if ((WindSpeed > 24.4) && (WindSpeed <= 28.4)) {
        pwm =MinRPM+200;
        WS = 10;
        Serial.println("Windstärke: 10");
      }                
      else if ((WindSpeed > 28.4) && (WindSpeed <= 32.6)) {
        pwm =MinRPM+500;
        WS = 11;
        Serial.println("Windstärke: 11");
      }        
      else {
        pwm =MaxRPM;
        WS = 12;
        Serial.println("Windstärke: 12");
      }      
      motorspeed(StartRPM, motor, pwm);
      
      if (WindDir > 315) {
        WindDir = 315;
      }
      //MillDir = stepsPerRevolution * WindDir / 360;
      MillDir = stepsPerRevolution * int((WindDir+22.5)/45) /8;
      Serial.print("Letzte Position: ");
      Serial.println(LastMillDir);
      Serial.print("Neue Position: ");
      Serial.println(stepsPerRevolution * WindDir / 360) + 85;
      Serial.print("Drehungsschritte: ");
      Serial.println(MillDir-LastMillDir);
      
      if (MillDir-LastMillDir != 0) {
        movestepper(MillDir-LastMillDir);
        Serial.println("Bewege Mühle");
        stepperaus();
      }
      LastMillDir = MillDir;
      
      Serial.print("Temperatur: ");
      Serial.println(Temperatur);
      Serial.print("Windgeschwindigkeit: ");
      Serial.println(WindSpeed);
      Serial.print("Windrichtung: ");
      Serial.println(WindDir);
      Serial.print("Luftdruck: ");
      Serial.println(LuftDruck);
      Serial.print("Luftfeuchte: ");
      Serial.println(LuftFeuchte);
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }

  writetext("Temp.", String(Temperatur)+"°C");
  writetext("Windst.", String(WS)+"Bft");
  writetext("Windrtg.", directions[int((WindDir+22.5)/45)]);
  writetext("Feuchte", String(LuftFeuchte)+"%");
  writetext("Druck", String(LuftDruck)+"hPa");
  
}

void writetext(String string1, String string2)
{
  display.clear();
  uint16_t firstline = display.drawStringMaxWidth(0, 0, 64, string1);
  uint16_t secondline = display.drawStringMaxWidth(0, 16, 128, string2);
  display.display();
  delay(2000);  
}

void motorspeed(int starttempo,int pin,int tempo)
{
  analogWrite(pin,starttempo);
  delay(20);
  analogWrite(pin,tempo);
  Serial.print("pwm: ");
  Serial.println(tempo);
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverName);
  int httpResponseCode = http.GET();
  String payload = "{}"; 
  if (httpResponseCode>0) {
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
  return payload;
}

void movestepper(int steps) {
  if (steps >= 0) {
    for (int i=0; i<steps; i++) {
      myStepper.step(1);
      delay(1);
    }

  }
  else {
    for (int i=0; i>steps; i--) {
      myStepper.step(-1);
      delay(1);
    }
  }
}

void stepperaus() {
  digitalWrite(Motorpin1,0);
    digitalWrite(Motorpin2,0);
    digitalWrite(Motorpin3,0);
    digitalWrite(Motorpin4,0);
}
