# create_model.py
import numpy as np
from sklearn.linear_model import LogisticRegression
from sklearn.preprocessing import StandardScaler
import pickle

print("=== انشاء نموذج تجريبي للانحدار اللوجستي ===")

X_train = np.array([
    [22, 35], [25, 40], [28, 50], [30, 60], [18, 30],
    [20, 45], [26, 55], [32, 65], [24, 38], [27, 48],
    [23, 42], [29, 58], [31, 62], [19, 32], [21, 37]
])
y_train = np.array([0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0])

print("بيانات التدريب:")
print("درجة الحرارة | الرطوبة | الصنف")
for i in range(len(X_train)):
    print(f"  {X_train[i][0]}°C      | {X_train[i][1]}%    | {y_train[i]}")

scaler = StandardScaler()
X_scaled = scaler.fit_transform(X_train)


model = LogisticRegression()
model.fit(X_scaled, y_train)


with open('logistic_model.pkl', 'wb') as f:
    pickle.dump(model, f)

with open('scaler.pkl', 'wb') as f:
    pickle.dump(scaler, f)

print("\n=== تم انشاء وحفظ النموذج بنجاح! ===")
print("\nمعاملات النموذج:")
print(f"  - MEAN (المتوسط): {scaler.mean_}")
print(f"  - STD (الانحراف المعياري): {scaler.scale_}")
print(f"  - WEIGHTS (الاوزان): {model.coef_[0]}")
print(f"  - BIAS (التحيز): {model.intercept_[0]}")

# اختبار التنبؤ
test_temp = 28
test_hum = 52
test_data = np.array([[test_temp, test_hum]])
test_scaled = scaler.transform(test_data)
prediction = model.predict_proba(test_scaled)[0][1]

print(f"\nاختبار التنبؤ: Temp={test_temp}°C, Hum={test_hum}%")
print(f"  احتمالية Class 1: {prediction:.4f}")
print(f"  التصنيف: {'Class 1' if prediction >= 0.5 else 'Class 0'}")