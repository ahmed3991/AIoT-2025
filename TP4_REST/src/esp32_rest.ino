#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"

#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "AndroidAP3CD1";
const char* password = "123456789";

String serverUrl = "http://192.168.43.30:5000/infer";

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(2000);
    return;
  }

  Serial.printf("Temp: %.2f°C | Humidity: %.2f%%\n", temp, hum);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // بناء JSON
    String jsonData = "{\"temperature\": " + String(temp, 2) + ", \"humidity\": " + String(hum, 2) + "}";

    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Server response: " + response);

      if (response.indexOf("\"prediction\": 1") > 0) {
        Serial.println("Fire detected!");
      } else {
        Serial.println("Normal state");
      }

    } else {
      Serial.printf("Error code: %d\n", httpResponseCode);
    }

    http.end();
  }

  delay(5000);  
}
