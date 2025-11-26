#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// ---------------------- إعداد الحساسات -----------------------
#define DHTPIN 15
#define DHTTYPE DHT22
#define LED_PIN 2

// ---------------------- إعداد Wi-Fi ---------------------------
const char *ssid = "Wokwi-GUEST";
const char *password = "";

// ---------------------- إعداد MQTT ----------------------------
const char *mqtt_server = "broker.mqtt.cool"; // يمكن تغييره إلى IP محلي مثل "192.168.1.100"
const int mqtt_port = 1883;
const char *mqtt_client_id = "ESP32Client";
const char *topic_pub = "esp32/data";
const char *topic_sub = "esp32/control";

// ---------------------- تعريف الكائنات ------------------------
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

String currentCommand = "---"; // الأمر الحالي
float lastProbability = 0.0;   // لحفظ آخر احتمال استلامه من الـAI

// ---------------------- الاتصال بالواي فاي --------------------
void setup_wifi()
{
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ---------------------- استقبال الرسائل من MQTT --------------
void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("\n📩 Message arrived on topic: ");
  Serial.println(topic);

  String msg;
  for (int i = 0; i < length; i++)
  {
    msg += (char)message[i];
  }
  msg.trim();
  Serial.println("Raw message: " + msg);

  // نحاول تحليلها كـ JSON
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, msg);
  if (error)
  {
    Serial.println("⚠️ JSON parse failed, message not JSON format.");
    // fallback بسيط: ربما الرسالة هي "ON"/"OFF"
    if (msg.equalsIgnoreCase("ON"))
    {
      digitalWrite(LED_PIN, HIGH);
      currentCommand = "ON";
    }
    else if (msg.equalsIgnoreCase("OFF"))
    {
      digitalWrite(LED_PIN, LOW);
      currentCommand = "OFF";
    }
    return;
  }

  // قراءة القيم من الرسالة JSON
  const char *prediction = doc["prediction"] | "OFF";
  float probability = doc["probability"] | 0.0;

  // حفظ القيم وعرضها
  currentCommand = String(prediction);
  lastProbability = probability;

  // تشغيل أو إيقاف LED
  if (currentCommand.equalsIgnoreCase("ON"))
  {
    digitalWrite(LED_PIN, HIGH);
  }
  else
  {
    digitalWrite(LED_PIN, LOW);
  }

  // تحديث الشاشة LCD
  lcd.setCursor(0, 1);
  lcd.print("CMD:");
  lcd.print(currentCommand);
  lcd.print(" P:");
  lcd.print(probability, 2);
  lcd.print("   "); // لمسح الباقي

  // طباعة في السيريال
  Serial.print("✅ Command: ");
  Serial.print(currentCommand);
  Serial.print(" | Probability: ");
  Serial.println(probability, 4);
}

// ---------------------- إعادة الاتصال بالـMQTT -----------------
void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id))
    {
      Serial.println("connected ✅");
      client.subscribe(topic_sub);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" -> retrying in 5 seconds");
      delay(5000);
    }
  }
}

// ---------------------- الإعداد الأولي -------------------------
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
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// ---------------------- الحلقة الرئيسية ------------------------
unsigned long lastMsg = 0;
const long interval = 3000; // 3 ثوانٍ بين الإرسال

void loop()
{
  if (!client.connected())
    reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > interval)
  {
    lastMsg = now;

    // قراءة البيانات من DHT
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t))
    {
      Serial.println("⚠️ Failed to read from DHT sensor!");
      return;
    }

    // طباعة القيم في السيريال
    Serial.print("🌡️ Temp: ");
    Serial.print(t);
    Serial.print(" °C | 💧 Humidity: ");
    Serial.print(h);
    Serial.println(" %");

    // تحديث القيم على LCD
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(t, 1);
    lcd.print("C H:");
    lcd.print(h, 0);
    lcd.print("%  ");

    lcd.setCursor(0, 1);
    lcd.print("CMD:");
    lcd.print(currentCommand);
    lcd.print(" P:");
    lcd.print(lastProbability, 2);
    lcd.print("   ");

    // إعداد الحمولة JSON للإرسال
    String payload = "{\"temperature\": " + String(t, 2) +
                     ", \"humidity\": " + String(h, 2) +
                     ", \"device_id\": \"esp32-01\"}";
    client.publish(topic_pub, payload.c_str());

    Serial.println("📤 Published: " + payload);
  }
}
