#include <DHT.h>
#include <EEPROM.h>
#include "CTBot.h"
#include "ESP8266WiFi.h"

CTBot myBot;
String ssid  = "mynet"    ; // REPLACE mySSID WITH YOUR WIFI SSID
String pass  = "utsenuta"; // REPLACE myPassword YOUR WIFI PASSWORD, IF ANY
String token = "725106403:AAGyPxBdKRfvHQYriS2HYjqHWDV8e9z-5YM"   ; // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN
// Data wire is plugged into pin D1 on the ESP8266 12-E - GPIO 5

#define PERIOD 0            // 1 - период в часах, 0 - в минутах
#define PUMPING 1           // 1 - время работы помпы в секундах, 0 - в минутах
#define DROP_ICON 1         // 1 - отображать капельку, 0 - будет буква "t" (time)

#define DHTPIN 14
int WET_CONTROL = A0;
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

int LIGHT_1 = 5;
int LIGHT_2 = 16;
int PUMP_1 = 4;
int PUMP_2 = 13;

bool  pump1Enabled = true;
bool  pump2Enabled = true;
bool  light1Enabled = true;
bool  light2Enabled = true;

bool  pump1Change = false;
bool  pump2Change = false;
bool  light1Change = false;
bool  light2Change = false;

String command_ligth1_on = "/light1on";
String command_ligth1_off = "/light1off";
String command_ligth2_on = "/light2on";
String command_ligth2_off = "/light2off";
String command_pump1_on = "/pump1on";
String command_pump1_off = "/pump1off";
String command_pump2_on = "/pump2on";
String command_pump2_off = "/pump2off";
String command_status = "/status";
String command_set = "/set";
String command_pump_timer = "/set_pump_timer";
String command_on_timer = "/set_on_timer";

int chat_id = 294499886;
byte current_set = 2;
byte current_pump;
boolean reDraw_flag, arrow_update;
boolean now_pumping;
unsigned long period_coef, pumping_coef;

void setup() {
    // initialize the Serial
  Serial.begin(115200);
  Serial.println("Starting TelegramBot...");

  dht.begin();
  
  // connect the ESP8266 to the desired access point
  myBot.wifiConnect(ssid, pass);

  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  
  // set the telegram bot token
  myBot.setTelegramToken(token);
  
  // check if all things are ok
  if (myBot.testConnection())
    Serial.println("\ntestConnection OK");
  else
    Serial.println("\ntestConnection NOK");

  // --------------------- КОНФИГУРИРУЕМ ПИНЫ ---------------------
  pinMode(LIGHT_1, OUTPUT); 
  pinMode(LIGHT_2, OUTPUT); 
  pinMode(PUMP_1, OUTPUT); 
  pinMode(PUMP_2, OUTPUT); 

  Serial.begin(115200);
  String welcome = String("Добро пожаловть в систему управления боксом.\n")
  +String("Узнать статус - /status\n")
  +String("Включить свет № 1 -  ") + command_ligth1_on  +   String("\n")
  +String("Вылючить свет № 1 -  ") + command_ligth1_off +   String("\n")
  +String("Включить свет № 2 -  ") + command_ligth2_on  +   String("\n")
  +String("Вылючить свет № 2 -  ") + command_ligth2_off +   String("\n")
  +String("Включить насос № 1 - ") + command_pump1_on  +  String("\n")
  +String("Вылючить насос № 1 - ") + command_pump1_off  +   String("\n")
  +String("Включить насос № 2 - ") + command_pump2_on  +  String("\n")
  +String("Вылючить насос № 2 - ") + command_pump2_off  +   String("\n")
  +String("Установить таймер № хх  колво минут хх - ") + command_set  +  String("\n")
  +String("Установить время полива секунд") + command_pump_timer  +  String("\n") 
  +String("Установить время включения таймера мин") + command_on_timer  +  String("\n")  ;
  Serial.println(welcome);
  Status();
  LogToTelegramm(welcome);
}

int period_milis_light1 = 12*60*60*1000;
int flow_light1 = 12*60*60*1000;
int period_milis_light1_prev = 0;
void PeriodLight1(){
  if (millis() - period_milis_light1_prev > period_milis_light1)  
   { 
      int controlLight1 = digitalRead(LIGHT_1);
      if(controlLight1 == 0)
      {
        digitalWrite(LIGHT_1, 1);
        LogToTelegramm("Включен свет № 1");   
        Status();     
      }     
      int period = millis() - period_milis_light1_prev - period_milis_light1;           
      if(period > flow_light1)
      {        
        digitalWrite(LIGHT_1, 0);
        LogToTelegramm("Выключен свет № 1");
        period_milis_light1_prev = millis();
        Status();
      } 
   }  
}

int period_milis_light2 = 12*60*60*1000;
int flow_light2 = 12*60*60*1000;
int period_milis_light2_prev = 0;
void PeriodLight2(){
  if (millis() - period_milis_light2_prev > period_milis_light2)  
   { 
      int controlLight2 = digitalRead(LIGHT_2);
      if(controlLight2 == 0)
      {
        digitalWrite(LIGHT_2, 1);
        LogToTelegramm("Включен свет № 2");   
        Status();     
      }     
      int period = millis() - period_milis_light2_prev - period_milis_light2;           
      if(period > flow_light2)
      {        
        digitalWrite(LIGHT_2, 0);
        LogToTelegramm("Выключен свет № 2");
        period_milis_light2_prev = millis();
        Status();
      } 
   }  
}

String GetPumpName(bool is_first){
  if(is_first)
  {
    return "вода";
  }
  else{
    return  "компот";
  }
}

int period_milis_pump1 = 24*60*60*1000;
int flow_pump1 = 15*1000;
int period_milis_pump1_prev = 0;
bool is_first = true;
void PeriodPump(){
  int pump = PUMP_1; 
  if(is_first)
  {
    pump = PUMP_1;
  }
  else{
    pump = PUMP_2;
  }
  String name_pump = GetPumpName(is_first);
  if (millis() - period_milis_pump1_prev > period_milis_pump1)  
   { 
      int controlPump1 = digitalRead(pump);
      if(controlPump1 == 0)
      {
        digitalWrite(pump, 1);
        LogToTelegramm("Включен полив " + name_pump);   
      }     
      int period = millis() - period_milis_pump1_prev - period_milis_pump1;           
      if(period > flow_pump1)
      {        
        digitalWrite(pump, 0);
        LogToTelegramm("Выключен полив " + name_pump);
        is_first = !is_first;
        period_milis_pump1_prev = millis();
      } 
   }  
}

void loop() {
  PeriodLight1();
  PeriodLight2();
  PeriodPump();
  TBMessage msg;
  if (myBot.getNewMessage(msg))
  {
    chat_id = msg.sender.id;
    Serial.println(msg.text); 
    CheckCommand(msg.text);
  }
  delay(500);
}

void LogToTelegramm(String  message){
  myBot.sendMessage(chat_id, message);
}

void CheckCommand(String command){
  if(command == command_ligth1_on) {
    digitalWrite(LIGHT_1, 1);
  }
  else if(command == command_ligth1_off){
    digitalWrite(LIGHT_1, 0);
  }
  else if(command == command_ligth2_on) {
    digitalWrite(LIGHT_2, 1); 
  }
  else if(command == command_ligth2_off){
    digitalWrite(LIGHT_2, 0); 
  }
  else if(command == command_pump1_on) {
    digitalWrite(PUMP_1, 1); 
  }
  else if(command == command_pump1_off){
    digitalWrite(PUMP_1, 0); 
  }
  else if(command == command_pump2_on) {
    digitalWrite(PUMP_2, 1); 
  }
  else if(command == command_pump2_off){
    digitalWrite(PUMP_2, 0);
  }
  else if(command.startsWith(command_set)){
    SetCommand(command);
  }
  else if(command.startsWith(command_pump_timer)){
    SetPumpTimerCommand(command);
  }
  else if(command.startsWith(command_on_timer)){
    SetOnTimerCommand(command);
  }
  else if(command == command_status){
    Status();
  }
  else{
    LogToTelegramm("Выберите команду.");
    return;
  }  
}

void SetOnTimerCommand(String command){
    command = command.substring(13);   
    int firstClosingBracket = command.indexOf(" ");
    int secondClosingBracket = command.indexOf(" ", firstClosingBracket + 1);
    String timer = command.substring(firstClosingBracket + 1, secondClosingBracket);
    String timer_on = command.substring(secondClosingBracket + 1);
    int t1 =  timer_on.toInt();
    if(timer == "1"){      
      int next_start = t1 * 60 * 1000;
      period_milis_light1_prev = millis() - period_milis_light1 - next_start;
    }
    else if(timer == "2"){
      int next_start = t1 * 60 * 1000;
      period_milis_light2_prev = millis() - period_milis_light2 - next_start;
    }
    else if(timer == "3"){
      int next_start = t1 * 60 * 1000;
      period_milis_pump1_prev = millis() - period_milis_pump1 - next_start;
    } 
}

void SetPumpTimerCommand(String command){
    command = command.substring(15);   
    int firstClosingBracket = command.indexOf(" ");
    String timer_off = command.substring(firstClosingBracket + 1);
    int t2 =  timer_off.toInt();
    flow_pump1 = t2 * 1000;    
}

void SetCommand(String command){
    command = command.substring(4);   
    int firstClosingBracket = command.indexOf(" ");
    int secondClosingBracket = command.indexOf(" ", firstClosingBracket + 1);
    int thirtClosingBracket = command.indexOf(" ", secondClosingBracket + 1);
    String timer = command.substring(firstClosingBracket + 1, secondClosingBracket);
    String timer_on = command.substring(secondClosingBracket + 1,thirtClosingBracket);
    String timer_off = command.substring(thirtClosingBracket + 1);
    int t1 =  timer_on.toInt();
    int t2 =  timer_off.toInt();
    if(timer == "1"){
      period_milis_light1 = t1 * 60 * 60 * 1000;
      flow_light1 = t2 * 60 * 60 * 1000;
    }
    else if(timer == "2"){
      period_milis_light2 = t1 * 60 * 60 * 1000;
      flow_light2 = t2 * 60 * 60 * 1000;
    }
    else if(timer == "3"){
      period_milis_pump1 = t1 * 60 * 60 * 1000;
      flow_pump1 = t2 * 1000;
    }
}

void Status(){
  Serial.println("Status");
  int controlLight1 = digitalRead(LIGHT_1);
  int controlLight2 = digitalRead(LIGHT_2);
  int controlPump1 = digitalRead(PUMP_1);
  int controlPump2 = digitalRead(PUMP_2);
  int controlWet = analogRead(WET_CONTROL);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int __millis = millis();
  String status_ =  "Свет № 1 - " +   GetStatusDevice(controlLight1) + String(" Таймер1 ")
  + String((period_milis_light1 - __millis + period_milis_light1_prev)/(60*1000)) + String(" Таймер2 ") 
  + String((flow_light2 - (__millis - period_milis_light1_prev - period_milis_light1))/(60*1000)) + String("\n")
    
  +String("Свет № 2 - "         ) +   GetStatusDevice(controlLight2) 
    + String(" Таймер1 ") + String((period_milis_light2 - __millis + period_milis_light2_prev)/(60*1000)) + String(" Таймер2 ") 
  + String((flow_light2 - (__millis - period_milis_light2_prev - period_milis_light2))/(60*1000)) + String("\n")  
  +String("Насос № 1 - " ) +     GetStatusDevice(controlPump1) 
  
  + String(" Таймер1 ") + String((period_milis_pump1 - __millis + period_milis_pump1_prev)/(60*1000)) + String(" Таймер2 ") 
  + String((flow_pump1 - (__millis - period_milis_pump1_prev - period_milis_pump1))/(60*1000)) + String("\n")

 +String("Насос "      ) + GetStatusDevice(controlPump1) 
  + String(" Таймер1 ") + String((period_milis_pump1 - __millis + period_milis_pump1_prev)/(60*1000)) + String(" Таймер2 ") 
  + String((flow_pump1 - (__millis - period_milis_pump1_prev - period_milis_pump1))/(60*1000)) + String("\n")
  
  +String("Текущий полив - " ) + GetPumpName(is_first) +  GetStatusDevice(controlPump1) 
   
  +String("Температура воздуха - "  ) + String(t)   + String("\n")
  +String("Влажность воздуха - "  ) +  String(h)         + String("\n")
  +String("Влажность почвы - "    ) +  String(controlWet)     + String("\n");
  Serial.println(status_);
  myBot.sendMessage(chat_id, status_);
}

String GetStatusDevice(int level){
  if(level == 1)
  {
    return " включен ";
  }
  else{
    return " выключен ";
  }
}
