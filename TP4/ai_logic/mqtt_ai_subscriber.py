# ===========================================
# File: TP4/ai_logic/mqtt_ai_subscriber.py
# Python MQTT subscriber that:
# - يستقبل JSON من "esp32/data"
# - يقرر prediction بسيط ويُنشر JSON إلى "esp32/control" مع qos=1
# - جاهز للاستبدال بالـ ML pipeline لاحقاً
# ===========================================
import json
import time
import argparse
import logging
import os
import pickle

import paho.mqtt.client as mqtt

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s: %(message)s")
LOGGER = logging.getLogger("mqtt_ai_subscriber")

BROKER = os.getenv("MQTT_BROKER", "broker.mqtt.cool")
PORT = int(os.getenv("MQTT_PORT", 1883))
TOPIC_IN = "esp32/data"
TOPIC_OUT = "esp32/control"
MODELS_DIR = "models"

def load_model_if_exists(name):
    """تحميل النموذج المحفوظ إذا وُجد، وإرجاع None خلاف ذلك"""
    path = os.path.join(MODELS_DIR, f"{name}_pipeline.pkl")
    if os.path.isfile(path):
        LOGGER.info("Loading model from: %s", path)
        with open(path, "rb") as f:
            m = pickle.load(f)
        return m
    LOGGER.info("Model file not found: %s", path)
    return None

def json_to_features(msg_json):
    """حوّل JSON الوارد إلى متجه ميزات حسب TP2.
       عدّلي هذه الدالة لتتناسب مع ميزاتك الحقيقية.
    """
    try:
        temp = float(msg_json.get("temperature"))
        hum = float(msg_json.get("humidity"))
    except Exception as e:
        LOGGER.error("Missing or invalid feature fields: %s", e)
        return None
    return [[temp, hum]]

def build_control_msg(device_id, model_name, pred, prob):
    return {
        "device_id": device_id,
        "model": model_name,
        "prediction": int(pred),
        "probability": float(prob),
        "timestamp": int(time.time())
    }

class MQTTInferenceClient:
    def __init__(self, model_name="lr"):
        self.model_name = model_name
        self.model = load_model_if_exists(model_name)
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        # set reconnection delays
        self.client.reconnect_delay_set(min_delay=1, max_delay=120)

    def on_connect(self, client, userdata, flags, rc):
        LOGGER.info("Connected to broker %s:%s rc=%s", BROKER, PORT, rc)
        client.subscribe(TOPIC_IN, qos=1)

    def on_message(self, client, userdata, msg):
        try:
            payload = msg.payload.decode()
            j = json.loads(payload)
        except Exception as e:
            LOGGER.error("Invalid JSON received: %s", e)
            return
        LOGGER.info("Received on %s: %s", msg.topic, j)

        device_id = j.get("device_id")
        if not device_id:
            LOGGER.warning("Message missing device_id; ignoring")
            return

        features = json_to_features(j)
        if features is None:
            LOGGER.warning("Feature extraction failed; ignoring")
            return

        # If a real model exists, use it; otherwise use simple rule
        if self.model is not None:
            try:
                import numpy as np
                feats = np.array(features)
                if hasattr(self.model, "predict_proba"):
                    prob = float(self.model.predict_proba(feats)[0][1])
                    pred = int(self.model.predict(feats)[0])
                else:
                    pred = int(self.model.predict(feats)[0])
                    prob = 1.0
            except Exception as e:
                LOGGER.exception("Model inference failed, falling back to rule: %s", e)
                # fallback rule
                pred = 1 if features[0][0] > 30 else 0
                prob = 0.9 if pred == 1 else 0.1
        else:
            # simple rule: temperature-based
            pred = 1 if features[0][0] > 30 else 0
            prob = 0.95 if pred == 1 else 0.05

        ctrl = build_control_msg(device_id, self.model_name, pred, prob)
        payload_out = json.dumps(ctrl)
        result = client.publish(TOPIC_OUT, payload_out, qos=1)
        if result.rc != mqtt.MQTT_ERR_SUCCESS:
            LOGGER.error("Publish failed with rc=%s", result.rc)
        else:
            LOGGER.info("Published control: %s", payload_out)

    def start(self):
        while True:
            try:
                LOGGER.info("Connecting to broker %s:%s", BROKER, PORT)
                self.client.connect(BROKER, PORT, keepalive=60)
                self.client.loop_forever()
            except Exception as e:
                LOGGER.exception("MQTT connection error, retrying in 5s: %s", e)
                time.sleep(5)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="MQTT AI subscriber")
    parser.add_argument("--model", choices=["lr", "xgb"], default="lr", help="Which model to use")
    args = parser.parse_args()

    cli = MQTTInferenceClient(model_name=args.model)
    cli.start()
