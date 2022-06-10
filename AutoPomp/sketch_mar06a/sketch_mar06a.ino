#include <EEPROM.h>

/*  ВЕРСИЯ 1.5! Установи новую версию библиотеки из реопзитория!
   Система управляет количеством помп PUPM_AMOUNT, подключенных подряд в пины платы, начиная с пина START_PIN.
   На каждую помпу заводится таймер, который включает помпу на заданное время через заданные промежутки времени.
   Промежутки времени (период работы) может быть в часах или минутах (настройка PERIOD).
   Время работы помпы может быть в минутах или секундах (настройка PUMPING).
   Включение производится сигналом уровня SWITCH_LEVEL. 0 - для реле низкого уровня (0 Вольт, все семейные модули реле),
   1 - высокого уровня (5 Вольт, редкие модули реле, все мосфеты).
   Примечание: катушка реле кушает около 60 мА, несколько включенных вместе катушек создадут лишнюю нагрузку
   на линию питания. Также несколько включенных одновременно помп сделают то же самое. Для устранения этого эффекта
   есть настройка PARALLEL. При её отключении помпы будут "вставать в очередь", совместное включение будет исключено.
   Управление:
   Нажатие на ручку энкодера - переключение выбора помпы/периода/времени работы
   Поворот ручки энкодера - изменение значения
   Кнопка энкодера удерживается при включении системы - сброс настроек
   Интерфейс отображается на дисплее 1602 с драйвером на I2C. Версий драйвера существует две.
   Смотри настройку DRIVER_VERSION ниже.
   
   Версия с помпой и клапанами: помпа подключена через реле на пин PUMP_PIN. Остальные
   реле подключены начиная с пина 4 и управляют клапанами. Логика такая:
   При срабатывании таймера включается помпа и один из клапанов!
*/

#define DRIVER_VERSION 0    // 0 - маркировка драйвера дисплея кончается на 4АТ, 1 - на 4Т
#define PUPM_AMOUNT 8       // количество помп, подключенных через реле/мосфет
#define START_PIN 4         // подключены начиная с пина
#define PUMP_PIN 3
#define SWITCH_LEVEL 0      // реле: 1 - высокого уровня (или мосфет), 0 - низкого
#define PARALLEL 0          // 1 - параллельный полив, 0 - полив в порядке очереди
#define TIMER_START 0       // 1 - отсчёт периода с момента ВЫКЛЮЧЕНИЯ помпы, 0 - с момента ВКЛЮЧЕНИЯ помпы

#define PERIOD 0            // 1 - период в часах, 0 - в минутах
#define PUMPING 1           // 1 - время работы помпы в секундах, 0 - в минутах

#define DROP_ICON 1         // 1 - отображать капельку, 0 - будет буква "t" (time)

// названия каналов управления. БУКВУ L НЕ ТРОГАТЬ БЛЕТ!!!!!!
static const wchar_t *relayNames[]  = {
  L"Pump 1",
  L"Pump 2",
  L"Pump 3",
  L"Помпа 4",
  L"Огурцы",
  L"Помидоры",
  L"Клубника",
  L"Папин куст",
};

#define CLK 1
#define DT 2
#define SW 0

unsigned long pump_timers[PUPM_AMOUNT];
unsigned int pumping_time[PUPM_AMOUNT];
unsigned int period_time[PUPM_AMOUNT];
unsigned int time_left[PUPM_AMOUNT];
boolean pump_state[PUPM_AMOUNT];
byte pump_pins[PUPM_AMOUNT];

byte current_set = 2;
byte current_pump;
boolean reDraw_flag, arrow_update;
boolean now_pumping;
unsigned long period_coef, pumping_coef;

void setup() {
  // --------------------- КОНФИГУРИРУЕМ ПИНЫ ---------------------
  for (byte i = 0; i < PUPM_AMOUNT; i++) {            // пробегаем по всем помпам
    pump_pins[i] = START_PIN + i;                     // настраиваем массив пинов
    pinMode(START_PIN + i, OUTPUT);                   // настраиваем пины
    digitalWrite(START_PIN + i, !SWITCH_LEVEL);       // выключаем от греха
  }
  pinMode(PUMP_PIN, OUTPUT);
  // --------------------- ИНИЦИАЛИЗИРУЕМ ЖЕЛЕЗО ---------------------
  Serial.begin(9600);

  // --------------------------- НАСТРОЙКИ ---------------------------
  if (PERIOD) period_coef = (long)1000 * (long)60 * 60;  // перевод в часы
  else period_coef = (long)1000 * 60;              // перевод в минуты

  if (PUMPING) pumping_coef = 1000;          // перевод в секунды
  else pumping_coef = (long)1000 * 60;             // перевод в минуты

  // в ячейке 100 должен быть записан флажок 1, если его нет - делаем (ПЕРВЫЙ ЗАПУСК)
  if (EEPROM.read(100) != 1) {
    EEPROM.write(100, 1);

    // для порядку сделаем 1 ячейки с 0 по 99
    for (byte i = 0; i < 100; i++) {
      EEPROM.write(i, 1);
    }
  }

  for (byte i = 0; i < PUPM_AMOUNT; i++) {            // пробегаем по всем помпам
    period_time[i] = EEPROM.read(2 * i);          // читаем данные из памяти. На чётных - период (ч)
    pumping_time[i] = EEPROM.read(2 * i + 1);     // на нечётных - полив (с)
  }
}

void loop() {
  periodTick();
  flowTick();
}

void periodTick() {
  for (byte i = 0; i < PUPM_AMOUNT; i++) {            // пробегаем по всем помпам
    if ( (millis() - pump_timers[i] > ( (long)period_time[i] * period_coef) )
         && (pump_state[i] != SWITCH_LEVEL)
         && !(now_pumping * !PARALLEL)) {
      pump_state[i] = SWITCH_LEVEL;
      digitalWrite(pump_pins[i], SWITCH_LEVEL);   // открыть КЛАПАН
      pump_timers[i] = millis();
      now_pumping = true;
      digitalWrite(PUMP_PIN, SWITCH_LEVEL);       // включить общую ПОМПУ
    }
  }
}

void flowTick() {
  for (byte i = 0; i < PUPM_AMOUNT; i++) {            // пробегаем по всем помпам
    if ( (millis() - pump_timers[i] > ( (long)pumping_time[i] * pumping_coef) )
         && (pump_state[i] == SWITCH_LEVEL) ) {
      pump_state[i] = !SWITCH_LEVEL;
      digitalWrite(pump_pins[i], !SWITCH_LEVEL);   // закрыть КЛАПАН
      if (TIMER_START) pump_timers[i] = millis();
      now_pumping = false;
      digitalWrite(PUMP_PIN, !SWITCH_LEVEL);       // выключить общую ПОМПУ
    }
  }
}

// обновляем данные в памяти
void update_EEPROM() {
  EEPROM.update(2 * current_pump, period_time[current_pump]);
  EEPROM.update(2 * current_pump + 1, pumping_time[current_pump]);
}
