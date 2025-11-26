import json
import argparse
import paho.mqtt.client as mqtt
import pickle
import time
from datetime import datetime
import numpy as np
import logging
import ssl

# ----------------------------
# 1. Setup logging
# ----------------------------
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s"
)

# ----------------------------
# 2. CLI argument to choose model
# ----------------------------
parser = argparse.ArgumentParser()
parser.add_argument("--model", choices=["lr", "xgb"], default="lr", help="Choose model: lr or xgb")
parser.add_argument("--secure", action="store_true", help="Use TLS/SSL for MQTT connection")
args = parser.parse_args()

MODEL_PATH = f"models/{args.model}_pipeline.pkl"

# ----------------------------
# 3. Load model
# ----------------------------
with open(MODEL_PATH, "rb") as f:
    model = pickle.load(f)

logging.info(f"Loaded model: {MODEL_PATH}")

# ----------------------------
# 4. MQTT callbacks
# ----------------------------
REQUIRED_FIELDS = ["temperature", "humidity"]

def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        logging.info("Connected to MQTT broker.")
        client.subscribe("esp32/data", qos=1)
    else:
        logging.error(f"Connection failed with code {reason_code}")

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode("utf-8")
        data = json.loads(payload)
        logging.info(f"Received data: {data}")

        # Validate required fields
        if not all(field in data for field in REQUIRED_FIELDS):
            logging.warning("Skipping message: missing required fields.")
            return

        # Fill all expected features with defaults (0 if missing)
        feature_names = [
            "temperature", "humidity", "light", "motion", "sound", "co2",
            "tvoc", "pressure", "altitude", "battery", "signal",
            "fan_speed", "heater_status", "occupancy"
        ]
        X = np.array([[float(data.get(f, 0)) for f in feature_names]])

        # Inference
        prediction = int(model.predict(X)[0])
        prob = None
        if hasattr(model, "predict_proba"):
            prob = float(model.predict_proba(X)[0][1])

        control_msg = {
            "device_id": data.get("device_id", "unknown"),
            "model": args.model,
            "prediction": prediction,
            "probability": prob,
            "timestamp": datetime.utcnow().isoformat() + "Z"
        }

        client.publish("esp32/control", json.dumps(control_msg), qos=1)
        logging.info(f"Published control message: {control_msg}")

    except json.JSONDecodeError:
        logging.warning("Received malformed JSON, ignoring message.")
    except Exception as e:
        logging.error(f"Error processing message: {e}")

# ----------------------------
# 5. Connect and loop
# ----------------------------
def connect_with_retry(client, broker, port):
    while True:
        try:
            client.connect(broker, port, 60)
            break
        except Exception as e:
            logging.warning(f"Connection failed: {e}. Retrying in 5s...")
            time.sleep(5)

client = mqtt.Client(protocol=mqtt.MQTTv5)
client.on_connect = on_connect
client.on_message = on_message

BROKER = "broker.mqtt.cool"
PORT = 8883 if args.secure else 1883

if args.secure:
    logging.info("Using secure TLS connection...")
    client.tls_set(cert_reqs=ssl.CERT_NONE)
    client.tls_insecure_set(True)

connect_with_retry(client, BROKER, PORT)
client.loop_forever()
