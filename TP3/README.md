# TP3 – Solution: Logistic Regression on the Arduino Uno

Statement: [TP3_Logistic_Arduino.md](TP3_Logistic_Arduino.md) · Setup: [install.md](install.md)

## Files
| File | Role |
|---|---|
| `models/LR-Pipeline.pkl` | The LR-Pipeline trained in TP2 (StandardScaler + LogisticRegression on the 12 sensor features) |
| `tools/export_lr_params.py` | **Step 1** – loads that model, prints μ, σ, W and b, and checks the C formula against scikit-learn on the TP1/TP2 test set |
| `src/main.cpp` | **Steps 2–4** – the parameters as constants, `standardize()`, `sigmoid()`, `predict()`, and the real-time loop on the DHT22 |
| `diagram.json`, `wokwi.toml`, `platformio.ini` | Wokwi circuit (Uno + DHT22 on pin 2) and build settings, as provided |
| `smoke_detection_iot.csv` | Dataset of TP1, used by the export script for the check |

## Model (TP2 LR-Pipeline, test set of 12,526 samples)
Accuracy **0.8951** · F1 **0.9281**. The C formula gives the same decision as scikit-learn on all 12,526 test samples.

| Feature | μ (mean) | σ (std) | W (weight) |
|---|---|---|---|
| Temperature_C | 15.9531 | 14.3580 | −1.1063 |
| Humidity_% | 48.5743 | 8.8212 | 1.3782 |
| TVOC_ppb | 1922.8844 | 7755.8950 | −18.2296 |
| eCO2_ppm | 667.7439 | 1903.7822 | 5.9192 |
| Raw H2 | 12942.3008 | 271.1686 | 3.3515 |
| Raw Ethanol | 19754.2500 | 606.9760 | −7.8964 |
| Pressure_hPa | 938.6320 | 1.3245 | −2.4279 |
| PM1.0 | 100.8873 | 925.3035 | −0.3308 |
| PM2.5 | 186.0739 | 1991.5278 | −0.1946 |
| NC0.5 | 490.2093 | 4259.4155 | −0.4543 |
| NC1.0 | 205.4578 | 2232.4849 | −0.1843 |
| NC2.5 | 81.3198 | 1095.2900 | −0.0682 |

Bias b = **−0.3844**.

On the Uno only Temperature and Humidity come from the DHT22. The 10 other features keep the values of the `X` array of the provided `main.cpp` (first row of the dataset).
With these values the model answers *normal* only for hot and dry air: about T ≥ 43 °C with H ≤ 18 %, for example 70 °C / 5 %.

## Run
1. Open the `TP3` folder in VS Code → PlatformIO **Build**.
2. `F1` → **Wokwi: Start Simulator**, then click the DHT22 to move the temperature and humidity sliders.

Expected serial output (same values as Python):
```
Temperature: 24.00 C | Humidity: 40.00 % | Predicted Probability: 0.9995 | Predicted Class: 1 (FIRE)
Temperature: 42.60 C | Humidity: 38.00 % | Predicted Probability: 0.9973 | Predicted Class: 1 (FIRE)
Temperature: 60.00 C | Humidity: 10.00 % | Predicted Probability: 0.5452 | Predicted Class: 1 (FIRE)
Temperature: 70.00 C | Humidity: 5.00 % | Predicted Probability: 0.2026 | Predicted Class: 0 (normal)
Temperature: 80.00 C | Humidity: 0.00 % | Predicted Probability: 0.0511 | Predicted Class: 0 (normal)
```
Firmware size: 6,362 bytes of Flash (19.7%) and 411 bytes of RAM (20.1%).
