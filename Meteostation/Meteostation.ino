// Подключаем необходимые библиотеки
#include <ESP8266WiFi.h>        // Библиотека для работы с Wi-Fi на ESP8266
#include <ESP8266WebServer.h>   // Библиотека для создания веб-сервера
#include <Wire.h>               // Библиотека для работы с шиной I2C (для датчиков и дисплея)
#include <Adafruit_AHTX0.h>     // Библиотека для датчика температуры/влажности AHT20
#include <Adafruit_BMP280.h>    // Библиотека для датчика температуры/давления BMP280
#include <Adafruit_GFX.h>       // Базовая графическая библиотека Adafruit
// Директива для отключения стандартной заставки Adafruit
#define SSD1306_NO_SPLASH 
#include <Adafruit_SSD1306.h>   // Библиотека для OLED-дисплея на контроллере SSD1306
#include <WiFiManager.h>        // Библиотека для удобной настройки Wi-Fi через веб-портал

// Настройки OLED-дисплея 128x32
#define SCREEN_WIDTH 128          // Ширина дисплея в пикселях
#define SCREEN_HEIGHT 32          // Высота дисплея в пикселях
#define OLED_RESET -1             // Пин сброса (или -1, если не используется)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Создание объекта дисплея

ESP8266WebServer server(80);      // Создание веб-сервера на порту 80

// Создание объектов для датчиков AHT20 и BMP280
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

// Высота вашего местоположения (город) в метрах для коррекции давления
const float altitude_m = 205.0; 

// Переменные для управления сменой страниц на дисплее
unsigned long previousMillis = 0;     // Переменная для хранения времени последней смены страницы
const long interval = 5000;           // Интервал смены страниц (5 секунд)
int currentPage = 0;                  // Текущая отображаемая страница (0 или 1)

// Вспомогательная функция для вывода текста по центру
void printCentered(const String& text, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h); // Получаем границы текста
  int x = (display.width() - w) / 2; // Вычисляем координату X для центрирования
  display.setCursor(x, y);           // Устанавливаем курсор
  display.println(text);             // Выводим текст
}

// Функция для вывода данных AHT20 на OLED-дисплей
void showAHTData(float temp, float hum) {
  display.cp437(true);               // Включаем поддержку кодовой страницы CP437 (для некоторых символов)
  display.clearDisplay();            // Очистка буфера дисплея
  display.setTextSize(1);            // Установка размера текста
  display.setTextColor(WHITE);       // Установка цвета текста
  printCentered("AHT20", 0);         // Вывод названия датчика по центру

  display.setCursor(0, 10);          // Установка курсора в позицию (0, 10)
  display.print(utf8rus("Температура: ")); // Вывод кириллицы через конвертер
  display.print(temp, 1);            // Вывод значения температуры
  display.println(" C");

  display.setCursor(0, 20);          // Установка курсора в позицию (0, 20)
  display.print(utf8rus("Влажность:   ")); // Вывод кириллицы через конвертер
  display.print(hum, 1);             // Вывод значения влажности
  display.println(" %");             
  display.display();                 // Обновление дисплея
}

// Функция для вывода данных BMP280 на OLED-дисплей
void showBMPData(float temp, float pressure) {
  display.cp437(true);               // Включаем поддержку кодовой страницы CP437
  display.clearDisplay();            // Очистка буфера дисплея
  display.setTextSize(1);            // Установка размера текста
  display.setTextColor(WHITE);       // Установка цвета текста
  printCentered("BMP280", 0);        // Вывод названия датчика по центру

  display.setCursor(0, 10);          // Установка курсора в позицию (0, 10)
  display.print(utf8rus("Температура: ")); // Вывод кириллицы через конвертер
  display.print(temp, 1);            // Вывод значения температуры
  display.println(" C");

  display.setCursor(0, 20);          // Установка курсора в позицию (0, 20)
  display.print(utf8rus("Давление:    ")); // Вывод кириллицы через конвертер
  display.print(pressure, 0);        // Вывод значения давления
  display.println(utf8rus(" мм"));   // Вывод единиц измерения и перевод строки
  display.display();                 // Обновление дисплея
}

// Функция для обработки запросов к корневому URL веб-сервера
void handleRoot() {
  sensors_event_t humidity, temp_aht;
  aht.getEvent(&humidity, &temp_aht);
  float temp_bmp = bmp.readTemperature();
  float pressure_hpa = bmp.readPressure() / 100.0; 
  float pressure_sea_level_hpa = bmp.seaLevelForAltitude(altitude_m, pressure_hpa);
  float pressure_mmhg = pressure_sea_level_hpa * 0.750062;

  // ... (расчеты градусов для графиков, как в предыдущем коде) ...
  int humidityPercent = (int)humidity.relative_humidity;
  if (humidityPercent < 0) humidityPercent = 0;
  if (humidityPercent > 100) humidityPercent = 100;
  int hum_deg = (int)((float)humidityPercent * 3.6);

  float temp_val_aht = temp_aht.temperature;
  int temp_deg_aht = (int)(((temp_val_aht + 30.0) / 80.0) * 360.0);
  if(temp_deg_aht < 0) temp_deg_aht = 0;
  if(temp_deg_aht > 360) temp_deg_aht = 360;

  float temp_val_bmp = temp_bmp;
  int temp_deg_bmp = (int)(((temp_val_bmp + 30.0) / 80.0) * 360.0);
  if(temp_deg_bmp < 0) temp_deg_bmp = 0;
  if(temp_deg_bmp > 360) temp_deg_bmp = 360;

  float pressure_val = pressure_mmhg;
  int pressure_deg = (int)((pressure_val / 1000.0) * 360.0);
  if(pressure_deg < 0) pressure_deg = 0;
  if(pressure_deg > 360) pressure_deg = 360;


  if (currentPage == 0) { showAHTData(temp_val_aht, humidity.relative_humidity); } else { showBMPData(temp_val_bmp, pressure_mmhg); }
  
  // *** HTML-код для веб-страницы ***
  String html = "<!DOCTYPE html><html><head>...</head><body>...</body></html>"; 
  html = "<!DOCTYPE html>";
  html += "<html lang=\"ru\">";
  html += "<head>";
  html += "<meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>Метеостанция ESP8266</title>";
  html += "<meta http-equiv=\"refresh\" content=\"15\">";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #2c3e50; color: #ecf0f1; margin: 0; padding: 20px; }";
  html += ".container { max-width: 800px; margin: auto; text-align: center; }";
  html += "h1 { color: #3498db; margin-bottom: 30px; }";
  html += ".dashboard { display: flex; flex-wrap: wrap; justify-content: center; gap: 20px; }";
  html += ".card { background-color: #34495e; padding: 20px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2); width: 150px; }";
  
  // Стили для карточек и центрирования их содержимого с помощью Flexbox
  html += ".card { background-color: #34495e; padding: 20px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2); width: 150px; display: flex; flex-direction: column; justify-content: center; align-items: center; }";
  html += ".value { font-size: 1.5em; font-weight: bold; }";
  html += ".unit { font-size: 1em; color: #7f8c8d; }";

  // Стили для круглого графика
  html += ".circle-wrap { width: 120px; height: 120px; background: #34495e; border-radius: 50%; overflow: hidden; position: relative; }";
  html += ".circle-wrap .circle .mask, .circle-wrap .circle .fill { width: 100%; height: 100%; position: absolute; border-radius: 50%; }";
  html += ".circle-wrap .circle .mask { clip-path: inset(0 50% 0 0); }";
  html += ".circle-wrap .circle .fill { clip-path: inset(0 0 0 50%); background-color: #e67e22; }"; /* Оранжевый для темп */
  html += ".circle-wrap.hum .fill { background-color: #3498db; }"; /* Голубой для влажности */
  html += ".circle-wrap.press .fill { background-color: #9b59b6; }"; /* Фиолетовый для давления */
  html += ".circle-wrap .circle .mask .fill { position: absolute; background-color: #e67e22; }";
  html += ".circle-wrap.hum .mask .fill { background-color: #3498db; }";
  html += ".circle-wrap.press .mask .fill { background-color: #9b59b6; }";
  html += ".circle-wrap .inside-circle { width: 90px; height: 90px; border-radius: 50%; background: #2c3e50; line-height: 90px; text-align: center; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); font-size: 1.5em; font-weight: bold; }";
  
  // Динамическое CSS для поворота графиков
  html += ".circle-wrap.temp-aht .fill { transform: rotate(" + String(temp_deg_aht) + "deg); }";
  html += ".circle-wrap.temp-aht .mask .fill { transform: rotate(" + String(temp_deg_aht) + "deg); }";
  html += ".circle-wrap.temp-bmp .fill { transform: rotate(" + String(temp_deg_bmp) + "deg); }";
  html += ".circle-wrap.temp-bmp .mask .fill { transform: rotate(" + String(temp_deg_bmp) + "deg); }";
  html += ".circle-wrap.hum .fill { transform: rotate(" + String(hum_deg) + "deg); }";
  html += ".circle-wrap.hum .mask .fill { transform: rotate(" + String(hum_deg) + "deg); }";
  html += ".circle-wrap.press .fill { transform: rotate(" + String(pressure_deg) + "deg); }";
  html += ".circle-wrap.press .mask .fill { transform: rotate(" + String(pressure_deg) + "deg); }";
  
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class=\"container\">";
  html += "<h1>Метеостанция балкон</h1>";
  html += "<div class=\"dashboard\">";

  // Виджет Температуры AHT20 (круглый график)
  html += "<div class=\"card\">";
  html += "<h2>Температура AHT20</h2>";
  html += "<div class=\"circle-wrap temp-aht\">";
  html += "<div class=\"circle\"><div class=\"mask half\"><div class=\"fill\"></div></div><div class=\"mask full\"><div class=\"fill\"></div></div></div>";
  html += "<div class=\"inside-circle\">" + String(temp_val_aht, 1) + "</div>";
  html += "</div>";
  html += "</div>";

  // Виджет Температуры BMP280 (круглый график)
  html += "<div class=\"card\">";
  html += "<h2>Температура BMP280</h2>";
  html += "<div class=\"circle-wrap temp-bmp\">";
  html += "<div class=\"circle\"><div class=\"mask half\"><div class=\"fill\"></div></div><div class=\"mask full\"><div class=\"fill\"></div></div></div>";
  html += "<div class=\"inside-circle\">" + String(temp_val_bmp, 1) + "</div>";
  html += "</div>";
  html += "</div>";

  // Виджет Влажности (круглый график)
  html += "<div class=\"card\">";
  html += "<h2>Влажность (%)</h2>";
  html += "<div class=\"circle-wrap hum\">";
  html += "<div class=\"circle\"><div class=\"mask half\"><div class=\"fill\"></div></div><div class=\"mask full\"><div class=\"fill\"></div></div></div>";
  html += "<div class=\"inside-circle\">" + String(humidityPercent) + "%</div>";
  html += "</div>";
  html += "</div>";

  // Виджет Давления (круглый график)
  html += "<div class=\"card\">";
  html += "<h2>Давление (мм рт. ст.)</h2>";
  html += "<div class=\"circle-wrap press\">";
  html += "<div class=\"circle\"><div class=\"mask half\"><div class=\"fill\"></div></div><div class=\"mask full\"><div class=\"fill\"></div></div></div>";
  html += "<div class=\"inside-circle\">" + String(pressure_val, 0) + "</div>";
  html += "</div>";
  html += "</div>";
  
  html += "</div>"; // Закрытие dashboard
  html += "<p style=\"margin-top: 20px;\"><small>IP-адрес: " + WiFi.localIP().toString() + "</small></p>";
  html += "</div>"; // Закрытие container
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html); // Отправка HTML-страницы клиенту
}

// Функция, которая выполняется один раз при запуске
void setup() {
  Serial.begin(115200);             // Инициализация последовательного порта
  Wire.begin();                     // Инициализация шины I2C
  
  // Инициализация OLED-дисплея
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed")); 
    for(;;);                          
  }
  display.display();                
  delay(100);                       

  // Инициализация датчиков
  if (!aht.begin() || !bmp.begin()) { 
    Serial.println("Ошибка при инициализации датчиков. Проверьте подключение!");
    for(;;);
  }
  Serial.println("Датчики инициализированы.");

  // --- Инициализация WiFiManager ---
  WiFiManager wifiManager;

  // Попытка подключения к сохраненной сети. Если не удается, запускается режим AP "МетеостанцияAP".
  if(!wifiManager.autoConnect("МетеостанцияAP")) {
    Serial.println("Не удалось подключиться и настроить AP");
    display.clearDisplay(); printCentered("Ошибка WiFi!", 0); printCentered("Перезапуск...", 16); display.display();
    delay(3000);
    ESP.restart(); // Перезапуск платы в случае неудачи
  }
  
  Serial.println("Подключено к Wi-Fi!");
  Serial.print("IP-адрес: ");
  Serial.println(WiFi.localIP()); // Вывод полученного IP-адреса

  server.on("/", handleRoot);        // Назначение функции handleRoot для обработки корневого URL
  server.begin();                   // Запуск сервера
  
  // Приветственное сообщение на дисплее после подключения
  display.cp437(true);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0); display.println(utf8rus("IP-адрес:"));
  display.setCursor(0, 10); display.println(WiFi.localIP());
  display.display();
  delay(3000);
}

// Функция, которая выполняется постоянно в цикле
void loop() {
  server.handleClient();              // Обработка запросов к веб-серверу
  
  // Проверка интервала для смены страниц на дисплее
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    currentPage++;
    if (currentPage > 1) {              // 2 страницы (0 и 1)
      currentPage = 0;                  // Сброс счетчика
    }
  }

  // Считывание данных с датчиков. Выполняется постоянно.
  sensors_event_t humidity, temp_aht;
  aht.getEvent(&humidity, &temp_aht);
  float temp_bmp = bmp.readTemperature();
  float pressure_hpa = bmp.readPressure() / 100.0;
  float pressure_sea_level_hpa = bmp.seaLevelForAltitude(altitude_m, pressure_hpa);
  float pressure_mmhg = pressure_sea_level_hpa * 0.750062;

  // Обновление дисплея в зависимости от текущей страницы
  if (currentPage == 0) { showAHTData(temp_aht.temperature, humidity.relative_humidity); } else { showBMPData(temp_bmp, pressure_mmhg); }
  delay(50);                          // Небольшая задержка для стабилизации работы
}

// Функция для конвертации UTF-8 строк в кодировку дисплея (CP437)
String utf8rus(String source)
{
  int i,k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };
  k = source.length(); i = 0;
  while (i < k) {
    n = source[i]; i++;
    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
          n = source[i]; i++;
          if (n == 0x81) { n = 0xA8; break; } // 'Ё'
          if (n >= 0x90 && n <= 0xBF) n = n + 0x30; // А-П
          break;
        }
        case 0xD1: {
          n = source[i]; i++;
          if (n == 0x91) { n = 0xB8; break; } // 'ё'
          if (n >= 0x80 && n <= 0x8F) n = n + 0x70; // Р-Я
          break;
        }
      }
    }
    m[0] = n; target = target + String(m);
  }
return target;
}

}
