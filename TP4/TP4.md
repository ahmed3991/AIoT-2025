# TP4 — Communication Protocols for AIoT and Practical: MQTT + HTTP/REST

## Overview
This practical explores communication protocols commonly used in IoT systems, with a focus on how they interact with AI pipelines. We'll cover the trade-offs between protocols (MQTT, HTTP/REST, CoAP, WebSockets) and why MQTT is often chosen for constrained devices. After the theory, you'll complete a hands-on assignment: implement an ESP32 publisher that sends sensor data, a Python subscriber that loads the ML pipelines you built in TP2, performs inference, and sends the decision back to the ESP32. Finally, you'll replace the MQTT transport with an HTTP/REST-based implementation.

---

## 1. Communication protocols used in IoT (short survey)

When building AI-enabled IoT systems, choosing the right communication protocol matters for latency, reliability, bandwidth, power, and developer ergonomics. Here are the most common options:

- MQTT (Message Queuing Telemetry Transport)
  - Lightweight publish/subscribe protocol.
  - Uses a broker (server) to route messages between publishers and subscribers.
  - Small packet overhead, persistent sessions, QoS levels (0,1,2), retained messages, and last-will testament.
  - Works well with intermittent connectivity and constrained devices (low memory/CPU).

- HTTP/REST
  - Request/response model; widely supported and easy to debug.
  - Higher overhead compared to MQTT because of verbose headers and stateless nature.
  - Easier to interact with from browsers and web services. Not ideal for low-power devices but simple and robust.

- CoAP (Constrained Application Protocol)
  - Designed for constrained devices, maps well to RESTful semantics but uses UDP.
  - Lower overhead than HTTP and supports multicast, but less ubiquitous tooling.

- WebSockets
  - Persistent, full-duplex communication channel over a single TCP connection.
  - Good for interactive dashboards or low-latency streams between browsers and backends.

Which to choose depends on your constraints. For many sensor-to-cloud AI workflows on devices like the ESP32, MQTT is a strong default because of its lightweight pub/sub model and broker-mediated decoupling.

---

## 2. Deep dive: MQTT (why it fits AIoT)

MQTT is a publish/subscribe messaging protocol designed for lightweight telemetry. Key concepts:

- Broker: Central server that receives messages from publishers and forwards them to subscribers based on topic filters.
- Topic: UTF-8 string used for routing messages (e.g., `sensors/esp32/temperature`). Topics are hierarchical and support wildcards in subscriptions.
- Publisher: Client that sends messages to a topic.
- Subscriber: Client that subscribes to topics and receives messages published to them.
- QoS levels:
  - 0: At most once — fastest but unreliable delivery.
  - 1: At least once — ensures delivery but may produce duplicates.
  - 2: Exactly once — slowest, ensures single delivery.
- Retained messages: Broker can store the last retained message per topic for new subscribers.
- Last Will and Testament (LWT): Broker publishes a defined message if a client disconnects unexpectedly.

Why MQTT for AI edge workflows?
- Decoupling: Multiple subscribers (e.g., logging, visualization, AI inference) can consume sensor streams without overloading the device.
- Lightweight: Small packet overhead and keep-alive support for sleeping devices.
- Flexible QoS: Tune reliability vs latency.

Security: Use TLS for transport security and strong authentication (username/password, client certificates) on public brokers.

---

## 3. Practical assignment — MQTT end-to-end (what you'll implement)

Goal: Implement a small system where an ESP32 publishes sensor data (e.g., temperature/humidity). A Python subscriber receives the data, runs the machine learning pipelines created in TP2 (Logistic Regression and XGBoost pipelines), and publishes the inference back to the ESP32. This mirrors a real-world pattern: edge device -> broker -> cloud/AI -> edge device.

Files and starting code:
- `TP4/src/main.cpp` — your ESP32 firmware (Wokwi/PlatformIO). It should publish sensor JSON to the topic `esp32/data` and subscribe to `esp32/control` to receive commands from the AI subscriber.
- `TP4/mqtt_client/mqtt_ai_subscriber.py` — a simple subscriber already included. It loads incoming JSON, prints it, and publishes `ON`/`OFF` control messages based on a simple rule. Your task will extend this to load the TP2 pipelines, run inference, and publish predictions instead.

Tasks (step-by-step)
1. ESP32 publisher
   - Ensure `main.cpp` publishes sensor data as JSON to `esp32/data` at a configurable interval (e.g., 1s or 5s).
   - Subscribe to `esp32/control` and toggle an LED according to the received command (e.g., `ON` = turn LED on, `OFF` = off).
   - Add a unique device ID in the JSON payload (e.g., `device_id`), plus sensor readings and a timestamp.

2. Python AI subscriber (complete `mqtt_ai_subscriber.py`)
   - Extend the provided script to:
     - Load pickled pipelines produced in TP2 (e.g., `lr_pipeline.pkl`, `xgb_pipeline.pkl`). These should be placed in a `models/` directory.
     - Parse incoming JSON messages from `esp32/data` and map them to the pipeline's expected feature vector. You may assume TP2 uses a fixed set of numeric features.
     - Run inference with both pipelines and publish a JSON message to `esp32/control` including: `device_id`, `model` ("lr" or "xgb"), `prediction` (class or probability), and `timestamp`.
     - Use QoS=1 for critical messages and handle transient errors (reconnect/backoff).
   - Add logging and a simple CLI flag to choose which pipeline to use (`--model lr` or `--model xgb`).

3. Testing and validation
   - Run the subscriber locally, publish test messages with the same JSON schema (you can use `mosquitto_pub` or another test client), and verify the ESP32 receives control messages and toggles the LED.
   - Measure end-to-end latency: time between ESP32 publish and ESP32 receiving the control message.

4. Robustness and security
   - Add basic validation: handle malformed JSON gracefully and ignore messages missing required fields.
   - For public brokers like https://testclient-cloud.mqtt.cool/, enable TLS and authentication. For classroom work, a local broker (Mosquitto) is fine.

---

## 4. Implementation notes and example code

Below are example snippets and guidance you can adapt.

Python subscriber (enhancement plan for `mqtt_ai_subscriber.py`):
- Load pipelines:
  - Use `pickle` to load `models/lr_pipeline.pkl` and `models/xgb_pipeline.pkl`.
- Parsing input:
  - Expect messages like:
    {
      "device_id": "esp-01",
      "temperature": 28.3,
      "humidity": 55.1,
      "timestamp": 1690000000
    }
- Output message format (published to `esp32/control`):
    {
      "device_id": "esp-01",
      "model": "lr",
      "prediction": 1,
      "probability": 0.87,
      "timestamp": 1690000001
    }

Important tips:
- Keep the message payloads small. Don't serialize entire pandas DataFrames — use flat JSON objects.
- If your TP2 pipelines expect scaled or transformed features, include the full pipeline (preprocessor + estimator) in the pickled object so incoming raw values can be passed directly to `.predict()`.

---

## 5. Alternative transport: HTTP/REST (exercise)

Replace the MQTT flow by a REST API. The ESP32 will POST sensor JSON to the inference server and receive a prediction in the HTTP response.

Server-side (Python, example with Flask or FastAPI):
- Endpoint: `POST /infer`
- Body: same JSON schema as MQTT input
- Response: inference JSON (same schema as MQTT output)

ESP32 side:
- Use `HTTPClient` (or `WiFiClientSecure` for TLS) to POST JSON to the server URL.
- Parse the JSON response and toggle the LED accordingly.

Why do this exercise?
- Demonstrates the different communication semantics: REST is request/response and often simpler to reason about, while MQTT is pub/sub and better for multiple subscribers and intermittent connectivity.

---

## 6. Deliverables and grading rubric

Students must submit a short report (PDF or markdown) and the following code files:
- `TP4/src/main.cpp` — ESP32 firmware implementing MQTT publish + subscribe (and alternative HTTP client implementation as optional).
- `TP4/ai_logic/mqtt_ai_subscriber.py` — extended Python subscriber that loads a TP2 pipeline and publishes inference results.
- `TP4/models/` — pickled pipelines used for inference (or instructions to point to models in TP2 folder).
- `TP4/report.md` — short write-up with measured model sizes, inference times (see TP2 tasks for measuring model footprint), and discussion about deployment feasibility on the ESP32.

Grading (suggested):
- Correctness and completeness (50%): ESP32 publishes correct JSON, subscriber performs inference and returns control messages.
- Robustness (20%): Error handling, reconnection, QoS settings.
- Documentation and report (20%): Clear instructions, measured results from TP2, deployment discussion.
- Extra (10%): TLS, authentication, HTTP/REST alternative implemented.

---

## 7. Next steps & hints

- Start by ensuring your TP2 pipelines are saved into `TP4/models/` using `pickle.dump(pipeline, open("models/lr_pipeline.pkl","wb"))`.
- Verify data shapes by running a small test script that loads a model and calls `.predict()` with a sample JSON mapped to a numpy array.
- Use `mosquitto` (local) broker or `https://testclient-cloud.mqtt.cool/` for development. For production, use a managed MQTT broker with TLS.


---

Good luck — implement the ESP32 publisher first, then extend `mqtt_ai_subscriber.py` to run the AI pipelines and publish predictions back to the device. Once MQTT is working, implement the HTTP/REST alternative.
