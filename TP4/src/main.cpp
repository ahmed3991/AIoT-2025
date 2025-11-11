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


const char *mqtt_server = "broker.mqtt.cool"; 
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD address 0x27 or 0x3F
String currentCommand = "---";      // default command

// const int N_FEATURES = 12;
// float X[N_FEATURES] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0};
const int N_FEATURES = 15;
// float X[N_FEATURES] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // WRONG ORDER
float X[N_FEATURES] = {
    0.0,           // index 0: Unnamed: 0
    1654733331.0,  // index 1: UTC (using first row's value as a placeholder)
    20.0,          // index 2: Temperature[C] (will be updated)
    57.36,         // index 3: Humidity[%] (will be updated)
    0.0,           // index 4: TVOC[ppb]
    400.0,         // index 5: eCO2[ppm]
    12306.0,       // index 6: Raw H2
    18520.0,       // index 7: Raw Ethanol
    939.735,       // index 8: Pressure[hPa]
    0.0,           // index 9: PM1.0
    0.0,           // index 10: PM2.5
    0.0,           // index 11: NC0.5
    0.0,           // index 12: NC1.0
    0.0,           // index 13: NC2.5
    0.0            // index 14: CNT
};
void setup_wifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void callback(char *topic, byte *message, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)message[i];
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

  // Update the LCD
  lcd.setCursor(0, 1);
  lcd.print("CMD:");
  lcd.print(currentCommand);
  lcd.print("   "); // clear leftovers
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
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

unsigned long lastMsg = 0;
const long interval = 3000; // 3s

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) return;

    // Update input array
    X[2] = t; // index 2 is Temperature[C]
    X[3] = h; // index 3 is Humidity[%]


    // Also update the UTC placeholder to be dynamic
    X[1] = millis();

    if (t > 50.0) { 
        X[14] = 20000.0; // Set CNT (index 14) to a value higher than the avg (10511)
    } else {
        X[14] = 0.0;     // Set CNT back to 0 otherwise
    }


    // LCD update
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

    // Build single payload (fixed redeclaration bug)
    String device_id = "esp32-wokwi-01";
    String payload = "{";
    payload += "\"device_id\": \"" + device_id + "\",";
    payload += "\"timestamp\": " + String(millis()) + ",";
    payload += "\"features\": [";
    for (int i = 0; i < N_FEATURES; i++) {
      payload += String(X[i]);
      if (i < N_FEATURES - 1) payload += ",";
    }
    payload += "]";
    payload += "}";

    Serial.println("Publishing message:");
    Serial.println(payload);

    client.publish("esp32/data", payload.c_str());
  }
}
