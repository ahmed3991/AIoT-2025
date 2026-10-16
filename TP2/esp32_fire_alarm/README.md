# ESP32 Fire Alarm – ML model on the device

Runs a fire-alarm model of Practical 2 on an ESP32, simulated with **Wokwi** and built with **PlatformIO**.
Two models are exported; `#define USE_XGB` at the top of `src/main.cpp` selects the one that drives the alarm:

| `USE_XGB` | Model | On the device | Test set (12,526 samples) | Unseen recording sessions |
|---|---|---|---|---|
| `1` (default) | XGBoost, 20 trees of depth 3 | 3,476 bytes | 6 fires missed, 3 false alarms | Recall 0.86 |
| `0` | Logistic Regression | 148 bytes | 464 fires missed, 850 false alarms | Recall 0.49 |

## What it does
- `include/xgb_model.h`: the 286 tree nodes of the compressed XGB-Pipeline (scaler folded into the split thresholds), plus `xgb_predict_proba()`.
- `include/lr_model.h`: scaler mean/std, the 12 weights and the bias as C `float` arrays (148 bytes in total), plus `lr_predict_proba()`.
- `include/test_samples.h`: 20 real samples from the 20% test set (10 fire, 10 normal), with their true label and the probability computed by scikit-learn for both models.
- `src/main.cpp`:
  - at start-up: CPU frequency, free heap, model size, and mean inference time over 20,000 calls;
  - then every second (like the real 1 Hz sensor): one prediction → **red LED + buzzer** for fire, **green LED** for normal;
  - the serial monitor compares each ESP32 decision with the true label and with Python, and prints a summary after each round.

## Run it in Wokwi (VS Code)
1. Open **this folder** (`esp32_fire_alarm`) in VS Code.
2. Build: PlatformIO sidebar → *esp32dev → Build* (or `pio run`).
3. `F1` → **Wokwi: Start Simulator**. The first time, Wokwi asks for its free licence (`F1` → *Wokwi: Request a New License*).
4. The circuit (`diagram.json`) and the serial monitor open; LEDs and buzzer react to each sample.

## Real ESP32 board
Same code: connect the board over USB, then PlatformIO → *Upload* and *Monitor* (115200 baud).
Wiring: GPIO25 → 220 Ω → red LED → GND, GPIO26 → 220 Ω → green LED → GND, GPIO27 → buzzer (+), buzzer (−) → GND.

## Re-export the models
After retraining `../models/LR-Pipeline.pkl`:
```
python tools/export_model.py
```
The script also retrains the compressed XGBoost (saved to `../models/XGB-20x3-Pipeline.pkl`). It checks that the float32 computation of the ESP32 gives the same decision as scikit-learn on all 12,526 test samples, for both models, before writing the headers.
