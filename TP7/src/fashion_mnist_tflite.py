# make_fashion_mnist_tflite.py
# يولّد: src/fashion_mnist_cnn_int8.tflite (Full-Integer INT8)

import os, numpy as np, tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers

# ===== إعداد مسارات الحفظ =====
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR  = os.path.join(BASE_DIR, "src")
os.makedirs(SRC_DIR, exist_ok=True)
OUT_PATH = os.path.join(SRC_DIR, "fashion_mnist_cnn_int8.tflite")

# ===== 1) تحميل Fashion-MNIST وتحضير البيانات =====
(x_train, y_train), (x_test, y_test) = keras.datasets.fashion_mnist.load_data()
x_train = (x_train.astype("float32") / 255.0)[..., None]  # (N, 28, 28, 1)
x_test  = (x_test.astype("float32")  / 255.0)[..., None]

# ===== 2) نموذج CNN بسيط (سريع التدريب) =====
def build_model():
    m = keras.Sequential([
        layers.Input(shape=(28, 28, 1)),
        layers.Conv2D(16, 3, activation="relu"),
        layers.MaxPooling2D(),
        layers.Conv2D(32, 3, activation="relu"),
        layers.MaxPooling2D(),
        layers.Flatten(),
        layers.Dense(64, activation="relu"),
        layers.Dense(10, activation="softmax"),
    ])
    m.compile(optimizer="adam",
              loss="sparse_categorical_crossentropy",
              metrics=["accuracy"])
    return m

model = build_model()
model.fit(x_train, y_train, epochs=2, batch_size=128, validation_split=0.1, verbose=2)
test_acc = model.evaluate(x_test, y_test, verbose=0)[1]
print(f"[INFO] Test accuracy (float32): {test_acc:.4f}")

# ===== 3) ممثل بيانات للتكمية (نفس الpreprocessing تمامًا) =====
def representative_data_gen(num_samples=300):
    # يكفي بضع مئات من العينات للمعايرة
    for i in range(num_samples):
        yield [x_train[i:i+1].astype(np.float32)]

# ===== 4) التحويل إلى TFLite مع تكمية INT8 كاملة =====
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_data_gen
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type  = tf.int8
converter.inference_output_type = tf.int8

tflite_bytes = converter.convert()
with open(OUT_PATH, "wb") as f:
    f.write(tflite_bytes)

# ===== 5) طباعة معلومات مفيدة =====
size_mb = os.path.getsize(OUT_PATH) / (1024*1024)
print(f"[OK] Saved: {OUT_PATH}")
print(f"[OK] Size: {size_mb:.3f} MB (INT8 full)")

# (اختياري) اختبار استدلال سريع على الكمبيوتر
try:
    interpreter = tf.lite.Interpreter(model_path=OUT_PATH)
    interpreter.allocate_tensors()
    in_det  = interpreter.get_input_details()[0]
    out_det = interpreter.get_output_details()[0]
    in_scale, in_zero = in_det["quantization"]
    sample = x_test[:1].astype(np.float32)
    sample_q = np.round(sample / in_scale + in_zero).astype(np.int8)
    interpreter.set_tensor(in_det["index"], sample_q)
    interpreter.invoke()
    out_q = interpreter.get_tensor(out_det["index"])
    out_scale, out_zero = out_det["quantization"]
    out = (out_q.astype(np.float32) - out_zero) * out_scale
    print("[OK] Inference ran locally. Pred class:", int(np.argmax(out)))
except Exception as e:
    print("[WARN] Local TFLite test skipped:", e)
