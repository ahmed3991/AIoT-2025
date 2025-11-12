/* ===========================================
   File: TP4/src/main.cpp
   ESP32 firmware (MQTT publisher + subscriber)
   - ينشر JSON على "esp32/data"
   - يشترك على "esp32/control" ويتعامل مع JSON تحكم
   - يعلن الحالة على "esp32/status/<device_id>" (online/offline)
   =========================================== */

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// ---------- Pins & Peripherals ----------
#define DHTPIN   15
#define DHTTYPE  DHT22
#define LED_PIN  2
#define LCD_ADDR 0x27 // غيّريها إلى 0x3F إذا الشاشة سوداء

// ---------- WiFi ----------
const char* ssid     = "Wokwi-GUEST";
const char* password = "";

// ---------- MQTT ----------
const char* mqtt_server = "broker.mqtt.cool";
const int   mqtt_port   = 1883;

#define MQTT_TOPIC_OUT "esp32/data"
#define MQTT_TOPIC_IN  "esp32/control"

WiFiClient        espClient;
PubSubClient      client(espClient);
DHT               dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

String currentCommand = "---";
const char* device_id = "esp-01";

// عدّاد تسلسلي للرسائل
static uint32_t SEQ = 0;

// payload التجريبي ذو 12 ميزة (نحدّث أول ميزتين من DHT)
const int N_FEATURES = 12;
float X[N_FEATURES] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0};

// ---------- Helpers ----------
String statusTopic() {
  return String("esp32/status/") + device_id;
}

void publishStatus(const char* state) {
  // retained=true حتى يعرف المشتركون الحالة فور الاشتراك
  String msg = String("{\"device_id\":\"") + device_id + "\",\"status\":\"" + state + "\"}";
  client.publish(statusTopic().c_str(), msg.c_str(), true);
}

void setup_wifi() {
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
}

void handleControlJson(const String& msg) {
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    // دعم نص بسيط ON/OFF
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
    return;
  }

  const char* incoming_id = doc["device_id"] | "";
  if (String(incoming_id) != String(device_id)) {
    Serial.println("Control not for this device (device_id mismatch). Ignoring.");
    return;
  }

  const char* model = doc["model"] | "";
  int   prediction  = doc["prediction"] | -1;
  float probability = doc["probability"] | 0.0;

  Serial.printf("model=%s prediction=%d prob=%.2f\n", model, prediction, probability);

  if (prediction == 1) {
    digitalWrite(LED_PIN, HIGH);
    currentCommand = "ON";
  } else if (prediction == 0) {
    digitalWrite(LED_PIN, LOW);
    currentCommand = "OFF";
  } else {
    currentCommand = "NONE";
  }

  lcd.setCursor(0, 1);
  lcd.print("CMD:");
  lcd.print(currentCommand);
  lcd.print("   ");
}

void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)message[i];
  msg.trim();

  Serial.print("Received on ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msg);

  handleControlJson(msg);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection... ");
    String clientId = "ESP32Client-";
    clientId += String((uint32_t)esp_random(), HEX);

    // إعداد LWT (offline)
    String willMsg = String("{\"device_id\":\"") + device_id + "\",\"status\":\"offline\"}";
    String willTop = statusTopic();

    // connect(clientId, willTopic, willQos, willRetain, willMessage)
    if (client.connect(clientId.c_str(), willTop.c_str(), 1, true, willMsg.c_str())) {
      Serial.println("connected");
      client.subscribe(MQTT_TOPIC_IN, 1);     // QoS=1 لاستقبال التحكم
      publishStatus("online");                // أعلن أننا Online
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5s");
      delay(5000);
    }
  }
}

void publishSensor(float t, float h, unsigned long now_ms) {
  // تحديث الميزات
  X[0] = t;
  X[1] = h;

  // تحديث شاشة LCD
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

  // بناء JSON
  DynamicJsonDocument outDoc(512);
  outDoc["device_id"]   = device_id;
  outDoc["temperature"] = t;
  outDoc["humidity"]    = h;
  outDoc["seq"]         = (uint32_t)SEQ++;     // عداد الرسائل
  outDoc["t_ms"]        = (uint32_t)now_ms;    // وقت الإرسال بالمللي على ESP32
  JsonArray arr = outDoc.createNestedArray("features");
  for (int i = 0; i < N_FEATURES; i++) arr.add(X[i]);
  outDoc["timestamp"] = (unsigned long)(now_ms / 1000); // seconds since boot

  // انشر (نستخدم String لتفادي مشاكل حجم البفر، ورفعنا بافر MQTT إلى 512)
  String payload;
  serializeJson(outDoc, payload);
  bool ok = client.publish(MQTT_TOPIC_OUT, payload.c_str());
  if (ok) {
    Serial.print("Published: ");
    Serial.println(payload);
  } else {
    Serial.println("Publish failed");
  }
}

// ---------- Arduino entry points ----------
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
  client.setBufferSize(512);  // مهم: حتى لا يُقص JSON
  client.setKeepAlive(30);
  client.setSocketTimeout(5);
}

unsigned long lastMsg = 0;
const unsigned long interval = 3000; // كل 3 ثوانٍ

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed reading from DHT sensor!");
      return;
    }

    publishSensor(t, h, now);
  }
}
