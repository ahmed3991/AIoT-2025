import paho.mqtt.publish as publish
import json
from datetime import datetime

msg = {
    "device_id": "esp32_01",
    "timestamp": datetime.utcnow().isoformat() + "Z",
    "temperature": 25.0,
    "humidity": 45.0,
    "light": 0,
    "motion": 0,
    "sound": 0,
    "co2": 0,
    "tvoc": 0,
    "pressure": 0,
    "altitude": 0,
    "battery": 0,
    "signal": 0,
    "fan_speed": 0,
    "heater_status": 0,
    "occupancy": 0
}

publish.single("esp32/data", json.dumps(msg), hostname="broker.mqtt.cool", qos=1)
print("Message published to esp32/data")
