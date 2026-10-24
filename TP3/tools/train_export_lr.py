"""TP3 step 1 - train the Logistic Regression for the Arduino Uno and export its parameters.

The TP2 LR-Pipeline (models/LR-Pipeline.pkl) learned that heat means "normal": its temperature weight is negative.
TP1 (D.4) found why: sessions 0/1 and 3/4 of the dataset are twins (same values for every sensor and label, except the
temperature), and in sessions 0 and 4 the temperature is a smooth artificial curve, with fires at 3.9 C on average.

This script:
  1. finds the twin sessions and, in each pair, the one with the artificial temperature;
  2. retrains the same pipeline as TP2 (StandardScaler + LogisticRegression, 12 sensor features, same 80/20 split)
     without those sessions, and saves it to models/LR-Pipeline-TP3.pkl;
  3. prints the values for src/main.cpp:
       MEAN     StandardScaler mean, one value per feature
       STD      StandardScaler standard deviation, same order
       WEIGHTS  LogisticRegression coefficients, same order
       BIAS     LogisticRegression intercept
       X        the 10 sensors that the Uno does not measure: median of the normal training samples (clean air)
  4. checks the C formula (float32, like on the Uno) against scikit-learn, and prints what the DHT22 readings give.

Run from the TP3 folder:  python tools/train_export_lr.py
"""
import os
import pickle

import numpy as np
import pandas as pd
from sklearn import metrics
from sklearn.linear_model import LogisticRegression
from sklearn.model_selection import train_test_split
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler

TP3 = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RANDOM_STATE = 42

# ---- data: same features and same split as TP1 / TP2 ----
df = pd.read_csv(os.path.join(TP3, "smoke_detection_iot.csv"))
X = df.drop(columns=["Fire Alarm", "Unnamed: 0", "UTC", "CNT"])
X.columns = X.columns.str.replace("[", "_", regex=False).str.replace("]", "", regex=False)
y = df["Fire Alarm"]
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.20, stratify=y, random_state=RANDOM_STATE)

# ---- 1. twin sessions and artificial temperature (TP1, D.4) ----
session = (df["CNT"].diff() < 0).cumsum()   # a new recording session starts when CNT goes back down


def direction_changes(values):
    # Share of samples where the signal changes direction: high for a noisy real sensor, ~0 for a smooth curve
    steps = np.diff(values)
    steps = steps[steps != 0]
    return (np.diff(np.sign(steps)) != 0).mean()


others = X.columns.drop("Temperature_C")
artificial = []
ids = sorted(session.unique())
for i, a in enumerate(ids):
    for b in ids[i + 1:]:
        A, B = session == a, session == b
        if A.sum() == B.sum() and (X.loc[A, others].values == X.loc[B, others].values).all() \
                and (y[A].values == y[B].values).all():
            fake = min((a, b), key=lambda s: direction_changes(X.loc[session == s, "Temperature_C"].values))
            print(f"Sessions {a} and {b} are twins; session {fake} has the artificial temperature "
                  f"(direction changes {direction_changes(X.loc[session == fake, 'Temperature_C'].values):.3f})")
            artificial.append(fake)

real = ~session.isin(artificial)
train_real, test_real = real.loc[X_train.index], real.loc[X_test.index]

# ---- 2. retrain the TP2 pipeline without the artificial sessions ----
pipe = Pipeline([
    ("scaler", StandardScaler()),
    ("model", LogisticRegression(max_iter=1000, random_state=RANDOM_STATE)),
])
pipe.fit(X_train[train_real], y_train[train_real])
os.makedirs(os.path.join(TP3, "models"), exist_ok=True)
with open(os.path.join(TP3, "models", "LR-Pipeline-TP3.pkl"), "wb") as f:
    pickle.dump(pipe, f)
old = pickle.load(open(os.path.join(TP3, "models", "LR-Pipeline.pkl"), "rb"))   # TP2 model, for comparison

scaler, lr = pipe.named_steps["scaler"], pipe.named_steps["model"]
features = list(X.columns)
mean = scaler.mean_.astype(np.float32)
std = scaler.scale_.astype(np.float32)
w = lr.coef_[0].astype(np.float32)
b = np.float32(lr.intercept_[0])

# The Uno only measures Temperature and Humidity; the other sensors get a typical clean-air value
x_fixed = X_train[train_real & (y_train == 0)].median().astype(np.float32)


# ---- 3. values for src/main.cpp ----
def flit(x):
    # C float literal with enough digits to keep the float32 value
    s = f"{x:.9g}"
    return s if any(ch in s for ch in ".e") else s + ".0"


def c_array(decl, v):
    return f"{decl} = {{{', '.join(flit(x) for x in v)}}};"


print("\n// ---- paste into src/main.cpp ----")
print(f"const int N_FEATURES = {len(features)};")
print("// Feature order: " + ", ".join(features))
print(c_array("const float MEAN[N_FEATURES]", mean))
print(c_array("const float STD[N_FEATURES]", std))
print(c_array("const float WEIGHTS[N_FEATURES]", w))
print(f"const float BIAS = {flit(b)};")
print(c_array("float X[N_FEATURES]", x_fixed) + "\n")


# ---- 4. checks ----
def c_proba(Xv):
    # standardize() + predict() + sigmoid() of main.cpp, in float32
    z = (((np.asarray(Xv, dtype=np.float32) - mean) / std) * w).sum(axis=1, dtype=np.float32) + b
    with np.errstate(over="ignore"):
        return 1.0 / (1.0 + np.exp(-z))


agree = ((c_proba(X_test.values) >= 0.5) == (pipe.predict_proba(X_test)[:, 1] >= 0.5)).mean()
print(f"C formula vs scikit-learn on the {len(X_test)} test samples: same decision {agree:.2%}")
assert agree == 1.0

Xt, yt = X_test[test_real], y_test[test_real]
print(f"\nTest samples with a real temperature ({len(Xt)}; always answering fire gives accuracy {yt.mean():.4f}):")
for name, model in [("TP2 model (all sessions)", old), ("TP3 model (no artificial sessions)", pipe)]:
    p = model.predict(Xt)
    print(f"  {name:35s} Accuracy {metrics.accuracy_score(yt, p):.4f} | Precision {metrics.precision_score(yt, p):.4f}"
          f" | Recall {metrics.recall_score(yt, p):.4f} | F1 {metrics.f1_score(yt, p):.4f}"
          f" | temperature weight {model.named_steps['model'].coef_[0][0]:+.4f}")


def dht_proba(t, h):
    x = x_fixed.values.copy()
    x[0], x[1] = t, h
    return c_proba([x])[0]


print("\nExpected serial output in Wokwi:")
for t, h in [(24.0, 40.0), (24.0, 55.0), (35.0, 30.0), (45.0, 20.0), (60.0, 10.0), (80.0, 5.0)]:
    p = dht_proba(t, h)
    print(f"Temperature: {t:.2f} C | Humidity: {h:.2f} % | Predicted Probability: {p:.4f} | "
          f"Predicted Class: {int(p >= 0.5)} ({'FIRE' if p >= 0.5 else 'normal'})")

# DHT22 range: -40..80 C, 0..100 %
T, H = np.meshgrid(np.arange(-40, 81), np.arange(0, 101))
T, H = T.ravel(), H.ravel()
grid = np.repeat(x_fixed.values[None, :], len(T), axis=0)
grid[:, 0], grid[:, 1] = T, H
fire = c_proba(grid) >= 0.5
room = (T >= 18) & (T <= 28) & (H >= 30) & (H <= 60)
print(f"\nDHT22 range: FIRE for {fire.mean():.1%} of the readings | room (18-28 C, 30-60 %): {fire[room].mean():.1%}"
      f" | T >= 50 C: {fire[T >= 50].mean():.1%}")
for h in (10, 20, 30, 40, 50, 60):
    on = T[(H == h) & fire]
    print(f"  H = {h:2d} %: FIRE from T = {on.min()} C" if len(on) else f"  H = {h:2d} %: never FIRE")
