# TP4 — Report

## 1) Setup
- Device ID: esp-01
- Broker: broker.mqtt.cool:1883
- Topics: esp32/data, esp32/control, esp32/status/esp-01
- QoS: publish=0 (ESP32) / subscribe=1 (control)
- Build time: 2025-11-12 00:40:20

## 2) Models
- lr_pipeline.pkl size: 1137 bytes
- Features used from ESP32: temperature, humidity (+ features[] placeholder)

## 3) MQTT Flow
- ESP32 → esp32/data (JSON: device_id, temperature, humidity, seq, t_ms, features[], timestamp)
- Python subscriber → esp32/control (JSON: device_id, model, prediction, probability, timestamp)
- LED toggled based on prediction (1=ON, 0=OFF)

## 4) Latency (to fill)
- Method: compare consecutive seq + timestamps in logs
- Sample E2E (ESP publish → control received): ____ ms
- Notes: public broker variability

## 5) Robustness
- Reconnect/backoff, LWT online/offline, JSON validation

## 6) Optional: HTTP/REST
- POST /infer {device_id, temperature, humidity, ...} → {prediction, probability}
- ESP32: WiFiClient/HTTPClient

## 7) Conclusions
- Feasibility on ESP32, trade-offs MQTT vs REST
\n\n## Latency Results\n- Samples: 10\n- Avg latency: 1762905387579.1 ms\n- P95 latency: 1762905422916.0 ms\n