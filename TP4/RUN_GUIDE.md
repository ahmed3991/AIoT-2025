# 🚀 How to Run the MQTT AI Subscriber Project

## 1️⃣ Activate the Virtual Environment

Before running anything, open PowerShell in the project folder and activate your venv:

```powershell
& E:/AIot/AIoT-2025/TP4/.venv/Scripts/Activate.ps1
```

(If you see `(.venv)` on the left of your prompt, it’s active ✅)

---

## 2️⃣ Run the AI Subscriber

Start the subscriber (choose model: `lr` or `xgb`):

```powershell
python ai_logic/mqtt_ai_subscriber.py --model lr
```

You should see logs like:

```
[INFO] Loaded model: models/lr_pipeline.pkl
[INFO] Connected to MQTT broker.
```

---

## 3️⃣ Publish a Test Message

Use the provided test publisher to simulate ESP32 data:

```powershell
python ai_logic/test_publish.py
```

You should see:

```
Message published to esp32/data
```

and in the subscriber window:

```
Received data: {...}
Publishing control message: {...}
```

---

## 4️⃣ (Optional) Using XGBoost Model

To test the XGBoost pipeline instead of Logistic Regression:

```powershell
python ai_logic/mqtt_ai_subscriber.py --model xgb
```

---

## 5️⃣ Stop the Program

Press **Ctrl + C** in PowerShell to stop the subscriber safely.

---

## 6️⃣ Notes

* Ensure `models/` folder contains:

  ```
  lr_pipeline.pkl
  xgb_pipeline.pkl
  ```
* Ensure you’re connected to the internet (for `broker.mqtt.cool`).
* If you get an error like “InconsistentVersionWarning,” it’s harmless — it just means scikit-learn version changed.

---

