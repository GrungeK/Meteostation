// Подключаем необходимые библиотеки
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#define SSD1306_NO_SPLASH 
#include <Adafruit_SSD1306.h>
#include <WiFiManager.h>

// === ДОБАВЛЕНО: OTA ===
#include <ArduinoOTA.h>
// === КОНЕЦ ДОБАВЛЕНИЯ ===

// Настройки OLED-дисплея 128x32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

ESP8266WebServer server(80);
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

const float altitude_m = 205.0; 

unsigned long previousMillis = 0;
const long interval = 5000;
int currentPage = 0;

// Вспомогательная функция для вывода текста по центру
void printCentered(const String& text, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (display.width() - w) / 2;
  display.setCursor(x, y);
  display.println(text);
}

// Функция для вывода данных AHT20 на OLED-дисплей
void showAHTData(float temp, float hum) {
  display.cp437(true);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  printCentered("AHT20", 0);

  display.setCursor(0, 10);
  display.print(utf8rus("Температура: "));
  display.print(temp, 1);
  display.println(" C");

  display.setCursor(0, 20);
  display.print(utf8rus("Влажность:   "));
  display.print(hum, 1);
  display.println(" %");
  display.display();
}

// Функция для вывода данных BMP280 на OLED-дисплей
void showBMPData(float temp, float pressure) {
  display.cp437(true);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  printCentered("BMP280", 0);

  display.setCursor(0, 10);
  display.print(utf8rus("Температура: "));
  display.print(temp, 1);
  display.println(" C");

  display.setCursor(0, 20);
  display.print(utf8rus("Давление:    "));
  display.print(pressure, 0);
  display.println(utf8rus(" мм"));
  display.display();
}

// Функция для обработки запросов к корневому URL веб-сервера
void handleRoot() {
  sensors_event_t humidity, temp_aht;
  aht.getEvent(&humidity, &temp_aht);
  float temp_bmp = bmp.readTemperature();
  float pressure_hpa = bmp.readPressure() / 100.0; 
  float pressure_sea_level_hpa = bmp.seaLevelForAltitude(altitude_m, pressure_hpa);
  float pressure_mmhg = pressure_sea_level_hpa * 0.750062;

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
  
  // HTML-код (ваш оригинальный)
  String html = "<!DOCTYPE html><html lang=\"ru\"><head>";
  html += "<meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>Метеостанция ESP8266</title>";
  html += "<meta http-equiv=\"refresh\" content=\"15\">";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #2c3e50; color: #ecf0f1; margin: 0; padding: 20px; }";
  html += ".container { max-width: 800px; margin: auto; text-align: center; }";
  html += "h1 { color: #3498db; margin-bottom: 30px; }";
  html += ".dashboard { display: flex; flex-wrap: wrap; justify-content: center; gap: 20px; }";
  html += ".card { background-color: #34495e; padding: 20px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2); width: 150px; display: flex; flex-direction: column; justify-content: center; align-items: center; }";
  html += ".value { font-size: 1.5em; font-weight: bold; }";
  html += ".unit { font-size: 1em; color: #7f8c8d; }";
  html += ".circle-wrap { width: 120px; height: 120px; background: #34495e; border-radius: 50%; overflow: hidden; position: relative; }";
  html += ".circle-wrap .circle .mask, .circle-wrap .circle .fill { width: 100%; height: 100%; position: absolute; border-radius: 50%; }";
  html += ".circle-wrap .circle .mask { clip-path: inset(0 50% 0 0); }";
  html += ".circle-wrap .circle .fill { clip-path: inset(0 0 0 50%); background-color: #e67e22; }";
  html += ".circle-wrap.hum .fill { background-color: #3498db; }";
  html += ".circle-wrap.press .fill { background-color: #9b59b6; }";
  html += ".circle-wrap .circle .mask .fill { position: absolute; background-color: #e67e22; }";
  html += ".circle-wrap.hum .mask .fill { background-color: #3498db; }";
  html += ".circle-wrap.press .mask .fill { background-color: #9b59b6; }";
  html += ".circle-wrap .inside-circle { width: 90px; height: 90px; border-radius: 50%; background: #2c3e50; line-height: 90px; text-align: center; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); font-size: 1.5em; font-weight: bold; }";
  
  html += ".circle-wrap.temp-aht .fill { transform: rotate(" + String(temp_deg_aht) + "deg); }";
  html += ".circle-wrap.temp-aht .mask .fill { transform: rotate(" + String(temp_deg_aht) + "deg); }";
  html += ".circle-wrap.temp-bmp .fill { transform: rotate(" + String(temp_deg_bmp) + "deg); }";
  html += ".circle-wrap.temp-bmp .mask .fill { transform: rotate(" + String(temp_deg_bmp) + "deg); }";
  html += ".circle-wrap.hum .fill { transform: rotate(" + String(hum_deg) + "deg); }";
  html += ".circle-wrap.hum .mask .fill { transform: rotate(" + String(hum_deg) + "deg); }";
  html += ".circle-wrap.press .fill { transform: rotate(" + String(pressure_deg) + "deg); }";
  html += ".circle-wrap.press .mask .fill { transform: rotate(" + String(pressure_deg) + "deg); }";
  
  html += "</style></head><body>";
  html += "<div class=\"container\">";
  html += "<h1>Метеостанция балкон</h1>";
  html += "<div class=\"dashboard\">";

  html += "<div class=\"card\"><h2>Температура AHT20</h2>";
  html += "<div class=\"circle-wrap temp-aht\"><div class=\"circle\"><div class=\"mask half\"><div class=\"fill\"></div></div><div class=\"mask full\"><div class=\"fill\"></div></div></div>";
  html += "<div class=\"inside-circle\">" + String(temp_val_aht, 1) + "</div></div></div>";

  html += "<div class=\"card\"><h2>Температура BMP280</h2>";
  html += "<div class=\"circle-wrap temp-bmp\"><div class=\"circle\"><div class=\"mask half\"><div class=\"fill\"></div></div><div class=\"mask full\"><div class=\"fill\"></div></div></div>";
  html += "<div class=\"inside-circle\">" + String(temp_val_bmp, 1) + "</div></div></div>";

  html += "<div class=\"card\"><h2>Влажность (%)</h2>";
  html += "<div class=\"circle-wrap hum\"><div class=\"circle\"><div class=\"mask half\"><div class=\"fill\"></div></div><div class=\"mask full\"><div class=\"fill\"></div></div></div>";
  html += "<div class=\"inside-circle\">" + String(humidityPercent) + "%</div></div></div>";

  html += "<div class=\"card\"><h2>Давление (мм рт. ст.)</h2>";
  html += "<div class=\"circle-wrap press\"><div class=\"circle\"><div class=\"mask half\"><div class=\"fill\"></div></div><div class=\"mask full\"><div class=\"fill\"></div></div></div>";
  html += "<div class=\"inside-circle\">" + String(pressure_val, 0) + "</div></div></div>";
  
  html += "</div>";
  html += "<p style=\"margin-top: 20px;\"><small>IP-адрес: " + WiFi.localIP().toString() + "</small></p>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

// Функция, которая выполняется один раз при запуске
void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(100);

  if (!aht.begin() || !bmp.begin()) {
    Serial.println("Ошибка при инициализации датчиков. Проверьте подключение!");
    for(;;);
  }
  Serial.println("Датчики инициализированы.");

  WiFiManager wifiManager;
  if(!wifiManager.autoConnect("МетеостанцияAP")) {
    Serial.println("Не удалось подключиться и настроить AP");
    display.clearDisplay(); printCentered("Ошибка WiFi!", 0); printCentered("Перезапуск...", 16); display.display();
    delay(3000);
    ESP.restart();
  }
  
  Serial.println("Подключено к Wi-Fi!");
  Serial.print("IP-адрес: ");
  Serial.println(WiFi.localIP());

  // === ИНИЦИАЛИЗАЦИЯ OTA ===
  ArduinoOTA.setHostname("Meteostation"); // имя устройства в сети
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("Начало OTA обновления (" + type + ")...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA завершено");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Прогресс: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Ошибка OTA[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Неверный пароль");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Ошибка начала");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Ошибка подключения");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Ошибка приёма");
    else if (error == OTA_END_ERROR) Serial.println("Ошибка завершения");
  });
  ArduinoOTA.begin();
  Serial.println("OTA готово. Имя хоста: Meteostation");
  // === КОНЕЦ OTA ===

  server.on("/", handleRoot);
  server.begin();
  
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
  server.handleClient();

  // === ОБЯЗАТЕЛЬНО ДЛЯ OTA ===
  ArduinoOTA.handle();
  // ==========================

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    currentPage++;
    if (currentPage > 1) {
      currentPage = 0;
    }
  }

  sensors_event_t humidity, temp_aht;
  aht.getEvent(&humidity, &temp_aht);
  float temp_bmp = bmp.readTemperature();
  float pressure_hpa = bmp.readPressure() / 100.0;
  float pressure_sea_level_hpa = bmp.seaLevelForAltitude(altitude_m, pressure_hpa);
  float pressure_mmhg = pressure_sea_level_hpa * 0.750062;

  if (currentPage == 0) { 
    showAHTData(temp_aht.temperature, humidity.relative_humidity); 
  } else { 
    showBMPData(temp_bmp, pressure_mmhg); 
  }
  delay(50);
}

// Функция для конвертации UTF-8 строк в кодировку дисплея (CP437)
String utf8rus(String source) {
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
          if (n == 0x81) { n = 0xA8; break; }
          if (n >= 0x90 && n <= 0xBF) n = n + 0x30;
          break;
        }
        case 0xD1: {
          n = source[i]; i++;
          if (n == 0x91) { n = 0xB8; break; }
          if (n >= 0x80 && n <= 0x8F) n = n + 0x70;
          break;
        }
      }
    }
    m[0] = n; target = target + String(m);
  }
  return target;
}
