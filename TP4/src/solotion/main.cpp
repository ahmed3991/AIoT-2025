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

const char *mqtt_server = "broker.mqtt.cool";
const int mqtt_port = 1883;

const char *USER_NAMESPACE = "aiot_lab_yourname";
const char *DEVICE_ID = "esp32-01";

String topic_data = String(USER_NAMESPACE) + "/" + DEVICE_ID + "/data";
String topic_control = String(USER_NAMESPACE) + "/" + DEVICE_ID + "/control";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
String currentCommand = "---";

const int N_FEATURES = 12;
float X[N_FEATURES] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0};

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
  Serial.println("\nWiFi connected! IP address:");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *message, unsigned int length)
{
  String msg;
  for (int i = 0; i < length; i++)
    msg += (char)message[i];
  msg.trim();

  Serial.print("Received command on [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

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

  lcd.setCursor(0, 1);
  lcd.print("CMD:");
  lcd.print(currentCommand);
  lcd.print("   ");
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(DEVICE_ID))
    {
      Serial.println("connected");
      client.subscribe(topic_control.c_str());
      Serial.print("Subscribed to: ");
      Serial.println(topic_control);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retry in 5s");
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
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

unsigned long lastMsg = 0;
const long interval = 3000;

void loop()
{
  if (!client.connected())
    reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > interval)
  {
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t))
    {
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

    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["temperature"] = t;
    doc["humidity"] = h;
    doc["timestamp"] = (long)(millis() / 1000);

    char payload[256];
    serializeJson(doc, payload);

    Serial.print("Publishing to ");
    Serial.print(topic_data);
    Serial.print(": ");
    Serial.println(payload);

    client.publish(topic_data.c_str(), payload);
  }
}