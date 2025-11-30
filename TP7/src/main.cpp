#include <Arduino.h>
#include <MicroTFLite.h>
#include <LiquidCrystal_I2C.h>
#include "image_list.h"
#include "label_data.h"
#include "model_data.h"
#include <vector>
#include <random>

#define BUTTONPIN 4

LiquidCrystal_I2C lcd(0x27, 16, 2);
String currentCommand = "---";

// تعريف هيكل محاكاة الكاميرا
typedef struct
{
    uint8_t *buf;
    size_t height;
    size_t width;
    size_t len;
} camera_fb_t;

// مولّد عشوائي لاختيار صورة
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> distrib(1, NUM_IMAGES);

int buttonState = 0;
bool takeNewPicture = false;

// ✅ تعريف مساحة الذاكرة للتنسور
#define TENSOR_ARENA_SIZE (93 * 1024)
uint8_t tensor_arena[TENSOR_ARENA_SIZE];

const int MODEL_INPUT_WIDTH = 28;
const int MODEL_INPUT_HEIGHT = 28;
const int MODEL_INPUT_SIZE = MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT;

const char *class_names[] = {
    "T-shirt/top",
    "Trouser",
    "Pullover",
    "Dress",
    "Coat",
    "Sandal",
    "Shirt",
    "Sneaker",
    "Bag",
    "Ankle boot"};

const tflite::Model *model;
tflite::MicroInterpreter *interpreter;
tflite::AllOpsResolver resolver;
TfLiteTensor *input;
TfLiteTensor *output;

// تحويل إطار الكاميرا إلى صيغة مناسبة للنموذج (محاكاة)
int8_t *convert_camera_frame_to_model_input(const camera_fb_t *fb)
{
    int8_t *model_input_buffer = (int8_t *)malloc(MODEL_INPUT_SIZE * sizeof(int8_t));
    if (!model_input_buffer)
    {
        Serial.println("Failed to allocate memory for model input buffer!");
        return nullptr;
    }

    memcpy(model_input_buffer, fb->buf, MODEL_INPUT_SIZE * sizeof(int8_t));
    return model_input_buffer;
}

void setup()
{
    Serial.begin(115200);
    pinMode(BUTTONPIN, INPUT);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.print("Starting...");

    while (!Serial)
        ;

    if (psramFound())
    {
        Serial.println("✅ PSRAM detected and enabled!");
        Serial.printf("Total PSRAM: %d bytes\n", ESP.getPsramSize());
        Serial.printf("Free PSRAM:  %d bytes\n", ESP.getFreePsram());
    }
    else
    {
        Serial.println("❌ PSRAM not detected. Check board_build.psram setting!");
    }

    Serial.println("=== Fashion Mnist CNN Model ===");
    Serial.printf("Free heap before: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM before: %d bytes\n", ESP.getFreePsram());

    // تحميل النموذج
    model = tflite::GetModel(fashion_mnist_cnn_int8_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        Serial.println("Model schema version mismatch!");
        while (1)
            ;
    }

    // ✅ إنشاء المفسر (Interpreter)
    interpreter = new tflite::MicroInterpreter(model, resolver, tensor_arena, TENSOR_ARENA_SIZE);

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        Serial.println("Tensor allocation failed!");
        while (1)
            ;
    }

    Serial.printf("Free heap after allocation: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM after allocation: %d bytes\n", ESP.getFreePsram());
    Serial.printf("Tensor arena size: %d bytes\n", TENSOR_ARENA_SIZE);

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.print("Input type: ");
    Serial.println(input->type == kTfLiteInt8 ? "int8" : "other");
    Serial.print("Input size: ");
    Serial.println(input->bytes);

    takeNewPicture = true;
}

void loop()
{
    buttonState = digitalRead(BUTTONPIN);

    if (buttonState == HIGH && takeNewPicture)
    {
        takeNewPicture = false;

        int image_index = distrib(gen);
        Serial.print("Selected random image: ");
        Serial.println(image_index);

        lcd.setCursor(0, 0);
        lcd.print("Predicting img" + String(image_index) + "...");

        const int8_t *selected_image_data = image_list[image_index - 1];

        camera_fb_t fake_fb;
        fake_fb.buf = (uint8_t *)selected_image_data;
        fake_fb.height = 28;
        fake_fb.width = 28;
        fake_fb.len = 28 * 28 * sizeof(int8_t);

        int8_t *model_input_data = convert_camera_frame_to_model_input(&fake_fb);
        if (!model_input_data)
        {
            lcd.setCursor(0, 1);
            lcd.print("Failed Input");
            Serial.println("Failed to convert image for model input!");
            takeNewPicture = true;
            return;
        }

        // ✅ نسخ الصورة إلى الإدخال
        memcpy(input->data.int8, model_input_data, MODEL_INPUT_SIZE);
        free(model_input_data);

        // ✅ تشغيل الاستدلال
        if (interpreter->Invoke() != kTfLiteOk)
        {
            lcd.setCursor(0, 1);
            lcd.print("Failed Inference");
            Serial.println("❌ Inference failed!");
            return;
        }

        Serial.printf("Free heap after inference: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("Free PSRAM after inference: %d bytes\n", ESP.getFreePsram());

        Serial.println("✅ Inference successful! Output values:");
        for (int i = 0; i < output->bytes; i++)
        {
            Serial.print(output->data.int8[i]);
            Serial.print(" ");
        }
        Serial.println();

        // استخراج النتيجة (أعلى احتمالية)
        int max_idx = 0;
        int8_t max_val = output->data.int8[0];
        for (int i = 1; i < output->bytes; i++)
        {
            if (output->data.int8[i] > max_val)
            {
                max_val = output->data.int8[i];
                max_idx = i;
            }
        }

        // ✅ عرض النتيجة
        Serial.print("Predicted class index: ");
        Serial.println(max_idx);
        Serial.print("Predicted class name: ");
        Serial.println(class_names[max_idx]);

        Serial.print("True class index: ");
        Serial.println(label_list[image_index - 1]);
        Serial.print("True class name: ");
        Serial.println(class_names[label_list[image_index - 1]]);

        lcd.setCursor(0, 1);
        lcd.print("Class:");
        lcd.print(class_names[max_idx]);
        lcd.print("   ");

        takeNewPicture = true;
    }
    else
    {
        lcd.setCursor(0, 0);
        lcd.print("Press BTN ");
    }
}
