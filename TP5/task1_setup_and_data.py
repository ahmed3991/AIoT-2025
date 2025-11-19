# task1_setup_and_data.py  (low-RAM friendly)
import numpy as np
import tensorflow as tf

print("TensorFlow version:", tf.__version__)

# 1) تحميل بيانات Fashion-MNIST (uint8)
(x_train, y_train), (x_test, y_test) = tf.keras.datasets.fashion_mnist.load_data()

# 2) التطبيع إلى [0,1] مع تقليل الذاكرة: استخدم float16 بدل float32
#    هذا يقلل الذّاكرة المطلوبة للنصف تقريبًا.
x_train = x_train.astype("float16") / 255.0
x_test  = x_test.astype("float16") / 255.0

# 3) تحضير أشكال البيانات للنموذجين
# للـ MLP: تفريغ 28x28 -> 784 (عادةً Flatten في النموذج يكفي، هنا نطبع الشكل فقط)
x_train_mlp = x_train.reshape((-1, 28 * 28))
x_test_mlp  = x_test.reshape((-1, 28 * 28))

# للـ CNN: إضافة بُعد القناة -> (N, 28, 28, 1)
# np.expand_dims يُعيد view غالبًا ولا يضاعف البيانات.
x_train_cnn = np.expand_dims(x_train, axis=-1)
x_test_cnn  = np.expand_dims(x_test, axis=-1)

# 4) طباعة الأشكال والقيم
print("MLP train shape:", x_train_mlp.shape)   # متوقع: (60000, 784)
print("MLP test  shape:", x_test_mlp.shape)    # متوقع: (10000, 784)

print("CNN train shape:", x_train_cnn.shape)   # متوقع: (60000, 28, 28, 1)
print("CNN test  shape:", x_test_cnn.shape)    # متوقع: (10000, 28, 28, 1)

print("Train labels shape:", y_train.shape)    # (60000,)
print("Test  labels shape:", y_test.shape)     # (10000,)
print("Dtype:", x_train.dtype, "| Range:", (float(x_train.min()), float(x_train.max())))
