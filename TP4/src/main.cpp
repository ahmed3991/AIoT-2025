

#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

#define DHTPIN 15
#define DHTTYPE DHT22
#define LED_PIN 2

// WiFi credentials
const char *ssid = "Wokwi-GUEST";
const char *password = "";

// MQTT broker (local machine IP)
const char *mqtt_server = "broker.mqtt.cool"; // or your LAN IP, e.g. "192.168.1.100"
const int mqtt_port = 1883;

// Set to true to send telemetry via HTTP instead of MQTT.
const bool USE_HTTP_TRANSPORT = false;
const char *http_endpoint = "http://192.168.1.100:8000/infer"; // Update with your server address

const char *DEVICE_ID = "esp32-aiot-demo";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD address 0x27 or 0x3F
String currentCommand = "---";      // default command

const int N_FEATURES = 12;
float X[N_FEATURES] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0}; // Input features

void setup_wifi()
{
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

bool applyControlPayload(const String &payload)
{
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload);

  bool ledOn = false;
  bool hasValidPrediction = false;

  if (!error)
  {
    if (doc.containsKey("prediction"))
    {
      float prediction = doc["prediction"].as<float>();
      ledOn = prediction >= 0.5f;
      hasValidPrediction = true;
    }
    else if (doc.containsKey("command"))
    {
      const char *command = doc["command"];
      ledOn = String(command).equalsIgnoreCase("ON");
      hasValidPrediction = true;
    }
  }

  if (!hasValidPrediction)
  {
    // Fallback to plain-text commands for backward compatibility
    String trimmed = payload;
    trimmed.trim();
    if (trimmed.equalsIgnoreCase("ON"))
    {
      ledOn = true;
      hasValidPrediction = true;
    }
    else if (trimmed.equalsIgnoreCase("OFF"))
    {
      ledOn = false;
      hasValidPrediction = true;
    }
  }

  if (hasValidPrediction)
  {
    digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
    currentCommand = ledOn ? "ON" : "OFF";

    lcd.setCursor(0, 1);
    lcd.print("CMD:");
    lcd.print(currentCommand);
    lcd.print("   "); // clear any leftover characters

    return true;
  }

  Serial.println("[WARN] Ignoring control message without prediction/command field");
  return false;
}

void callback(char *topic, byte *message, unsigned int length)
{
  String payload;
  for (unsigned int i = 0; i < length; i++)
    payload += static_cast<char>(message[i]);

  Serial.print("Received message on ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(payload);

  applyControlPayload(payload);
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client"))
    {
      Serial.println("connected");
      client.subscribe("esp32/control");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5s");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Starting...");
  dht.begin();

  setup_wifi();

  if (!USE_HTTP_TRANSPORT)
  {
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
  }
}

unsigned long lastMsg = 0;
const long interval = 3000; // update every 3 seconds

size_t buildTelemetryPayload(char *buffer, size_t bufferSize, float temperature, float humidity)
{
  StaticJsonDocument<512> doc;
  doc["device_id"] = DEVICE_ID;
  doc["timestamp"] = millis();
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;

  JsonArray features = doc.createNestedArray("features");
  for (int i = 0; i < N_FEATURES; i++)
  {
    features.add(X[i]);
  }

  return serializeJson(doc, buffer, bufferSize);
}

void publishTelemetryMqtt(const char *buffer, size_t length)
{
  client.publish("esp32/data", reinterpret_cast<const uint8_t *>(buffer), length);
}

void publishTelemetryHttp(const char *buffer, size_t length)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[WARN] WiFi disconnected, attempting reconnect before HTTP POST");
    setup_wifi();
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("[ERROR] Unable to reconnect to WiFi for HTTP transport");
      return;
    }
  }

  HTTPClient http;
  http.begin(http_endpoint);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(reinterpret_cast<const uint8_t *>(buffer), length);
  if (httpResponseCode > 0)
  {
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
    String responsePayload = http.getString();
    Serial.print("HTTP response payload: ");
    Serial.println(responsePayload);
    applyControlPayload(responsePayload);
  }
  else
  {
    Serial.print("[ERROR] HTTP POST failed: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void loop()
{
  if (!USE_HTTP_TRANSPORT)
  {
    if (!client.connected())
      reconnect();
    client.loop();
  }

  unsigned long now = millis();
  if (now - lastMsg > interval)
  {
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t))
      return;

    // add data to input array

    X[0] = t;
    X[1] = h;

    // Update LCD with temperature and humidity
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(t, 1);
    lcd.print("C H:");
    lcd.print(h, 0);
    lcd.print("%  ");

    // Update the command line
    lcd.setCursor(0, 1);
    lcd.print("CMD:");
    lcd.print(currentCommand);
    lcd.print("   ");

    char buffer[512];
    size_t n = buildTelemetryPayload(buffer, sizeof(buffer), t, h);
    if (n > 0)
    {
      if (USE_HTTP_TRANSPORT)
      {
        publishTelemetryHttp(buffer, n);
      }
      else
      {
        publishTelemetryMqtt(buffer, n);
      }
    }
  }
}
