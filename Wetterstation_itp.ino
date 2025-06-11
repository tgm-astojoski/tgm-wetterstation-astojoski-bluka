#include "DHT.h"
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "index.h"
#include <time.h>

#define LED_PIN      8
#define LED_COUNT    1
#define DHTPIN       4
#define DHTTYPE      DHT11
#define BUZZER_PIN   2

Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "IOT";        // Dein WLAN-Name
const char* password = "20tgmiot18"; // Dein WLAN-Passwort

#define COLOR_RED     pixel.Color(255, 0, 0)
#define COLOR_ORANGE  pixel.Color(255, 165, 0)
#define COLOR_BLUE    pixel.Color(0, 0, 255)
#define COLOR_OFF     pixel.Color(0, 0, 0)

bool ledState = false;

// Zeitkonfiguration
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

AsyncWebServer server(80);

// Funktion zur Zeitformatierung
String printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Zeit konnte nicht abgerufen werden.");
    return "Zeitfehler";
  }

  char timeString[64];
  strftime(timeString, sizeof(timeString), "Datum: %d.%m.%Y  Zeit: %H:%M:%S", &timeinfo);
  return String(timeString);
}

void setup() {
  Serial.begin(9600);
  pixel.begin();
  pixel.setBrightness(50);
  pixel.show();

  dht.begin();
  pinMode(BUZZER_PIN, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.print("Verbinde mit WLAN...");

  int timeout = 10000;
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
    pixel.setPixelColor(0, COLOR_RED);
    pixel.show();
    delay(500);
    pixel.setPixelColor(0, COLOR_OFF);
    pixel.show();
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Verbunden!");
  } else {
    Serial.println("WLAN-Verbindung fehlgeschlagen.");
  }

  Serial.print("ESP32 Web Server's IP address: ");
  Serial.println(WiFi.localIP());

  // Zeit konfigurieren
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");

  Serial.println("Warte auf Zeitdaten...");
  struct tm timeinfo;
  int retry = 0;
  const int maxRetries = 10;

  while (!getLocalTime(&timeinfo) && retry < maxRetries) {
    Serial.println("Fehler beim Abrufen der Zeit");
    delay(1000);
    retry++;
  }

  if (retry == maxRetries) {
    Serial.println("Zeit konnte nicht geladen werden – fahre trotzdem fort.");
  } else {
    Serial.println("Zeit erfolgreich abgerufen!");
    Serial.println(printLocalTime());
  }

  // Webserver-Routen
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", webpage);
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest* request) {
    float temperature = dht.readTemperature();
    String temperatureStr = String(temperature, 2);
    request->send(200, "text/plain", temperatureStr);
  });

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest* request) {
    float humidity = dht.readHumidity();
    String humidityStr = String(humidity, 2);
    request->send(200, "text/plain", humidityStr);
  });

  server.on("/time", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", printLocalTime());
  });

  server.on("/led", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("state")) {
      String state = request->getParam("state")->value();
      ledState = (state == "on");
      pixel.show();
      request->send(200, "text/plain", ledState ? "LED ist AN" : "LED ist AUS");
    } else {
      request->send(400, "text/plain", "Fehlender Parameter");
    }
  });

  server.on("/ledStatus", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", ledState ? "LED ist AN" : "LED ist AUS");
  });

  server.begin();
}

void loop() {
  if (ledState) {
    if (WiFi.status() != WL_CONNECTED) {
      pixel.setPixelColor(0, COLOR_RED);
    } else if (dht.readTemperature() > 25) {
      tone(BUZZER_PIN, 1536);
      delay(500);
      noTone(BUZZER_PIN);
      delay(500);
      pixel.setPixelColor(0, COLOR_ORANGE);
    } else {
      pixel.setPixelColor(0, COLOR_BLUE);
    }
  } else {
    pixel.setPixelColor(0, COLOR_OFF);
  }

  pixel.show();
}
