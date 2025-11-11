#!/usr/bin/env python3
"""
mqtt_ai_subscriber.py
Subscribe to esp32/data -> run ML pipeline -> publish control to esp32/control

Usage:
  python mqtt_ai_subscriber.py --broker test.mosquitto.org --models-dir ../TP2/models --model xgb
"""
import argparse
import json
import logging
import os
import pickle
import sys
import time
from typing import Any, Dict, List, Optional

import numpy as np
import paho.mqtt.client as mqtt

LOG = logging.getLogger("mqtt_ai_subscriber")

# Default feature order (must match what ESP32 sends and what TP2 pipeline expects)
FEATURE_ORDER = [
    "temperature",
    "humidity",
    "feature2",
    "feature3",
    "feature4",
    "feature5",
    "feature6",
    "feature7",
    "feature8",
    "feature9",
    "feature10",
    "feature11",
    "feature12",
    "feature13",
    "feature14",
]

def load_pipeline(path: str):
    LOG.info("Loading pipeline from %s", path)
    with open(path, "rb") as f:
        pipeline = pickle.load(f)
    LOG.info("Loaded pipeline type: %s", type(pipeline))
    return pipeline

def get_prediction_and_prob(pipeline, X: np.ndarray):
    """
    pipeline: any scikit-like pipeline or model
    X: shape (1, n_features)
    returns (prediction, probability_or_null)
    """
    try:
        preds = pipeline.predict(X)
        pred = preds[0].item() if hasattr(preds[0], "item") else preds[0]
    except Exception as e:
        LOG.exception("Error during predict(): %s", e)
        raise

    prob = None
    try:
        if hasattr(pipeline, "predict_proba"):
            proba = pipeline.predict_proba(X)
            
            if proba is not None:
                
                if hasattr(pred, "__int__"):
                    cls_idx = int(pred)
                else:
                    cls_idx = np.argmax(proba, axis=1)[0]
                prob = float(proba[0][cls_idx])
    except Exception:
        LOG.debug("predict_proba() unavailable or failed", exc_info=True)
    return pred, prob

# Build numpy array from JSON payload using FEATURE_ORDER
def json_to_feature_array(msg: Dict[str, Any]) -> np.ndarray:
    vals: List[float] = []
    for feat in FEATURE_ORDER:
        vals.append(float(msg.get(feat, 0.0)))
    arr = np.array(vals, dtype=float).reshape(1, -1)
    return arr


class MqttAI:
    def __init__(self, broker, port, models_dir, model_name, client_id="mqtt_ai_subscriber"):
        self.broker = broker
        self.port = port
        self.models_dir = models_dir
        self.model_name = model_name  # 'lr' or 'xgb' or full filename
        self.client = mqtt.Client(client_id=client_id)
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        self.pipeline = None
        self.connected = False
        self._load_pipeline()

    def _load_pipeline(self):
        
        candidates = [
            os.path.join(self.models_dir, f"{self.model_name}_pipeline.pkl"),
            os.path.join(self.models_dir, f"{self.model_name}.pkl"),
            os.path.join(self.models_dir, f"{self.model_name}_model.pkl"),
            os.path.join(self.models_dir, self.model_name),
        ]
        for path in candidates:
            if path and os.path.exists(path):
                self.pipeline = load_pipeline(path)
                return
        # fallback: try common filenames in models_dir
        for fn in os.listdir(self.models_dir):
            if fn.lower().endswith(".pkl") and self.model_name in fn.lower():
                self.pipeline = load_pipeline(os.path.join(self.models_dir, fn))
                return
        LOG.error("Could not find pipeline for '%s' in %s. Tried: %s", self.model_name, self.models_dir, candidates)
        raise FileNotFoundError("Model pipeline not found")

    def connect_and_loop(self):
        backoff = 1
        while True:
            try:
                LOG.info("Connecting to MQTT broker %s:%s ...", self.broker, self.port)
                self.client.connect(self.broker, self.port, keepalive=60)
                
                self.client.loop_start()
                
                for _ in range(10):
                    if self.connected:
                        break
                    time.sleep(0.2)
                if not self.connected:
                    LOG.warning("Not connected after connect attempt, will retry")
                return
            except Exception as e:
                LOG.exception("MQTT connect failed: %s", e)
                LOG.info("Reconnect backoff %s seconds", backoff)
                time.sleep(backoff)
                backoff = min(backoff * 2, 60)

    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            LOG.info("Connected to MQTT (rc=%s)", rc)
            self.connected = True
            # subscribe to esp32/data
            client.subscribe("esp32/data", qos=1)
            LOG.info("Subscribed to esp32/data")
        else:
            LOG.error("Bad connect result: %s", rc)

    def on_disconnect(self, client, userdata, rc):
        LOG.warning("Disconnected from broker (rc=%s)", rc)
        self.connected = False
        

    def on_message(self, client, userdata, msg):
        topic = msg.topic
        payload_raw = msg.payload.decode(errors="ignore").strip()
        LOG.info("Message arrived topic=%s payload=%s", topic, payload_raw)

        # If this is sensor data -> run inference
        if topic == "esp32/data":
            try:
                payload = json.loads(payload_raw)
            except Exception as e:
                LOG.warning("Malformed JSON on esp32/data: %s", e)
                return

            if "device_id" not in payload:
                LOG.warning("Ignoring esp32/data message without device_id")
                return

            try:
                X = json_to_feature_array(payload)
            except Exception as e:
                LOG.warning("Bad feature vector: %s", e)
                return

            try:
                pred, prob = get_prediction_and_prob(self.pipeline, X)
                try:
                    pred_val = int(pred)
                except Exception:
                    pred_val = pred
                prob_val = None if prob is None else float(prob)
                LOG.info("Prediction: %s prob=%s", pred_val, prob_val)
            except Exception as e:
                LOG.exception("Inference failed: %s", e)
                return

            command = "ON" if (isinstance(pred_val, int) and pred_val == 1) else "OFF"
            control_msg = {
                "device_id": payload.get("device_id"),
                "model": self.model_name,
                "prediction": pred_val,
                "probability": prob_val,
                "command": command,
                "timestamp": int(time.time()),
            }

            control_json = json.dumps(control_msg)
            try:
                result = client.publish("esp32/control", control_json, qos=1, retain=True)
                LOG.info("Published control -> %s (mid=%s)", control_json, getattr(result, "mid", None))
            except Exception:
                LOG.exception("Failed to publish control message")
            return

        if topic == "esp32/control":
            try:
                ctrl = json.loads(payload_raw)
                LOG.info("Control message received (debug): %s", ctrl)
            except Exception:
                LOG.info("Control message received (debug, raw): %s", payload_raw)
            return

        # For any other topic - just log
        LOG.debug("Received message on unexpected topic %s", topic)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--broker", default="test.mosquitto.org", help="MQTT broker hostname/IP")
    parser.add_argument("--port", type=int, default=1883, help="MQTT broker port")
    parser.add_argument("--models-dir", default="models", help="Directory containing pickled pipelines")
    parser.add_argument("--model", default="xgb", help="Which model to use (lr or xgb or filename)")
    parser.add_argument("--log", default="info", help="Logging level")
    args = parser.parse_args()

    logging.basicConfig(level=getattr(logging, args.log.upper()), format="%(asctime)s %(levelname)s %(message)s")
    LOG.info("Starting mqtt_ai_subscriber")

    if not os.path.isdir(args.models_dir):
        LOG.error("Models dir does not exist: %s", args.models_dir)
        sys.exit(1)

    ai = MqttAI(broker=args.broker, port=args.port, models_dir=args.models_dir, model_name=args.model)
    ai.connect_and_loop()

    try:
        LOG.info("Subscriber running. Press Ctrl+C to exit.")
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        LOG.info("Stopping...")
    finally:
        ai.client.loop_stop()
        ai.client.disconnect()

if __name__ == "__main__":
    main()