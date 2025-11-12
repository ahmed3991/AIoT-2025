import json
import pickle
import paho.mqtt.client as mqtt

# === تحميل النموذجين ===
with open("../models/model_lr.pkl", "rb") as f:
    model_lr = pickle.load(f)

with open("../models/model_xgb.pkl", "rb") as f:
    model_xgb = pickle.load(f)


# النموذج الحالي المستخدم
current_model = "lr"
models = {"lr": model_lr, "xgb": model_xgb}

print("✅ Models loaded successfully (LR + XGB)")
print("🧠 Current model:", current_model)

# === دالة التبديل بين النماذج ===
def switch_model(new_model):
    global current_model
    if new_model in models:
        current_model = new_model
        print(f"🔁 Model switched to: {new_model}")
    else:
        print(f"⚠️ Unknown model: {new_model}")

# === عند استقبال رسالة ===
def on_message(client, userdata, msg):
    global current_model

    topic = msg.topic
    payload = msg.payload.decode("utf-8")

    # إذا كانت رسالة التحكم بالنموذج
    if topic == "esp32/model":
        switch_model(payload.strip())
        return

    # رسائل البيانات من Arduino
    if topic == "esp32/data":
        try:
            data = json.loads(payload)
            temp = data.get("temperature")
            humid = data.get("humidity")

            if temp is None or humid is None:
                print("⚠️ Missing temperature/humidity in data.")
                return

            # تجهيز الإدخال للنموذج
            X = [[temp, humid]]
            model = models[current_model]
            pred = model.predict(X)[0]

            # إرسال القرار إلى ESP32
            command = "ON" if pred == 1 else "OFF"
            print(f"📩 Data: T={temp:.1f}°C, H={humid:.1f}% → Model={current_model} → {command}")
            client.publish("esp32/control", command)

        except Exception as e:
            print("❌ Error handling message:", e)

# === إعداد MQTT ===
client = mqtt.Client()
client.on_message = on_message

broker = "broker.mqtt.cool"
port = 1883

client.connect(broker, port, 60)
client.subscribe("esp32/data")
client.subscribe("esp32/model")  # لتبديل النموذج

print(f"🚀 Connected to MQTT broker at {broker}:{port}")
print("📡 Listening for data...")

client.loop_forever()
