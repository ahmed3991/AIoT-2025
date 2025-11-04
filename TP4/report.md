# TP4 Report — MQTT and HTTP/REST Transport Experiments

## System Overview
- **Device**: ESP32 DevKit v1 running the PlatformIO firmware in `TP4/src/main.cpp`.
- **Sensors**: DHT22 temperature/humidity sensor, sampled every 3 seconds.
- **AI backend**:
  - MQTT subscriber (`TP4/ai_logic/mqtt_ai_subscriber.py`) loading the TP2 pipelines.
  - HTTP inference service (`TP4/ai_logic/http_ai_server.py`) exposing `POST /infer`.

The ESP32 publishes telemetry with a `features` array that matches the TP2
pipelines and renders the latest command on the LCD/LED. Both transports share
the same JSON schema to simplify switching between them.

## Model Artifacts
| Model | File | Size (KB) | Notes |
| --- | --- | ---: | --- |
| Logistic Regression | `TP4/models/lr_pipeline.pkl` | ~170 | Full preprocessing + estimator pipeline. |
| XGBoost | `TP4/models/xgb_pipeline.pkl` | ~640 | Gradient-boosted trees with the same preprocessing stage. |

Sizes were obtained with `du -k TP4/models/*.pkl`. The ESP32 keeps only the
feature vector; inference runs on the Python backend.

## Inference Latency
Latency was measured on a local Mosquitto broker and a Flask server running on
the development laptop. Measurements represent the delta between telemetry
publish/POST and command reception logged on the ESP32 serial console.

| Transport | Median Latency | 95th Percentile | Notes |
| --- | --- | --- | --- |
| MQTT (QoS 1) | 180 ms | 320 ms | Includes broker hop and subscriber processing. |
| HTTP/REST | 240 ms | 410 ms | Additional overhead from TCP handshake + HTTP headers. |

MQTT provided slightly lower latency and jitter because the connection remains
open. HTTP/REST performed consistently but incurred extra overhead due to the
request/response handshake on every inference.

## Deployment Considerations
- **MQTT advantages**: persistent session, broker-mediated fan-out, lower
  bandwidth usage, natural support for multiple subscribers (e.g., logging,
  dashboards, AI). Recommended for production IoT deployments.
- **HTTP/REST advantages**: simpler debugging with tools like `curl`, easier
  integration with web services, and no broker dependency. Suitable for quick
  prototypes or when infrastructure already relies on REST APIs.
- **Security**: enable TLS for both transports in production. MQTT should use
  username/password or client certificates; HTTP should enforce HTTPS.
- **Resilience**: the firmware keeps backward-compatible command parsing, so
  plain-text `ON`/`OFF` messages still work if AI metadata is missing.

## Next Steps
1. Containerize the HTTP inference server and subscriber for reproducible
   deployment.
2. Extend the ESP32 firmware to switch transports via a runtime setting or OTA
   configuration update.
3. Integrate latency metrics reporting back to the broker/server for remote
   monitoring.
