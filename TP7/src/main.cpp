#include <Arduino.h>
#include <MicroTFLite.h>
#include "model_data.h"
#include "input_image_int8.h" // the test image

// Define memory for tensors
#define TENSOR_ARENA_SIZE 93 * 1024
uint8_t tensor_arena[TENSOR_ARENA_SIZE];

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

// Define interpreter, model, etc.
const tflite::Model *model;
tflite::MicroInterpreter *interpreter;
tflite::AllOpsResolver resolver;
TfLiteTensor *input;
TfLiteTensor *output;

void setup()
{
    Serial.begin(115200);
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

    // Load model
    model = tflite::GetModel(fashion_mnist_cnn_int8_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        Serial.println("Model schema version mismatch!");
        while (1)
            ;
    }

    // Create interpreter
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

    // Get input/output tensors
    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.print("Input type: ");
    Serial.println(input->type == kTfLiteInt8 ? "int8" : "other");
    Serial.print("Input size: ");
    Serial.println(input->bytes);

    // Copy your int8 image into input tensor
    memcpy(input->data.int8, input_image, 28 * 28 * sizeof(int8_t));

    // Run inference
    if (interpreter->Invoke() != kTfLiteOk)
    {
        Serial.println("❌ Inference failed!");
        while (1)
            ;
    }

    Serial.printf("Free heap after inference: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM after inference: %d bytes\n", ESP.getFreePsram());

    // Print output values
    Serial.println("✅ Inference successful! Output values:");
    for (int i = 0; i < output->bytes; i++)
    {
        Serial.print(output->data.int8[i]);
        Serial.print(" ");
    }
    Serial.println();

    // Find the predicted class
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

    Serial.print("Predicted class index: ");
    Serial.println(max_idx);
    Serial.print("Predicted class name: ");
    Serial.println(class_names[max_idx]);
}

void loop()
{
    delay(3000);
}
