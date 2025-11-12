# train_model.py
import numpy as np
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LogisticRegression
import joblib

# ثابت للتكرار
RANDOM_SEED = 0
np.random.seed(RANDOM_SEED)

# -------------------------
# توليد بيانات تركيبية بسيطة
# ميزتان: Temperature (°C) و Humidity (%)
n_samples = 200
temp = np.random.normal(loc=25, scale=3, size=n_samples)   # متوسط ~25°C
hum  = np.random.normal(loc=50, scale=10, size=n_samples)  # متوسط ~50%

X = np.vstack([temp, hum]).T

# نبني وسمة هدف ثنائية (قانون خطي + دالة لوجستية ثم عتبة)
logit = 0.6*(temp - 26) + 0.04*(hum - 55)
prob = 1.0 / (1.0 + np.exp(-logit))
y = (prob > 0.5).astype(int)

# نضيف بعض الضوضاء (flip) لواقعية بسيطة
flip_idx = np.random.choice(n_samples, size=int(0.05 * n_samples), replace=False)
y[flip_idx] = 1 - y[flip_idx]

# -------------------------
# تدريب StandardScaler و LogisticRegression
scaler = StandardScaler().fit(X)
X_scaled = scaler.transform(X)

clf = LogisticRegression().fit(X_scaled, y)

# حفظ الكائنات
joblib.dump(scaler, "scaler.pkl")
joblib.dump(clf, "model.pkl")

print("Saved scaler.pkl and model.pkl")
print("Train score (on scaled training set):", clf.score(X_scaled, y))
