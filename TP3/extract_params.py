# extract_params.py
import joblib
import numpy as np

# حمّلي الكائنات المدربة
scaler = joblib.load("scaler.pkl")
clf = joblib.load("model.pkl")

# استخرج المتوسط والانحراف المعياري (ترتيب الميزات: [Temperature, Humidity])
means = scaler.mean_
stds  = scaler.scale_

# استخرج الأوزان والباياس
weights = clf.coef_.ravel()
bias = float(clf.intercept_[0])

# طباعة بصيغة جاهزة للصق في C++ (main.cpp)
def fmt_list_c(values):
    return ", ".join(f"{float(v):.6f}f" for v in values)

print("/* Copy these constants into TP3/src/main.cpp */")
print(f"const float MEAN[2]    = {{ {fmt_list_c(means)} }};")
print(f"const float STD[2]     = {{ {fmt_list_c(stds)} }};")
print(f"const float WEIGHTS[2] = {{ {fmt_list_c(weights)} }};")
print(f"const float BIAS = {float(bias):.6f}f;")

# طباعة أيضاً بصيغة بشرية للرجوع إليها
print()
print("Human-readable:")
print("MEAN =", list(map(float, means)))
print("STD  =", list(map(float, stds)))
print("WEIGHTS =", list(map(float, weights)))
print("BIAS =", bias)
