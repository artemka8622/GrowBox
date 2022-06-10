#include <EEPROM.h>

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

#define DRIVER_VERSION 0    // 0 - маркировка драйвера дисплея кончается на 4АТ, 1 - на 4Т
#define PUPM_AMOUNT 4       // количество помп, подключенных через реле/мосфет
#define START_PIN 5         // подключены начиная с пина
#define PUMP_PIN 3
#define SWITCH_LEVEL 0      // реле: 1 - высокого уровня (или мосфет), 0 - низкого
#define PARALLEL 0          // 1 - параллельный полив, 0 - полив в порядке очереди
#define TIMER_START 0       // 1 - отсчёт периода с момента ВЫКЛЮЧЕНИЯ помпы, 0 - с момента ВКЛЮЧЕНИЯ помпы

#define PERIOD 0            // 1 - период в часах, 0 - в минутах
#define PUMPING 1           // 1 - время работы помпы в секундах, 0 - в минутах

#define DROP_ICON 1         // 1 - отображать капельку, 0 - будет буква "t" (time)


#define CLK 1
#define DT 2
#define SW 0

int CONTROL_LIGHT_1 = 16;
int CONTROL_LIGHT_2 = 5;
int CONTROL_PUMP_1 = 4;
int CONTROL_PUMP_2 = 2;

int WET_CONTROL = A0;

int LIGHT_1 = 14;
int LIGHT_2 = 12;
int PUMP_1 = 13;
int PUMP_2 = 15;

bool  pump1Enabled = true;
bool  pump2Enabled = true;
bool  light1Enabled = true;
bool  light2Enabled = true;

bool  pump1Change = false;
bool  pump2Change = false;
bool  light1Change = false;
bool  light2Change = false;

byte current_set = 2;
byte current_pump;
boolean reDraw_flag, arrow_update;
boolean now_pumping;
unsigned long period_coef, pumping_coef;

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
  // --------------------- КОНФИГУРИРУЕМ ПИНЫ ---------------------
  pinMode(CONTROL_LIGHT_1, INPUT); 
  pinMode(CONTROL_LIGHT_2, INPUT); 
  pinMode(CONTROL_PUMP_1, INPUT); 
  pinMode(CONTROL_PUMP_2, INPUT); 

  pinMode(LIGHT_1, OUTPUT); 
  pinMode(LIGHT_2, OUTPUT); 
  pinMode(PUMP_1, OUTPUT); 
  pinMode(PUMP_2, OUTPUT); 

  Serial.begin(115200);

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

void loop() {
  periodTick();
  //flowTick();
  delay(500);
}

void periodTick() {
  int controlLight1 = digitalRead(CONTROL_LIGHT_1);
  int controlLight2 = digitalRead(CONTROL_LIGHT_2);
  int controlPump1 = digitalRead(CONTROL_PUMP_1);
  int controlPump2 = digitalRead(CONTROL_PUMP_2);
  int controlWet = analogRead(WET_CONTROL);
  
  if(controlPump1 == HIGH || pump1Change){
    pump1Enabled = !pump1Enabled;
    pump1Change = false;
  }

  if(controlPump2 == HIGH  || pump2Change){
    pump2Enabled = !pump2Enabled;
    pump2Change = false;
  }

  if(controlLight1 == HIGH || light1Change){
    light1Enabled = !light1Enabled;
    light1Change = false;
  }

  if(controlLight2 == HIGH || light2Change){
    light2Enabled = !light2Enabled;
    light2Change = false;
  }
  Serial.println("Контроль влажности 2 - " + String(controlWet));
  //Serial.println(digitalRead(controlLight2));
   //Serial.println("Контроль света 1 - " + String(controlLight1) + " Переключение света 1 - " + String(light1Change));
  //Serial.println("Контроль света 2 - " + String(controlLight2) + " Переключение света 2 - " + String(light2Change));
  //Serial.println("Контроль полив 1 - " + String(controlPump1) + " Переключение полив 1 - " + String(pump1Change));
  //Serial.println("Контроль полив 2 - " + String(controlPump2) + " Переключение полив 2 - " + String(pump2Change));
  digitalWrite(LIGHT_1, light1Enabled); 
  digitalWrite(LIGHT_2, light2Enabled); 
  digitalWrite(PUMP_1, pump1Enabled); 
  digitalWrite(PUMP_2, pump2Enabled); 
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
