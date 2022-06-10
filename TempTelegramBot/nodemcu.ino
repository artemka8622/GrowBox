#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <TelegramBot.h>

#define LED0 3 //D3
#define LED1 1 //D10
#define LED2 2 //D4
#define LED3 10 //SD3
#define DS18B21 0  //D9
 
const char* ssid = "xxx";
const char* password = "yyy";
const char BotToken[] = "zzz";

OneWire oneWire(DS18B21);
DallasTemperature sensors(&oneWire);
DeviceAddress Thermometer1 = {0x28, 0xFF, 0x4E, 0x13, 0x50, 0x17, 0x04, 0x2B}; 
WiFiClientSecure net_ssl;
TelegramBot bot (BotToken, net_ssl);

void setup() {
  sensors.begin();
  sensors.setResolution(Thermometer1, 10);
  
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  bot.begin(); 
    
  pinMode(LED0, OUTPUT); 
  pinMode(LED1, OUTPUT); 
  pinMode(LED2, OUTPUT); 
  pinMode(LED3, OUTPUT); 
}

void loop() {

  message m = bot.getUpdates(); 
  
  if (m.text.equals("1on")){  
    digitalWrite(LED0, HIGH);   
    bot.sendMessage(m.chat_id, "The Led 1 is now ON");  
    
  }else if (m.text.equals("1off")){  
    digitalWrite(LED0, LOW);   
    bot.sendMessage(m.chat_id, "The Led 1 is now OFF"); 
     
  }else if (m.text.equals("2on")){
    digitalWrite(LED1, HIGH);   
    bot.sendMessage(m.chat_id, "The Led 2 is now ON"); 
    
  }else if (m.text.equals("2off")){
    digitalWrite(LED1, LOW);   
    bot.sendMessage(m.chat_id, "The Led 2 is now OFF");
    
  }else if (m.text.equals("3on")){
    digitalWrite(LED2, HIGH);   
    bot.sendMessage(m.chat_id, "The Led 3 is now ON"); 
    
  }else if (m.text.equals("3off")){
    digitalWrite(LED2, LOW);   
    bot.sendMessage(m.chat_id, "The Led 3 is now OFF");
    
  }else if(m.text.equals("4on")){
    digitalWrite(LED3, HIGH);   
    bot.sendMessage(m.chat_id, "The Led 4 is now ON"); 
    
  }else if (m.text.equals("4off")){
    digitalWrite(LED3, LOW);   
    bot.sendMessage(m.chat_id, "The Led 4 is now OFF");
    
  }else if (m.text.equals("temp")){
    sensors.requestTemperatures();
    bot.sendMessage(m.chat_id, String(printTemperature(Thermometer1))); 
  }
}

float printTemperature(DeviceAddress deviceAddress) {
  return sensors.getTempC(deviceAddress);
}
