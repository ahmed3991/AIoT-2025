# task3_train_eval.py
import numpy as np
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers

# ثباتية بسيطة
tf.random.set_seed(42)
np.random.seed(42)

# --- تحميل البيانات وتجهزيها ---
(x_train, y_train), (x_test, y_test) = keras.datasets.fashion_mnist.load_data()
x_train = x_train.astype("float32") / 255.0
x_test  = x_test.astype("float32") / 255.0

# للـ MLP: لا حاجة لتفريغ يدوي (Flatten بالطبقة كافٍ)
# للـ CNN: إضافة قناة -> (N, 28, 28, 1)
x_train_cnn = np.expand_dims(x_train, axis=-1)
x_test_cnn  = np.expand_dims(x_test, axis=-1)

# --- نموذج الـ MLP (Task 2.1) ---
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
    metrics=["accuracy"]
)

# --- نموذج الـ CNN (Task 2.2) ---
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
cnn_model.compile(
    optimizer="adam",
    loss="sparse_categorical_crossentropy",
    metrics=["accuracy"]
)

# --- Task 3.1: تدريب الـ MLP ---
print("\n=== Training MLP (5 epochs, batch_size=64) ===")
hist_mlp = mlp_model.fit(
    x_train, y_train,          # نستخدم الصور بشكلها 28x28
    validation_split=0.1,
    epochs=5,
    batch_size=64,
    verbose=2
)

# --- Task 3.2: تدريب الـ CNN ---
print("\n=== Training CNN (5 epochs, batch_size=64) ===")
hist_cnn = cnn_model.fit(
    x_train_cnn, y_train,      # الشكل (N, 28, 28, 1)
    validation_split=0.1,
    epochs=5,
    batch_size=64,
    verbose=2
)

# --- Task 3.3: التقييم على بيانات الاختبار ---
print("\n=== Evaluate on Test Set ===")
test_loss_mlp, test_acc_mlp = mlp_model.evaluate(x_test, y_test, verbose=0)        # 28x28
test_loss_cnn, test_acc_cnn = cnn_model.evaluate(x_test_cnn, y_test, verbose=0)    # 28x28x1

print(f"MLP  -> Test Accuracy: {test_acc_mlp:.4f} | Test Loss: {test_loss_mlp:.4f}")
print(f"CNN  -> Test Accuracy: {test_acc_cnn:.4f} | Test Loss: {test_loss_cnn:.4f}")

# حفظ السجلين كـ .npy لاستخدامهما لاحقًا
np.save("hist_mlp.npy", {k: np.array(v) for k, v in hist_mlp.history.items()}, allow_pickle=True)
np.save("hist_cnn.npy", {k: np.array(v) for k, v in hist_cnn.history.items()}, allow_pickle=True)
