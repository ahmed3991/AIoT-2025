# تحويل ملف .tflite إلى C Header لتضمينه في Arduino

tflite_file = "cnn_model_quant.tflite"   # اسم نموذجك
header_file = "src/model_data.h"         # مسار ملف الـ header الناتج
array_name = "cnn_model_quant_tflite"    # اسم المصفوفة في C

# اقرأ الملف الثنائي
with open(tflite_file, "rb") as f:
    data = f.read()

# اكتب ملف الـ header
with open(header_file, "w") as f:
    f.write(f"unsigned char {array_name}[] = {{\n")
    for i, byte in enumerate(data):
        f.write(f"0x{byte:02x},")
        if (i + 1) % 12 == 0:  # 12 بايت في كل سطر
            f.write("\n")
    f.write("\n};\n")
    f.write(f"unsigned int {array_name}_len = {len(data)};\n")

print(f"تم إنشاء {header_file} بنجاح!")
