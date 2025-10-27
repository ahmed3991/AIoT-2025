import json
import joblib
import paho.mqtt.client as mqtt
import numpy as np

pipeline = joblib.load("pipeline_Lr.pkl")
FEATURES = ['Temperature[C]', 'Humidity[%]', 'TVOC[ppb]', 'eCO2[ppm]', 'Raw H2',
            'Raw Ethanol', 'Pressure[hPa]', 'PM1.0', 'PM2.5', 'NC0.5', 'NC1.0', 'NC2.5']

FEATURES_VALUES = {
    'TVOC[ppb]': 79,
    'eCO2[ppm]': 16,
    'Raw H2': 1500,
    'Raw Ethanol': 1030,
    'Pressure[hPa]': 856.34,
    'PM1.0': 3.7,
    'PM2.5': 4.7,
    'NC0.5': 19.85,
    'NC1.0': 3.783,
    'NC2.5': 1.063
}

def predict_with_pipeline(temperature, humidity):
    data = {
        'Temperature[C]': temperature,
        'Humidity[%]': humidity
    }
    data.update(FEATURES_VALUES)

    X = np.array([[data[feature] for feature in FEATURES]])

    prob = pipeline.predict_proba(X)[0][1]
    pred = pipeline.predict(X)[0]
    return pred, prob

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
        temp = data.get("temperature")
        hum = data.get("humidity")
        print(f"Received data -> Temperature: {temp}°C | Humidity: {hum}%")

        if temp is None or hum is None:
            print("Missing temperature or humidity data.")
            return

        pred, prob = predict_with_pipeline(temp, hum)

        print(f"rediction: {pred} | Probability: {prob:.2f}")
        action = "ON" if pred == 1 else "OFF"
        client.publish("esp32/control", action)
        print(f" Action sent: {action}\n")

    except Exception as e:
        print(f" Error: {e}")

client = mqtt.Client()
client.connect("broker.mqtt.cool", 1883, 60)
client.subscribe("esp32/data")
client.on_message = on_message
print("MQTT Client running... Waiting for sensor data.")
client.loop_forever()
