# === create_models.py ===
import os
import pickle
import numpy as np
from sklearn.linear_model import LogisticRegression
from xgboost import XGBClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score

# إنشاء مجلد models إذا لم يكن موجودًا
os.makedirs("models", exist_ok=True)

# توليد بيانات تجريبية (حرارة ورطوبة)
# الهدف هنا تصنيف حالة الطقس مثلاً (0 = عادي، 1 = خطر)
np.random.seed(42)
temp = np.random.uniform(15, 40, 200)   # حرارة بين 15 و40
humid = np.random.uniform(20, 90, 200)  # رطوبة بين 20 و90
labels = (temp > 30) & (humid > 60)     # إذا الحرارة >30 والرطوبة >60 = خطر
labels = labels.astype(int)

X = np.column_stack((temp, humid))
y = labels

# تقسيم البيانات
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# --- نموذج Logistic Regression ---
model_lr = LogisticRegression()
model_lr.fit(X_train, y_train)
y_pred_lr = model_lr.predict(X_test)
print("Logistic Regression Accuracy:", accuracy_score(y_test, y_pred_lr))

# حفظ النموذج
with open("models/model_lr.pkl", "wb") as f:
    pickle.dump(model_lr, f)

# --- نموذج XGBoost ---
model_xgb = XGBClassifier(use_label_encoder=False, eval_metric='logloss')
model_xgb.fit(X_train, y_train)
y_pred_xgb = model_xgb.predict(X_test)
print("XGBoost Accuracy:", accuracy_score(y_test, y_pred_xgb))

# حفظ النموذج
with open("models/model_xgb.pkl", "wb") as f:
    pickle.dump(model_xgb, f)

print("\n✅ تم إنشاء النموذجين model_lr.pkl و model_xgb.pkl داخل مجلد models")
