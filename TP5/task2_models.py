# task2_models.py
import numpy as np
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers

print("TensorFlow:", tf.__version__)

# --- تحميل البيانات ومعالجتها كما في الخطوة السابقة ---
(x_train, y_train), (x_test, y_test) = keras.datasets.fashion_mnist.load_data()

# التطبيع
x_train = x_train.astype("float32") / 255.0
x_test  = x_test.astype("float32") / 255.0

# تحضير بيانات MLP: تفريغ 28x28 -> 784
x_train_mlp = x_train.reshape((-1, 28 * 28))
x_test_mlp  = x_test.reshape((-1, 28 * 28))

# تحضير بيانات CNN: إضافة قناة واحدة -> (N, 28, 28, 1)
x_train_cnn = np.expand_dims(x_train, axis=-1)
x_test_cnn  = np.expand_dims(x_test, axis=-1)

print("MLP train shape:", x_train_mlp.shape)   # (60000, 784)
print("CNN train shape:", x_train_cnn.shape)   # (60000, 28, 28, 1)

# --- Task 2.1: نموذج الـ MLP ---
mlp_model = keras.Sequential(
    [
        layers.Flatten(input_shape=(28, 28)),
        layers.Dense(256, activation="relu"),
        layers.Dense(128, activation="relu"),
        layers.Dense(10, activation="softmax"),
    ],
    name="mlp_model",
)

mlp_model.compile(
    optimizer="adam",
    loss="sparse_categorical_crossentropy",
    metrics=["accuracy"],
)

print("\n=== MLP Model Summary ===")
mlp_model.summary()

# --- Task 2.2: نموذج الـ CNN ---
cnn_model = keras.Sequential(
    [
        layers.Conv2D(16, kernel_size=3, activation="relu", input_shape=(28, 28, 1)),
        layers.MaxPooling2D(pool_size=2),
        layers.Conv2D(32, kernel_size=3, activation="relu"),
        layers.MaxPooling2D(pool_size=2),
        layers.Flatten(),
        layers.Dense(64, activation="relu"),
        layers.Dense(10, activation="softmax"),
    ],
    name="cnn_model",
)

cnn_model.compile(
    optimizer="adam",
    loss="sparse_categorical_crossentropy",
    metrics=["accuracy"],
)

print("\n=== CNN Model Summary ===")
cnn_model.summary()
