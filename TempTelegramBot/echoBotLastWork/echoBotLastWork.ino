
/*
 Name:		    echoBot.ino
 Created:	    12/21/2017
 Author:	    Stefano Ledda <shurillu@tiscalinet.it>
 Description: a simple example that check for incoming messages
              and reply the sender with the received message
*/
#include "CTBot.h"
CTBot myBot;
#include <OneWire.h>
#include <DallasTemperature.h>

String ssid  = "mynet"    ; // REPLACE mySSID WITH YOUR WIFI SSID
String pass  = "utsenuta"; // REPLACE myPassword YOUR WIFI PASSWORD, IF ANY
String token = "725106403:AAGyPxBdKRfvHQYriS2HYjqHWDV8e9z-5YM"   ; // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN
// Data wire is plugged into pin D1 on the ESP8266 12-E - GPIO 5
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature DS18B20(&oneWire);
char temperatureCString[7];

void setup() {
	// initialize the Serial
	Serial.begin(115200);
	Serial.println("Starting TelegramBot...");

  DS18B20.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  
	// connect the ESP8266 to the desired access point
	myBot.wifiConnect(ssid, pass);

	// set the telegram bot token
	myBot.setTelegramToken(token);
	
	// check if all things are ok
	if (myBot.testConnection())
		Serial.println("\ntestConnection OK");
	else
		Serial.println("\ntestConnection NOK");
  TBMessage msg;
  getTemperature();
  myBot.getNewMessage(msg);
}

void loop() {
	// a variable to store telegram message data
	TBMessage msg;
  getTemperature();
	// if there is an incoming message...
	if (myBot.getNewMessage(msg))
	{// ...forward it to the sender
		myBot.sendMessage(msg.sender.id, String("Температура в боксе - ") + String(temperatureCString));
	}
 myBot.sendMessage(msg.sender.id, String("Температура в боксе - ") + String(temperatureCString));
	Serial.println("Temp is "); 
  Serial.println(temperatureCString); 
	// wait 100 milliseconds
	delay(1000);
}

void getTemperature() {
  float tempC;
  float tempF;
  do {
    DS18B20.requestTemperatures(); 
    tempC = DS18B20.getTempCByIndex(0);
    dtostrf(tempC, 2, 2, temperatureCString);
    delay(100);
  } while (tempC == 85.0 || tempC == (-127.0));
}
