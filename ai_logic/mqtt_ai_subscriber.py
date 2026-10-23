
import json
import math
import time
import logging
import paho.mqtt.client as mqtt

# MQTT configuration
BROKER = "broker.mqtt.cool"
PORT = 1883

USER_NAMESPACE = "aiot_lab_yourname"
DEVICE_ID = "esp32-01"

TOPIC_DATA = f"{USER_NAMESPACE}/{DEVICE_ID}/data"
TOPIC_CONTROL = f"{USER_NAMESPACE}/{DEVICE_ID}/control"

# Logistic Regression parameters from TP3
MEAN = [31.350938, 40.726445]
STD = [9.210854, 16.888109]
WEIGHTS = [3.401914, -1.696985]
BIAS = 13.062134
THRESHOLD = 0.5

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s: %(message)s"
)


def predict(temperature, humidity):
    raw = [temperature, humidity]

    scaled = [
        (raw[i] - MEAN[i]) / STD[i]
        for i in range(2)
    ]

    z = BIAS + sum(
        WEIGHTS[i] * scaled[i]
        for i in range(2)
    )

    if z >= 0:
        probability = 1.0 / (1.0 + math.exp(-z))
    else:
        ez = math.exp(z)
        probability = ez / (1.0 + ez)

    predicted_class = int(probability >= THRESHOLD)

    return probability, predicted_class


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        logging.info("Connected to MQTT broker")
        client.subscribe(TOPIC_DATA, qos=1)
        logging.info("Subscribed to %s", TOPIC_DATA)
    else:
        logging.error("Connection failed: %s", reason_code)


def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode("utf-8"))

        device_id = data.get("device_id")
        temperature = data.get("temperature")
        humidity = data.get("humidity")

        if device_id != DEVICE_ID:
            logging.warning("Ignoring unknown device: %s", device_id)
            return

        if (
            isinstance(temperature, bool)
            or not isinstance(temperature, (int, float))
            or isinstance(humidity, bool)
            or not isinstance(humidity, (int, float))
            or not math.isfinite(temperature)
            or not math.isfinite(humidity)
        ):
            logging.warning("Invalid sensor values: %s", data)
            return

        start = time.perf_counter()
        probability, predicted_class = predict(
            float(temperature), float(humidity)
        )
        inference_ms = (time.perf_counter() - start) * 1000

        # TP3 class mapping: 1 = FIRE ALARM, 0 = SAFE
        command = "ON" if predicted_class == 1 else "OFF"

        response = {
            "device_id": device_id,
            "model": "lr",
            "prediction": command,
            "predicted_class": predicted_class,
            "probability": round(probability, 4),
            "timestamp": int(time.time()),
            "inference_ms": round(inference_ms, 3)
        }

        info = client.publish(
            TOPIC_CONTROL,
            json.dumps(response),
            qos=1
        )

        if info.rc == mqtt.MQTT_ERR_SUCCESS:
            logging.info("Input: %s", data)
            logging.info("Prediction: %s", response)
        else:
            logging.error("Failed to publish response")

    except (json.JSONDecodeError, UnicodeDecodeError):
        logging.warning("Ignoring malformed JSON message")
    except Exception:
        logging.exception("Error processing MQTT message")


client = mqtt.Client(
    callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
    client_id="aiot-python-lr-subscriber"
)

client.on_connect = on_connect
client.on_message = on_message
client.reconnect_delay_set(min_delay=1, max_delay=30)

while True:
    try:
        client.connect(BROKER, PORT, 60)
        client.loop_forever(retry_first_connection=True)
    except (OSError, mqtt.MqttError) as error:
        logging.error("MQTT error: %s", error)
        time.sleep(5)
