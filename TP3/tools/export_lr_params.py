"""TP3 step 1 - export the parameters of the TP2 Logistic Regression for the Arduino Uno.

Loads models/LR-Pipeline.pkl, the LR-Pipeline trained in TP2 (StandardScaler + LogisticRegression on the 12 sensor
features), and prints the four sets of values used in src/main.cpp:
  MEAN     StandardScaler mean, one value per feature
  STD      StandardScaler standard deviation, same order
  WEIGHTS  LogisticRegression coefficients, same order
  BIAS     LogisticRegression intercept

It then checks that the C formula, computed in float32 like on the Uno, gives the same decision as scikit-learn on the
TP1/TP2 test set, and prints the expected P(fire) for a few DHT22 readings. On the Uno only Temperature and Humidity
come from the DHT22; the 10 other features keep the values of the X array in main.cpp (first row of the dataset).

Run from the TP3 folder:  python tools/export_lr_params.py
"""
import os
import pickle

import numpy as np
import pandas as pd
from sklearn import metrics
from sklearn.model_selection import train_test_split

TP3 = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
pipe = pickle.load(open(os.path.join(TP3, "models", "LR-Pipeline.pkl"), "rb"))
scaler, lr = pipe.named_steps["scaler"], pipe.named_steps["model"]
features = list(pipe.feature_names_in_)

mean = scaler.mean_.astype(np.float32)
std = scaler.scale_.astype(np.float32)
w = lr.coef_[0].astype(np.float32)
b = np.float32(lr.intercept_[0])


def flit(x):
    # C float literal with enough digits to keep the float32 value
    s = f"{x:.9g}"
    return s if any(ch in s for ch in ".e") else s + ".0"


def c_array(name, v):
    return f"const float {name}[N_FEATURES] = {{{', '.join(flit(x) for x in v)}}};"


print("// ---- paste into src/main.cpp ----")
print(f"const int N_FEATURES = {len(features)};")
print("// Feature order: " + ", ".join(features))
print(c_array("MEAN", mean))
print(c_array("STD", std))
print(c_array("WEIGHTS", w))
print(f"const float BIAS = {flit(b)};\n")


def c_proba(Xv):
    # standardize() + predict() + sigmoid() of main.cpp, in float32
    z = (((np.asarray(Xv, dtype=np.float32) - mean) / std) * w).sum(axis=1, dtype=np.float32) + b
    with np.errstate(over="ignore"):
        return 1.0 / (1.0 + np.exp(-z))


# ---- check on the same test set as TP1 and TP2 ----
df = pd.read_csv(os.path.join(TP3, "smoke_detection_iot.csv"))
X = df.drop(columns=["Fire Alarm", "Unnamed: 0", "UTC", "CNT"])
X.columns = X.columns.str.replace("[", "_", regex=False).str.replace("]", "", regex=False)
y = df["Fire Alarm"]
_, X_test, _, y_test = train_test_split(X, y, test_size=0.20, stratify=y, random_state=42)
p_sk = pipe.predict_proba(X_test)[:, 1]
p_c = c_proba(X_test.values)
y_pred = (p_sk >= 0.5).astype(int)
print(f"TP2 LR-Pipeline on the test set ({len(X_test)} samples): Accuracy {metrics.accuracy_score(y_test, y_pred):.4f}"
      f" | F1 {metrics.f1_score(y_test, y_pred):.4f}")
agree = ((p_c >= 0.5) == (p_sk >= 0.5)).mean()
print(f"C formula vs scikit-learn: same decision {agree:.2%}, max |P diff| = {np.abs(p_c - p_sk).max():.1e}")
assert agree == 1.0

# ---- expected serial output in Wokwi ----
X_FIXED = [20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0]   # X in main.cpp


def dht_proba(t, h):
    return c_proba([[t, h] + X_FIXED[2:]])[0]


print("\nExpected P(fire) in Wokwi (DHT22 temperature / humidity, other 10 features = X in main.cpp):")
for t, h in [(24.0, 40.0), (42.6, 38.0), (60.0, 10.0), (70.0, 5.0), (80.0, 0.0)]:
    p = dht_proba(t, h)
    print(f"  T = {t:5.1f} C, H = {h:5.1f} %  ->  P(fire) = {p:.4f}  class {int(p >= 0.5)}")

# DHT22 range: -40..80 C, 0..100 %. Where does the model answer "normal"?
normal = [(t, h) for t in range(-40, 81) for h in range(0, 101) if dht_proba(t, h) < 0.5]
if normal:
    print(f"\nClass 0 for {len(normal)} of {121 * 101} integer readings, for example "
          f"T >= {min(t for t, _ in normal)} C with H <= {max(h for _, h in normal)} %")
