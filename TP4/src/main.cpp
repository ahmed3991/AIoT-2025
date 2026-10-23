
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// ================================
// WiFi Configuration (Wokwi)
// ================================
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// ================================
// MQTT Configuration
// ================================
const char* MQTT_BROKER = "broker.mqtt.cool";
const int MQTT_PORT = 1883;

// Change this namespace to a unique name.
// Use the SAME namespace in mqtt_ai_subscriber.py.
const char* USER_NAMESPACE = "aiot_lab_yourname";
const char* DEVICE_ID = "esp32-01";

// ================================
// Hardware Configuration
// ================================
#define DHT_PIN 15
#define DHT_TYPE DHT22
#define LED_PIN 2

#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

// ================================
// Objects
// ================================
WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(
    LCD_ADDRESS,
    LCD_COLUMNS,
    LCD_ROWS
);

// ================================
// MQTT Topics
// ================================
String topic_data;
String topic_control;

// ================================
// Timing
// ================================
unsigned long lastPublish = 0;
const unsigned long PUBLISH_INTERVAL = 5000;

// ================================
// Prediction State
// ================================
String lastPrediction = "WAITING";

// ================================
// WiFi Connection
// ================================
void connectWiFi() {
    Serial.print("Connecting to WiFi");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

// ================================
// MQTT Callback
// Receives prediction from Python
// ================================
void callback(char* topic, byte* payload, unsigned int length) {
    String message = "";

    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.println();
    Serial.print("Message received on: ");
    Serial.println(topic);

    Serial.print("Payload: ");
    Serial.println(message);

    // Process only control messages
    if (String(topic) != topic_control) {
        return;
    }

    String prediction = "";

    // Parse JSON response from Python
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (!error) {
        // Ignore messages intended for another device
        String targetDevice = doc["device_id"] | "";

        if (targetDevice != DEVICE_ID) {
            Serial.println("Message for another device. Ignored.");
            return;
        }

        prediction = doc["prediction"] | "";
    } else {
        // Also support simple ON / OFF messages
        prediction = message;
    }

    prediction.trim();
    prediction.toUpperCase();

    // Update LED and LCD
    if (prediction == "ON") {
        digitalWrite(LED_PIN, HIGH);
        lastPrediction = "FIRE ALARM";

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("FIRE ALARM!");
        lcd.setCursor(0, 1);
        lcd.print("LED: ON");

        Serial.println("Prediction: FIRE ALARM");
        Serial.println("LED turned ON");

    } else if (prediction == "OFF") {
        digitalWrite(LED_PIN, LOW);
        lastPrediction = "SAFE";

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Status: SAFE");
        lcd.setCursor(0, 1);
        lcd.print("LED: OFF");

        Serial.println("Prediction: SAFE");
        Serial.println("LED turned OFF");

    } else {
        Serial.println("Unknown prediction. Ignored.");
    }
}

// ================================
// MQTT Connection
// ================================
void connectMQTT() {
    while (!client.connected()) {
        Serial.print("Connecting to MQTT broker...");

        String clientId = String(DEVICE_ID) + "-" +
                          String((uint32_t)ESP.getEfuseMac(), HEX);

        if (client.connect(clientId.c_str())) {
            Serial.println("connected!");

            // Subscribe to control topic
            if (client.subscribe(topic_control.c_str(), 1)) {
                Serial.print("Subscribed to: ");
                Serial.println(topic_control);
            } else {
                Serial.println("Subscription failed.");
            }

        } else {
            Serial.print("Failed, state = ");
            Serial.println(client.state());
            Serial.println("Retrying in 3 seconds...");
            delay(3000);
        }
    }
}

// ================================
// Publish Sensor Data
// ================================
void publishSensorData() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Failed to read DHT22 sensor!");
        return;
    }

    // Create JSON payload
    JsonDocument doc;

    doc["device_id"] = DEVICE_ID;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["timestamp"] = millis() / 1000;

    String payload;
    serializeJson(doc, payload);

    // Publish sensor data
    bool published = client.publish(
        topic_data.c_str(),
        payload.c_str()
    );

    Serial.println();
    Serial.println("----- SENSOR DATA -----");

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("MQTT payload: ");
    Serial.println(payload);

    if (published) {
        Serial.println("Sensor data published!");
    } else {
        Serial.println("Failed to publish sensor data.");
    }
}

// ================================
// Setup
// ================================
void setup() {
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    dht.begin();

    Wire.begin(21, 22);
    lcd.init();
    lcd.backlight();

    lcd.setCursor(0, 0);
    lcd.print("AIoT TP4");
    lcd.setCursor(0, 1);
    lcd.print("Starting...");

    // Construct matching MQTT topics
    topic_data = String(USER_NAMESPACE) + "/" +
                 String(DEVICE_ID) + "/data";

    topic_control = String(USER_NAMESPACE) + "/" +
                    String(DEVICE_ID) + "/control";

    Serial.println();
    Serial.println("===== AIoT TP4 =====");

    Serial.print("Data topic: ");
    Serial.println(topic_data);

    Serial.print("Control topic: ");
    Serial.println(topic_control);

    connectWiFi();

    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(callback);
    client.setBufferSize(512);

    connectMQTT();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi: Connected");
    lcd.setCursor(0, 1);
    lcd.print("MQTT: Connected");

    delay(1500);
    lcd.clear();
}

// ================================
// Main Loop
// ================================
void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    if (!client.connected()) {
        connectMQTT();
    }

    client.loop();

    unsigned long currentMillis = millis();

    if (currentMillis - lastPublish >= PUBLISH_INTERVAL) {
        lastPublish = currentMillis;
        publishSensorData();
    }

    delay(10);
}
