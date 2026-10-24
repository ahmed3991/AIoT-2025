# TP3 – Solution: Logistic Regression on the Arduino Uno

Statement: [TP3_Logistic_Arduino.md](TP3_Logistic_Arduino.md) · Setup: [install.md](install.md)

## Files
| File | Role |
|---|---|
| `models/LR-Pipeline-TP3.pkl` | The model that runs on the Uno: the TP2 LR-Pipeline (StandardScaler + LogisticRegression on the 12 sensor features) retrained without the two sessions whose temperature is artificial |
| `models/LR-Pipeline.pkl` | The TP2 LR-Pipeline, unchanged, kept for comparison |
| `tools/train_export_lr.py` | **Step 1** – finds the artificial sessions, retrains the pipeline, prints μ, σ, W, b and the `X` values, checks the C formula against scikit-learn, and prints the expected Wokwi output |
| `src/main.cpp` | **Steps 2–4** – the parameters as constants, `standardize()`, `sigmoid()`, `predict()`, the real-time loop on the DHT22, and the alarm lights and buzzer |
| `diagram.json`, `wokwi.toml` | Wokwi circuit: Uno, DHT22 on pin 2, green LED on pin 4, red LED on pin 5, buzzer on pin 8 |
| `platformio.ini` | Two builds of the same `main.cpp`: `uno` for Wokwi, `uno_proteus` for Proteus |
| `proteus/libraries/DHT_proteus` | DHT sensor library 1.4.7 with a 20 ms start signal, used only by the `uno_proteus` build (see [Proteus](#proteus)) |
| `smoke_detection_iot.csv` | Dataset of TP1 |

## Why the TP2 model was retrained
With the TP2 model and the `X` values of the template, the Uno answered **FIRE for 96.9 % of the DHT22 range** (−40…80 °C, 0…100 %), and for **every reading of a normal room** (18–28 °C, 30–60 %).
Only hot and dry air (T ≥ 43 °C with H ≤ 18 %) gave *normal*. There were two causes.

**1. The model had learned that heat means *normal*.** Its temperature weight was **−1.1063**. The data audit of TP1 (sections D.1 and D.4 of `TP1/Fire_Alarm_Detection.ipynb`, branch `tp1-solution`) explains why:
- sessions 0 and 1 of the dataset are twins, and so are sessions 3 and 4: every sensor and every label is the same, only the temperature differs;
- in sessions 0 and 4 the temperature is a smooth artificial curve that goes down to −22 °C. In session 0 it averages **3.9 °C during fires** and 21.9 °C in normal conditions;
- in the sessions with a real temperature, fires are hotter: 24.1 vs 16.5 °C in session 1, and 34.6 vs 21.7 °C in session 2.

→ The same pipeline (same features, settings and 80/20 split) is **retrained without sessions 0 and 4**. Its temperature weight becomes **+2.8097**: heat now pushes towards fire.

**2. The fixed values of the 10 other sensors were a warm-up reading.** The `X` array of the template is the first row of the dataset, recorded while the gas sensors were warming up (Raw H2 goes 12306 → 12419 and Raw Ethanol 18520 → 18998 in the first 6 seconds).
This row is labelled *normal*, but both models give it P(fire) = 1.0000, whatever the temperature.

→ The 10 sensors that the Uno does not have take a **clean-air value: the median of the normal training samples**.

## Model (test samples with a real temperature: 6,333)
| Model | Accuracy | Precision | Recall | F1 | Temperature weight |
|---|---|---|---|---|---|
| TP2 model (all sessions) | 0.8550 | 0.9000 | 0.8980 | 0.8990 | −1.1063 |
| **TP3 model (no artificial sessions)** | **0.9346** | **0.9287** | **0.9846** | **0.9558** | **+2.8097** |

Always answering *fire* gives an accuracy of 0.7186 on these samples. The C formula gives the same decision as scikit-learn on all 12,526 test samples.

| Feature | μ (mean) | σ (std) | W (weight) | Fixed value on the Uno (`X`) |
|---|---|---|---|---|
| Temperature_C | 25.4014 | 7.1445 | 2.8097 | DHT22 |
| Humidity_% | 48.1189 | 9.5307 | 3.1595 | DHT22 |
| TVOC_ppb | 1903.8857 | 7702.4521 | −25.2314 | 86 |
| eCO2_ppm | 733.6653 | 2172.8215 | 5.3172 | 400 |
| Raw H2 | 12937.0908 | 278.8783 | 1.9517 | 12839 |
| Raw Ethanol | 19759.1543 | 608.9777 | −5.6790 | 20157 |
| Pressure_hPa | 938.4949 | 1.6563 | −2.0735 | 937.479 |
| PM1.0 | 122.7869 | 1036.8063 | 0.5625 | 1.45 |
| PM2.5 | 235.3737 | 2324.0818 | 0.1122 | 1.52 |
| NC0.5 | 574.2354 | 4574.7422 | 1.0665 | 9.95 |
| NC1.0 | 260.7111 | 2611.9739 | 0.0816 | 1.575 |
| NC2.5 | 107.6195 | 1312.5526 | −0.2444 | 0.039 |

Bias b = **−0.4278**.

## Behaviour on the Uno
| | TP2 model + template `X` | TP3 model + clean-air `X` |
|---|---|---|
| FIRE over the whole DHT22 range | 96.9 % | 49.8 % |
| FIRE in a normal room (18–28 °C, 30–60 %) | 100 % | 44.3 % |
| FIRE at T ≥ 50 °C | 88.4 % | 96.6 % |

With the TP3 model the alarm starts at **55 °C** for 10 % humidity, 46 °C for 20 %, 38 °C for 30 %, 29 °C for 40 %, 21 °C for 50 % and 12 °C for 60 %.

**What is still wrong.** The humidity weight is large (+3.1595), so humid air also pushes towards fire: a room at 24 °C and 55 % gives FIRE.
In this data the large fire sessions were recorded in humid air (about 51 %) and the normal session in drier air (about 38 %), so the model links humidity with fire.
TP1 (D.5) also shows that no model trained on this data works on a recording it has never seen.
This firmware demonstrates the deployment chain (Python → C → Uno) with a model that reacts to heat in the right direction, not a reliable fire detector.

## Lights and sound
| Output | Pin | Behaviour |
|---|---|---|
| Green LED (220 Ω) | D4 | On while the model predicts *normal* |
| Red LED (220 Ω) | D5 | Blinks (250 ms on / 250 ms off) while the model predicts *fire* |
| Buzzer | D8 | Beeps at 2 kHz with the red LED while the model predicts *fire* |

The red LED and the buzzer run during the 2 s wait between two readings (`waitWithAlarm()`), and the buzzer is silent while the DHT22 is read, because the sensor exchange is timing-sensitive.

## Run in Wokwi
1. Open the `TP3` folder in VS Code → PlatformIO **Build** (environment `uno`).
2. `F1` → **Wokwi: Start Simulator**, then click the DHT22 to move the temperature and humidity sliders.

Expected serial output (same values as Python):
```
Temperature: 24.00 C | Humidity: 40.00 % | Predicted Probability: 0.1355 | Predicted Class: 0 (normal)
Temperature: 24.00 C | Humidity: 55.00 % | Predicted Probability: 0.9577 | Predicted Class: 1 (FIRE)
Temperature: 35.00 C | Humidity: 30.00 % | Predicted Probability: 0.3010 | Predicted Class: 0 (normal)
Temperature: 45.00 C | Humidity: 20.00 % | Predicted Probability: 0.4440 | Predicted Class: 0 (normal)
Temperature: 60.00 C | Humidity: 10.00 % | Predicted Probability: 0.9137 | Predicted Class: 1 (FIRE)
Temperature: 80.00 C | Humidity: 5.00 % | Predicted Probability: 0.9998 | Predicted Class: 1 (FIRE)
```
Firmware size: 7,232 bytes of Flash (22.4%) and 429 bytes of RAM (20.9%).

## Proteus
The same `main.cpp` also runs in Proteus (tested with Proteus 9.1 and the Virtual Terminal: the probabilities match Python).
Its DHT22 model does not answer the 1.1 ms start signal of DHT sensor library 1.4.7 ("DHT timeout waiting for start signal high pulse"), but it answers a 20 ms one, the value older versions of the library used.
So the `uno_proteus` build uses `proteus/libraries/DHT_proteus`, a copy of 1.4.7 where only that line changes. Build it with PlatformIO (`pio run -e uno_proteus`) and load `.pio/build/uno_proteus/firmware.hex`.

| Part (Proteus name) | Connection |
|---|---|
| ATMEGA328P | Program File = `.pio/build/uno_proteus/firmware.hex`, CLKDIV8 = Unprogrammed, CKSEL = Ext. Crystal, Clock Frequency = 16MHz |
| DHT22 | DATA → PD2 (D2), VDD → POWER, GND → GROUND |
| VIRTUAL TERMINAL | RXD → PD1/TXD (D1), 9600 baud, 8 data bits, no parity, 1 stop bit |
| LED-GREEN + RES 220 Ω | PD4 (D4) → resistor → LED anode, LED cathode → GROUND |
| LED-RED + RES 220 Ω | PD5 (D5) → resistor → LED anode, LED cathode → GROUND |
| SOUNDER | PB0 (D8) → sounder → GROUND |
