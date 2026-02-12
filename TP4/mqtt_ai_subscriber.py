# mqtt_ai_subscriber.py
# MQTT AI Subscriber for Fire Detection System

import json
import pickle
import time
import numpy as np
import paho.mqtt.client as mqtt
from datetime import datetime

# ==========================================
# Configuration
# ==========================================
MQTT_BROKER = "broker.mqtt.cool"
MQTT_PORT = 1883
TOPIC_DATA = "esp32/data"
TOPIC_CONTROL = "esp32/control"

# Load ML models
print("="*60)
print("Loading ML models...")
print("="*60)

try:
    with open('lr_pipeline.pkl', 'rb') as f:
        lr_pipeline = pickle.load(f)
    print("[OK] Loaded: Logistic Regression Pipeline")
except FileNotFoundError:
    print("[ERROR] lr_pipeline.pkl not found!")
    lr_pipeline = None

try:
    with open('xgb_pipeline.pkl', 'rb') as f:
        xgb_pipeline = pickle.load(f)
    print("[OK] Loaded: XGBoost Pipeline")
except FileNotFoundError:
    print("[ERROR] xgb_pipeline.pkl not found!")
    xgb_pipeline = None

# Choose active model
ACTIVE_MODEL = "lr"  # Change to "xgb" for XGBoost

# ==========================================
# MQTT Callbacks
# ==========================================
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"\n[OK] Connected to MQTT Broker: {MQTT_BROKER}")
        client.subscribe(TOPIC_DATA)
        print(f"[OK] Subscribed to: {TOPIC_DATA}")
        print("\nWaiting for messages...\n")
    else:
        print(f"[ERROR] Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    try:
        # Parse incoming JSON
        data = json.loads(msg.payload.decode())
        
        print(f"\n{'='*60}")
        print(f"[RECEIVED] Message from: {data.get('device_id', 'unknown')}")
        print(f"  Temperature: {data['temperature']:.2f} C")
        print(f"  Humidity: {data['humidity']:.2f} %")
        print(f"  Timestamp: {data.get('timestamp', 'N/A')}")
        
        # Extract features
        temperature = data['temperature']
        humidity = data['humidity']
        
        # Create input for 2-feature model
        # For the full 13-feature model, we'd need all sensor readings
        # Here we'll use a simplified approach with padding
        X_input = np.array([[
            temperature, 
            humidity,
            0,      # TVOC[ppb]
            400,    # eCO2[ppm]
            12306,  # Raw H2
            18520,  # Raw Ethanol
            939.735,# Pressure[hPa]
            0.0,    # PM1.0
            0.0,    # PM2.5
            0.0,    # NC0.5
            0.0,    # NC1.0
            0.0,    # NC2.5
            0       # CNT
        ]])
        
        # Select model
        if ACTIVE_MODEL == "lr" and lr_pipeline:
            model_name = "Logistic Regression"
            pipeline = lr_pipeline
        elif ACTIVE_MODEL == "xgb" and xgb_pipeline:
            model_name = "XGBoost"
            pipeline = xgb_pipeline
        else:
            print("[ERROR] No model available!")
            return
        
        # Perform inference
        prediction = int(pipeline.predict(X_input)[0])
        probability = float(pipeline.predict_proba(X_input)[0][1])
        
        print(f"\n[INFERENCE] Model: {model_name}")
        print(f"  Prediction: {prediction} ({'FIRE DETECTED' if prediction == 1 else 'NO FIRE'})")
        print(f"  Probability: {probability*100:.2f}%")
        
        # Create response JSON
        response = {
            "device_id": data.get('device_id', 'unknown'),
            "model": ACTIVE_MODEL,
            "prediction": prediction,
            "probability": probability,
            "timestamp": int(time.time())
        }
        
        # Publish to control topic
        response_payload = json.dumps(response)
        client.publish(TOPIC_CONTROL, response_payload)
        
        print(f"\n[PUBLISHED] To: {TOPIC_CONTROL}")
        print(f"  Payload: {response_payload}")
        print("="*60)
        
    except json.JSONDecodeError:
        print("[ERROR] Invalid JSON received")
    except KeyError as e:
        print(f"[ERROR] Missing key in JSON: {e}")
    except Exception as e:
        print(f"[ERROR] {type(e).__name__}: {e}")

# ==========================================
# Main
# ==========================================
def main():
    print("\n" + "="*60)
    print("   MQTT AI Subscriber for Fire Detection System")
    print("="*60)
    print(f"Active Model: {ACTIVE_MODEL.upper()}")
    print(f"MQTT Broker: {MQTT_BROKER}:{MQTT_PORT}")
    print(f"Listening to: {TOPIC_DATA}")
    print(f"Publishing to: {TOPIC_CONTROL}")
    print("="*60)
    
    if not lr_pipeline and not xgb_pipeline:
        print("\n[ERROR] No models loaded! Cannot proceed.")
        print("Please ensure lr_pipeline.pkl and/or xgb_pipeline.pkl are in this directory.")
        return
    
    # Create MQTT client
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    
    # Connect to broker
    try:
        print(f"\nConnecting to {MQTT_BROKER}...")
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
    except Exception as e:
        print(f"[ERROR] Could not connect to broker: {e}")
        return
    
    # Start loop
    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("\n\n[INFO] Disconnecting...")
        client.disconnect()
        print("Goodbye!")

if __name__ == "__main__":
    main()
