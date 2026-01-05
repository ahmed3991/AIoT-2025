# test_publish_control.py
import time, json
import paho.mqtt.client as mqtt

broker = "test.mosquitto.org"
client = mqtt.Client()
client.connect(broker, 1883, 60)

control_msg = {
    "device_id": "esp32-dev-01",
    "model": "xgb",
    "prediction": 0,
    "probability": 0.87,
    "command": "OFF",
    "timestamp": int(time.time())
}

# نشر مع qos=1 و retain=True لضمان التسليم/التخزين عند الـ broker
client.publish("esp32/control", json.dumps(control_msg), qos=1, retain=True)
print("published control (qos=1, retain=True):", control_msg)
client.disconnect()
