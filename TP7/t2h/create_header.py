import sys

tflite_path = "cnn_model.tflite"
header_path = "model_data.h"

with open(tflite_path, "rb") as f:
    data = f.read()

with open(header_path, "w") as f:
    f.write("const unsigned char model_tflite[] = {\n")

    for i, b in enumerate(data):
        if i % 12 == 0:
            f.write("  ")
        f.write(f"0x{b:02X}, ")
        if i % 12 == 11:
            f.write("\n")

    f.write("\n};\n")
    f.write(f"const int model_tflite_len = {len(data)};\n")
