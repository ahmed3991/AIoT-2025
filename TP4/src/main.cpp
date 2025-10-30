

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 15
#define DHTTYPE DHT22
#define LED_PIN 2

// WiFi credentials
const char *ssid = "Wokwi-GUEST";
const char *password = "";

// MQTT broker (local machine IP)
const char *mqtt_server = "broker.mqtt.cool"; // or your LAN IP, e.g. "192.168.1.100"
const int mqtt_port = 1883;

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

void callback(char *topic, byte *message, unsigned int length)
{
  String msg;
  for (int i = 0; i < length; i++)
    msg += (char)message[i];
  msg.trim();

  Serial.print("Received command: ");
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

  // Update the LCD immediately when a command arrives
  lcd.setCursor(0, 1);
  lcd.print("CMD:");
  lcd.print(currentCommand);
  lcd.print("   "); // clear any leftover characters
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
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

unsigned long lastMsg = 0;
const long interval = 3000; // update every 3 seconds

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

    // TODO: Publish all features to MQTT
    String payload = "{\"temperature\": " + String(t) + ", \"humidity\": " + String(h) + "}";
    client.publish("esp32/data", payload.c_str());
  }
}