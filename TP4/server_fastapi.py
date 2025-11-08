# server_fastapi.py
from fastapi import FastAPI, HTTPException, Query
from pydantic import BaseModel
import pickle
import os
import time
import numpy as np
import logging

# -----------------------------
# Logging setup
# -----------------------------
logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")

# -----------------------------
# Model paths and loading
# -----------------------------
MODEL_DIR = "models"
LR_PATH = os.path.join(MODEL_DIR, "lr_pipeline.pkl")
XGB_PATH = os.path.join(MODEL_DIR, "xgb_pipeline.pkl")

def load_model(path):
    try:
        with open(path, "rb") as f:
            return pickle.load(f)
    except Exception as e:
        logging.error(f"Error loading model {path}: {e}")
        return None

lr_model = load_model(LR_PATH)
xgb_model = load_model(XGB_PATH)
logging.info(f"Loaded models: lr={'yes' if lr_model else 'no'}, xgb={'yes' if xgb_model else 'no'}")

# -----------------------------
# Feature definition (MATCHES your pipeline)
# -----------------------------
FEATURE_NAMES = [
    "Unnamed: 0", "UTC", "Temperature[C]", "Humidity[%]", "TVOC[ppb]", "eCO2[ppm]",
    "Raw H2", "Raw Ethanol", "Pressure[hPa]", "PM1.0", "PM2.5", "NC0.5", "NC1.0", "NC2.5", "CNT"
]

# -----------------------------
# Pydantic model
# -----------------------------
class SensorPayload(BaseModel):
    device_id: str
    timestamp: int | None = None

    class Config:
        extra = "allow"  # allow additional fields (features)

# -----------------------------
# FastAPI app
# -----------------------------
app = FastAPI(title="AIoT Inference API", description="REST endpoint for ESP32 sensor data", version="1.0")

# -----------------------------
# Helper functions
# -----------------------------
def build_feature_vector(data: dict):
    """Build a feature vector matching model’s expected input."""
    values = []
    for feature in FEATURE_NAMES:
        try:
            values.append(float(data.get(feature, 0)))
        except Exception:
            values.append(0.0)
    return np.array([values])

def run_inference(model, data: dict):
    """Run inference and return prediction + optional probability."""
    X = build_feature_vector(data)
    pred = model.predict(X)[0]
    prob = None
    if hasattr(model, "predict_proba"):
        prob = float(model.predict_proba(X)[0][1])
    return int(pred), prob

# -----------------------------
# API endpoint
# -----------------------------
@app.post("/infer")
async def infer(payload: SensorPayload, model: str | None = Query(None)):
    body = payload.dict()
    model_choice = model or body.get("model") or "lr"

    # Select model
    pipeline = lr_model if model_choice == "lr" else xgb_model if model_choice == "xgb" else None
    if pipeline is None:
        raise HTTPException(status_code=400, detail=f"Model '{model_choice}' not available")

    # Run inference
    try:
        prediction, probability = run_inference(pipeline, body)
    except Exception as e:
        logging.exception("Inference failed")
        raise HTTPException(status_code=500, detail=str(e))

    return {
        "device_id": body.get("device_id"),
        "model": model_choice,
        "prediction": prediction,
        "probability": probability,
        "timestamp": int(time.time())
    }
