#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include "image_list.h" // the test images
#include "label_data.h" // label names
#include <vector>
#include <random>

#define BUTTONPIN 4

// Define camera_fb_t structure for mocking camera input
typedef struct
{
  uint8_t *buf;
  size_t height;
  size_t width;
  size_t len;
} camera_fb_t;

const int MODEL_INPUT_WIDTH  = 28;
const int MODEL_INPUT_HEIGHT = 28;
const int MODEL_INPUT_SIZE   = MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT;

// variable for storing the pushbutton status
int buttonState = 0;
bool takeNewPicture = false;

// WiFi credentials
const char *ssid     = "Wokwi-GUEST"; // أو شبكة بيتك إذا ستستخدمين اللوحة الحقيقية
const char *password = "";

// MQTT broker
// ملاحظة: هذا هو الـ broker الذي سيتصل به ESP32
// إذا استخدمتِ broker على نفس جهازك غيّري هذا لاحقًا إلى IP جهازك
const char *mqtt_server = "broker.mqtt.cool";
const int   mqtt_port   = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD address 0x27 or 0x3F
String currentCommand = "---";      // default command

// Function to convert camera_fb_t to model input size (28x28 int8_t)
int8_t *convert_camera_frame_to_model_input(const camera_fb_t *fb)
{
  int8_t *model_input_buffer = (int8_t *)malloc(MODEL_INPUT_SIZE * sizeof(int8_t));
  if (!model_input_buffer)
  {
    Serial.println("Failed to allocate memory for model input buffer!");
    return nullptr;
  }

  // هنا نفترض أن البيانات أصلاً 28x28 int8 جاهزة
  memcpy(model_input_buffer, fb->buf, MODEL_INPUT_SIZE * sizeof(int8_t));
  return model_input_buffer;
}

// Random number generator for selecting mock images
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
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *message, unsigned int length)
{
  String msg;
  for (unsigned int i = 0; i < length; i++)
  {
    msg += (char)message[i];
  }
  msg.trim();

  Serial.print("Received command: ");
  Serial.println(msg);

  // Update the LCD with predicted class name
  lcd.setCursor(0, 1);
  lcd.print("Class:");
  lcd.print(msg);
  lcd.print("   "); // clear any leftover characters

  takeNewPicture = true;
}

void reconnect()
{
  takeNewPicture = false;
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
  pinMode(BUTTONPIN, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Starting...");

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  takeNewPicture = true;
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  buttonState = digitalRead(BUTTONPIN);

  // إذا الضغط HIGH و ما زال مسموح نرسل صورة
  if (buttonState == HIGH && takeNewPicture)
  {
    takeNewPicture = false;

    lcd.setCursor(0, 0);
    lcd.print("Predicting...  ");

    // اختيار صورة عشوائية
    int image_index = distrib(gen);
    Serial.print("Selected random image: ");
    Serial.println(image_index);

    // get selected image
    const int8_t *selected_image_data = image_list[image_index - 1];

    camera_fb_t fake_fb;
    fake_fb.buf    = (uint8_t *)selected_image_data; // Cast to uint8_t*
    fake_fb.height = 28;
    fake_fb.width  = 28;
    fake_fb.len    = 28 * 28 * sizeof(int8_t);

    // تحويل الصورة لمدخل النموذج
    int8_t *model_input_data = convert_camera_frame_to_model_input(&fake_fb);
    if (!model_input_data)
    {
      Serial.println("Failed to convert image for model input!");
      lcd.setCursor(0, 0);
      lcd.print("Input error    ");
      takeNewPicture = true;
      return;
    }

    // 🔹 Task 1: تكوين String t يحتوي كل قيم الصورة مفصولة بفواصل
    String t = "";
    for (int i = 0; i < MODEL_INPUT_SIZE; i++)
    {
      t += String(model_input_data[i]);
      if (i < MODEL_INPUT_SIZE - 1)
      {
        t += ",";
      }
    }

    // 🔹 Task 2: بناء JSON payload ونشره على esp32/data
    // الشكل سيكون: {"encoded_image":[v1,v2,v3,...]}
    String payload = "{\"encoded_image\":[" + t + "]}";

    Serial.print("Payload length: ");
    Serial.println(payload.length());

    Serial.println("Sending image data to MQTT...");
    client.publish("esp32/data", payload.c_str());
    Serial.println("Image data published.");

    // تحرير الذاكرة
    free(model_input_data);

    lcd.setCursor(0, 0);
    lcd.print("Waiting result ");

    // ننتظر النتيجة من البايثون عبر callback
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("Press BTN      ");
  }
}
