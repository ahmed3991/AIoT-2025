#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

#define DHTPIN 15
#define DHTTYPE DHT22
#define LED_PIN 2

// WiFi credentials (Wokwi uses Wokwi-GUEST)
const char *ssid = "Wokwi-GUEST";
const char *password = "";

// MQTT broker
const char *mqtt_server = "broker.mqtt.cool";
const int mqtt_port = 1883;

// Client ID (make it unique)
const char *device_id = "ESP32-Fire-Detector-001";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

String currentCommand = "---";
unsigned long lastMsg = 0;
const long interval = 5000; // Publish every 5 seconds

// ==========================================
// WiFi Setup
// ==========================================
void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

// ==========================================
// MQTT Callback (receives messages)
// ==========================================
void callback(char *topic, byte *message, unsigned int length) {
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    
    String msg;
    for (int i = 0; i < length; i++) {
        msg += (char)message[i];
    }
    Serial.println(msg);
    
    // Parse JSON response from Python AI subscriber
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error) {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        return;
    }
    
    // Extract prediction
    int prediction = doc["prediction"] | -1;
    const char* model = doc["model"] | "unknown";
    float probability = doc["probability"] | 0.0;
    
    Serial.print("Model: ");
    Serial.println(model);
    Serial.print("Prediction: ");
    Serial.println(prediction);
    Serial.print("Probability: ");
    Serial.println(probability, 2);
    
    // Control LED based on prediction
    if (prediction == 1) {
        digitalWrite(LED_PIN, HIGH);
        currentCommand = "FIRE!";
    } else if (prediction == 0) {
        digitalWrite(LED_PIN, LOW);
        currentCommand = "SAFE";
    } else {
        currentCommand = "ERROR";
    }
    
    // Update LCD
    lcd.setCursor(0, 1);
    lcd.print("CMD:");
    lcd.print(currentCommand);
    lcd.print("      "); // clear leftover chars
}

// ==========================================
// MQTT Reconnect
// ==========================================
void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        
        if (client.connect(device_id)) {
            Serial.println("connected");
            
            // Subscribe to control topic
            client.subscribe("esp32/control");
            Serial.println("Subscribed to: esp32/control");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" retrying in 5s");
            delay(5000);
        }
    }
}

// ==========================================
// Setup
// ==========================================
void setup() {
    Serial.begin(115200);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fire Detector");
    lcd.setCursor(0, 1);
    lcd.print("Starting...");
    
    // Initialize DHT sensor
    dht.begin();
    
    // Connect to WiFi
    setup_wifi();
    
    // Setup MQTT
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    
    lcd.clear();
    lcd.print("Ready!");
    delay(1000);
}

// ==========================================
// Main Loop
// ==========================================
void loop() {
    // Ensure MQTT connection
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    
    // Publish sensor data periodically
    unsigned long now = millis();
    if (now - lastMsg > interval) {
        lastMsg = now;
        
        // Read sensor data
        float humidity = dht.readHumidity();
        float temperature = dht.readTemperature();
        
        // Check if readings are valid
        if (isnan(humidity) || isnan(temperature)) {
            Serial.println("Failed to read from DHT sensor!");
            return;
        }
        
        // Update LCD with sensor readings
        lcd.setCursor(0, 0);
        lcd.print("T:");
        lcd.print(temperature, 1);
        lcd.print("C H:");
        lcd.print(humidity, 0);
        lcd.print("%  ");
        
        // Create JSON payload
        StaticJsonDocument<200> doc;
        doc["device_id"] = device_id;
        doc["temperature"] = temperature;
        doc["humidity"] = humidity;
        doc["timestamp"] = now / 1000;
        
        String payload;
        serializeJson(doc, payload);
        
        // Publish to MQTT
        Serial.print("Publishing: ");
        Serial.println(payload);
        
        client.publish("esp32/data", payload.c_str());
    }
}
