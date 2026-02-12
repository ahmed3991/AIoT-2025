# mqtt_ai_subscriber.py
# MQTT AI Subscriber for Fire Detection (IoT Version - 2 features)

import json
import pickle
import time
import numpy as np
import paho.mqtt.client as mqtt

# ==========================================
# SimplePipeline Class Definition
# ==========================================
class SimplePipeline:
    """Simple pipeline wrapper for scaler + model"""
    def __init__(self, scaler, model):
        self.scaler = scaler
        self.model = model
    
    def predict(self, X):
        X_scaled = self.scaler.transform(X)
        return self.model.predict(X_scaled)
    
    def predict_proba(self, X):
        X_scaled = self.scaler.transform(X)
        return self.model.predict_proba(X_scaled)

# ==========================================
# Configuration
# ==========================================
MQTT_BROKER = "broker.mqtt.cool"
MQTT_PORT = 1883
TOPIC_DATA = "esp32/data"
TOPIC_CONTROL = "esp32/control"

# ==========================================
# Load IoT Model (2 features only)
# ==========================================
print("="*60)
print("Loading IoT ML model (2 features: Temperature, Humidity)")
print("="*60)

try:
    with open('lr_iot_pipeline.pkl', 'rb') as f:
        model_pipeline = pickle.load(f)
    print("[OK] Loaded: IoT Logistic Regression Pipeline (77.11% accuracy)")
except FileNotFoundError:
    print("[ERROR] lr_iot_pipeline.pkl not found!")
    print("Please run: python retrain_for_iot.py")
    model_pipeline = None
except Exception as e:
    print(f"[ERROR] Failed to load model: {e}")
    model_pipeline = None

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
        
        # Extract and validate temperature and humidity
        temperature = data.get('temperature')
        humidity = data.get('humidity')
        
        if temperature is None or humidity is None:
            print("[ERROR] Missing temperature or humidity in message")
            return
        
        print(f"  Temperature: {temperature:.2f} C")
        print(f"  Humidity: {humidity:.2f} %")
        print(f"  Timestamp: {data.get('timestamp', 'N/A')}")
        
        if not model_pipeline:
            print("[ERROR] No model loaded!")
            return
        
        # Create input array (2 features)
        X_input = np.array([[temperature, humidity]])
        
        # Perform inference
        prediction = int(model_pipeline.predict(X_input)[0])
        probability = float(model_pipeline.predict_proba(X_input)[0][1])
        
        print(f"\n[INFERENCE] Model: Logistic Regression (IoT - 2 features)")
        print(f"  Prediction: {prediction} ({'FIRE DETECTED' if prediction == 1 else 'NO FIRE'})")
        print(f"  Probability: {probability*100:.2f}%")
        
        # Create response JSON
        response = {
            "device_id": data.get('device_id', 'unknown'),
            "model": "lr_iot",
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
        import traceback
        traceback.print_exc()

# ==========================================
# Main Function
# ==========================================
def main():
    print("\n" + "="*60)
    print("   MQTT AI Subscriber for Fire Detection (IoT)")
    print("="*60)
    print(f"Model: IoT Logistic Regression (2 features)")
    print(f"Features: Temperature, Humidity")
    print(f"MQTT Broker: {MQTT_BROKER}:{MQTT_PORT}")
    print(f"Listening to: {TOPIC_DATA}")
    print(f"Publishing to: {TOPIC_CONTROL}")
    print("="*60)
    
    if not model_pipeline:
        print("\n[ERROR] No model loaded! Cannot proceed.")
        print("Run: python retrain_for_iot.py")
        return
    
    # Create MQTT client
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
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
