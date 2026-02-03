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


// #include <MQTTClient.h>

//const char ssid[] = "OnePlus 10 Pro 5G-76e8";
//const char pass[] = "g3se674x";

const char ssid[] = "4G-UFI-C70D";
const char pass[] = "9546595465Hotspot!";


// const char ssid[] = "HOMELAB-10";
// const char pass[] = "9546595465Homelab!";



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
const char MQTT_BROKER_ADRRESS[] = "192.168.100.237"; // CHANGE TO MQTT BROKER'S ADDRESS
// const char MQTT_BROKER_ADRRESS[] = "192.168.10.72"; // CHANGE TO MQTT BROKER'S ADDRESS
const int MQTT_PORT = 1883;
//const int MQTT_PORT = 8883;
const char MQTT_CLIENT_ID[] = "Umbrella-esp32-001"; // CHANGE IT AS YOU DESIRE
const char MQTT_USERNAME[] = "telegraf"; // CHANGE IT IF REQUIRED, empty if not required
const char MQTT_PASSWORD[] = "telegraf"; // CHANGE IT IF REQUIRED, empty if not required


//Внешний датчик Холла
const int hallPin = 25;     // Пин, к которому подключен DO датчика



const int ledPin = 2;       // Встроенный светодиод (или внешний)
int hallState = 0;          // Состояние датчика Холла
volatile unsigned int holl_pulseCount = 0; // Счетчик импульсов датчика Холла
volatile unsigned int holl_pulseCount_ditry = 0; // Счетчик импульсов датчика Холла c Дребезгом
unsigned long lastMillis_rpm = 0;
int rpm = 0; // Оборотов в минуту (float)

const int ir_Pin = 18;     // Пин, к которому подключен DO датчика Инфракрасного
int ir_State = 0;          // Состояние датчика Инфракрасного
volatile unsigned int ir_pulseCount = 0; // Счетчик импульсов датчика Инфракрасного
volatile unsigned int ir_pulseCount_ditry = 0; // Счетчик импульсов датчика Инфракрасного c Дребезгом
int ir_rpm = 0; // Оборотов в минуту  датчика Инфракрасного

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;
unsigned long lastMillis_wifi = 0;
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

  /* client.subscribe("/hello");
  client.subscribe("/times");
  client.subscribe("/fps");
  client.subscribe("/rpm");
  client.subscribe("/pulseCount");
  client.unsubscribe("/pulseCount_ditry");
  client.subscribe("/ir_rpm");
  client.subscribe("/ir_pulseCount");
  client.unsubscribe("/ir_pulseCount_ditry");
   */
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


const int drebezg_time = 5000;       // Длина времени на дребезг, микросекунд
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
volatile unsigned long ir_turnover_time = 0; 

const int ir_drebezg_time = 5000;       // Длина времени на дребезг, микросекунд датчика Инфракрасного
                                      // 20000 микросекунд = 50 Гц = 3000 об\мин
                                      // 5000 vмикросекунд = 200 Гц = 12000 об\мин

// Прерывание: срабатывает при появлении сигнала
void IRAM_ATTR ir_handleInterrupt() {
  ir_turnover = micros()-ir_last_turnover; //Вычисляет время между двумя обротами (почему двумя а не одним??)
  if (ir_turnover > ir_drebezg_time)
  {
    ir_turnover_time=ir_turnover;
    Serial.println(ir_turnover_time);
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

  pinMode(ledPin, OUTPUT);
  pinMode(hallPin, INPUT); // Датчик A3144 выдает логический сигнал
  pinMode(ir_Pin, INPUT); // Датчик  выдает логический сигнал

  //pinMode(hallPin, INPUT_PULLUP); // Используем встроенную подтяжку, если модуль без нее
  attachInterrupt(digitalPinToInterrupt(hallPin), handleInterrupt, FALLING); // FALLING - переход с HIGH на LOW

  //pinMode(hallPin, INPUT_PULLUP); // Используем встроенную подтяжку, если модуль без нее
 // attachInterrupt(digitalPinToInterrupt(ir_Pin), ir_handleInterrupt, FALLING); // FALLING - переход с HIGH на LOW


}



void loop() {


   client.loop();


  //delay(500);  // <- fixes some issues with WiFi stability
  count_fps=count_fps+1; // счетчик итераций
  
  // Временной флаг для соединения по WiFi - не по WiFi а по MQTT
  lastMillis_wifi = millis();

  // publish a message roughly every second.
  // По моему тут коннектимся к МКУТТ серверу на чаще раза в секунду, если коннекта нету
  if (millis() - lastMillis_wifi > 3000) {
    //lastMillis = millis();
    if (!client.connected()) {
      connect();
    }

   }


    

   // Расчет RPM каждую секунду
   if (millis() - lastMillis_rpm >= 1000) {

     detachInterrupt(digitalPinToInterrupt(hallPin)); // Отключаем прерывания на время расчета

     detachInterrupt(digitalPinToInterrupt(ir_Pin)); // Отключаем прерывания на время расчета

     //client.publish("/hello", "world");
 
  


     // RPM = (импульсы за сек) * 60
     rpm = (holl_pulseCount * 60); 
 
     Serial.print("RPM: ");
     Serial.println(rpm);
 
     Serial.print("holl_pulseCount: ");
     Serial.println(holl_pulseCount);
 
     Serial.print("holl_pulseCount_ditry: ");
     Serial.println(holl_pulseCount_ditry);

     Serial.print("count_fps: ");
     Serial.println(count_fps);


     //time = millis();
     char buffer[12]; // Буфер достаточного размера
     sprintf(buffer, "%lu", lastMillis_rpm); // %lu для unsigned long
     // Теперь buffer содержит строку, например, "12345"
     client.publish("/times", buffer);
 

     //char buffer[12]; // Буфер достаточного размера
     sprintf(buffer, "%i", count_fps); // %lu для unsigned long
     // Теперь buffer содержит строку, например, "12345"
     client.publish("/fps", buffer);

     sprintf(buffer, "%i", rpm); // %lu для unsigned long
     client.publish("/rpm", buffer);
     sprintf(buffer, "%d", holl_pulseCount); // %lu для unsigned long
     client.publish("/holl_pulseCount", buffer); 
     sprintf(buffer, "%d", holl_pulseCount_ditry); // %lu для unsigned long
     client.publish("/holl_pulseCount_ditry", buffer);    





// Датчик инфракрасный - НАЧАЛО


     // RPM = (импульсы за сек) * 60
     ir_rpm = (ir_pulseCount * 60); 
 
     Serial.print("IR_RPM: ");
     Serial.println(ir_rpm);
 
     Serial.print("IR_pulseCount: ");
     Serial.println(ir_pulseCount);
 
     Serial.print("ir_pulseCount_ditry: ");
     Serial.println(ir_pulseCount_ditry);

     sprintf(buffer, "%i", ir_rpm); // %lu для unsigned long
     client.publish("/ir_rpm", buffer);
     sprintf(buffer, "%d", ir_pulseCount); // %lu для unsigned long
     client.publish("/ir_pulseCount", buffer); 
     sprintf(buffer, "%d", ir_pulseCount_ditry); // %lu для unsigned long
     client.publish("/ir_pulseCount_ditry", buffer);   



// Датчик инфракрасный - КОНЕЦ



  // Создание JSON документа
  StaticJsonDocument<200> doc;
  doc["sensor"] = "esp32_01";
  doc["count_fps"] = count_fps; 
  doc["lastMillis_rpm"] = lastMillis_rpm; 
  doc["holl_pulseCount"] = holl_pulseCount;
  doc["holl_pulseCount_ditry"] = holl_pulseCount_ditry;
  doc["ir_pulseCount"] = ir_pulseCount;
  doc["ir_pulseCount_ditry"] = ir_pulseCount_ditry;

  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer); // Преобразование в строку

  // Публикация в MQTT топик
  client.publish("/UmblellaEsp32/data", jsonBuffer);
  



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