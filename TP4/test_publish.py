# test_publish.py
import json, time
import paho.mqtt.client as mqtt

broker = "test.mosquitto.org"
client = mqtt.Client()
client.connect(broker, 1883, 60)

payload = {
  "device_id": "esp32-dev-01",
  "timestamp": int(time.time()),
  "temperature": 24.0,
  "humidity": 40.0,
  "feature2": 0.0,
  "feature3": 400.0,
  "feature4": 12306.0,
  "feature5": 18520.0,
  "feature6": 939.735,
  "feature7": 0.0,
  "feature8": 0.0,
  "feature9": 0.0,
  "feature10": 0.0,
  "feature11": 0.0,
  "feature12": 0.0,
  "feature13": 0.0,
  "feature14": 0.0
}

client.publish("esp32/data", json.dumps(payload), qos=1)
print("published test payload")
client.disconnect()
