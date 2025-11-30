import paho.mqtt.client as mqtt
import json
import numpy as np
import tensorflow as tf

BROKER = "broker.mqtt.cool"
TOPIC_DATA = "esp32/data"
TOPIC_CONTROL = "esp32/control"

class_names = [
    "T-shirt/top", "Trouser", "Pullover", "Dress", "Coat",
    "Sandal", "Shirt", "Sneaker", "Bag", "Ankle boot"
]

# Load TensorFlow Lite model
interpreter = tf.lite.Interpreter(model_path="../fashion_mnist_cnn_int8.tflite")
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

def on_connect(client, userdata, flags, rc):
    print("Connected with result code", rc)
    client.subscribe(TOPIC_DATA)

def on_message(client, userdata, msg):
    print("📥 Message received:", msg.topic)

    # Parse incoming JSON
    data = json.loads(msg.payload)

    # Convert & reshape image data ----- FIXED HERE -----
    image_data = np.array(data["encoded_image"], dtype=np.int8)
    image_data = image_data.reshape(1, 28, 28, 1)   # <-- CORRECT SHAPE

    # Feed input to model
    interpreter.set_tensor(input_details[0]['index'], image_data)

    # Run inference
    interpreter.invoke()

    # Extract model output
    output = interpreter.get_tensor(output_details[0]['index'])

    # Determine predicted class
    prediction_index = np.argmax(output[0])
    predicted_name = class_names[prediction_index]

    print("🧠 Prediction:", predicted_name)

    # Send class name back to ESP32
    client.publish(TOPIC_CONTROL, predicted_name)
    print("📤 Sent prediction back to ESP32")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, 1883, 60)
client.loop_forever()
