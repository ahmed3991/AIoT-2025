import numpy as np
import tensorflow as tf

def run_inference(tflite_path, sample):
    # sample: float32 بعد نفس الpreprocessing المستخدمة في التدريب
    interpreter = tf.lite.Interpreter(model_path=tflite_path)
    interpreter.allocate_tensors()
    in_details = interpreter.get_input_details()
    out_details = interpreter.get_output_details()

    # قراءة scale/zero-point
    in_scale, in_zero = in_details[0]["quantization"]

    # تحويل العينة من float32 -> int8 وفق المعايرة
    sample_q = np.round(sample / in_scale + in_zero).astype(np.int8)

    interpreter.set_tensor(in_details[0]["index"], sample_q)
    interpreter.invoke()
    out_q = interpreter.get_tensor(out_details[0]["index"])

    # إعادة فك-الكمّية للإخراج (اختياري للقراءة البشرية)
    out_scale, out_zero = out_details[0]["quantization"]
    out_deq = (out_q.astype(np.float32) - out_zero) * out_scale
    return out_deq

if __name__ == "__main__":
    # مثال MLP بمدخل 784
    x = np.random.rand(1, 784).astype(np.float32)  # استبدلي بعينة حقيقية بعد preprocessing
    print(run_inference("tflite_out/mlp_int8.tflite", x))

    # مثال CNN بمدخل 28x28x1
    x_img = np.random.rand(1, 28, 28, 1).astype(np.float32)  # استبدلي بعينة حقيقية
    print(run_inference("tflite_out/cnn_int8.tflite", x_img))
