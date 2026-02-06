// This example uses an ESP32 Development Board
// to connect to shiftr.io.
//
// You can check on your device after a successful
// connection here: https://www.shiftr.io/try.
//
// by Joël Gähwiler
// https://github.com/256dpi/arduino-mqtt



// Датчик Холла


#include <WiFi.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include <cmath> // Необходимо для round, ceil, floor, trunc 

#include "HX711.h"       // Библиотека для работы с АЦП HX711
// Создание объектов
HX711 scale;                      // Объект для работы с тензодатчиком
// Определение пинов подключения
#define DT_PIN 21          // Пин DATA (DT) HX711
#define SCK_PIN 22        // Пин CLOCK (SCK) HX711
#define BUTTON_PIN 0  // Пин кнопки обнуления (GPIO0 - обычно кнопка BOOT на ESP32)
// Калибровочные параметры
float calibration_factor = 7.60; // Калибровочный коэффициент (подбирается экспериментально)
float units;                       // Переменная для измерений в граммах
float ounces;                   // Переменная для измерений в унциях

float Power_Watt_ir;                   // Мощность в Ваттах, инфракрасный датчик
int int_PW_ir;
float Power_Watt_holl;                   // Мощность в Ваттах, датчик Холла
int int_PW_holl;
volatile unsigned int COUNT_MAGNIT = 22; // Количество магнитов на оборот для датчика Холла
volatile unsigned int COUNT_PULSE_IR = 1; // Количество пульсаций на оборот для IR датчика

// Функция обнуления весов (тарирование)
void tareScale()
{
  // tft.fillScreen(TFT_BLACK);        // Очистка экрана
  // tft.setTextColor(TFT_GREEN); // Зеленый цвет для лучшей читаемости
  // tft.setTextSize(1);
  // tft.setCursor(10, 10);
  // tft.println("Taring...");              // Сообщение о процессе тарирования
  scale.tare();   // Сброс текущего веса в 0
  Serial.println(".........Сброс текущего веса в 0..........");
  // delay(1000);   // Задержка для стабилизации
  // tft.fillScreen(TFT_BLACK);        // Очистка экрана
  // tft.setTextColor(TFT_GREEN); // Зеленый цвет для лучшей читаемости
  // tft.setTextSize(1);
  // tft.setCursor(10, 10);
  // tft.println("Tared!"); // Сообщение о процессе тарирования
  // delay(1000);           // Пауза перед возвратом к измерениям
}

// #include <MQTTClient.h>

//const char ssid[] = "OnePlus 10 Pro 5G-76e8";
//const char pass[] = "g3se674x";

// const char ssid[] = "4G-UFI-C70D";
// const char pass[] = "9546595465Hotspot!";


// const char ssid[] = "HOMELAB-10";
// const char pass[] = "9546595465Homelab!";


const char ssid[] = "Keenetic-0606";
const char pass[] = "hkCXMKY9";




/* 
//const char MQTT_BROKER_ADRRESS[] = "91.149.232.230";  // CHANGE TO MQTT BROKER'S ADDRESS
const char MQTT_BROKER_ADRRESS[] = "77.51.217.41"; // CHANGE TO MQTT BROKER'S ADDRESS
const int MQTT_PORT = 1883;
//const int MQTT_PORT = 8883;
const char MQTT_CLIENT_ID[] = "YOUR-NAME-esp32-001"; // CHANGE IT AS YOU DESIRE
const char MQTT_USERNAME[] = "userMosquitoPSP"; // CHANGE IT IF REQUIRED, empty if not required
const char MQTT_PASSWORD[] = "9546595465Psp!"; // CHANGE IT IF REQUIRED, empty if not required
 */

//const char MQTT_BROKER_ADRRESS[] = "91.149.232.230";  // CHANGE TO MQTT BROKER'S ADDRESS
// const char MQTT_BROKER_ADRRESS[] = "192.168.100.237"; // CHANGE TO MQTT BROKER'S ADDRESS
// const char MQTT_BROKER_ADRRESS[] = "192.168.10.72"; // CHANGE TO MQTT BROKER'S ADDRESS
const char MQTT_BROKER_ADRRESS[] = "192.168.20.152"; // CHANGE TO MQTT BROKER'S ADDRESS

const int MQTT_PORT = 1883;
//const int MQTT_PORT = 8883;
const char MQTT_CLIENT_ID[] = "Umbrella-esp32-001"; // CHANGE IT AS YOU DESIRE
const char MQTT_USERNAME[] = "telegraf"; // CHANGE IT IF REQUIRED, empty if not required
const char MQTT_PASSWORD[] = "telegraf"; // CHANGE IT IF REQUIRED, empty if not required


//Внешний датчик Холла
const int hallPin = 25;     // Пин, к которому подключен DO датчика

int hallState = 0;          // Состояние датчика Холла
volatile unsigned int holl_pulseCount = 0; // Счетчик импульсов датчика Холла
volatile unsigned int holl_pulseCount_ditry = 0; // Счетчик импульсов датчика Холла c Дребезгом
unsigned long lastMillis_rpm = 0;
float holl_rpm = 0; // Оборотов в минуту датчкика Холла

const int ir_Pin = 18;     // Пин, к которому подключен DO датчика Инфракрасного
int ir_State = 0;          // Состояние датчика Инфракрасного
volatile unsigned int ir_pulseCount = 0; // Счетчик импульсов датчика Инфракрасного
volatile unsigned int ir_pulseCount_ditry = 0; // Счетчик импульсов датчика Инфракрасного c Дребезгом
float ir_rpm = 0; // Оборотов в минуту  датчика Инфракрасного

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;
unsigned long lastMillis_wifi = 0;
unsigned long lastMillis_mqtt = 0;
unsigned long count_fps = 0;
//unsigned long time = 0;

void connect() {
  Serial.print("checking wifi...  (Проверка WiFi... ) ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".//.");
    delay(5000);
  }


  Serial.print("\nconnecting...");
  while (!client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.print(".>.");
    delay(3000);
  }

  Serial.println("\nconnected!");


}

void messageReceived(String &topic, String &payload) {
  //Serial.println("incoming: " + topic + " - " + payload);

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

volatile unsigned long turnover = 0;
volatile unsigned long last_turnover = 0;
volatile unsigned long turnover_time = 0; 


const int drebezg_time = 200;       // Длина времени на дребезг, микросекунд
                                      // 20000 микросекунд = 50 Гц = 3000 об\мин
                                      // 5000 vмикросекунд = 200 Гц = 12000 об\мин

// Прерывание: срабатывает при появлении магнита
void IRAM_ATTR handleInterrupt() {
  turnover = micros()-last_turnover; //Вычисляет время между двумя обротами (почему двумя а не одним??)
  if (turnover > drebezg_time)
  {
    turnover_time=turnover;
    Serial.println(turnover_time);
    last_turnover=micros();
    holl_pulseCount++;
  }
  holl_pulseCount_ditry++;
}

volatile unsigned long ir_turnover = 0;
volatile unsigned long ir_last_turnover = 0;
// volatile unsigned long ir_turnover_time = 0; 

const int ir_drebezg_time = 5000;       // Длина времени на дребезг, микросекунд датчика Инфракрасного
                                      // 20000 микросекунд = 50 Гц = 3000 об\мин
                                      // 5000 vмикросекунд = 200 Гц = 12000 об\мин

// Прерывание: срабатывает при появлении сигнала
void IRAM_ATTR ir_handleInterrupt() {
  ir_turnover = micros()-ir_last_turnover; //Вычисляет время между двумя обротами (почему двумя а не одним??)
  if (ir_turnover > ir_drebezg_time)
  {
    // ir_turnover_time=ir_turnover;
    Serial.println(ir_turnover);
    ir_last_turnover=micros();
    ir_pulseCount++;
  }
  ir_pulseCount_ditry++;
}


void setup() {
  //Serial.begin(115200);
  Serial.begin(9600);
  WiFi.begin(ssid, pass);
  esp_log_level_set("wifi", ESP_LOG_VERBOSE);
  Serial.println("\nПодключено к Wi-Fi!");
  Serial.print("IP адрес: ");
  Serial.println(WiFi.localIP());
  client.begin(MQTT_BROKER_ADRRESS, net);  
  client.onMessage(messageReceived);
  connect();

  // pinMode(ledPin, OUTPUT);
  pinMode(hallPin, INPUT); // Датчик A3144 выдает логический сигнал
  pinMode(ir_Pin, INPUT); // Датчик  выдает логический сигнал

  //pinMode(hallPin, INPUT_PULLUP); // Используем встроенную подтяжку, если модуль без нее
  attachInterrupt(digitalPinToInterrupt(hallPin), handleInterrupt, FALLING); // FALLING - переход с HIGH на LOW

  //pinMode(hallPin, INPUT_PULLUP); // Используем встроенную подтяжку, если модуль без нее
 // attachInterrupt(digitalPinToInterrupt(ir_Pin), ir_handleInterrupt, FALLING); // FALLING - переход с HIGH на LOW

  // Далее для тензодатчика
   pinMode(BUTTON_PIN, INPUT_PULLUP); // Подтяжка к питанию (кнопка замыкает на GND)
  // // Инициализация TFT-дисплея
  // tft.init();                  // Инициализация дисплея
  // tft.setRotation(3);          // Установка ориентации (3 = 180 градусов)
  // tft.fillScreen(TFT_BLACK);   // Очистка экрана
  // tft.setTextColor(TFT_GREEN); // Зеленый цвет для лучшей читаемости
  // tft.setTextSize(1);
  // tft.setCursor(10, 10);
  // tft.println("Init..."); // Сообщение о инициализации
  // // Инициализация весов
  scale.begin(DT_PIN, SCK_PIN);        // Инициализация HX711 с указанием пинов
  scale.set_scale();                             // Установка масштаба (без коэффициента)
  scale.tare();                                     // Первоначальное обнуление веса
  scale.set_scale(calibration_factor); // Установка калибровочного коэффициента
  
}


void loop() {

  client.loop();
  //delay(500);  // <- fixes some issues with WiFi stability
  count_fps=count_fps+1; // счетчик итераций
  Power_Watt_ir = 0; // Сброс мощности
  int_PW_ir=0;
  Power_Watt_holl = 0; // Сброс мощности
  int_PW_holl=0;

  // Временной флаг для соединения по MQTT
  lastMillis_mqtt = millis();

  // publish a message roughly every second.
  // По моему тут коннектимся к МКУТТ серверу на чаще раза в секунду, если коннекта нету
  if (millis() - lastMillis_mqtt > 3000) {
    lastMillis_mqtt = millis();
    if (!client.connected()) {
      connect();
    }

   }
    

   // Расчет всего каждую секунду
   if (millis() - lastMillis_rpm >= 1000) {

     detachInterrupt(digitalPinToInterrupt(hallPin)); // Отключаем прерывания на время расчета
     detachInterrupt(digitalPinToInterrupt(ir_Pin)); // Отключаем прерывания на время расчета

     //client.publish("/hello", "world"); // Проверка связи
 
     // RPM = (импульсы за сек) * 60
     holl_rpm = round((holl_pulseCount * 60)/COUNT_MAGNIT); 
 
     Serial.print("holl_rpm---->: ");
     Serial.print(holl_rpm);
     Serial.print(" ===== количество магнитов: ");
     Serial.println(COUNT_MAGNIT);
 
     Serial.print("holl_pulseCount: ");
     Serial.println(holl_pulseCount);
 
     Serial.print("holl_pulseCount_ditry: ");
     Serial.println(holl_pulseCount_ditry);

     Serial.print("count_fps: ");
     Serial.println(count_fps);




    // Датчик инфракрасный - НАЧАЛО
     // RPM = (импульсы за сек) * 60
     ir_rpm = (round(ir_pulseCount * 60)/COUNT_PULSE_IR); 
 
     Serial.print("IR_RPM: ");
     Serial.println(ir_rpm);
 
     Serial.print("IR_pulseCount: ");
     Serial.println(ir_pulseCount);
 
     Serial.print("ir_pulseCount_ditry: ");
     Serial.println(ir_pulseCount_ditry);

    // Датчик инфракрасный - КОНЕЦ

    bool reading = digitalRead(BUTTON_PIN); // Чтение текущего состояния кнопки
      if (reading == LOW)
      {
        // delay(20);
        if (reading == LOW)
        {
          tareScale(); // Вызов функции обнуления весов
        }
      }
    // Измерение веса с усреднением
    // ounces - унции
    // units - граммы

    ounces = 0; // Обнуление переменной для накопления значений
    ounces = scale.get_units(15); // Каждый вызов функции возвращает среднее значение из 5 измерений
    units = round(ounces * 0.035274); // Конвертация в граммы (1 унция = 28.3495 грамм), округляем до целых
    // Вывод в Serial Monitor для отладки
    Serial.print("Weight: ");
    Serial.print(units, 2); // Вывод с двумя знаками после запятой
    Serial.println(" grams");
  
    // Расчет мощности Инфракрасный датчик
    Power_Watt_ir = units * 0.00981 * ir_rpm * 0.1047;
    int_PW_ir = round(Power_Watt_ir);
    Serial.print("Power_Watt_ir: ");
    Serial.print(Power_Watt_ir, 2); // Вывод с двумя знаками после запятой
    Serial.println(" Watt");

    // char buffer[12]; // Буфер достаточного размера
    // sprintf(buffer, "%i", int_PW_ir); // %lu для unsigned long
    // client.publish("/int_PW_ir", buffer);

    // Расчет мощности датчик Холла
    Power_Watt_holl = units * 0.00981 * holl_rpm * 0.1047;
    int_PW_holl = round(Power_Watt_holl);
    Serial.print("Power_Watt_holl: ");
    Serial.print(Power_Watt_holl, 2); // Вывод с двумя знаками после запятой
    Serial.println(" Watt");

    // // char buffer[12]; // Буфер достаточного размера
    // sprintf(buffer, "%i", int_PW_holl); // %lu для unsigned long
    // client.publish("/int_PW_holl", buffer);


    // Создание JSON документа
    StaticJsonDocument<300> doc;
    doc["sensor"] = "esp32_01";
    doc["count_fps"] = count_fps; 
    doc["lastMillis_rpm"] = lastMillis_rpm; 
    doc["holl_pulseCount"] = holl_pulseCount;
    doc["holl_pulseCount_ditry"] = holl_pulseCount_ditry;
    doc["holl_rpm"] = holl_rpm;
    doc["ir_pulseCount"] = ir_pulseCount;
    doc["ir_pulseCount_ditry"] = ir_pulseCount_ditry;
    doc["ir_rpm"] = ir_rpm;
    doc["units"] = units;
    doc["COUNT_MAGNIT"] = COUNT_MAGNIT;
    doc["COUNT_PULSE_IR"] = COUNT_PULSE_IR;
    doc["int_PW_holl"] = int_PW_holl;
    doc["int_PW_ir"] = int_PW_ir;

    


    char jsonBuffer[300];
    serializeJson(doc, jsonBuffer); // Преобразование в строку

     // Публикация в MQTT топик
     client.publish("/UmbrellaEsp32/data", jsonBuffer);
      
     holl_pulseCount = 0;            // Сбрасываем счетчик датчика Холла
     holl_pulseCount_ditry = 0;

     ir_pulseCount = 0;            // Сбрасываем счетчик датчика инфракрасного
     ir_pulseCount_ditry = 0;

     lastMillis_rpm = millis(); // Обновляем время
     count_fps=0;               // Сбрасываем счетчик fps

     attachInterrupt(digitalPinToInterrupt(hallPin), handleInterrupt, FALLING); // Включаем прерывания датчика Холла  
     attachInterrupt(digitalPinToInterrupt(ir_Pin), ir_handleInterrupt, FALLING); // Включаем прерывания Инфракрасного датчика

   }
  
}