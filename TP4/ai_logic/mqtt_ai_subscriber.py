"""MQTT subscriber that loads TP2 pipelines and publishes inference results.

This script subscribes to ESP32 telemetry published on ``esp32/data``. When a
message arrives it loads the requested machine learning pipeline, performs
inference, and publishes a structured control message to ``esp32/control``.
"""

from __future__ import annotations

import argparse
import json
import logging
import pickle
import signal
import sys
import time
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional

import paho.mqtt.client as mqtt

LOGGER = logging.getLogger("mqtt_ai_subscriber")


def parse_args(argv: Optional[Iterable[str]] = None) -> argparse.Namespace:
    """Parse command line arguments."""

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--model",
        choices=("lr", "xgb"),
        default="lr",
        help="Select which pipeline to use for inference (default: lr).",
    )
    parser.add_argument(
        "--models-dir",
        type=Path,
        default=Path(__file__).resolve().parent.parent / "models",
        help="Directory that contains the pickled pipelines.",
    )
    parser.add_argument(
        "--broker",
        default="broker.mqtt.cool",
        help="MQTT broker hostname or IP address.",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=1883,
        help="MQTT broker port (default: 1883).",
    )
    parser.add_argument(
        "--topic-in",
        default="esp32/data",
        help="Topic to subscribe to for telemetry messages.",
    )
    parser.add_argument(
        "--topic-out",
        default="esp32/control",
        help="Topic where inference results will be published.",
    )
    parser.add_argument(
        "--qos",
        type=int,
        choices=(0, 1, 2),
        default=1,
        help="QoS level for outgoing control messages (default: 1).",
    )
    parser.add_argument(
        "--log-level",
        default="INFO",
        help="Logging level (default: INFO).",
    )
    return parser.parse_args(argv)


def load_pipeline(model_name: str, models_dir: Path):
    """Load the pickled pipeline selected by the user."""

    file_map = {
        "lr": models_dir / "lr_pipeline.pkl",
        "xgb": models_dir / "xgb_pipeline.pkl",
    }
    model_path = file_map[model_name]
    if not model_path.exists():
        raise FileNotFoundError(
            f"Could not locate model '{model_name}' at {model_path}. Did you copy the TP2 pipeline?"
        )

    LOGGER.info("Loading %s model from %s", model_name, model_path)
    with model_path.open("rb") as f:
        return pickle.load(f)


def _coerce_float(value: Any) -> float:
    try:
        return float(value)
    except (TypeError, ValueError):
        raise ValueError(f"Cannot convert value '{value}' to float") from None


def extract_features(payload: Dict[str, Any], expected: Optional[int]) -> List[float]:
    """Extract the feature vector from the telemetry payload."""

    if "features" in payload and isinstance(payload["features"], list):
        features = [_coerce_float(v) for v in payload["features"]]
    else:
        # Accept flattened feature_* keys if present.
        feature_keys = sorted(k for k in payload if k.startswith("feature_"))
        if feature_keys:
            features = [_coerce_float(payload[k]) for k in feature_keys]
        else:
            base = []
            for key in ("temperature", "humidity"):
                if key not in payload:
                    raise ValueError(
                        "Telemetry payload must include either a 'features' array or"
                        " both 'temperature' and 'humidity' fields."
                    )
                base.append(_coerce_float(payload[key]))
            features = base

    if expected is not None and len(features) != expected:
        LOGGER.warning(
            "Telemetry features length (%s) does not match the model expectation (%s).",
            len(features),
            expected,
        )
    return features


def compute_probability(pipeline, prediction, features: List[float]) -> Optional[float]:
    """Return the probability associated with the predicted class if available."""

    if not hasattr(pipeline, "predict_proba"):
        return None

    try:
        proba = pipeline.predict_proba([features])[0]
    except Exception:  # pragma: no cover - defensive against model issues
        LOGGER.exception("Failed to compute prediction probabilities")
        return None

    try:
        classes = list(pipeline.classes_)
        if hasattr(prediction, "item"):
            prediction = prediction.item()
        if prediction in classes:
            idx = classes.index(prediction)
            return float(proba[idx])
    except Exception:  # pragma: no cover - be robust even if metadata missing
        pass

    return None


def prediction_to_command(prediction: Any, probability: Optional[float]) -> str:
    """Convert model output to an ON/OFF command for the ESP32."""

    if isinstance(prediction, (int, float)):
        return "ON" if float(prediction) >= 0.5 else "OFF"
    if isinstance(prediction, str):
        upper = prediction.upper()
        if upper in {"ON", "OFF"}:
            return upper
        if upper in {"TRUE", "YES"}:
            return "ON"
        if upper in {"FALSE", "NO"}:
            return "OFF"
    if probability is not None:
        return "ON" if probability >= 0.5 else "OFF"
    return "OFF"


class MqttAiSubscriber:
    """MQTT client wrapper that performs inference on telemetry messages."""

    def __init__(self, args: argparse.Namespace) -> None:
        self.args = args
        self.pipeline = load_pipeline(args.model, args.models_dir)
        self.expected_features = getattr(self.pipeline, "n_features_in_", None)
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        self.client.reconnect_delay_set(min_delay=1, max_delay=60)

    # ------------------------------------------------------------------
    # MQTT callbacks
    # ------------------------------------------------------------------
    def on_connect(self, client: mqtt.Client, userdata, flags, rc):  # noqa: D401
        if rc == 0:
            LOGGER.info("Connected to MQTT broker")
            client.subscribe(self.args.topic_in, qos=self.args.qos)
        else:
            LOGGER.error("MQTT connection failed with code: %s", rc)

    def on_disconnect(self, client: mqtt.Client, userdata, rc):  # noqa: D401
        if rc != 0:
            LOGGER.warning("Unexpected MQTT disconnect (code=%s). Will retry...", rc)

    def on_message(self, client: mqtt.Client, userdata, msg: mqtt.MQTTMessage):
        try:
            payload = json.loads(msg.payload.decode("utf-8"))
            LOGGER.debug("Telemetry received: %s", payload)
        except json.JSONDecodeError:
            LOGGER.exception("Ignoring malformed JSON payload: %r", msg.payload)
            return

        for key in ("device_id", "timestamp"):
            if key not in payload:
                LOGGER.warning("Telemetry payload missing required field '%s'", key)

        try:
            features = extract_features(payload, self.expected_features)
        except ValueError as exc:
            LOGGER.warning("Cannot process payload: %s", exc)
            return

        try:
            prediction = self.pipeline.predict([features])[0]
        except Exception:
            LOGGER.exception("Model inference failed")
            return

        if hasattr(prediction, "item"):
            prediction = prediction.item()
        probability = compute_probability(self.pipeline, prediction, features)
        command = prediction_to_command(prediction, probability)

        control_message = {
            "device_id": payload.get("device_id"),
            "model": self.args.model,
            "prediction": float(prediction)
            if isinstance(prediction, (int, float))
            else prediction,
            "timestamp": time.time(),
            "command": command,
        }
        if probability is not None:
            control_message["probability"] = probability

        message = json.dumps(control_message)
        LOGGER.info("Publishing control message: %s", message)
        result = client.publish(self.args.topic_out, payload=message, qos=self.args.qos)
        status = result[0] if isinstance(result, tuple) else result.rc
        if status != mqtt.MQTT_ERR_SUCCESS:
            LOGGER.error("Failed to publish control message (status=%s)", status)

    # ------------------------------------------------------------------
    def run(self) -> None:
        LOGGER.debug(
            "Connecting to MQTT broker %s:%s", self.args.broker, self.args.port
        )
        self.client.connect(self.args.broker, self.args.port, keepalive=60)
        self.client.loop_start()

        # Allow graceful shutdown on Ctrl+C
        stop = False

        def _handle_sigint(signum, frame):  # noqa: D401
            nonlocal stop
            LOGGER.info("Received shutdown signal. Stopping MQTT loop...")
            stop = True

        signal.signal(signal.SIGINT, _handle_sigint)
        signal.signal(signal.SIGTERM, _handle_sigint)

        try:
            while not stop:
                time.sleep(0.25)
        finally:
            self.client.loop_stop()
            self.client.disconnect()


def main(argv: Optional[Iterable[str]] = None) -> int:
    args = parse_args(argv)
    logging.basicConfig(
        level=getattr(logging, args.log_level.upper(), logging.INFO),
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    )

    try:
        subscriber = MqttAiSubscriber(args)
    except FileNotFoundError as exc:
        LOGGER.error("%s", exc)
        return 1

    subscriber.run()
    return 0


if __name__ == "__main__":  # pragma: no cover
    sys.exit(main())
