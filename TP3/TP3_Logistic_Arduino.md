# 🧠 Task 3: Deploying Logistic Regression on Arduino Uno

This project requires you to take your **pre-trained Logistic Regression model** and implement its core logic on the **Arduino Uno** within the provided **Wokwi environment**.  
The model will use **Temperature** and **Humidity** data from the connected **DHT sensor** to perform a **real-time binary classification**.

Your entire solution must be contained in the file:
```
TP3/src/main.cpp
```

---

## 1. Exporting Model Parameters (Python/ML Environment)

Before writing any C++ code, you must extract the learned parameters from your Logistic Regression model in Python.

You need to identify and export the following **four critical sets** of numerical values.

### 🔹 StandardScaler Mean (μ)
- Retrieve the **mean value** used for each input feature (Temperature and Humidity).
- Ensure these values are extracted in a **clear, ordered array format**.

### 🔹 StandardScaler Standard Deviation (σ)
- Retrieve the **standard deviation** used for each input feature.
- Ensure the array order matches that of the mean array.

### 🔹 Logistic Regression Weights (W)
- Retrieve the **coefficients (weights)** learned by your Logistic Regression model.
- This will be an array of values corresponding to each feature.

### 🔹 Logistic Regression Bias (b)
- Retrieve the **intercept (bias)** term from your model.
- This is typically a **single scalar value**.

---

## 2. Defining Parameters in C++ (`TP3/src/main.cpp`)

Once exported, define these parameters as constant global variables in your main C++ file.

Example:
```cpp
const int N_FEATURES = 2;
const float MEAN[N_FEATURES] = { /* μ_Temperature, μ_Humidity */ };
const float STD[N_FEATURES]  = { /* σ_Temperature, σ_Humidity */ };
const float WEIGHTS[N_FEATURES] = { /* W_Temperature, W_Humidity */ };
const float BIAS = /* b */;
```

---

## 3. Implementing the Prediction Functions (C++)

You will now implement all the mathematical components required for Logistic Regression prediction.

### 3.1. Standardization Function
Implement the **feature standardization formula**:

\[
x_{scaled} = \frac{x_{raw} - \mu}{\sigma}
\]

Example C++ implementation:
```cpp
float standardize(float x_raw, int idx) {
  return (x_raw - MEAN[idx]) / STD[idx];
}
```

---

### 3.2. Sigmoid Function
Implement the **Sigmoid activation** function:

\[
\hat{y} = \frac{1}{1 + e^{-z}}
\]

Example C++ implementation:
```cpp
#include <math.h>

float sigmoid(float z) {
  return 1.0 / (1.0 + exp(-z));
}
```

---

### 3.3. Core Prediction Function
Compute the linear combination and the final probability.

Steps:
1. Calculate the **dot product** of the weights and standardized features.
2. Add the bias.
3. Pass the result through the Sigmoid function.

\[
z = \sum_{i=1}^{N_{features}} (W_i \cdot x_{scaled,i}) + b
\]

Example C++ implementation:
```cpp
float predict(float features[]) {
  float z = 0.0;
  for (int i = 0; i < N_FEATURES; i++) {
    z += WEIGHTS[i] * features[i];
  }
  z += BIAS;
  return sigmoid(z);
}
```

---

## 4. Integrating into the Arduino `loop()`

Inside the `loop()` function, perform the real-time prediction steps:

1. **Read DHT Sensor Data**  
   - Obtain Temperature and Humidity values (`x_raw` features).

2. **Pre-process (Standardization)**  
   - Use your `standardize()` function on both readings.
   - Maintain consistent feature ordering:
     - Feature 0 → Temperature  
     - Feature 1 → Humidity

3. **Prediction**  
   - Call your `predict()` function with the standardized values.

4. **Output Results**
   - Print the predicted probability to the Serial Monitor.
   - Perform binary classification using a threshold (e.g., 0.5).

Example:
```cpp
void loop() {
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  float x_scaled[2];
  x_scaled[0] = standardize(temp, 0);
  x_scaled[1] = standardize(hum, 1);

  float y_pred = predict(x_scaled);

  Serial.print("Predicted Probability: ");
  Serial.println(y_pred, 4);

  if (y_pred >= 0.5)
    Serial.println("Predicted Class: 1");
  else
    Serial.println("Predicted Class: 0");

  delay(2000);
}
```

---

## ✅ Summary of Required Deliverables

| Step | Description | Output |
|------|--------------|--------|
| 1 | Extract μ, σ, W, and b from Python model | Arrays and scalar |
| 2 | Define them as constants in `main.cpp` | C++ constants |
| 3 | Implement Standardization, Sigmoid, and Prediction functions | Functions |
| 4 | Integrate logic in Arduino `loop()` | Real-time classification |

---

## 🧩 Tips
- Ensure **the order of features** (Temperature → Humidity) is **consistent** across Python and C++.
- Print intermediate results for debugging if needed.
- Use `Serial.begin(9600)` in `setup()` to enable output.

---

**Author:** Ahmed Ghenabzia  
**Course:** Advanced Embedded AI Systems  
**Task:** TP3 – Logistic Regression on Arduino Uno  
