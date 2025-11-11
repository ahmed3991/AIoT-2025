import json
import joblib
import numpy as np
import paho.mqtt.client as mqtt
import argparse
import time
import os
import warnings

# --- Ignore harmless warnings ---
warnings.filterwarnings("ignore", category=UserWarning)
warnings.filterwarnings("ignore", category=DeprecationWarning)

# --- CLI argument to choose model ---
parser = argparse.ArgumentParser()
parser.add_argument("--model", choices=["lr", "xgb"], default="lr", help="Select model pipeline (lr or xgb)")
args = parser.parse_args()

# --- Load model ---
BASE_DIR = os.path.dirname(__file__)
MODEL_DIR = os.path.join(BASE_DIR, "../models")
model_path = os.path.join(MODEL_DIR, f"{args.model}_pipeline.pkl")

print(f"🔍 Loading model from: {model_path}")
model = joblib.load(model_path)

# --- MQTT setup ---
BROKER = "broker.mqtt.cool"
PORT = 1883
TOPIC_SUB = "esp32/data"
TOPIC_PUB = "esp32/control"

client = mqtt.Client()

def on_connect(client, userdata, flags, rc):
    print("✅ Connected to MQTT broker")
    client.subscribe(TOPIC_SUB, qos=1)

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode("utf-8"))
        print(f"📩 Received: {data}")

        # --- Extract temperature & humidity ---
        t = float(data.get("temperature", 0.0))
        h = float(data.get("humidity", 0.0))

        # --- Build 8-feature vector (match your TP2 model) ---
        # Using first two from ESP32 and filling the rest with zeros
        X = np.array([[t, h] + [0.0] * 13])


        # --- Run inference ---
        pred = model.predict(X)[0]

        prob = None
        if hasattr(model, "predict_proba"):
            prob = float(model.predict_proba(X)[0][int(pred)])

        # --- Build JSON response ---
        result = {
            "device_id": data.get("device_id", "esp32"),
            "model": args.model,
            "prediction": int(pred),
            "probability": round(prob, 3) if prob is not None else None,
            "timestamp": int(time.time())
        }

        print(f"📤 Publishing: {result}")

        # --- Publish ON/OFF command for ESP32 LED ---
        command = "ON" if pred == 1 else "OFF"
        client.publish(TOPIC_PUB, command, qos=1)

    except Exception as e:
        print(f"⚠️ Error processing message: {e}")

# --- MQTT client setup ---
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, 60)
client.loop_forever()
