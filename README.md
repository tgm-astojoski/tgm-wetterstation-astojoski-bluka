# tgm-wetterstation-astojoski-bluka
## Unser Systemtechnikprojekt, wo wir mit unserem ESP32-C3 eine Wetterstation aufgebaut haben

Verfasser: **Aleksa Stojoski & Bartosz Luka**

Datum: **11.6.2025**

## 1.  Einführung

Unsere Aufgabe war es eine Wetterstation mit einem ESP32-C3, einem DHT11-Sensor und einem Buzzer zu schaffen

## 2.  Projektbeschreibung

Eine Schaltung wurde mit einem ESP32-C3, einem DHT11-Sensor, der die Temperatur und Luftfeuchtigkeit misst, und einem Buzzer aufgebaut, die die Temperatur und Luftfeuchtigkeit misst und via auf einer auf dem ESP gehosteten Website anzeigt. Die LED_BUILTIN des ESP32 zeigt an ob der ESP WLAN hat (blau), kein WLAN hat. (rot) oder die Temperatur zu hoch ist (orange). Ursprünglich war auch geplant eine Datenbank zu erstellen, die die Messwerte hätte speichern sollen.


## 3.  Theorie

Damit die Wetterstation überhaupt funktioniert, müssen die benötigten Libraries in der Arduino IDE installiert sein.
Diese wären:

- NTPClient
- Adafruit DMA neopixel-library
- Adafruit NeoPixel
- Adafruit Unified Sensor
- Async TCP
- DHT sensor library
- ESP Async WebServer
- ESPAsyncTCP

## 4.   Arbeitsschritte

Zunächst haben wir den Webserver auf dem ESP32 eingerichtet. Anschließend wurde der DHT11-Sensor implementiert dessen Messwerte erfolgreich auf der Webseite des Webservers angezeigt wurden. Danach ergänzten wir das Projekt um einen Buzzer. Im nächsten Schritt haben wir eine Status-LED integriert die sich über den Webserver ein und ausschalten lässt.

Daraufhin versuchten wir die erfassten Sensordaten in einer Datenbank zu speichern. Zuerst testeten wir die Verbindung zu einer MariaDB-Datenbank über phpMyAdmin mithilfe der MySQL Connector-Library von Dr. Charles Bell. Allerdings konnten die Messwerte nicht erfolgreich vom ESP32 an die Datenbank übertragen werden. Auch ein alternativer Ansatz mit einem PHP-Skript schlug fehl. Aus diesem Grund haben wir die Datenbankanbindung vorerst ausgelassen da wir die genaue Fehlerursache nicht feststellen konnten.

### Code

```c++
#include "DHT.h"
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "index.h"
#include <time.h>

// Hardware-Konstanten
#define LED_PIN      8
#define LED_COUNT    1
#define DHTPIN       4
#define DHTTYPE      DHT11
#define BUZZER_PIN   2

// Initialisierung der NeoPixel-LED, des DHT11-Sensors und Farbdefinitionen
Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
DHT dht(DHTPIN, DHTTYPE);

// WLAN-Zugangsdaten
const char* ssid = "IOT";
const char* password = "20tgmiot18";

// LED-Farben
#define COLOR_RED     pixel.Color(255, 0, 0)
#define COLOR_ORANGE  pixel.Color(255, 165, 0)
#define COLOR_BLUE    pixel.Color(0, 0, 255)
#define COLOR_OFF     pixel.Color(0, 0, 0)

bool ledState = false;  // Zustand der LED (an/aus)

// Zeiteinstellungen für NTP
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// Webserver auf Port 80
AsyncWebServer server(80);

// Funktion zur Formatierung der aktuellen Zeit als String
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
  Serial.begin(9600);             // Serielle Verbindung starten
  pixel.begin();                  // LED initialisieren
  pixel.setBrightness(50);
  pixel.show();

  dht.begin();                    // DHT-Sensor starten
  pinMode(BUZZER_PIN, OUTPUT);    // Buzzer-Pin als Ausgang definieren

  WiFi.begin(ssid, password);     // Verbindung mit WLAN herstellen
  Serial.print("Verbinde mit WLAN...");

  // Verbindungs-Timeout mit LED-Signal (blinkend bei Warten)
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

  // WLAN-Status anzeigen
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Verbunden!");
  } else {
    Serial.println("WLAN-Verbindung fehlgeschlagen.");
  }

  Serial.print("ESP32 Web Server's IP address: ");
  Serial.println(WiFi.localIP());

  // Zeitsynchronisation über NTP
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");

  // Warten auf Zeitdaten (max. 10 Versuche)
  Serial.println("Warte auf Zeitdaten...");
  struct tm timeinfo;
  int retry = 0;
  const int maxRetries = 10;
  while (!getLocalTime(&timeinfo) && retry < maxRetries) {
    Serial.println("Fehler beim Abrufen der Zeit");
    delay(1000);
    retry++;
  }

  // Zeitstatus ausgeben
  if (retry == maxRetries) {
    Serial.println("Zeit konnte nicht geladen werden – fahre trotzdem fort.");
  } else {
    Serial.println("Zeit erfolgreich abgerufen!");
    Serial.println(printLocalTime());
  }

  // HTTP-Route für Startseite
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", webpage);
  });

  // Route für Temperaturdaten
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest* request) {
    float temperature = dht.readTemperature();
    String temperatureStr = String(temperature, 2);
    request->send(200, "text/plain", temperatureStr);
  });

  // Route für Luftfeuchtigkeit
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest* request) {
    float humidity = dht.readHumidity();
    String humidityStr = String(humidity, 2);
    request->send(200, "text/plain", humidityStr);
  });

  // Route für aktuelle Zeit
  server.on("/time", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", printLocalTime());
  });

  // Route zum Steuern der LED über URL-Parameter (?state=on/off)
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

  // Route für den LED-Status
  server.on("/ledStatus", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", ledState ? "LED ist AN" : "LED ist AUS");
  });

  server.begin(); // Webserver starten
}

void loop() {
  // Wenn LED aktiv:
  if (ledState) {
    if (WiFi.status() != WL_CONNECTED) {
      pixel.setPixelColor(0, COLOR_RED); // Rot bei Verbindungsfehler
    } else if (dht.readTemperature() > 25) {
      tone(BUZZER_PIN, 1536);             // Buzzer bei hoher Temperatur
      delay(500);
      noTone(BUZZER_PIN);
      delay(500);
      pixel.setPixelColor(0, COLOR_ORANGE); // Orange bei Überhitzung
    } else {
      pixel.setPixelColor(0, COLOR_BLUE);   // Blau bei normalem Zustand
    }
  } else {
    pixel.setPixelColor(0, COLOR_OFF);      // LED aus
  }

  pixel.show(); // LED-Zustand anzeigen
}

```

Webserver:
Zuerst haben wir einen Webserver mit dem ESP32 eingerichtet. Damit können wir über das WLAN eine Webseite aufrufen, die Sensordaten anzeigt und verschiedene Funktionen steuert.

DHT11 Sensor:
Danach haben wir den DHT11-Sensor eingebunden. Dieser misst die Temperatur und Luftfeuchtigkeit. Die Werte werden über zwei separate Routen /temperature und /humidity vom Webserver ausgegeben.

Buzzer:
Anschließend haben wir einen Buzzer hinzugefügt. Wenn die Temperatur über 25 °C steigt, wird ein Warnton ausgegeben, um auf mögliche Überhitzung aufmerksam zu machen.

Status-LED (NeoPixel):
Wir haben eine NeoPixel-LED als Statusanzeige eingebaut:

Blau: Normale Temperatur

Orange: Überhitzung

Rot blinkend: Keine WLAN-Verbindung

Aus: LED ist deaktiviert

Über die Weboberfläche kann die LED ein- und ausgeschaltet werden (/led?state=on/off).

Zeit über NTP:
Die aktuelle Zeit wird über NTP-Server abgerufen und kann über /time angezeigt werden.

Design der Website:
Zum Schluss haben wir die Webseite optisch angepasst, um die Daten übersichtlich darzustellen.


### Tabellen

| Bauteil                              | Menge | 
| ------------------------------------ | ----- | 
| 1. ESP32-C3                          |   1   | 
| 2. DHT11 Sensor	                     |   1   | 
| 3. Buzzer                            |   1   | 
| 4. RGB LED (intern)                  |   1   | 
| 5. Breadboard                        |   1   | 
| 6. Jumperkabel                       |   6   | 

Auch die Aussage der Tabelle muss ausformuliert werden.

## 5.  Zusammenfassung

Das Projekt war eine interessante Aufgabe, weil wir zum ersten Mal so ein großes Projekt mit unserem Microcontroller ESP32-C3 umgesetzt haben. Wir hatten Schwierigkeiten mit der Datenbank, weil die Messwerte nicht vom ESP an sie gesendet werden konnte, weil der ESP sich nicht mit ihr verbinden konnte, bzw. stürzte er im Versuch ab.

## 6.  Bilder und Schaltung 
<img src="/bilder/wetterstation.jpg" />
Hier die Schaltung in real life aufgebaut, die LED leuchtet orange weil die Temperatur zu hoch ist.
<img src="/bilder/wokwi_schaltung.jpg" />
In wokwi gab es leider keinen DHT11 deswegen haben wir den DHT22 genommen. Es gab einen Buzzer der allerdings nur 2 Pins und unser 3 Pins hatte.
<img src="/bilder/website_wetterstation.jpg" />
Hier noch ein Screenshot wie unsere Website ausschaut
## 7.  Quellen

- https://randomnerdtutorials.com/esp32-async-web-server-espasyncwebserver-library/
- https://hartmut-waller.info/arduinoblog/esp-wifi-und-webserver-starten/
- https://esp32io.com/tutorials/esp32-mysql
- https://randomnerdtutorials.com/esp32-dht11-dht22-temperature-humidity-sensor-arduino-ide/
- https://randomnerdtutorials.com/esp32-wi-fi-manager-asyncwebserver/
