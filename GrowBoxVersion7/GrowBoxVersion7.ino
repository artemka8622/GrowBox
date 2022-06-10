#include <DHT.h>
#include <EEPROM.h>
#include "CTBot.h"
#include "ESP8266WiFi.h"

CTBot myBot;
CTBotInlineKeyboard myKbd; 
String ssid  = "mynet"    ; // REPLACE mySSID WITH YOUR WIFI SSID
String pass  = "utsenuta"; // REPLACE myPassword YOUR WIFI PASSWORD, IF ANY
String token = "903658352:AAF2WkPyqqJGARaFvyVCVWm6nCJsJD8-8Es"   ; // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN
// Data wire is plugged into pin D1 on the ESP8266 12-E - GPIO 5

#define LIGHT_1_ON_CALLBACK  "light1ON"  // callback data sent when "LIGHT ON" button is pressed
#define LIGHT_1_OFF_CALLBACK "light1OFF" // callback data sent when "LIGHT OFF" button is pressed
#define LIGHT_2_ON_CALLBACK  "light2ON"  // callback data sent when "LIGHT ON" button is pressed
#define LIGHT_2_OFF_CALLBACK "light2OFF" // callback data sent when "LIGHT OFF" button is pressed
#define PUMP_1_ON_CALLBACK  "pump1ON"  // callback data sent when "LIGHT ON" button is pressed
#define PUMP_1_OFF_CALLBACK "pump1OFF" // callback data sent when "LIGHT OFF" button is pressed
#define PUMP_2_ON_CALLBACK  "pump2ON"  // callback data sent when "LIGHT ON" button is pressed
#define PUMP_2_OFF_CALLBACK "pump2OFF" // callback data sent when "LIGHT OFF" button is pressed
#define STATUS_CALLBACK  "status"  // callback data sent when "LIGHT ON" button is pressed
#define VEGA_CALLBACK "vega" // callback data sent when "LIGHT OFF" button is pressed
#define BLOOM_CALLBACK "bloom" // callback data sent when "LIGHT OFF" button is pressed

#define COLOR_CALLBACKK "colorCallback" // callback data sent when "LIGHT OFF" button is pressed
#define COLOR_RED_P10_CALLBACKK  "redp10"  // callback data sent when "LIGHT ON" button is pressed
#define COLOR_RED_N10_CALLBACKK "redn10" // callback data sent when "LIGHT OFF" button is pressed
#define COLOR_GREEN_P10_CALLBACKK  "greenp10"  // callback data sent when "LIGHT ON" button is pressed
#define COLOR_GREEN_N10_CALLBACKK "greenn10" // callback data sent when "LIGHT OFF" button is presse

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

int chat_ids[10] = {294499886,0,0,0,0,0,0,0,0,0};
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
  Status();
}

String GetStatusDevice(int level){
  if(level == 1)
  {
    return " вкл. ";
  }
  else{
    return " выкл. ";
  }
}


int period_milis_light1 = 6*60*60*1000;
int period_milis_light1_prev = 0;
void PeriodLight1(){
  int calc_time = millis() - period_milis_light1_prev;
  int controlLight1 = digitalRead(LIGHT_1);
  if (calc_time < period_milis_light1)  
  {       
    if(controlLight1 == 1)
    {
      digitalWrite(LIGHT_1, 0);
      LogToTelegramm("Выключен свет");   
      Status();     
    }               
  }
  else
  {  
    if(controlLight1 == 0)
    {
        digitalWrite(LIGHT_1, 1);
        LogToTelegramm("Включен свет");
        Status();
    }
  }  
  if(calc_time > 24 *60 *60 * 1000){
    period_milis_light1_prev = millis();
  }
}

int period_milis_light2 = 6*60*60*1000;
int period_milis_light2_prev = 0;
void PeriodLight2(){
  int calc_time = millis() - period_milis_light2_prev;
  int controlLight2 = digitalRead(LIGHT_2);
  if (calc_time < period_milis_light2)  
  { 
      if(controlLight2 == 1)
      {
        digitalWrite(LIGHT_2, 0);
        LogToTelegramm("Выключена вентиляция");   
        Status();     
      }     
  }
  else
  {  
    if(controlLight2 == 0)
    {
        digitalWrite(LIGHT_2, 1);
        LogToTelegramm("Включена вентиляция");
        Status();
    }
  }  
  if(calc_time > 24 *60 *60 * 1000){
    period_milis_light2_prev = millis();
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

int period_milis_pump1 = 4800*60*60*1000;
int flow_pump1 = 1*1000;
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

void UpdateChat(int chat_id){
  int i = 0;
  StateChats();
  for (i = 0; i < 10; i = i + 1) {
    Serial.println("Обновляем чат - " + String(chat_ids[i]) + String(chat_id));
    if(chat_ids[i] == chat_id){
      return;
    }
      if(chat_ids[i] == 0 )
      {
        chat_ids[i] = chat_id;
        return;
      }
  }
  StateChats();
}

void StateChats(){ 
  Serial.print("Обновляем чат ");
  int i=0;
    for (i = 0; i < 10; i = i + 1) {
      Serial.print("  " + String(chat_ids[i]));
    }
    Serial.println("." );
}

void loop() {
  PeriodLight1();
  PeriodLight2();
  PeriodPump();
  TBMessage msg;
  if (myBot.getNewMessage(msg))
  {
      int chat_id = msg.sender.id;
      UpdateChat(chat_id);
      if (msg.messageType == CTBotMessageQuery) {
        CheckCommand2(msg.callbackQueryData, msg.callbackQueryID, chat_id);
      }else{
        Serial.println(msg.text); 
        CheckCommand(msg.text, chat_id);
      }
  }
  delay(1000);
}

void CheckCommand(String command, int chat_id){
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
  else if(command.startsWith(command_on_timer)){
    SetOnTimerCommand(command);
  }
  else if(command.startsWith(command_set)){
    SetCommand(command);
  }
  else if(command.startsWith(command_pump_timer)){
    SetPumpTimerCommand(command);
  }
  else if(command.startsWith(command_status)){
    StatusForChat(chat_id);
  }
  else{
    LogToTelegrammForChat("Выберите команду.", chat_id);
    return;
  }  
}

void CheckCommand2(String callbackQueryData, String callbackQueryID, int chat_id){
    // received a callback query message
  if (callbackQueryData.equals(STATUS_CALLBACK)) {
    StatusForChat(chat_id);
    myBot.endQuery(callbackQueryID, STATUS_CALLBACK);
  } else  if (callbackQueryData.equals(LIGHT_1_ON_CALLBACK)) {
    digitalWrite(LIGHT_1, 1);
    myBot.endQuery(callbackQueryID, LIGHT_1_ON_CALLBACK);
  } else  if (callbackQueryData.equals(LIGHT_1_OFF_CALLBACK)) {
    digitalWrite(LIGHT_1, 0);
    myBot.endQuery(callbackQueryID, LIGHT_1_OFF_CALLBACK);
  } else  if (callbackQueryData.equals(LIGHT_2_ON_CALLBACK)) {
    digitalWrite(LIGHT_2, 1);
    myBot.endQuery(callbackQueryID, LIGHT_2_ON_CALLBACK);
  } else  if (callbackQueryData.equals(LIGHT_2_OFF_CALLBACK)) {
    digitalWrite(LIGHT_2, 0);
    myBot.endQuery(callbackQueryID, LIGHT_2_OFF_CALLBACK);
  } else  if (callbackQueryData.equals(PUMP_1_ON_CALLBACK)) {
    digitalWrite(PUMP_1, 1);
    myBot.endQuery(callbackQueryID, PUMP_1_ON_CALLBACK);
  } else  if (callbackQueryData.equals(PUMP_1_OFF_CALLBACK)) {
    digitalWrite(PUMP_1, 0);
    myBot.endQuery(callbackQueryID, PUMP_1_OFF_CALLBACK);
  } else if (callbackQueryData.equals(VEGA_CALLBACK)) {
    period_milis_light1 = 8 * 60 * 60 * 1000;
    period_milis_light2 =  8 * 60 * 60 * 1000;
    myBot.endQuery(callbackQueryID, VEGA_CALLBACK);
  } else if (callbackQueryData.equals(BLOOM_CALLBACK)) {
    period_milis_light1 = 12 * 60 * 60 * 1000;
    period_milis_light2 =  12 * 60 * 60 * 1000;
    myBot.endQuery(callbackQueryID, BLOOM_CALLBACK);
  } else if (callbackQueryData.equals(COLOR_RED_P10_CALLBACKK)) {
    period_milis_light1 =  period_milis_light1 + 60 * 60 * 1000;  
    myBot.endQuery(callbackQueryID, GetColorMsg());
  }
  else if (callbackQueryData.equals(COLOR_RED_N10_CALLBACKK)) {
    period_milis_light1 =  period_milis_light1 - 60 * 60 * 1000;
    myBot.endQuery(callbackQueryID, GetColorMsg());
  } else if (callbackQueryData.equals(COLOR_GREEN_P10_CALLBACKK)) {
    period_milis_light2 =  period_milis_light2 + 60 * 60 * 1000;
    myBot.endQuery(callbackQueryID, GetColorMsg());
  }
  else if (callbackQueryData.equals(COLOR_GREEN_N10_CALLBACKK)) {
    period_milis_light2 =  period_milis_light2 - 60 * 60 * 1000;
    myBot.endQuery(callbackQueryID, GetColorMsg());
  }
}

String GetColorMsg()
{
  return String(period_milis_light2 / (60 * 60 * 1000)) 
  + String("\r\n") + String(period_milis_light1 / (60 * 60 * 1000));
}

void SetOnTimerCommand(String command){
   Serial.println("Set on timer command " + String(command)); 
    command = command.substring(13);   
    int firstClosingBracket = command.indexOf(" ");
    int secondClosingBracket = command.indexOf(" ", firstClosingBracket + 1);
    String timer = command.substring(firstClosingBracket + 1, secondClosingBracket);
    String timer_on = command.substring(secondClosingBracket + 1);
    int t1 =  timer_on.toInt();
    if(timer == "1"){      
      int next_start = t1 * 60 * 1000;
      period_milis_light1_prev = millis() - next_start;
      Serial.println("Set on timer 1 " + String(period_milis_light1_prev)); 
    }
    else if(timer == "2"){
      int next_start = t1 * 60 * 1000;
      period_milis_light2_prev = millis() - next_start;
      Serial.println("Set on timer 1 " + String(period_milis_light2_prev)); 
    }
    else if(timer == "3"){
      int next_start = t1 * 60 * 1000;
      period_milis_pump1_prev = millis() - next_start;
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
    }
    else if(timer == "2"){
      period_milis_light2 = t1 * 60 * 60 * 1000;
    }
    else if(timer == "3"){
      period_milis_pump1 = t1 * 60 * 60 * 1000;
      flow_pump1 = t2 * 1000;
    }
}

void Status()
{
  int i = 0;
  for (i = 0; i < 10; i = i + 1) {
    Serial.println("Отправляем в чат - " + String(chat_ids[i]) );
    if( chat_ids[i] == 0){
      return;
    }
    StatusForChat(chat_ids[i]);
  }
}

void StatusForChat(int chat_id){
  int controlLight1 = digitalRead(LIGHT_1);
  int controlLight2 = digitalRead(LIGHT_2);
  int controlPump1 = digitalRead(PUMP_1);
  int controlPump2 = digitalRead(PUMP_2);
  int controlWet = analogRead(WET_CONTROL);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int __millis = millis();
  myKbd.flushData();  

  // считаем сколько осталось работы текущего режива
  int remain_time1 = __millis - period_milis_light1_prev;
  if(remain_time1 > period_milis_light1)
  {
    remain_time1 = 24*60*60*1000 - remain_time1;
  }else{
    remain_time1 = period_milis_light1 - remain_time1;
  }
  
  int remain_time2 = __millis - period_milis_light2_prev;
  if(remain_time2 > period_milis_light2)
  {
    remain_time2 = 24*60*60*1000 - remain_time2;
  }else{
    remain_time2 = period_milis_light2 - remain_time2;
  }
  
  String status_1 =  String("Свет: ") + GetStatusDevice(controlLight1)  + "осталось " + String(remain_time1/(1000*60)) +" мин.";
    
  String status_2 =  String("Вентиляция: ") + GetStatusDevice(controlLight2) + " осталось " + String(remain_time2/(1000*60)) +" мин.";
   
   String pump_1 = String("Насос: " ) +  GetStatusDevice(controlPump1) + " осталось "
  + String((period_milis_pump1 - __millis + period_milis_pump1_prev)/(60*1000)) + String(" время включения: ")
  + String(flow_pump1/1000);

  String temp = String("температура: "  ) + String(t);
  String hum = String("влажность: "  ) +  String(h);

  myKbd.addButton("Статус "  , STATUS_CALLBACK, CTBotKeyboardButtonQuery);
  myKbd.addButton(temp , STATUS_CALLBACK, CTBotKeyboardButtonQuery);
  myKbd.addButton(hum, STATUS_CALLBACK, CTBotKeyboardButtonQuery);
  myKbd.addRow();
  myKbd.addButton("Полив:", STATUS_CALLBACK, CTBotKeyboardButtonQuery);
  //myKbd.addButton("ВЫКЛ", LIGHT_1_OFF_CALLBACK, CTBotKeyboardButtonQuery);
  //myKbd.addButton("ВКЛ", LIGHT_2_ON_CALLBACK, CTBotKeyboardButtonQuery);
  //myKbd.addButton("ВЫКЛ", LIGHT_2_OFF_CALLBACK, CTBotKeyboardButtonQuery);
  myKbd.addButton("Включить", PUMP_1_ON_CALLBACK, CTBotKeyboardButtonQuery);
  myKbd.addButton("Выключить", PUMP_1_OFF_CALLBACK, CTBotKeyboardButtonQuery);
    // 2
  myKbd.addRow();
  myKbd.addButton("Выберете режим:", STATUS_CALLBACK, CTBotKeyboardButtonQuery);
  myKbd.addButton("Цвет",BLOOM_CALLBACK , CTBotKeyboardButtonQuery);
  myKbd.addButton("Вега",VEGA_CALLBACK, CTBotKeyboardButtonQuery);
  // 3
  myKbd.addRow();
  myKbd.addButton("Световой день", COLOR_CALLBACKK, CTBotKeyboardButtonQuery);
  myKbd.addButton("увеличить на 1ч.", COLOR_RED_P10_CALLBACKK, CTBotKeyboardButtonQuery);
  myKbd.addButton("уменьшить на 1ч", COLOR_RED_N10_CALLBACKK, CTBotKeyboardButtonQuery);
  myKbd.addRow();
  myKbd.addButton("Время вентиляции", COLOR_CALLBACKK, CTBotKeyboardButtonQuery);
  myKbd.addButton("увеличить на 1ч.", COLOR_GREEN_P10_CALLBACKK, CTBotKeyboardButtonQuery);
  myKbd.addButton("уменьшить на 1ч", COLOR_GREEN_N10_CALLBACKK, CTBotKeyboardButtonQuery);

  

  myBot.sendMessage(chat_id, String("Текущий статус") + status_1 + String("\r\n") + status_2 + String("\r\n") + String(pump_1) + String("\r\n") + hum + temp + String("\r\n"), myKbd);
  
  Serial.println("Текущий статус" + status_1 + status_2 + pump_1 + hum + temp );
}

void LogToTelegrammForChat(String  message, int chat_id){
  int i = 0;
  myBot.sendMessage(chat_id, message);
}

void LogToTelegramm(String  message){
  int i = 0;
    for (i = 0; i < 10; i = i + 1) {
      if(0 == chat_ids[i]){
        break;
      }
      myBot.sendMessage(chat_ids[i], message);
    }
}
