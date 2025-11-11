# extract_parameters.py
import pickle
import numpy as np

print("=== Extracting Model Parameters ===")

try:
    # Load model and scaler
    with open('logistic_model.pkl', 'rb') as f:
        model = pickle.load(f)
    
    with open('scaler.pkl', 'rb') as f:
        scaler = pickle.load(f)
    
    # Extract parameters
    means = scaler.mean_
    stds = scaler.scale_
    weights = model.coef_[0]
    bias = model.intercept_[0]
    
    print("Extracted Model Parameters:")
    print("=" * 40)
    print(f"MEAN: {[float(x) for x in means]}")
    print(f"STD: {[float(x) for x in stds]}")
    print(f"WEIGHTS: {[float(x) for x in weights]}")
    print(f"BIAS: {float(bias)}")
    print()
    print("C++ Code Ready to Copy:")
    print("=" * 40)
    print(f"const float MEAN[N_FEATURES] = {{ {float(means[0]):.6f}f, {float(means[1]):.6f}f }};")
    print(f"const float STD[N_FEATURES] = {{ {float(stds[0]):.6f}f, {float(stds[1]):.6f}f }};")
    print(f"const float WEIGHTS[N_FEATURES] = {{ {float(weights[0]):.6f}f, {float(weights[1]):.6f}f }};")
    print(f"const float BIAS = {float(bias):.6f}f;")
    
except FileNotFoundError:
    print("ERROR: Model files not found!")
    print("Please run create_model.py first")
except Exception as e:
    print(f"ERROR: {e}")