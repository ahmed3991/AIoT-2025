import os, numpy as np, tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers

# مسارات الحفظ
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
MODELS_DIR = os.path.join(BASE_DIR, "models")
os.makedirs(MODELS_DIR, exist_ok=True)

# 1) تحميل بيانات MNIST (28x28 صور أرقام رمادية)
(x_train, y_train), (x_test, y_test) = keras.datasets.mnist.load_data()
x_train = x_train.astype("float32") / 255.0
x_test  = x_test.astype("float32") / 255.0

# بضمان نفس الأبعاد لكل نموذج:
# للـ MLP: نفرد الصور إلى متجهات 784
x_train_mlp = x_train.reshape((-1, 28*28))
x_test_mlp  = x_test.reshape((-1, 28*28))

# للـ CNN: نضيف قناة رمادية (H,W,1)
x_train_cnn = x_train[..., None]
x_test_cnn  = x_test[..., None]

# 2) نموذج MLP بسيط
mlp_model = keras.Sequential([
    layers.Input(shape=(784,)),
    layers.Dense(128, activation="relu"),
    layers.Dense(64, activation="relu"),
    layers.Dense(10, activation="softmax"),
])
mlp_model.compile(optimizer="adam",
                  loss="sparse_categorical_crossentropy",
                  metrics=["accuracy"])
mlp_model.fit(x_train_mlp, y_train, epochs=3, batch_size=128,
              validation_split=0.1, verbose=2)
mlp_model.evaluate(x_test_mlp, y_test, verbose=0)
mlp_path = os.path.join(MODELS_DIR, "mlp_model.h5")
mlp_model.save(mlp_path)  # يتولّد ملف H5
print(f"[SAVED] {mlp_path} ({os.path.getsize(mlp_path)/1024/1024:.3f} MB)")

# 3) نموذج CNN بسيط
cnn_model = keras.Sequential([
    layers.Input(shape=(28, 28, 1)),
    layers.Conv2D(16, 3, activation="relu"),
    layers.MaxPooling2D(),
    layers.Conv2D(32, 3, activation="relu"),
    layers.MaxPooling2D(),
    layers.Flatten(),
    layers.Dense(64, activation="relu"),
    layers.Dense(10, activation="softmax"),
])
cnn_model.compile(optimizer="adam",
                  loss="sparse_categorical_crossentropy",
                  metrics=["accuracy"])
cnn_model.fit(x_train_cnn, y_train, epochs=3, batch_size=128,
              validation_split=0.1, verbose=2)
cnn_model.evaluate(x_test_cnn, y_test, verbose=0)
cnn_path = os.path.join(MODELS_DIR, "cnn_model.h5")
cnn_model.save(cnn_path)  # يتولّد ملف H5
print(f"[SAVED] {cnn_path} ({os.path.getsize(cnn_path)/1024/1024:.3f} MB)")

print("[DONE] Models saved to:", MODELS_DIR)
