#include <DHT.h>
#include <EEPROM.h>
#include <UniversalTelegramBot.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

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


String chat_id = "294499886";
byte current_set = 2;
byte current_pump;
boolean reDraw_flag, arrow_update;
boolean now_pumping;
unsigned long period_coef, pumping_coef;

WiFiClientSecure client;
UniversalTelegramBot bot(token, client);

void setup() {
    // initialize the Serial
  Serial.begin(115200);
  Serial.println("Starting TelegramBot...");

  dht.begin();
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

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
  +String("Вылючить насос № 2 - ") + command_pump2_off  +   String("\n");
  Serial.println(welcome);
  Status();
  LogToTelegramm(welcome);

  // --------------------------- НАСТРОЙКИ ---------------------------
  //if (PERIOD) period_coef = (long)1000 * (long)60 * 60;  // перевод в часы
  //else period_coef = (long)1000 * 60;              // перевод в минуты

 // if (PUMPING) pumping_coef = 1000;          // перевод в секунды
  //else pumping_coef = (long)1000 * 60;             // перевод в минуты

  // в ячейке 100 должен быть записан флажок 1, если его нет - делаем (ПЕРВЫЙ ЗАПУСК)
 // if (EEPROM.read(100) != 1) {
 //   EEPROM.write(100, 1);

    // для порядку сделаем 1 ячейки с 0 по 99
 //   for (byte i = 0; i < 100; i++) {
  //    EEPROM.write(i, 1);
 //   }
  //}

  //for (byte i = 0; i < PUPM_AMOUNT; i++) {            // пробегаем по всем помпам
  //  period_time[i] = EEPROM.read(2 * i);          // читаем данные из памяти. На чётных - период (ч)
  //  pumping_time[i] = EEPROM.read(2 * i + 1);     // на нечётных - полив (с)
  //}
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
  else if(command == command_status){
    Status();
  }
  else{
    LogToTelegramm("Выберите команду.");
    return;
  }  
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
      }     
      int period = millis() - period_milis_light1_prev - period_milis_light1;           
      if(period > flow_light1)
      {        
        digitalWrite(LIGHT_1, 0);
        LogToTelegramm("Выключен свет № 1");
        period_milis_light1_prev = millis();
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
      }     
      int period = millis() - period_milis_light2_prev - period_milis_light2;           
      if(period > flow_light2)
      {        
        digitalWrite(LIGHT_2, 0);
        LogToTelegramm("Выключен свет № 2");
        period_milis_light2_prev = millis();
      } 
   }  
}

int period_milis_pump1 = 24*60*60*1000;
int flow_pump1 = 15*1000;
int period_milis_pump1_prev = 0;
void PeriodPump1(){
  if (millis() - period_milis_pump1_prev > period_milis_pump1)  
   { 
      int controlPump1 = digitalRead(PUMP_1);
      if(controlPump1 == 0)
      {
        digitalWrite(PUMP_1, 1);
        LogToTelegramm("Включен полив № 1");   
      }     
      int period = millis() - period_milis_pump1_prev - period_milis_pump1;           
      if(period > flow_pump1)
      {        
        digitalWrite(PUMP_1, 0);
        LogToTelegramm("Выключен полив № 1");
        period_milis_pump1_prev = millis();
      } 
   }  
}

int period_milis_pump2 = 24*60*60*1000;
int flow_pump2 = 15*1000;
int period_milis_pump2_prev = 0;
void PeriodPump2(){
  if (millis() - period_milis_pump2_prev > period_milis_pump2)  
   { 
      int controlPump2 = digitalRead(PUMP_2);
      if(controlPump2 == 0)
      {
        digitalWrite(PUMP_2, 1);
        LogToTelegramm("Включен полив № 2");   
      }     
      int period = millis() - period_milis_pump2_prev - period_milis_pump2;           
      if(period > flow_pump2)
      {        
        digitalWrite(PUMP_2, 0);
        LogToTelegramm("Выключен полив № 1");
        period_milis_pump2_prev = millis();
      } 
   }  
}

int Bot_mtbs = 1000; //mean time between scan messages
long Bot_lasttime;   //last time messages' scan has been done

void loop() {
  periodTick();
  PeriodLight1();
  PeriodLight2();
  PeriodPump1();
  PeriodPump2();

  if (millis() > Bot_lasttime + Bot_mtbs)  {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  while(numNewMessages) {
    Serial.println("got response");
    for (int i=0; i<numNewMessages; i++) {
      chat_id = bot.messages[i].chat_id;
      CheckCommand(bot.messages[i].text);
    }
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }

  Bot_lasttime = millis();
}  
}

void Light1On(){
  LogToTelegramm("Включен свет 1 - off");
}

void Light1Off(){
  LogToTelegramm("Выключен свет 1 - off");
}

void LogToTelegramm(String  message){
  bot.sendMessage(chat_id, message);
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
  
  +String("Насос № 1 - "      ) +     GetStatusDevice(controlPump1) 
    + String(" Таймер1 ") + String((period_milis_pump1 - __millis + period_milis_pump1_prev)/(60*1000)) + String(" Таймер2 ") 
  + String((flow_pump1 - (__millis - period_milis_pump1_prev - period_milis_pump1))/(60*1000)) + String("\n")
  
  +String("Насос № 2 - "      ) +    GetStatusDevice(controlPump2)  
    + String(" Таймер1 ") + String((period_milis_pump2 - __millis + period_milis_pump2_prev)/(60*1000)) + String(" Таймер2 ") 
  + String((flow_pump2 - (__millis - period_milis_pump2_prev - period_milis_pump2))/(60*1000)) + String("\n")
  
  +String("Температура воздуха - "  ) + String(t)   + String("\n")
  +String("Влажность воздуха - "  ) +  String(h)         + String("\n")
  +String("Влажность почвы - "    ) +  String(controlWet)     + String("\n");
  Serial.println(status_);
  bot.sendMessage(chat_id, status_, "");
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

void periodTick() {

}

void flowTick() {
  //for (byte i = 0; i < PUPM_AMOUNT; i++) {            // пробегаем по всем помпам
   // if ( (millis() - pump_timers[i] > ( (long)pumping_time[i] * pumping_coef) )
   //      && (pump_state[i] == SWITCH_LEVEL) ) {
  //    pump_state[i] = !SWITCH_LEVEL;
   //   digitalWrite(pump_pins[i], !SWITCH_LEVEL);   // закрыть КЛАПАН
   //   if (TIMER_START) pump_timers[i] = millis();
   //   now_pumping = false;
   //   digitalWrite(PUMP_PIN, !SWITCH_LEVEL);       // выключить общую ПОМПУ
   // }
 // }
}

// обновляем данные в памяти
void update_EEPROM() {
 // EEPROM.write(2 * current_pump, period_time[current_pump]);
 // EEPROM.write(2 * current_pump + 1, pumping_time[current_pump]);
}
