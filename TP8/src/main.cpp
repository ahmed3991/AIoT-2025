#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include "image_list.h"
#include "label_data.h"
#include <vector>
#include <random>

#define BUTTONPIN 4

typedef struct
{
  uint8_t *buf;
  size_t height;
  size_t width;
  size_t len;
} camera_fb_t;

const int MODEL_INPUT_WIDTH = 28;
const int MODEL_INPUT_HEIGHT = 28;
const int MODEL_INPUT_SIZE = MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT;

int buttonState = 0;
bool takeNewPicture = false;

const char *ssid = "Wokwi-GUEST";
const char *password = "";

const char *mqtt_server = "broker.mqtt.cool";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

int8_t *convert_camera_frame_to_model_input(const camera_fb_t *fb)
{
  int8_t *model_input_buffer = (int8_t *)malloc(MODEL_INPUT_SIZE);
  if (!model_input_buffer)
    return nullptr;

  memcpy(model_input_buffer, fb->buf, MODEL_INPUT_SIZE);
  return model_input_buffer;
}

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> distrib(1, NUM_IMAGES);

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
  Serial.print("Received class: ");
  Serial.println(msg);

  lcd.setCursor(0, 1);
  lcd.print("Class: ");
  lcd.print(msg);
  lcd.print("   ");

  takeNewPicture = true;
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
      takeNewPicture = true;
    }
    else
    {
      Serial.println("Retrying in 5s");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(BUTTONPIN, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.print("Starting...");

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop()
{
  if (!client.connected())
    reconnect();
  client.loop();

  buttonState = digitalRead(BUTTONPIN);

  if (buttonState == HIGH && takeNewPicture)
  {
    takeNewPicture = false;

    lcd.setCursor(0, 0);
    lcd.print("Predicting...   ");

    int index = distrib(gen);
    Serial.print("Selected image: ");
    Serial.println(index);

    const int8_t *selected = image_list[index - 1];

    camera_fb_t fake_fb;
    fake_fb.buf = (uint8_t *)selected;
    fake_fb.height = 28;
    fake_fb.width = 28;
    fake_fb.len = 28 * 28;

    int8_t *model_input_data = convert_camera_frame_to_model_input(&fake_fb);

    if (!model_input_data)
    {
      Serial.println("Model conversion failed!");
      takeNewPicture = true;
      return;
    }

    Serial.println("Sending image to MQTT...");

    // Convert to comma separated string
    String t = "";
    for (int i = 0; i < MODEL_INPUT_SIZE; i++)
    {
      t += String(model_input_data[i]);
      if (i < MODEL_INPUT_SIZE - 1)
        t += ",";
    }

    // Prepare JSON
    String payload = "{\"encoded_image\": [" + t + "]}";

    // Publish
    client.publish("esp32/data", payload.c_str());

    Serial.println("Payload:");
    Serial.println(payload);

    free(model_input_data);

    Serial.println("done");
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("Press BTN       ");
  }
}
