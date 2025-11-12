# task1_setup_and_data.py
import numpy as np
import tensorflow as tf

# 1) معلومات الإصدار (اختياري للتأكد)
print("TensorFlow version:", tf.__version__)

# 2) تحميل بيانات Fashion-MNIST
(x_train, y_train), (x_test, y_test) = tf.keras.datasets.fashion_mnist.load_data()

# 3) التطبيع إلى المدى [0, 1]
x_train = x_train.astype("float32") / 255.0
x_test  = x_test.astype("float32") / 255.0

# 4) تحضير أشكال البيانات للنموذجين

## للـ MLP: نفرد الصورة 28x28 إلى متجه بطول 784
x_train_mlp = x_train.reshape((-1, 28 * 28))
x_test_mlp  = x_test.reshape((-1, 28 * 28))

## للـ CNN: نضيف بُعد القناة (1) ليصبح الشكل (N, 28, 28, 1)
x_train_cnn = np.expand_dims(x_train, axis=-1)
x_test_cnn  = np.expand_dims(x_test, axis=-1)

# 5) طباعة الأشكال الجديدة
print("MLP train shape:", x_train_mlp.shape)   # متوقع: (60000, 784)
print("MLP test  shape:", x_test_mlp.shape)    # متوقع: (10000, 784)

print("CNN train shape:", x_train_cnn.shape)   # متوقع: (60000, 28, 28, 1)
print("CNN test  shape:", x_test_cnn.shape)    # متوقع: (10000, 28, 28, 1)

# 6) تأكيدات بسيطة (اختياري)
assert x_train_mlp.shape == (60000, 784)
assert x_test_mlp.shape  == (10000, 784)
assert x_train_cnn.shape == (60000, 28, 28, 1)
assert x_test_cnn.shape  == (10000, 28, 28, 1)

# 7) طباعة بعض المعلومات الإضافية (اختياري)
print("Train labels shape:", y_train.shape)    # (60000,)
print("Test  labels shape:", y_test.shape)     # (10000,)
print("Dtype:", x_train.dtype, "| Range:", (float(x_train.min()), float(x_train.max())))
