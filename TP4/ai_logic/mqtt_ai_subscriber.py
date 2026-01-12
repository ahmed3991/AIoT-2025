import json
import paho.mqtt.client as mqtt
import pickle
import argparse
import os
import time

# --- Configuration ---
MQTT_BROKER_HOST = "broker.mqtt.cool"
MQTT_BROKER_PORT = 1883
MQTT_TOPIC_DATA = "esp32/data"
MQTT_TOPIC_CONTROL = "esp32/control"
MODEL_DIR = "models"

# --- Global Variable ---
# This will hold our loaded AI/ML pipeline
model_pipeline = None
model_name = "unknown"

def load_model(name):
    """Loads the specified .pkl model from the models directory."""
    global model_name
    model_name = name
    model_path = os.path.join(MODEL_DIR, f"{name}_pipeline.pkl")
    
    if not os.path.exists(model_path):
        print(f"Error: Model file not found at {model_path}")
        print("Please make sure 'lr_pipeline.pkl' and 'xgb_pipeline.pkl' are in the 'models' directory.")
        return None
        
    try:
        with open(model_path, 'rb') as f:
            loaded_model = pickle.load(f)
            print(f"Successfully loaded model: {model_path}")
            return loaded_model
    except Exception as e:
        print(f"Error loading model {model_path}: {e}")
        return None

def on_connect(client, userdata, flags, rc):
    """Callback for when the client connects to the broker."""
    if rc == 0:
        print(f"Connected to MQTT Broker at {MQTT_BROKER_HOST}")
        # Subscribe to the data topic from the ESP32
        client.subscribe(MQTT_TOPIC_DATA)
        print(f"Subscribed to topic: {MQTT_TOPIC_DATA}")
    else:
        print(f"Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    """
    This is the main logic.
    It replaces the simple 'if temperature > 30' rule.
    """
    global model_pipeline, model_name
    
    try:
        # 1. Parse the incoming JSON data from the ESP32
        data = json.loads(msg.payload.decode())
        print(f"\nReceived data: {data}")
        
        # 2. Extract the features
        # (This must match the JSON you built in main.cpp)
        features = data.get("features")
        
        if features is None:
            print("Error: 'features' key not found in JSON payload.")
            return

        # 3. Run inference using your loaded AI model
        #    scikit-learn pipelines expect a 2D array, so we wrap 'features'
        #    in a list: [features]
        prediction = model_pipeline.predict([features])[0] # Get the first (and only) prediction
        
        # 4. Determine the command
        #    (Assuming your model outputs 1 for 'ON' and 0 for 'OFF')
        command = "ON" if prediction == 1 else "OFF"
        
        print(f"Model: {model_name} | Prediction: {prediction} | Sending Command: {command}")

        # 5. Publish the control message back to the ESP32
        client.publish(MQTT_TOPIC_CONTROL, command)
        
    except json.JSONDecodeError:
        print(f"Error: Could not decode JSON: {msg.payload.decode()}")
    except Exception as e:
        print(f"An error occurred during processing: {e}")

# --- Main execution ---
if __name__ == "__main__":
    
    # 1. Set up argument parser to handle '--model lr' or '--model xgb'
    parser = argparse.ArgumentParser(description="AIoT MQTT Subscriber for Model Inference")
    parser.add_argument(
        "--model", 
        type=str, 
        choices=["lr", "xgb"], 
        required=True, 
        help="Model to use for inference (lr or xgb)"
    )
    args = parser.parse_args()

    # 2. Load the specified model
    model_pipeline = load_model(args.model)
    if model_pipeline is None:
        print("Exiting due to model loading failure.")
        exit(1) # Exit if model loading failed

    # 3. Set up MQTT client
    #    We use a unique client_id to avoid broker collisions
    client = mqtt.Client(client_id=f"ai_subscriber_{int(time.time())}")
    client.on_connect = on_connect
    client.on_message = on_message
    
    try:
        client.connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT, 60)
    except Exception as e:
        print(f"Error connecting to MQTT broker: {e}")
        exit(1)

    # 4. Start the loop
    #    This is a blocking call that handles all MQTT traffic
    print("Starting MQTT client loop... Press Ctrl+C to stop.")
    client.loop_forever()