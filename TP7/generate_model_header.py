import os

# اسم ملف النموذج
import os

MODEL_FILENAME = os.path.join("src", "fashion_mnist_cnn_int8.tflite")

OUTPUT_HEADER = os.path.join("src", "model_data.h")

with open(MODEL_FILENAME, "rb") as f:
    data = f.read()

array_name = "fashion_mnist_cnn_int8_tflite"

with open(OUTPUT_HEADER, "w") as f:
    f.write("#ifndef MODEL_DATA_H\n")
    f.write("#define MODEL_DATA_H\n\n")
    f.write("#include <stdint.h>\n\n")
    f.write(f"const unsigned char {array_name}[] = {{\n")

    for i, b in enumerate(data):
        if i % 12 == 0:
            f.write("  ")
        f.write(f"0x{b:02x}, ")
        if i % 12 == 11:
            f.write("\n")

    f.write("\n};\n\n")
    f.write(f"const unsigned int {array_name}_len = {len(data)};\n\n")
    f.write("#endif // MODEL_DATA_H\n")
