#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

#define DHTPIN 15
#define DHTTYPE DHT22
#define LED_PIN 2

const char *ssid = "Wokwi-GUEST";
const char *password = "";

const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

String device_id = "esp32-dev-01";
String currentCommand = "---";

const int N_FEATURES = 15;
float X[N_FEATURES] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

unsigned long lastReconnectAttempt = 0;
unsigned long lastMsg = 0;
const long interval = 3000;

String makeClientId() {
  uint32_t rnd = esp_random();
  String cid = device_id + "-" + String(rnd & 0xffff, HEX);
  return cid;
}

void setup_wifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void handle_control_json(const String &msg) {
  
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial.print("Callback: JSON parse error: ");
    Serial.println(err.c_str());
    return;
  }

  if (doc.containsKey("command")) {
    String cmd = doc["command"].as<String>();
    cmd.trim();
    if (cmd.equalsIgnoreCase("ON")) {
      digitalWrite(LED_PIN, HIGH);
      currentCommand = "ON";
    } else if (cmd.equalsIgnoreCase("OFF")) {
      digitalWrite(LED_PIN, LOW);
      currentCommand = "OFF";
    }
    Serial.print("Applied command (field): ");
    Serial.println(cmd);
  } else if (doc.containsKey("prediction")) {
    int p = doc["prediction"];
    if (p == 1) {
      digitalWrite(LED_PIN, HIGH);
      currentCommand = "ON";
    } else {
      digitalWrite(LED_PIN, LOW);
      currentCommand = "OFF";
    }
    Serial.print("Applied prediction (field): ");
    Serial.println(p);
  } else {
    Serial.println("Callback: no actionable field (command/prediction).");
  }

  lcd.setCursor(0, 1);
  lcd.print("CMD:");
  lcd.print(currentCommand);
  lcd.print("   ");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();

  Serial.print("Received on topic ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msg);

  handle_control_json(msg);
}

bool reconnect() {
  if (client.connected()) return true;

  unsigned long now = millis();
  // backoff simple: try at most every 3s
  if (now - lastReconnectAttempt < 3000) return false;
  lastReconnectAttempt = now;

  String cid = makeClientId();
  Serial.print("Attempting MQTT connection with clientId=");
  Serial.println(cid);

  if (client.connect(cid.c_str())) {
    Serial.println("connected");
    // subscribe with QoS 1
    if (client.subscribe("esp32/control", 1)) {
      Serial.println("Subscribed to esp32/control (qos=1)");
    } else {
      Serial.println("Failed to subscribe to esp32/control");
    }
    return true;
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" - will retry");
    return false;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Starting...");
  dht.begin();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void publish_sensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) return;

  X[0] = t;
  X[1] = h;

  StaticJsonDocument<512> doc;
  doc["device_id"] = device_id;
  doc["timestamp"] = millis();
  doc["temperature"] = t;
  doc["humidity"] = h;
  for (int i = 2; i < N_FEATURES; ++i) {
    String fname = "feature" + String(i);
    doc[fname] = X[i];
  }

  String jsonString;
  serializeJson(doc, jsonString);

  // publish with retain=true
  bool ok = client.publish("esp32/data", jsonString.c_str(), true);
  Serial.print("Published: ");
  Serial.println(jsonString);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    publish_sensor();

    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(X[0], 1);
    lcd.print("C H:");
    lcd.print(X[1], 0);
    lcd.print("%  ");
    lcd.setCursor(0, 1);
    lcd.print("CMD:");
    lcd.print(currentCommand);
    lcd.print("   ");
  }
}
