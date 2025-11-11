# TP8: Offloading AI Inference to a Python MQTT Subscriber

This practical work guides you through offloading the TensorFlow Lite model inference from the ESP32 to a Python application running on a host machine. The ESP32 will capture image data, publish it to an MQTT broker, and a Python subscriber will receive this data, perform the inference using the TensorFlow Lite model, and publish the predicted class name back to the ESP32. The ESP32 will then display the received prediction on its LCD.

## 1. Overview

In TP7, you integrated a TensorFlow Lite model directly onto the ESP32 for on-device inference. In this TP, we shift the inference task to a more powerful host machine running a Python application. The ESP32 will act as a data publisher, sending raw image data, while the Python application will subscribe to this data, perform the AI inference, and then publish the prediction result back to the ESP32.

### System Architecture

The system will operate as follows:

1.  **ESP32 (Publisher):**
    *   Captures mock image data (from `image_list.h`).
    *   Converts the image data into a suitable format (e.g., a flat array of `int8_t`).
    *   Publishes the image data as a JSON payload to the MQTT topic `esp32/data`.
    *   Subscribes to the MQTT topic `esp32/control` to receive the prediction result.
    *   Displays the received prediction on the LCD.

2.  **Python AI Subscriber (Subscriber/Publisher):**
    *   **Your task is to implement this subscriber.**
    *   Subscribes to the MQTT topic `esp32/data`.
    *   Receives the image data published by the ESP32.
    *   Loads the pre-trained TensorFlow Lite model (`fashion_mnist_cnn.tflite`).
    *   Performs inference on the received image data.
    *   Determines the predicted class name.
    *   Publishes the predicted class name to the MQTT topic `esp32/control`.

3.  **MQTT Broker:**
    *   Facilitates communication between the ESP32 and the Python AI Subscriber. We will use `broker.mqtt.cool`.

## 2. Python AI Subscriber Implementation

The `mqtt_ai_subscriber.py` script is provided as a starting point. Your main task in this section is to understand and complete the implementation of the subscriber to correctly process the image data from the ESP32 and perform AI inference.

### a. Dependencies

Ensure you have the necessary Python libraries installed. You can install them using pip:

```bash
pip install -r requirements.txt
```

### b. Code Structure (`mqtt_ai_subscriber.py`)

The `mqtt_ai_subscriber.py` file will contain the logic for connecting to the MQTT broker, receiving image data, performing inference, and publishing the result. You will need to complete the `on_message` function.

**Your Task: Implement the `on_message` function**

The `on_message` function is the core of your subscriber. It will be called every time a message is received on the `esp32/data` topic. You need to add the following logic within this function:

1.  **Parse the incoming JSON payload:** The `msg.payload` will contain a JSON string with the `encoded_image` data. You need to parse this string into a Python dictionary.
    ```python
    data = json.loads(msg.payload)
    ```
2.  **Extract and reshape image data:** The `encoded_image` will be a flat list of `int8_t` values. Convert this list into a NumPy array and reshape it to the model's expected input shape (1, 28, 28).
    ```python
    image_data = np.array(data["encoded_image"], dtype=np.int8).reshape(1, 28, 28)
    ```
3.  **Set the input tensor:** Provide the prepared `image_data` to the TensorFlow Lite interpreter's input tensor.
    ```python
    interpreter.set_tensor(input_details[0]['index'], image_data)
    ```
4.  **Invoke the interpreter:** Run the inference using the loaded model.
    ```python
    interpreter.invoke()
    ```
5.  **Get the output tensor:** Retrieve the results from the interpreter's output tensor.
    ```python
    output = interpreter.get_tensor(output_details[0]['index'])
    ```
6.  **Determine the predicted class:** Find the index of the class with the highest probability from the output tensor.
    ```python
    predicted_class_index = np.argmax(output[0])
    ```
7.  **Map to class name:** Use the `predicted_class_index` to get the human-readable `predicted_class_name` from the `class_names` array.
    ```python
    predicted_class_name = class_names[predicted_class_index]
    ```
8.  **Publish the prediction:** Send the `predicted_class_name` back to the ESP32 on the `esp32/control` MQTT topic.
    ```python
    client.publish("esp32/control", predicted_class_name)
    ```

## 3. ESP32 Publisher Setup and Execution

The `src/main.cpp` file in the `TP8` directory is already configured to send image data and receive predictions.

### a. Review `platformio.ini`

Ensure your `platformio.ini` file in the `TP8` directory is correctly configured for your ESP32-S3 board and includes the necessary libraries (e.g., `PubSubClient`, `LiquidCrystal_I2C`).

### b. Code Review (`src/main.cpp`)

The `src/main.cpp` file handles the ESP32's role as an MQTT publisher and subscriber. Key aspects to review include:

*   **WiFi and MQTT Setup:** The `setup_wifi()` function connects the ESP32 to your Wi-Fi network, and the `setup()` function initializes the MQTT client and sets up the `callback` function for incoming messages.
*   **`loop()` function:** This function continuously checks for MQTT connection, processes incoming messages, and handles button presses.
*   **Image Data Publishing:** When the button is pressed, a random mock image from `image_list.h` is selected, converted, and published as a JSON payload to `esp32/data`. Your task is to ensure the image data is correctly formatted as a JSON array within the payload.
    *   **Task 1: Format image data into a string:** The `main.cpp` code includes a loop that iterates through `model_input_data` and concatenates each `int8_t` value into a `String t`. This `String t` should represent a comma-separated list of the image's pixel values. Ensure this loop correctly builds the string `t` so that it can be embedded directly into a JSON array.
        ```cpp
        String t = "";
        for (int i = 0; i < MODEL_INPUT_SIZE; i++)
        {
          t += String(model_input_data[i]);
          if (i < MODEL_INPUT_SIZE - 1)
            t += ",";
        }
        ```
    *   **Task 2: Publish the JSON payload:** After formatting the image data into `String t`, you need to construct a complete JSON payload. This payload should be a JSON object with a key `"encoded_image"` whose value is a JSON array containing the `t` string. Finally, publish this JSON string to the `esp32/data` MQTT topic.
        ```cpp
        String payload = "{\"encoded_image\": [" + t + "]}";
        client.publish("esp32/data", payload.c_str());
        ```
*   **Prediction Display:** The `callback` function receives the predicted class name from the Python subscriber on `esp32/control` and updates the LCD.

### c. Compile and Upload

1.  Open the `TP8` folder in VS Code.
2.  Build the PlatformIO project (PlatformIO: Build).
3.  Upload the firmware to your ESP32-S3 board (PlatformIO: Upload).

### d. Observe the Results

Once the ESP32 is running and connected to Wi-Fi, and the Python subscriber is active:

1.  Press the button connected to `BUTTONPIN` (GPIO 4) on your ESP32.
2.  The ESP32 will send image data to the MQTT broker.
3.  The Python subscriber will receive the data, perform inference, and send back the predicted class name.
4.  The ESP32's LCD will update to display the predicted class name received from the Python subscriber.

This setup demonstrates a practical approach to offloading computationally intensive tasks like AI inference from resource-constrained edge devices (ESP32) to more powerful host machines, leveraging MQTT for seamless communication.
