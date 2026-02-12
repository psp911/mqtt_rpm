// Программа измеряет обороты и силу на реактивной штанге

// Если нужен Wi-Fi и MQTT, то использовать эти библиотеки
#include <WiFi.h>
#include <MQTT.h>

#include <ArduinoJson.h>
#include <cmath> // Необходимо для round, ceil, floor, trunc 
#include <LiquidCrystal_I2C.h> // Двухстрочный дисплей 1602
#include "HX711.h"       // Библиотека для работы с АЦП HX711
// Создание объектов
HX711 scale;                      // Объект для работы с тензодатчиком
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

// Будем ли пользовать Wi-Fi и публиковать в MQTT
boolean wifi_mqtt_ON = false; // Значение, использовать ли коннект и публикацию

// Определение пинов подключения весов
#define DT_PIN 21          // Пин DATA (DT) HX711
#define SCK_PIN 22        // Пин CLOCK (SCK) HX711      

// Определение пинов подключения Дисплея 1602
int sda_pin = 16; // GPIO16 as I2C SDA
int scl_pin = 17; // GPIO17 as I2C SCL

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


// const char ssid[] = "OpenWrt";
// const char pass[] = "9546595465Openwrt!";


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
// const char MQTT_BROKER_ADRRESS[] = "192.168.20.152"; // CHANGE TO MQTT BROKER'S ADDRESS

const char MQTT_BROKER_ADRRESS[] = "192.168.1.170"; // CHANGE TO MQTT BROKER'S ADDRESS


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


void connect() {

  Serial.println("Мы тут 50 ======== ");

  Serial.print("checking wifi...  (Проверка WiFi... ) ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".//.");
    // lcd.setCursor(0,0);
    // lcd.print("WiFi conn...");
    Serial.println("Мы тут 60 ======== ");
    delay(2000);
  }

  Serial.println("Мы тут 70 ======== ");
  Serial.println("Подключено к Wi-Fi!");

  
  // lcd.setCursor(0,1);
  // lcd.print("WiFi connected!");
  // delay(2000);
  // lcd.clear();

  Serial.print("IP адрес: ");
  Serial.print(WiFi.localIP());
  
  // lcd.setCursor(0,0);
  // lcd.print(WiFi.localIP());

  Serial.println("===============");
  Serial.print("Теперь подключаемся к MQTT брокеру на адресе:");
  Serial.println(MQTT_BROKER_ADRRESS);

  Serial.print("\nconnecting... (Соединение с MQTT брокером...)");
  while (!client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {


    Serial.println("Мы тут 80 ======== ");
    Serial.print(".>.");

    // lcd.setCursor(0,1);
    // lcd.print(".>.");
    delay(1000);
  }


  Serial.println("Мы тут 90 ======== ");
  Serial.println("\nconnected!");

  // lcd.setCursor(0,1);
  // lcd.print("MQTT connected!");
  // delay(2000);

}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

volatile unsigned long turnover = 0;
volatile unsigned long last_turnover = 0;
volatile unsigned long turnover_time = 0; 


const int drebezg_time = 150;       // Длина времени на дребезг, микросекунд
                                    // 20000 микросекунд = 50 Гц = 3000 об\мин
                                    // 5000 vмикросекунд = 200 Гц = 12000 об\мин
                                    // При 10000 об/мин и 22 магнитах импульсов 3666, 
                                    //         интервал 272 мкс между импульсами

// Прерывание: срабатывает при появлении магнита
void IRAM_ATTR handleInterrupt() {
  turnover = micros()-last_turnover; //Вычисляет время между двумя обротами (почему двумя а не одним??)
  if (turnover > drebezg_time)
  {
    last_turnover=micros();
    holl_pulseCount++;
  }
  holl_pulseCount_ditry++;
}

volatile unsigned long ir_turnover = 0;
volatile unsigned long ir_last_turnover = 0;

const int ir_drebezg_time = 5000;     // Длина времени на дребезг, микросекунд датчика Инфракрасного
                                      // 20000 микросекунд = 50 Гц = 3000 об\мин
                                      // 5000 vмикросекунд = 200 Гц = 12000 об\мин

// Прерывание: срабатывает при появлении сигнала
void IRAM_ATTR ir_handleInterrupt() {
  ir_turnover = micros()-ir_last_turnover; //Вычисляет время между двумя обротами (почему двумя а не одним??)
  if (ir_turnover > ir_drebezg_time)
  {
    ir_last_turnover=micros();
    ir_pulseCount++;
  }
  ir_pulseCount_ditry++;
}


void setup() {
  
  Serial.begin(9600);
 

  Wire.setPins(sda_pin, scl_pin); // Set the I2C pins before begin
	lcd.init(sda_pin, scl_pin); // initialize the lcd to use user defined I2C pins

	lcd.backlight();            // Включение подсветки
  Serial.println("Мы тут 100 ========");
  delay(1000);
	lcd.noBacklight();            // Включение подсветки
  Serial.println("Мы тут 200 ========");
  delay(1000);
	lcd.backlight();            // Включение подсветки
	lcd.setCursor(0,0);         // Установить курсор
	lcd.print("Welcome!");
	delay(3000);


  // Если используем WIFI то давайте подключаться к WiFi и MQTT - НАЧАЛО
  if (wifi_mqtt_ON){
    // Serial.println("Мы тут 10 ========");
    WiFi.begin(ssid, pass);
    esp_log_level_set("wifi", ESP_LOG_VERBOSE);

    // Serial.println("Мы тут 20 ======== ");
    client.begin(MQTT_BROKER_ADRRESS, net);  

    // Serial.println("Мы тут 30 ======== ");
    client.onMessage(messageReceived);
    
    // Serial.println("Мы тут 40 ======== ");
    connect();
  }
  // Если используем WIFI то давайте подключаться к WiFi и MQTT - КОНЕЦ


  // pinMode(ledPin, OUTPUT);
  pinMode(hallPin, INPUT); // Датчик A3144 выдает логический сигнал
  pinMode(ir_Pin, INPUT); // Датчик  выдает логический сигнал

  //pinMode(hallPin, INPUT_PULLUP); // Используем встроенную подтяжку, если модуль без нее
  attachInterrupt(digitalPinToInterrupt(hallPin), handleInterrupt, FALLING); // FALLING - переход с HIGH на LOW

  //pinMode(hallPin, INPUT_PULLUP); // Используем встроенную подтяжку, если модуль без нее
  attachInterrupt(digitalPinToInterrupt(ir_Pin), ir_handleInterrupt, FALLING); // FALLING - переход с HIGH на LOW


  Serial.println("Мы тут 4000 ======== Начинаем инициализацию тензодатчика");

  // Далее для тензодатчика
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Подтяжка к питанию (кнопка замыкает на GND)
  // Serial.println("Мы тут 4050 ======== Идет инициализация тензодатчика");
  lcd.setCursor(0,1);         // Установить курсор
	lcd.print("Init tenzo...");

  delay(1000);

  // // Инициализация весов
  scale.begin(DT_PIN, SCK_PIN);        // Инициализация HX711 с указанием пинов

  // Serial.println("Мы тут 4100 ======== Идет инициализация тензодатчика");
  scale.set_scale();                             // Установка масштаба (без коэффициента)

  // Serial.println("Мы тут 4200 ======== Идет инициализация тензодатчика");
  scale.tare();                                     // Первоначальное обнуление веса

  // Serial.println("Мы тут 4300 ======== Идет инициализация тензодатчика");
  scale.set_scale(calibration_factor); // Установка калибровочного коэффициента

  // Serial.println("Мы тут 1000 ======== Выход из Setup");
  lcd.clear();
	lcd.setCursor(0,0);         // Установить курсор
	lcd.print("Congratulation!");
	lcd.setCursor(0,1);         // Установить курсор
	lcd.print("Init completed!");
  
  delay(2000);
  lcd.clear();
  lcd.print("Start of measurements...");
  delay(2000);
  lcd.clear();
  
  lcd.setCursor(0,0);         // Установить курсор
	lcd.print("IR RPM:");
	lcd.setCursor(0,1);
	lcd.print("Tenzo :");
}


void loop() {

  
  count_fps=count_fps+1; // счетчик итераций
  Power_Watt_ir = 0; // Сброс мощности
  int_PW_ir=0;
  Power_Watt_holl = 0; // Сброс мощности
  int_PW_holl=0;


  //// ======= Этот блок отвечает за коннект к MQTT и Wi-Fi ========   НАЧАЛО  ///////
  if (wifi_mqtt_ON){
    client.loop();
    // Тут коннектимся к MQTT серверу на чаще раза во сколько то времени, если коннекта нету
    if (millis() - lastMillis_mqtt >= 3000) {
      lastMillis_mqtt = millis();    
      // Serial.println("Мы тут 2000 ======== тут коннектимся к МКУТТ серверу на");
      if (!client.connected()) {
        connect();
      }
    }
  }
  //// ======= Этот блок отвечает за коннект к MQTT и Wi-Fi ========    КОНЕЦ  ///////

   // Расчет всего каждую секунду
   if (millis() - lastMillis_rpm >= 1000) {

    detachInterrupt(digitalPinToInterrupt(hallPin)); // Отключаем прерывания на время расчета
    detachInterrupt(digitalPinToInterrupt(ir_Pin)); // Отключаем прерывания на время расчета

    // client.publish("/hello", "world"); // Проверка связи
 

     holl_rpm = round((holl_pulseCount * 60)/COUNT_MAGNIT); 
 

    // Датчик инфракрасный - НАЧАЛО
     
     ir_rpm = (round(ir_pulseCount * 60)/COUNT_PULSE_IR); 
 


    // Датчик инфракрасный - КОНЕЦ

    bool reading = digitalRead(BUTTON_PIN); // Чтение текущего состояния кнопки
      if (reading == LOW)
      {
        // delay(20);
        if (reading == LOW)
        {
          tareScale(); // Вызов функции обнуления весов
          lcd.setCursor(0,1);
	        lcd.print("Tenzo : Reset...");
          delay(2000);
          lcd.setCursor(0,1);
	        lcd.print("Tenzo : 0         ");
        }
      }


    // Измерение веса с усреднением
    // ounces - унции
    // units - граммы

    ounces = 0; // Обнуление переменной для накопления значений
    ounces = scale.get_units(15); // Каждый вызов функции возвращает среднее значение из 15 измерений
    units = round(ounces * 0.035274); // Конвертация в граммы (1 унция = 28.3495 грамм), округляем до целых

    // // Расчет мощности Инфракрасный датчик
    Power_Watt_ir = units * 0.00981 * ir_rpm * 0.1047;
    int_PW_ir = round(Power_Watt_ir);


    // char buffer[12]; // Буфер достаточного размера
    // sprintf(buffer, "%i", int_PW_ir); // %lu для unsigned long
    // client.publish("/int_PW_ir", buffer);

    // // Расчет мощности датчик Холла
    Power_Watt_holl = units * 0.00981 * holl_rpm * 0.1047;
    int_PW_holl = round(Power_Watt_holl);


    /* 
    // =========================================================================== //
    //                         вывод данных в терминал                             //
    // =========================================================================== //
    Serial.print("holl_rpm =================================== holl_rpm : ");
    Serial.println(holl_rpm);
    Serial.println(); 
    Serial.print("holl_pulseCount: ");
    Serial.println(holl_pulseCount); 
    Serial.print("holl_pulseCount_ditry: ");
    Serial.println(holl_pulseCount_ditry);
    Serial.print("count_fps: ");
    Serial.println(count_fps);
    Serial.print("Вес на тензодатчике, грамм =================: ");
    Serial.print(units, 2); // Вывод с двумя знаками после запятой
    Serial.println();
    Serial.print("Power_Watt_ir: ");
    Serial.print(Power_Watt_ir, 2); // Вывод с двумя знаками после запятой
    Serial.println(" Watt");
    Serial.print("IR_RPM ============================================== IR_RPM : ");
    Serial.println(ir_rpm);
    Serial.println();
    Serial.print("IR_pulseCount: ");
    Serial.println(ir_pulseCount);
    Serial.print("ir_pulseCount_ditry: ");
    Serial.println(ir_pulseCount_ditry);
    Serial.print("Power_Watt_holl: ");
    Serial.print(Power_Watt_holl, 2); // Вывод с двумя знаками после запятой
    Serial.println(" Watt");

 */

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


    if (wifi_mqtt_ON){
        // Публикация в MQTT топик
        client.publish("/UmbrellaEsp32/data", jsonBuffer);
    }


    serializeJson(doc, Serial); // выдача в порт UART
    Serial.println(); // Перенос строки для удобства

     holl_pulseCount = 0;            // Сбрасываем счетчик датчика Холла
     holl_pulseCount_ditry = 0;

     ir_pulseCount = 0;            // Сбрасываем счетчик датчика инфракрасного
     ir_pulseCount_ditry = 0;

     lastMillis_rpm = millis(); // Обновляем время
     count_fps=0;               // Сбрасываем счетчик fps

    lcd.setCursor(8, 0);
    String s = String((int)round(ir_rpm)); // Результат: целое число
    lcd.print(s);
    lcd.print("    ");

    lcd.setCursor(8, 1);
    s = String((int)round(units)); // Результат: целое число
    lcd.print(s); 
    lcd.print("       ");

     attachInterrupt(digitalPinToInterrupt(hallPin), handleInterrupt, FALLING); // Включаем прерывания датчика Холла  
     attachInterrupt(digitalPinToInterrupt(ir_Pin), ir_handleInterrupt, FALLING); // Включаем прерывания Инфракрасного датчика

   }
  
}