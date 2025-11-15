import json, paho.mqtt.client as mqtt
import numpy as np
import tensorflow as tf

# Load TFLite model
interpreter = tf.lite.Interpreter(model_path="cnn_model_quantized.tflite")
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# Label names
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
    "Ankle boot"
]

def on_message(client, userdata, msg):
    print("\nMessage received from ESP32")

    # Decode JSON
    data = json.loads(msg.payload.decode())
    encoded = data["encoded_image"]

    # Convert to NumPy
    image_data = np.array(encoded, dtype=np.int8).reshape(28, 28)  
    image_data = np.expand_dims(image_data, axis=0) 
    image_data = np.expand_dims(image_data, axis=-1) 

    # Send to model
    interpreter.set_tensor(input_details[0]['index'], image_data)

    interpreter.invoke()

    # Read output
    output = interpreter.get_tensor(output_details[0]['index'])[0]

    predicted_index = np.argmax(output)
    predicted_name = class_names[predicted_index]

    print(f"Predicted class = {predicted_name}")

    # Publish back to ESP32
    client.publish("esp32/control", predicted_name)
    print("Sent prediction back to ESP32 ")

client = mqtt.Client()
client.connect("broker.mqtt.cool", 1883, 60)

client.subscribe("esp32/data")
client.on_message = on_message

print("Python AI Subscriber running waiting for ESP32 images")
client.loop_forever()
