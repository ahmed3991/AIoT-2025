# mqtt_ai_subscriber.py
# Python AI subscriber: receives image data from ESP32 via MQTT,
# runs TFLite inference, and sends back predicted class name.

import os
import json
import time

import numpy as np
import tensorflow as tf
import paho.mqtt.client as mqtt

# ===== MQTT settings =====
BROKER = "broker.mqtt.cool"
PORT = 1883
TOPIC_SUB = "esp32/data"     # ESP32 -> Python
TOPIC_PUB = "esp32/control"  # Python -> ESP32

# ===== Model + labels =====
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
MODEL_PATH = os.path.join(BASE_DIR, "fashion_mnist_cnn.tflite")  # تأكد أن الملف موجود هنا

class_names = [
    "T-shirt/top",
    "Trouser",
    "Pullover",
    "Dress",
    "Coat",
    "Sandal",
    "Shirt",
    "Sneaker",
    "Bag",
    "Ankle boot",
]

print("[INFO] Loading TFLite model from:", MODEL_PATH)
interpreter = tf.lite.Interpreter(model_path=MODEL_PATH)
interpreter.allocate_tensors()
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print("[INFO] Model input details:", input_details)
print("[INFO] Model output details:", output_details)


# ===== MQTT callbacks =====
def on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        print("[MQTT] Connected successfully to broker.")
        client.subscribe(TOPIC_SUB)
        print(f"[MQTT] Subscribed to topic '{TOPIC_SUB}'")
    else:
        print(f"[MQTT] Failed to connect. Reason code: {reason_code}")


def on_message(client, userdata, msg):
    """Called whenever a message is received on esp32/data."""
    try:
        print(f"\n[MQTT] Message received on topic '{msg.topic}'")

        # 1) Parse JSON payload
        payload_str = msg.payload.decode("utf-8")
        data = json.loads(payload_str)
        # Expected format: {"encoded_image": [v1, v2, ..., v784]}
        encoded = data["encoded_image"]

        # 2) Convert to NumPy array and reshape
        image_data = np.array(encoded, dtype=np.int8).reshape(1, 28, 28)

        # If the model expects 4D input (1,28,28,1), add channel dimension
        if len(input_details[0]["shape"]) == 4:
            image_data = image_data[..., np.newaxis]

        print("[INFO] Image data shape for model:", image_data.shape)

        # 3) Set the input tensor
        interpreter.set_tensor(input_details[0]["index"], image_data)

        # 4) Run inference
        interpreter.invoke()

        # 5) Get output tensor
        output = interpreter.get_tensor(output_details[0]["index"])
        probs = output[0]

        # 6) Determine predicted class
        predicted_class_index = int(np.argmax(probs))
        predicted_class_name = class_names[predicted_class_index]

        print("[INFO] Raw output:", probs)
        print("[INFO] Predicted class index:", predicted_class_index)
        print("[INFO] Predicted class name :", predicted_class_name)

        # 7) Publish prediction back to ESP32
        client.publish(TOPIC_PUB, predicted_class_name)
        print(f"[MQTT] Published prediction to '{TOPIC_PUB}'")

    except Exception as e:
        print("[ERROR] Exception in on_message:", e)


# ===== Main =====
def main():
    print("[INFO] Starting MQTT AI subscriber...")

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(BROKER, PORT, keepalive=60)

    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user.")


if __name__ == "__main__":
    main()
