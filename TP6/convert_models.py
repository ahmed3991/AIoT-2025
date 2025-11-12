import os
import sys
import numpy as np
import tensorflow as tf

# ===== إعداد المسارات =====
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
MODELS_DIR = os.path.join(BASE_DIR, "models")   # هنا يجب وضع ملفات .h5
OUT_DIR = os.path.join(BASE_DIR, "tflite_out")
os.makedirs(OUT_DIR, exist_ok=True)

MLP_H5 = os.path.join(MODELS_DIR, "mlp_model.h5")
CNN_H5 = os.path.join(MODELS_DIR, "cnn_model.h5")
MLP_TFL = os.path.join(OUT_DIR, "mlp_int8.tflite")
CNN_TFL = os.path.join(OUT_DIR, "cnn_int8.tflite")

def check_file(path):
    if not os.path.exists(path):
        print(f"[ERROR] الملف غير موجود:\n{path}\n"
              f"↪ ضَع/ي الملف داخل المجلد: {MODELS_DIR}\n")
        sys.exit(1)
    if os.path.getsize(path) < 10 * 1024:  # أقل من 10KB على الأرجح ليس نموذجاً صحيحاً
        print(f"[ERROR] حجم الملف يبدو غير صحيح (صغير جداً):\n{path}\n"
              f"↪ أَعِدي حفظ النموذج من كود التدريب: model.save('اسم.h5')\n")
        sys.exit(1)

# ===== بيانات ممثلة للتكمية (استبدليها ببياناتك الحقيقية بعد نفس الpreprocessing) =====
def rep_mlp(num_samples=300):
    # مثال: مدخل مسطح 784 (مثل MNIST 28*28). عدّلي حسب مدخل نموذجك.
    dim = 784
    for _ in range(num_samples):
        yield [np.random.rand(1, dim).astype(np.float32)]

def rep_cnn(num_samples=300):
    # مثال: صورة 28x28x1. عدّلي H, W, C حسب نموذجك.
    H, W, C = 28, 28, 1
    for _ in range(num_samples):
        yield [np.random.rand(1, H, W, C).astype(np.float32)]

def convert_int8(keras_path, out_path, rep_fn):
    print(f"[INFO] Loading Keras model: {keras_path}")
    model = tf.keras.models.load_model(keras_path)

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = rep_fn
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8

    tflite_model = converter.convert()
    with open(out_path, "wb") as f:
        f.write(tflite_model)

    keras_size = os.path.getsize(keras_path) / (1024*1024)
    tflite_size = os.path.getsize(out_path) / (1024*1024)
    print(f"[OK] Saved: {out_path}")
    print(f"     Keras .h5 size:   {keras_size:.3f} MB")
    print(f"     TFLite INT8 size: {tflite_size:.3f} MB "
          f"(~{(tflite_size/keras_size*100):.1f}% of Keras)\n")

if __name__ == "__main__":
    # تأكيد وجود ملفات النماذج داخل models/
    check_file(MLP_H5)
    check_file(CNN_H5)

    # تحويل MLP
    convert_int8(MLP_H5, MLP_TFL, rep_mlp)

    # تحويل CNN
    convert_int8(CNN_H5, CNN_TFL, rep_cnn)

    print("[DONE] ستجدين ملفات .tflite داخل:", OUT_DIR)
