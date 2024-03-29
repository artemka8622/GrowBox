#include <DHT.h>
#include <EEPROM.h>
#include "ESP8266WiFi.h"
#include <FastBot.h>

String ssid  = "mynet3"    ; // REPLACE mySSID WITH YOUR WIFI SSID
String pass  = "utsenuta"; // REPLACE myPassword YOUR WIFI PASSWORD, IF ANY
String token = "903658352:AAF2WkPyqqJGARaFvyVCVWm6nCJsJD8-8Es"   ; // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN
// Data wire is plugged into pin D1 on the ESP8266 12-E - GPIO 5
FastBot myBot(token);
#define CHAT_ID "294499886"

#define AUTO_MODE  "auto_mode"  // callback data sent when "LIGHT ON" button is pressed
#define MANUAL_MODE  "manual_mode"  // callback data sent when "LIGHT ON" button is pressed

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

#define BEGIN_COPMPOT "beginCompot" // callback data sent when "LIGHT OFF" button is pressed
#define FACTOR_1  "1_day"  // callback data sent when "LIGHT ON" button is pressed
#define FACTOR_2 "2_day" // callback data sent when "LIGHT OFF" button is pressed
#define FACTOR_3  "3_day"  // callback data sent when "LIGHT ON" button is pressed
#define CAHNGE_PUMP "changePump" // callback data sent when "LIGHT OFF" button is presse

#define PERIOD 0            // 1 - период в часах, 0 - в минутах
#define PUMPING 1           // 1 - время работы помпы в секундах, 0 - в минутах

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

boolean reDraw_flag, arrow_update;
boolean now_pumping;
unsigned long period_coef, pumping_coef;
int curr_millis = 0;
int start_millis = 0;
int delta_millis = 0;


/*+++++++++++++ Инициализация НАЧАЛО+++++++++++++++++++++++*/
void setup() {
  EEPROM.begin(512);
    // initialize the Serial
  Serial.begin(115200);
  Serial.println("Starting TelegramBot...");
  //ReadSettings();
  dht.begin();  

  connectWiFi();
  myBot.setChatID(CHAT_ID);
  myBot.attach(newMsg);
  InlineMenu();
  
  pinMode(LIGHT_1, OUTPUT); 
  pinMode(LIGHT_2, OUTPUT); 
  pinMode(PUMP_1, OUTPUT); 
  pinMode(PUMP_2, OUTPUT); 
  //Status();
}

int mode = 0; // 0- автоматический режим
void loop() {
  curr_millis = (start_millis - delta_millis) + millis();
  Serial.println("loop  " + String(curr_millis));  
  
  if(mode == 0){
    AutoMode();
  } else {  
  }
  
  myBot.tick();
  delay(500);
}

void AutoMode(){
  PeriodLight1();
  PeriodLight2();
  PeriodPump();
}

void connectWiFi() {
  delay(2000);
  Serial.begin(115200);
  Serial.println();

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() > 15000) ESP.restart();
  }
  Serial.println("Connected");
}

/*+++++++++++++ Инициализация КОНЕЦ +++++++++++++++++++++++*/

/*++++++++++++++++++++++++Переменные НАЧАЛО+++++++++++++++++++++++++++*/
int period_milis_light1 = 6*60*60*1000;
int period_milis_light1_prev = 0;
int period_milis_light2 = 6*60*60*1000;
int period_milis_light2_prev = 0;
int period_milis_pump1 = 24*60*60*1000;
int flow_pump1 = 1*1000;
int period_milis_pump1_prev = 0;
int cycle_factor = 1;
int next_cycle_copot = 1;
int save_timer = 0;
/*++++++++++++++++++++++++Переменные КОНЕЦ+++++++++++++++++++++++++++*/

/*+++++++++++++ ЛОГИКА НАЧАЛО+++++++++++++++++++++++*/

/*+++++++++++++ Обработка НАЧАЛО+++++++++++++++++++++++*/
byte depth = 0;
void newMsg(FB_msg& msg) {
  InlineMenu();    
  CheckCommand2(msg.text);
  CheckCommand(msg.text, chat_ids[0]);
  Serial.println(msg.text);
}

void InlineMenu(){
  String statusDevice = GetStatus();  
  String menu1 = (String(STATUS_CALLBACK) + " \t " + AUTO_MODE  + " \t " + MANUAL_MODE  +" \n ");
  String menu2 = String(PUMP_1_ON_CALLBACK) + " \t "  + String(PUMP_1_OFF_CALLBACK) + " \t " + String(PUMP_2_ON_CALLBACK) + " \t " + String(PUMP_2_OFF_CALLBACK) + " \n ";
  String menu6 = String(LIGHT_1_ON_CALLBACK) + " \t "  + String(LIGHT_1_OFF_CALLBACK) + " \t " + String(LIGHT_2_ON_CALLBACK) + " \t " + String(LIGHT_2_OFF_CALLBACK) + " \n ";
  String menu3 = String(BLOOM_CALLBACK) + " \t " + String(VEGA_CALLBACK) + " \n ";
  String menu4 = String(BEGIN_COPMPOT) + " \t " + String(CAHNGE_PUMP) + " \n ";
  String menu5 = String(FACTOR_1) + " \t " + String(FACTOR_2) + " \t " +String(FACTOR_3)+" ";  
  String menu = menu1 + menu2 + menu6 +menu3 + menu4 + menu5;  
  myBot.inlineMenu(statusDevice, menu); 
}

void CheckCommand(String command, int chat_id_1){
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
  else{
    return;
  }  
}

void CheckCommand2(String callbackQueryData){
    // received a callback query message
  if (callbackQueryData.equals(STATUS_CALLBACK)) {
    //bot.inlineMenu(STATUS_CALLBACK, menu); 
  } else  if (callbackQueryData.equals(LIGHT_1_ON_CALLBACK)) {
    digitalWrite(LIGHT_1, 1);    
  } else  if (callbackQueryData.equals(LIGHT_1_OFF_CALLBACK)) {
    digitalWrite(LIGHT_1, 0);   
  } else  if (callbackQueryData.equals(LIGHT_2_ON_CALLBACK)) {
    digitalWrite(LIGHT_2, 1);
  } else  if (callbackQueryData.equals(LIGHT_2_OFF_CALLBACK)) {
    digitalWrite(LIGHT_2, 0);
  } else  if (callbackQueryData.equals(PUMP_1_ON_CALLBACK)) {
    digitalWrite(PUMP_1, 1);
  } else  if (callbackQueryData.equals(PUMP_1_OFF_CALLBACK)) {
    digitalWrite(PUMP_1, 0);
  }  else  if (callbackQueryData.equals(PUMP_2_ON_CALLBACK)) {
    digitalWrite(PUMP_2, 1);
  } else  if (callbackQueryData.equals(PUMP_2_OFF_CALLBACK)) {
    digitalWrite(PUMP_2, 0);
  } else if (callbackQueryData.equals(VEGA_CALLBACK)) {
    period_milis_light1 = 8 * 60 * 60 * 1000;
    period_milis_light2 =  8 * 60 * 60 * 1000;
  } else if (callbackQueryData.equals(BLOOM_CALLBACK)) {
    period_milis_light1 = 12 * 60 * 60 * 1000;
    period_milis_light2 =  12 * 60 * 60 * 1000;
  } else if (callbackQueryData.equals(FACTOR_1)) {
    cycle_factor = 1;  
  }
  else if (callbackQueryData.equals(FACTOR_2)) {
    cycle_factor = 2;
  } else if (callbackQueryData.equals(FACTOR_3)) {
    cycle_factor = 3;
  }
  else if (callbackQueryData.equals(CAHNGE_PUMP)) {
    next_cycle_copot = 1;
  }
    else if (callbackQueryData.equals(BEGIN_COPMPOT)) {
    next_cycle_copot = 0;
  }
  else if (callbackQueryData.equals(AUTO_MODE)) {
    mode = 0;
  }
  else if (callbackQueryData.equals(MANUAL_MODE)) {
    mode = 1;
  }
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
      period_milis_light1_prev = curr_millis - next_start;
      Serial.println("Set on timer 1 " + String(period_milis_light1_prev)); 
    }
    else if(timer == "2"){
      int next_start = t1 * 60 * 1000;
      period_milis_light2_prev = curr_millis - next_start;
      Serial.println("Set on timer 1 " + String(period_milis_light2_prev)); 
    }
    else if(timer == "3"){
      int next_start = t1 * 60 * 1000;
      period_milis_pump1_prev = curr_millis - next_start;
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

String GetStatus(){
  int controlLight1 = digitalRead(LIGHT_1);
  int controlLight2 = digitalRead(LIGHT_2);
  int controlPump1 = digitalRead(PUMP_1);
  int controlPump2 = digitalRead(PUMP_2);
  int controlWet = analogRead(WET_CONTROL);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int __millis = curr_millis;

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

   String name_pump = GetPumpName();
   String pump_1 = String("Полив: " ) +  name_pump + ", ост. "
  + String((period_milis_pump1 - __millis + period_milis_pump1_prev)/(60*1000)) + String(" время включения: ")
  + String(flow_pump1/1000);

  String temp = String("темп: "  ) + String(t);
  String hum = String("влаж: "  ) +  String(h);
  
  return String("Текущий статус")+ String("\r\n") + status_1 + String("\r\n") + status_2 + String("\r\n") + String(pump_1) + String("\r\n") + hum + temp + String("\r\n");
}

void StatusForChat(int chat_id_1){
  String statusdevice = GetStatus();
  LogToTelegrammForChat(statusdevice, CHAT_ID);
  Serial.println("Текущий статус" + statusdevice );
  //SaveSettings();
}

void LogToTelegrammForChat(String  message, String chat_id){
  myBot.sendMessage(message, chat_id);
}

void LogToTelegramm(String  message){
  int i = 0;
    for (i = 0; i < 10; i = i + 1) {
      if(0 == chat_ids[i]){
        break;
      }
      myBot.sendMessage(message, String(chat_ids[i]));
    }
}
/*+++++++++++++ ОБРАБОТКА КОНЕЦ+++++++++++++++++++++++*/

/*+++++++++++++ СВЕТ НАЧАЛО+++++++++++++++++++++++*/
void PeriodLight1(){
  int calc_time = curr_millis - period_milis_light1_prev;
  int controlLight1 = digitalRead(LIGHT_1);
  if (calc_time < period_milis_light1)  
  {       
    if(controlLight1 == 1)
    {
      digitalWrite(LIGHT_1, 0);
      LogToTelegramm("Выключен свет");       
    }               
  }
  else
  {  
    if(controlLight1 == 0)
    {
        digitalWrite(LIGHT_1, 1);
        LogToTelegramm("Включен свет");
    }
  }  
  if(calc_time > 24 *60 *60 * 1000){
    period_milis_light1_prev = curr_millis;
  }
}

void PeriodLight2(){
  int calc_time = curr_millis - period_milis_light2_prev;
  int controlLight2 = digitalRead(LIGHT_2);
  if (calc_time < period_milis_light2)  
  { 
      if(controlLight2 == 1)
      {
        digitalWrite(LIGHT_2, 0);
        LogToTelegramm("Выключена вентиляция");   
      }     
  }
  else
  {  
    if(controlLight2 == 0)
    {
        digitalWrite(LIGHT_2, 1);
        LogToTelegramm("Включена вентиляция");
    }
  }  
  if(calc_time > 24 *60 *60 * 1000){
    period_milis_light2_prev = curr_millis;
  }
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
/*+++++++++++++ СВЕТ КОНЕЦ+++++++++++++++++++++++*/


/*+++++++++++++ НАСОСЫ НАЧАЛО+++++++++++++++++++++++*/
void PeriodPump(){  
  int pump = PUMP_1; 
  if(next_cycle_copot == 0){
    pump = PUMP_2;
  }
  else{
    pump = PUMP_1;
  }
  String name_pump = GetPumpName();
  if (curr_millis - period_milis_pump1_prev > period_milis_pump1)  
   { 
      int controlPump1 = digitalRead(pump);
      if(controlPump1 == 0)
      {
        digitalWrite(pump, 1);
        LogToTelegramm("Включен полив " + name_pump);   
      }     
      int period = curr_millis - period_milis_pump1_prev - period_milis_pump1;           
      if(period > flow_pump1)
      {        
        digitalWrite(pump, 0);
        if(next_cycle_copot == 0){
          next_cycle_copot += cycle_factor;
        }
        else{
          next_cycle_copot -=1;
        }
        LogToTelegramm("Выключен полив " + name_pump);
        period_milis_pump1_prev = curr_millis;
      } 
   }  
}

String GetPumpName(){
  if(next_cycle_copot == 0)
  {
    return "компот";
  }
  else{
    return  "вода";
  }
}

/*+++++++++++++ НАСОСЫ КОНЕЦ +++++++++++++++++++++++*/

/*+++++++++++++ ЛОГИКА  КОНЕЦ+++++++++++++++++++++++*/

/*+++++++++++++ Настройки  НАЧАЛО+++++++++++++++++++++++*/
void SaveSettings(){
  int cur_poss_memory = 0;
  EEPROM.put(0, period_milis_light1_prev);  
  cur_poss_memory += sizeof(period_milis_light1_prev);
  EEPROM.put(cur_poss_memory, period_milis_light2_prev); 

  cur_poss_memory += sizeof(period_milis_light2_prev);
  EEPROM.put(cur_poss_memory, period_milis_light1); 

    cur_poss_memory += sizeof(period_milis_light1);
  EEPROM.put(cur_poss_memory, period_milis_light2); 

    cur_poss_memory += sizeof(period_milis_light2);
  EEPROM.put(cur_poss_memory, period_milis_pump1); 

    cur_poss_memory += sizeof(period_milis_pump1);
  EEPROM.put(cur_poss_memory, period_milis_pump1_prev); 

  cur_poss_memory += sizeof(period_milis_pump1_prev);
  EEPROM.put(cur_poss_memory, flow_pump1);
  
  cur_poss_memory += sizeof(flow_pump1);
  EEPROM.put(cur_poss_memory, chat_ids[0]); 

  cur_poss_memory += sizeof(chat_ids[0]);
  EEPROM.put(cur_poss_memory, chat_ids[1]); 

  start_millis = curr_millis;
  delta_millis = millis();
  
  cur_poss_memory += sizeof(chat_ids[1]);
  EEPROM.put(cur_poss_memory, start_millis); 
  
  Serial.println(" Сохраняем настройки текущее время " + String(start_millis/(60*1000))  );
   EEPROM.commit();
  
}

void ReadSettings(){
  int cur_poss_memory = 0;

  EEPROM.get(0, period_milis_light1_prev);  

  cur_poss_memory += sizeof(period_milis_light1_prev);
  EEPROM.get(cur_poss_memory, period_milis_light2_prev); 

  cur_poss_memory += sizeof(period_milis_light2_prev);
  EEPROM.get(cur_poss_memory, period_milis_light1); 

    cur_poss_memory += sizeof(period_milis_light1);
  EEPROM.get(cur_poss_memory, period_milis_light2); 

    cur_poss_memory += sizeof(period_milis_light2);
  EEPROM.get(cur_poss_memory, period_milis_pump1); 

    cur_poss_memory += sizeof(period_milis_pump1);
  EEPROM.get(cur_poss_memory, period_milis_pump1_prev);

  cur_poss_memory += sizeof(period_milis_pump1_prev);
  EEPROM.get(cur_poss_memory, flow_pump1);
  
  cur_poss_memory += sizeof(flow_pump1);
  //EEPROM.get(cur_poss_memory, chat_ids[0]); 

  cur_poss_memory += sizeof(chat_ids[0]);
  //EEPROM.get(cur_poss_memory, chat_ids[1]); 

  cur_poss_memory += sizeof(chat_ids[1]);
  EEPROM.get(cur_poss_memory, curr_millis); 
  Serial.println("Читаем настройки, время " + String((curr_millis/(60*1000))));

}

void UpdateChat(int chat_id_1){
  int i = 0;
  //StateChats();
  for (i = 0; i < 10; i = i + 1) {
    Serial.println("Обновляем чат - " + String(chat_ids[i]) + String(chat_id_1));
    if(chat_ids[i] == chat_id_1){
      return;
    }
    if(chat_ids[i] == 0)
    {
      chat_ids[i] = chat_id_1;
      return;
    }
  }
  //StateChats();
}

void StateChats(){ 
  Serial.println("Обновляем чат ");
  int i=0;
    for (i = 0; i < 10; i = i + 1) {
      Serial.println("  " + String(chat_ids[i]));
    }
    Serial.println("." );
}

/*+++++++++++++ Настройки КОНЕЦ +++++++++++++++++++++++*/
