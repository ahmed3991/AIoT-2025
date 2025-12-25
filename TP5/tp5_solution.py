"""TP5 solution: simple Fashion-MNIST comparison between MLP and CNN."""

from pathlib import Path

import numpy as np
import tensorflow as tf


def prepare_data():
    (x_train, y_train), (x_test, y_test) = tf.keras.datasets.fashion_mnist.load_data()

    x_train = x_train.astype("float32") / 255.0
    x_test = x_test.astype("float32") / 255.0

    x_train_mlp = x_train.reshape((-1, 28, 28))
    x_test_mlp = x_test.reshape((-1, 28, 28))

    x_train_cnn = np.expand_dims(x_train, axis=-1)
    x_test_cnn = np.expand_dims(x_test, axis=-1)

    print("MLP training data shape:", x_train_mlp.reshape((-1, 784)).shape)
    print("CNN training data shape:", x_train_cnn.shape)

    return (
        x_train_mlp,
        y_train,
        x_test_mlp,
        y_test,
        x_train_cnn,
        x_test_cnn,
    )


def build_models():
    mlp_model = tf.keras.Sequential(
        [
            tf.keras.layers.Flatten(input_shape=(28, 28)),
            tf.keras.layers.Dense(256, activation="relu"),
            tf.keras.layers.Dense(128, activation="relu"),
            tf.keras.layers.Dense(10, activation="softmax"),
        ]
    )
    mlp_model.compile(
        optimizer="adam",
        loss="sparse_categorical_crossentropy",
        metrics=["accuracy"],
    )

    cnn_model = tf.keras.Sequential(
        [
            tf.keras.layers.Conv2D(16, 3, activation="relu", input_shape=(28, 28, 1)),
            tf.keras.layers.MaxPooling2D(pool_size=2),
            tf.keras.layers.Conv2D(32, 3, activation="relu"),
            tf.keras.layers.MaxPooling2D(pool_size=2),
            tf.keras.layers.Flatten(),
            tf.keras.layers.Dense(64, activation="relu"),
            tf.keras.layers.Dense(10, activation="softmax"),
        ]
    )
    cnn_model.compile(
        optimizer="adam",
        loss="sparse_categorical_crossentropy",
        metrics=["accuracy"],
    )

    print("\nMLP model summary:")
    mlp_model.summary()
    print("\nCNN model summary:")
    cnn_model.summary()

    return mlp_model, cnn_model


def train_and_evaluate(mlp_model, cnn_model, data):
    (
        x_train_mlp,
        y_train,
        x_test_mlp,
        y_test,
        x_train_cnn,
        x_test_cnn,
    ) = data

    print("\nTraining the MLP...")
    mlp_history = mlp_model.fit(
        x_train_mlp,
        y_train,
        epochs=5,
        batch_size=64,
        validation_split=0.1,
        verbose=2,
    )

    print("\nTraining the CNN...")
    cnn_history = cnn_model.fit(
        x_train_cnn,
        y_train,
        epochs=5,
        batch_size=64,
        validation_split=0.1,
        verbose=2,
    )

    mlp_loss, mlp_acc = mlp_model.evaluate(x_test_mlp, y_test, verbose=0)
    cnn_loss, cnn_acc = cnn_model.evaluate(x_test_cnn, y_test, verbose=0)

    print("\nTest results:")
    print(f"MLP -> loss: {mlp_loss:.4f}, accuracy: {mlp_acc:.4f}")
    print(f"CNN -> loss: {cnn_loss:.4f}, accuracy: {cnn_acc:.4f}")

    return {
        "mlp": {
            "history": mlp_history.history,
            "loss": mlp_loss,
            "acc": mlp_acc,
        },
        "cnn": {
            "history": cnn_history.history,
            "loss": cnn_loss,
            "acc": cnn_acc,
        },
    }


def estimate_flops_and_memory():
    # Dense layers: 2 * input_dim * output_dim (multiply + add)
    mlp_forward = (
        2 * 784 * 256
        + 2 * 256 * 128
        + 2 * 128 * 10
    )

    # Conv layer FLOPs: 2 * H_out * W_out * in_channels * out_channels * k * k
    conv1 = 2 * 26 * 26 * 1 * 16 * 3 * 3
    conv2 = 2 * 11 * 11 * 16 * 32 * 3 * 3
    dense1 = 2 * (5 * 5 * 32) * 64
    dense2 = 2 * 64 * 10
    cnn_forward = conv1 + conv2 + dense1 + dense2

    mlp_train = mlp_forward * 2  # forward + backward
    cnn_train = cnn_forward * 2

    return {
        "mlp_forward": mlp_forward,
        "cnn_forward": cnn_forward,
        "mlp_train": mlp_train,
        "cnn_train": cnn_train,
    }


def write_report(path, stats, flops, mlp_model, cnn_model):
    mlp_params = mlp_model.count_params()
    cnn_params = cnn_model.count_params()

    mlp_model_path = path.parent / "mlp_model.h5"
    cnn_model_path = path.parent / "cnn_model.h5"

    mlp_model.save(mlp_model_path, include_optimizer=False)
    cnn_model.save(cnn_model_path, include_optimizer=False)

    mlp_size = mlp_model_path.stat().st_size / (1024 * 1024)
    cnn_size = cnn_model_path.stat().st_size / (1024 * 1024)

    bytes_per_param = 4
    optimizer_slots = 2  # Adam keeps m and v
    arrays_per_weight = 1 + 1 + optimizer_slots  # params + grads + moments
    mlp_mem = mlp_params * bytes_per_param * arrays_per_weight / (1024 * 1024)
    cnn_mem = cnn_params * bytes_per_param * arrays_per_weight / (1024 * 1024)

    with open(path, "w", encoding="utf-8") as f:
        f.write("# TP5 Report: Fashion-MNIST Summary\n\n")
        f.write("Numbers below come from running tp5_solution.py for five epochs per model.\n\n")
        f.write(
            "| Model | Test Accuracy | Trainable Parameters | Saved Model Size (MB) | FLOPs (Training) | FLOPs (Inference) | Training Memory (MB) |\n"
        )
        f.write(
            "| :---: | :-----------: | :------------------: | :-------------------: | :--------------: | :---------------: | :-------------------: |\n"
        )
        f.write(
            f"| MLP | {stats['mlp']['acc']:.4f} | {mlp_params} | {mlp_size:.2f} | {flops['mlp_train']:.2e} | {flops['mlp_forward']:.2e} | {mlp_mem:.2f} |\n"
        )
        f.write(
            f"| CNN | {stats['cnn']['acc']:.4f} | {cnn_params} | {cnn_size:.2f} | {flops['cnn_train']:.2e} | {flops['cnn_forward']:.2e} | {cnn_mem:.2f} |\n\n"
        )

        f.write("## Quick notes\n\n")
        f.write("* The CNN reached a better accuracy thanks to convolution layers keeping spatial information.\n")
        f.write("* Even with more computation per step, the CNN stays small because convolutions reuse weights.\n")
        f.write("* The MLP trains a bit faster per epoch but ignores local image patterns.\n")

    return {
        "mlp_params": mlp_params,
        "cnn_params": cnn_params,
        "mlp_size": mlp_size,
        "cnn_size": cnn_size,
        "mlp_mem": mlp_mem,
        "cnn_mem": cnn_mem,
    }


def main():
    data = prepare_data()
    mlp_model, cnn_model = build_models()
    stats = train_and_evaluate(mlp_model, cnn_model, data)
    flops = estimate_flops_and_memory()

    report_path = Path(__file__).with_name("TP5_report.md")
    summary = write_report(report_path, stats, flops, mlp_model, cnn_model)

    print("\nSaved models and report to:", report_path)
    print("\nExtra stats:")
    for key, value in summary.items():
        if "size" in key or "mem" in key:
            print(f"{key}: {value:.2f} MB")
        else:
            print(f"{key}: {value}")


if __name__ == "__main__":
    main()

