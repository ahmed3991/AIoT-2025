from flask import Flask, request, jsonify
import joblib
import numpy as np
import os

app = Flask(__name__)

MODEL_PATH = os.path.join(os.path.dirname(__file__), "..", "pipeline_Lr.pkl")
pipeline = joblib.load(MODEL_PATH)

FEATURES = [
    'Temperature[C]', 'Humidity[%]', 'TVOC[ppb]', 'eCO2[ppm]', 'Raw H2',
    'Raw Ethanol', 'Pressure[hPa]', 'PM1.0', 'PM2.5',
    'NC0.5', 'NC1.0', 'NC2.5'
]
DEFAULTS = {
    'TVOC[ppb]': 759,
    'eCO2[ppm]': 711,
    'Raw H2': 13000,
    'Raw Ethanol': 19030,
    'Pressure[hPa]': 856.34,
    'PM1.0': 3.7,
    'PM2.5': 4.7,
    'NC0.5': 19.85,
    'NC1.0': 3.783,
    'NC2.5': 1.063
}

def predict_with_pipeline(temp, hum):
    data = {'Temperature[C]': temp, 'Humidity[%]': hum}
    data.update(DEFAULTS)

    X = np.array([[data[f] for f in FEATURES]])

    prob = float(pipeline.predict_proba(X)[0][1])
    pred = int(pipeline.predict(X)[0])

    return pred, prob


@app.route("/infer", methods=["POST"])
def infer():

    content = request.json

    temp = content.get("temperature")
    hum = content.get("humidity")

    if temp is None or hum is None:
        return jsonify({"error": "missing temperature or humidity"}), 400

    pred, prob = predict_with_pipeline(temp, hum)
    action = "ON" if pred == 1 else "OFF"

    return jsonify({
        "temperature": temp,
        "humidity": hum,
        "prediction": pred,
        "probability": round(prob, 2),
        "action": action
    })


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
