import os
import json
import time
import numpy as np
import tensorflow as tf
import paho.mqtt.client as mqtt

# ===================== إعدادات عامة =====================
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
MODEL_PATH = os.path.join(BASE_DIR, "fashion_mnist_cnn.tflite")

BROKER = "broker.mqtt.cool"
   # بركر الـ TP
PORT = 1883
TOPIC_DATA = "esp32/data"
TOPIC_CONTROL = "esp32/control"

# أسماء الكلاسات (نفس TP7)
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

# ===================== تحميل الموديل =====================
print(f"[INFO] Loading TFLite model from: {MODEL_PATH}")
interpreter = tf.lite.Interpreter(model_path=MODEL_PATH)
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print("[INFO] Model input details:", input_details)
print("[INFO] Model output details:", output_details)

# مثال: شكل الإدخال المتوقع غالباً (1, 28, 28, 1)
input_index = input_details[0]["index"]
output_index = output_details[0]["index"]
expected_shape = input_details[0]["shape"]  # مثل [1 28 28 1]

# ===================== Callbacks للمـQTT =====================
def on_connect(client, userdata, flags, reason_code, properties=None):
    print(f"[INFO] Connected to MQTT broker with code {reason_code}")
    # الاشتراك في توبك البيانات من الـ ESP32
    client.subscribe(TOPIC_DATA)
    print(f"[INFO] Subscribed to topic: {TOPIC_DATA}")


def on_message(client, userdata, msg):
    """تُستدعى كلما وصلت رسالة على esp32/data"""
    try:
        payload_str = msg.payload.decode("utf-8")
        data = json.loads(payload_str)

        encoded = data.get("encoded_image", [])
        print(f"[INFO] Received message on {msg.topic}, length={len(encoded)}")

        # 1) تحويل البيانات إلى numpy array من نوع int8
        image_data = np.array(encoded, dtype=np.int8)

        # 2) إعادة تشكيلها حسب شكل الإدخال للموديل
        #    في موديلنا: [1, 28, 28, 1]
        if len(expected_shape) == 4:
            _, h, w, c = expected_shape
            image_data = image_data.reshape(1, h, w, c)
        elif len(expected_shape) == 3:
            _, h, w = expected_shape
            image_data = image_data.reshape(1, h, w)
        else:
            print(f"[WARN] Unexpected input shape: {expected_shape}")
            return

        # 3) وضعها في الـ input tensor
        interpreter.set_tensor(input_index, image_data)

        # 4) تنفيذ الـ inference
        interpreter.invoke()

        # 5) أخذ الـ output
        output = interpreter.get_tensor(output_index)  # شكلها (1, 10)
        probs = output[0]

        # 6) اختيار الكلاس ذو أكبر قيمة
        predicted_class_index = int(np.argmax(probs))
        predicted_class_name = class_names[predicted_class_index]

        print(
            f"[INFO] Prediction: index={predicted_class_index}, "
            f"class='{predicted_class_name}'"
        )

        # 7) نشر اسم الكلاس في توبك التحكم
        client.publish(TOPIC_CONTROL, predicted_class_name)
        print(f"[INFO] Published to {TOPIC_CONTROL}: {predicted_class_name}")

    except Exception as e:
        print(f"[ERROR] on_message failed: {e}")


# ===================== main =====================
def main():
    print("[INFO] Starting MQTT AI subscriber...")

    # ملاحظة: paho-mqtt 2.x ما زال يدعم هذا الأسلوب
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    # محاولة الاتصال بالبركر
    client.connect(BROKER, PORT, keepalive=60)

    # حلقة الانتظار (تستقبل الرسائل بلا توقف)
    client.loop_forever()


if __name__ == "__main__":
    main()
