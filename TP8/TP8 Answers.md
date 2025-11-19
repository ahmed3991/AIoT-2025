# TP8: Offloading AI Inference via MQTT – Theoretical Answers

## 1. System Architecture Overview
The system implements distributed inference where the ESP32 captures image data and transmits it over MQTT to a Python-based AI inference module running on a host machine.  
The Python module processes the data using a TensorFlow Lite model and sends the prediction back to the ESP32.

### Components:
- **ESP32 (Publisher/Subscriber)**  
  - Captures an image (mocked as a 28×28 array).
  - Publishes image data to the MQTT topic `esp32/data`.
  - Subscribes to `esp32/control` to receive prediction results.
  - Displays the predicted class on the LCD.

- **Python AI Subscriber (Subscriber/Publisher)**  
  - Subscribes to `esp32/data`.
  - Loads and runs a quantized TensorFlow Lite model.
  - Publishes the predicted class name to `esp32/control`.

- **MQTT Broker**  
  - Routes messages between ESP32 and Python.

---

## 2. Advantages of Offloading Inference to a Host Machine
1. **Higher computational power:**  
   The PC can run heavier models that cannot fit in the ESP32’s constrained RAM and flash.

2. **Ease of model updates:**  
   Updating the TFLite model on the host requires no firmware flashing.

3. **Reduced memory usage on ESP32:**  
   The ESP32 only prepares and sends raw image data.

4. **Scalability:**  
   Multiple ESP32 boards can offload inference to the same host.

5. **Improved accuracy and flexibility:**  
   The host machine can run more complex models (e.g., CNNs, transformers).

---

## 3. MQTT Topics Used in TP8
Two MQTT topics are used:

### **`esp32/data`**
- ESP32 publishes image data to this topic.
- Payload: JSON object `{ "encoded_image": [...] }`.

### **`esp32/control`**
- Python AI module publishes the predicted class name.
- ESP32 receives it via MQTT callback.

This publish/subscribe separation ensures asynchronous, low‑latency communication.

---

## 4. JSON Payload Format
The ESP32 sends the image as:

```json
{
  "encoded_image": [v1, v2, v3, ..., v784]
}
```

Where:
- Each value is an **int8 pixel** from the 28×28 image.
- Total size = 784 values (28×28).

This simple format allows easy parsing on Python using `json.loads`.

---

## 5. TensorFlow Lite Model Input/Output Details

### **Input tensor:**
- Shape: `(1, 28, 28, 1)`
- Data type: **int8**
- Quantization:  
  - scale = 0.0039215689  
  - zero‑point = −128

### **Output tensor:**
- Shape: `(1, 10)`
- Data type: **int8**
- Represents class scores for the Fashion‑MNIST dataset.

### **Class selection:**
The predicted class index is obtained by:

```python
predicted_class = np.argmax(output[0])
```

---

## 6. How Inference is Performed in Python
### Steps:
1. Parse MQTT message → read JSON.
2. Convert to NumPy array.
3. Reshape to `(1, 28, 28, 1)`.
4. Load TFLite model with `tf.lite.Interpreter`.
5. Set input using `set_tensor`.
6. Invoke inference using `invoke`.
7. Read output using `get_tensor`.
8. Publish class name back to ESP32.

This pipeline simulates real-world AI edge offloading where the model runs remotely.

---

## 7. Why MQTT is Suitable for AI Offloading
1. **Lightweight communication protocol** ideal for IoT.
2. **Publish/subscribe architecture** simplifies multi-device setups.
3. **Low latency** for real-time inference tasks.
4. **Broker-based routing** avoids direct connections between devices.
5. **Scalable** to many ESP32 boards sending data simultaneously.

---

## 8. Comparisons: On‑Device vs Offloaded Inference
| Feature | ESP32 On‑Device | Python Host Offloaded |
|--------|------------------|------------------------|
| Model size | Very limited | Large/complex allowed |
| Latency | Deterministic, local | Depends on network |
| Memory usage | High | Very low |
| Flexibility | Requires reflashing | Update model instantly |
| Power usage | Higher | Lower on ESP32 |

---

## 9. Conclusion
TP8 demonstrates a real-world architecture where a microcontroller offloads heavy AI computations to a cloud/PC system using lightweight MQTT messaging.  
This technique is widely used in modern IoT systems, robotics, and smart sensors where embedded devices cannot perform complex inference locally.

---

## 10. References
- Eclipse Mosquitto MQTT Documentation  
- TensorFlow Lite Model Optimization Guide  
- Fashion-MNIST Dataset Specification
