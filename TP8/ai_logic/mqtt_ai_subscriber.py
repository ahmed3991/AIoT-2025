import json
import numpy as np
import tensorflow as tf
import paho.mqtt.client as mqtt

# ------------------------------------------------------
# 1️⃣ Load TFLite Model (cnn_model_quant.tflite)
# ------------------------------------------------------
interpreter = tf.lite.Interpreter(model_path="cnn_model_quant.tflite")
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# Class labels (Fashion MNIST)
class_names = [
    "T-shirt/top", "Trouser", "Pullover", "Dress",
    "Coat", "Sandal", "Shirt", "Sneaker", "Bag", "Ankle boot"
]

# ------------------------------------------------------
# 2️⃣ MQTT Callback — When ESP32 Sends Image
# ------------------------------------------------------
def on_message(client, userdata, msg):
    try:
        # Parse JSON payload
        data = json.loads(msg.payload.decode("utf-8"))
        print("📩 Received image data from ESP32")

        # Convert to numpy array and reshape
        image_data = np.array(data["encoded_image"], dtype=np.int8).reshape(1, 28, 28, 1)

        # Run inference
        interpreter.set_tensor(input_details[0]['index'], image_data)
        interpreter.invoke()
        output = interpreter.get_tensor(output_details[0]['index'])

        # Get prediction
        predicted_class_index = np.argmax(output[0])
        predicted_class_name = class_names[predicted_class_index]

        print(f"✅ Predicted: {predicted_class_name}")

        # Send result back to ESP32
        client.publish("esp32/control", predicted_class_name)

    except Exception as e:
        print("❌ Error during inference:", e)

# ------------------------------------------------------
# 3️⃣ MQTT Setup
# ------------------------------------------------------
def main():
    client = mqtt.Client()
    client.on_message = on_message
    client.connect("broker.mqtt.cool", 1883, 60)

    # Subscribe to the topic where ESP32 sends images
    client.subscribe("esp32/data")
    print("🚀 MQTT AI Subscriber started, waiting for data...")

    client.loop_forever()

if __name__ == "__main__":
    main()
