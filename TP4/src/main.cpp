#include <ArduinoJson.h>
#include <WiFi.h>
// #include <PubSubClient.h>  // --- MQTT SECTION ---
#include <HTTPClient.h>    // --- FASTAPI SECTION ---
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include "time.h"

// Hardware pins and sensor
#define DHTPIN 15
#define DHTTYPE DHT22
#define LED_PIN 2

// WiFi credentials
const char *ssid = "Wokwi-GUEST";
const char *password = "";

//const char *ssid = "ZTE_2.4G_7SRa3c";
//const char *password = "iXwM3hxx";

// --- MQTT SECTION ---
/*
const char *mqtt_server = "broker.mqtt.cool";
const int mqtt_port = 1883;
*/

// --- FASTAPI SECTION ---
const char *api_url = "http://192.168.1.152:8000/infer";

// Device and publishing configuration
const char *DEVICE_ID = "esp32_01";
const long PUBLISH_INTERVAL_MS = 3000;

// Globals
WiFiClient espClient;
/* PubSubClient client(espClient); */ // --- MQTT SECTION ---
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
String currentCommand = "---";
const int N_FEATURES = 12;
float X[N_FEATURES] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0};
unsigned long lastMsg = 0;

// --- Time utilities ---
void initTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  delay(200);
}

String getTimestamp() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    return String(buf);
  } else {
    char buf[32];
    snprintf(buf, sizeof(buf), "ms:%lu", millis());
    return String(buf);
  }
}

// --- WiFi ---
void setup_wifi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 40) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
  }
}

// --- MQTT SECTION ---
/*
void callback(char *topic, byte *message, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)message[i];
  msg.trim();

  Serial.print("Received command: ");
  Serial.println(msg);

  if (msg.equalsIgnoreCase("ON")) {
    digitalWrite(LED_PIN, HIGH);
    currentCommand = "ON";
  } else if (msg.equalsIgnoreCase("OFF")) {
    digitalWrite(LED_PIN, LOW);
    currentCommand = "OFF";
  }

  lcd.setCursor(0, 1);
  lcd.print("CMD:");
  lcd.print(currentCommand);
  lcd.print("   ");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = String(DEVICE_ID) + "-" + String((uint32_t)esp_random());
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("esp32/control");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5s");
      delay(5000);
    }
  }
}
*/

// --- FASTAPI SECTION ---
 // add this at the top of your sketch

// ... other code ...

long getEpoch() {
  time_t now;
  time(&now);        // requires initTime() called in setup()
  return (long)now;
}

void sendToAPI(float temperature, float humidity) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return;
  }

  HTTPClient http;

  // Use URL you set earlier (example: http://192.168.1.152:8000/infer)
  http.begin(api_url);
  http.addHeader("Content-Type", "application/json");

  // Build JSON body with epoch timestamp (integer)
  long ts = getEpoch();
  String payload = "{";
  payload += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  payload += "\"timestamp\":" + String(ts) + ",";
  payload += "\"Temperature[C]\":" + String(temperature, 2) + ",";
  payload += "\"Humidity[%]\":" + String(humidity, 2);
  payload += "}";

  Serial.print("Posting JSON: ");
  Serial.println(payload);

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("POST sent. Response code: ");
    Serial.println(httpResponseCode);
    Serial.print("Response body: ");
    Serial.println(response);

    // Try to parse JSON response and toggle LED based on "prediction"
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, response);
    if (!err) {
      if (doc.containsKey("prediction")) {
        int pred = doc["prediction"];
        if (pred == 1) {
          digitalWrite(LED_PIN, HIGH);
          Serial.println("LED ON (prediction=1)");
        } else {
          digitalWrite(LED_PIN, LOW);
          Serial.println("LED OFF (prediction=0)");
        }
      } else {
        Serial.println("No prediction field in response.");
      }
    } else {
      Serial.print("Failed to parse JSON response: ");
      Serial.println(err.c_str());
    }
  } else {
    Serial.print("Error sending POST, code: ");
    Serial.println(httpResponseCode);
    // If you see -1 or connection reset: server unreachable from simulator
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Starting...");
  dht.begin();
  setup_wifi();
  initTime();

  // --- MQTT SECTION ---
  /*
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  */
}

void loop() {
  // --- MQTT SECTION ---
  /*
  if (!client.connected()) reconnect();
  client.loop();
  */

  unsigned long now = millis();
  if (now - lastMsg > PUBLISH_INTERVAL_MS) {
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    X[0] = t;
    X[1] = h;

    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(t, 1);
    lcd.print("C H:");
    lcd.print(h, 0);
    lcd.print("%  ");
    lcd.setCursor(0, 1);
    lcd.print("CMD:");
    lcd.print(currentCommand);
    lcd.print("   ");

    // --- MQTT SECTION ---
    /*
    publishSensorData(t, h);
    */

    // --- FASTAPI SECTION ---
    sendToAPI(t, h);
  }
}
