# task4_resources.py
import os, json
import numpy as np
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers

def human_mb(bytes_):
    return bytes_ / (1024 * 1024)

def count_params(model):
    return int(np.sum([np.prod(v.shape) for v in model.trainable_variables]))

def dense_flops(m, n, include_bias=True):
    # 2 FLOPs لكل (multiply+add) + (اختياري) 1 FLOP لتحيز كل خرج
    mac = m * n * 2
    bias = n if include_bias else 0
    return mac + bias

def conv2d_flops(H, W, Cin, Cout, Kh, Kw, include_bias=True):
    # لكل عنصر خرج: Kh*Kw*Cin ضرب + نفس العدد جمع => 2*Kh*Kw*Cin
    per_out = 2 * Kh * Kw * Cin
    total = H * W * Cout * per_out
    if include_bias:
        total += H * W * Cout  # جمع التحيز
    return total

# ===== 1) أبني نفس النموذجين كما في الخطوات السابقة =====
mlp_model = keras.Sequential(
    [
        layers.Flatten(input_shape=(28, 28)),
        layers.Dense(256, activation="relu"),
        layers.Dense(128, activation="relu"),
        layers.Dense(10, activation="softmax"),
    ],
    name="mlp_model",
)
mlp_model.compile(optimizer="adam", loss="sparse_categorical_crossentropy", metrics=["accuracy"])

cnn_model = keras.Sequential(
    [
        layers.Conv2D(16, 3, activation="relu", input_shape=(28, 28, 1)),
        layers.MaxPooling2D(2),
        layers.Conv2D(32, 3, activation="relu"),
        layers.MaxPooling2D(2),
        layers.Flatten(),
        layers.Dense(64, activation="relu"),
        layers.Dense(10, activation="softmax"),
    ],
    name="cnn_model",
)
cnn_model.compile(optimizer="adam", loss="sparse_categorical_crossentropy", metrics=["accuracy"])

# ===== 2) Task 4.1: عدد البارامترات =====
P_mlp = count_params(mlp_model)     # متوقع ≈ 235,146
P_cnn = count_params(cnn_model)     # متوقع ≈ 56,714

# ===== 3) Task 4.2: حفظ النماذج وقياس الحجم على القرص =====
mlp_path = "mlp_model.h5"
cnn_path = "cnn_model.h5"
mlp_model.save(mlp_path)
cnn_model.save(cnn_path)

size_mlp_bytes = os.path.getsize(mlp_path)
size_cnn_bytes = os.path.getsize(cnn_path)

# ===== 4) Task 4.3: تقدير FLOPs والذاكرة =====
# ميثاق الحساب:
# - FLOPs الاستدلال (Forward): نحسبه دقيقًا للطبقات Dense/Conv كما بالأسفل.
# - FLOPs التدريب: تقدير تقريبي = 4 × FLOPs الاستدلال (Forward)
#   (Forward + Backward + تحديث Adam) — تقدير شائع مع Adam.
# - ذاكرة التدريب (Parameters + Gradients + Adam states) فقط (بدون تفعيلات):
#   لكل بارامتر float32: 4 بايت
#   مجموع تقريبي = P * (Weights + Grads + m + v) * 4B = P * 16 بايت.

# --- MLP FLOPs ---
mlp_infer_flops = (
    dense_flops(28*28, 256) +
    dense_flops(256, 128) +
    dense_flops(128, 10)
)
mlp_train_flops = mlp_infer_flops * 4

# --- CNN FLOPs ---
# أشكال الخرائط كما في الملخص:
# Conv1: input 28x28x1 -> output 26x26x16 (فلتر 3x3)
# MaxPool -> 13x13x16
# Conv2: 13x13x16 مع kernel 3x3 padding='valid' يعطينا 11x11x32
# MaxPool -> 5x5x32
# Dense 800->64 ثم 64->10
cnn_infer_flops = (
    conv2d_flops(26, 26, 1, 16, 3, 3) +
    conv2d_flops(11, 11, 16, 32, 3, 3) +
    dense_flops(800, 64) +
    dense_flops(64, 10)
)
cnn_train_flops = cnn_infer_flops * 4

# --- ذاكرة التدريب (بدون التفعيلات) ---
mlp_train_mem_bytes = P_mlp * 16
cnn_train_mem_bytes = P_cnn * 16

report = {
    "MLP": {
        "trainable_params": P_mlp,
        "saved_model_size_MB": round(human_mb(size_mlp_bytes), 3),
        "FLOPs_inference_per_image": int(mlp_infer_flops),
        "FLOPs_training_per_image_step_est": int(mlp_train_flops),
        "training_memory_MB_params_grads_adam": round(human_mb(mlp_train_mem_bytes), 3),
        "paths": {"model": mlp_path},
    },
    "CNN": {
        "trainable_params": P_cnn,
        "saved_model_size_MB": round(human_mb(size_cnn_bytes), 3),
        "FLOPs_inference_per_image": int(cnn_infer_flops),
        "FLOPs_training_per_image_step_est": int(cnn_train_flops),
        "training_memory_MB_params_grads_adam": round(human_mb(cnn_train_mem_bytes), 3),
        "paths": {"model": cnn_path},
    },
}

print(json.dumps(report, indent=2))
print("\nNote: Training FLOPs are estimates (≈4× inference) and include the approximate cost of the Adam update.")

