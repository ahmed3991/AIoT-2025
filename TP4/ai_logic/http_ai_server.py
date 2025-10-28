"""HTTP inference service that mirrors the MQTT control flow.

This module exposes a REST endpoint (``POST /infer``) that accepts the same
telemetry JSON produced by the ESP32 firmware. It loads the TP2 pipelines,
performs inference, and returns the control payload that the firmware expects.
"""

from __future__ import annotations

import argparse
import json
import logging
import pickle
import time
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional

from flask import Flask, jsonify, request

LOGGER = logging.getLogger("http_ai_server")


def parse_args(argv: Optional[Iterable[str]] = None) -> argparse.Namespace:
    """Parse command line arguments."""

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--models-dir",
        type=Path,
        default=Path(__file__).resolve().parent.parent / "models",
        help="Directory that contains the pickled pipelines.",
    )
    parser.add_argument(
        "--default-model",
        choices=("lr", "xgb"),
        default="lr",
        help="Pipeline to use when a model is not specified in the request.",
    )
    parser.add_argument(
        "--host",
        default="0.0.0.0",
        help="Host interface to bind the HTTP server (default: 0.0.0.0).",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=8000,
        help="Port for the HTTP server (default: 8000).",
    )
    parser.add_argument(
        "--log-level",
        default="INFO",
        help="Logging level (default: INFO).",
    )
    return parser.parse_args(argv)


def load_pipelines(models_dir: Path) -> Dict[str, Any]:
    """Load all supported pipelines from the models directory."""

    files = {
        "lr": models_dir / "lr_pipeline.pkl",
        "xgb": models_dir / "xgb_pipeline.pkl",
    }

    pipelines: Dict[str, Any] = {}
    for name, path in files.items():
        if not path.exists():
            LOGGER.warning("Model file missing: %s", path)
            continue

        LOGGER.info("Loading %s model from %s", name, path)
        with path.open("rb") as f:
            pipelines[name] = pickle.load(f)

    if not pipelines:
        raise FileNotFoundError(
            "No pipelines were loaded. Ensure TP2 artifacts are present in TP4/models/."
        )

    return pipelines


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


def create_app(pipelines: Dict[str, Any], default_model: str) -> Flask:
    """Create the Flask application with configured pipelines."""

    expected_features = {
        name: getattr(pipeline, "n_features_in_", None) for name, pipeline in pipelines.items()
    }

    app = Flask(__name__)

    @app.post("/infer")
    def infer() -> Any:  # type: ignore[override]
        payload = request.get_json(silent=True)
        if not isinstance(payload, dict):
            LOGGER.warning("Invalid request payload: %s", request.data)
            return jsonify({"error": "Invalid JSON payload"}), 400

        model_name = request.args.get("model") or payload.get("model") or default_model
        if model_name not in pipelines:
            LOGGER.warning("Requested model '%s' is not available", model_name)
            return jsonify({"error": f"Unknown model '{model_name}'"}), 400

        pipeline = pipelines[model_name]
        try:
            features = extract_features(payload, expected_features.get(model_name))
        except ValueError as exc:
            LOGGER.warning("Cannot process payload: %s", exc)
            return jsonify({"error": str(exc)}), 400

        try:
            prediction = pipeline.predict([features])[0]
        except Exception:
            LOGGER.exception("Model inference failed")
            return jsonify({"error": "Model inference failed"}), 500

        if hasattr(prediction, "item"):
            prediction = prediction.item()

        probability = compute_probability(pipeline, prediction, features)
        command = prediction_to_command(prediction, probability)
        response = {
            "device_id": payload.get("device_id"),
            "model": model_name,
            "prediction": float(prediction)
            if isinstance(prediction, (int, float))
            else prediction,
            "timestamp": time.time(),
            "command": command,
        }
        if probability is not None:
            response["probability"] = probability

        LOGGER.info("Inference response: %s", json.dumps(response))
        return jsonify(response)

    @app.get("/healthz")
    def healthcheck() -> Any:  # type: ignore[override]
        return jsonify({"status": "ok", "models": sorted(pipelines.keys())})

    return app


def main(argv: Optional[Iterable[str]] = None) -> int:
    args = parse_args(argv)
    logging.basicConfig(
        level=getattr(logging, args.log_level.upper(), logging.INFO),
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    )

    try:
        pipelines = load_pipelines(args.models_dir)
    except FileNotFoundError as exc:
        LOGGER.error("%s", exc)
        return 1

    app = create_app(pipelines, args.default_model)
    LOGGER.info(
        "Starting HTTP inference server on %s:%s with default model '%s'",
        args.host,
        args.port,
        args.default_model,
    )
    app.run(host=args.host, port=args.port)
    return 0


if __name__ == "__main__":  # pragma: no cover
    raise SystemExit(main())
