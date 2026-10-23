# AI for IoT Practical Work - 2: Optimization and Deployment Readiness

This practical builds on the previous classification task by introducing software best practices (pipelines) and crucial hardware constraints (resource analysis) relevant to IoT deployment.

## Objective
To encapsulate preprocessing and modeling into robust pipelines, measure the computational and memory footprints of the selected algorithms, and critically evaluate their suitability for deployment on a resource-constrained microcontroller, specifically the ESP32.

## 1. Core Task: Pipeline Construction

The use of `sklearn.pipeline.Pipeline` ensures that all preprocessing steps (like scaling) are consistently applied both during training and inference, preventing data leakage and simplifying the MLOps process.

### Task 1.1: Logistic Regression Pipeline (LR-Pipeline)

- **Define the Preprocessor**: Use `StandardScaler` (or a similar scaling mechanism) as the first step.
- **Define the Estimator**: Use `LogisticRegression` as the final step.
- **Create the Pipeline**: Combine the preprocessor and estimator into a single `Pipeline` object and train it on the full training data (`X_train`, `y_train`).
- **Evaluate**: Score the pipeline on the test set (`X_test`, `y_test`) and record the accuracy and F1 score.

### Task 1.2: XGBoost Pipeline (XGB-Pipeline)

- **Define the Preprocessor**: Although tree-based models like XGBoost are generally scale-invariant, include the `StandardScaler` step in the pipeline for consistency and MLOps standardization.
- **Define the Estimator**: Use `xgboost.XGBClassifier` as the final step.
- **Create the Pipeline**: Combine the preprocessor and estimator into a single `Pipeline` object and train it on the full training data.
- **Evaluate**: Score the pipeline on the test set and record the accuracy and F1 score.

## 2. Advanced Challenge: Resource and Efficiency Analysis (Mandatory for Credit)

The most critical factor for an IoT device is resource consumption. You will analyze your two models (LR-Pipeline and XGB-Pipeline) based on memory and time.

### Task 2.1: Model Size Measurement (Memory Footprint)

The size of the trained model file directly relates to the necessary Flash storage on the ESP32.

- **Serialization**: Serialize both trained pipeline objects (LR-Pipeline and XGB-Pipeline) using Python's `pickle` library.
- **Memory in Bytes**: Calculate the size of the serialized files (or use `sys.getsizeof` on the pickled object) in kilobytes (KB).

| Model | Size (KB) |
|-------|-----------|
| LR-Pipeline | |
| XGB-Pipeline | |

### Task 2.2: Computational Time Measurement

You must measure the time required for both training and inference.

- **Inference Time (Whole Dataset)**: Measure the total time (in seconds) taken to execute the `.predict()` method for each pipeline over the entire test dataset (`X_test`).
- **Single Inference Time (Average)**: Calculate the average time required for a single data point inference by dividing the total inference time (from step 2) by the number of samples in the test set.

| Model | Total Test Inference Time (s) | Single Inference Time (ms) |
|-------|-------------------------------|----------------------------|
| LR-Pipeline | | |
| XGB-Pipeline | | |

### Task 2.3: Critical Deployment Analysis (The ESP32 Question)

Based on the resource measurements above, provide a comprehensive answer to the following questions in a well-structured paragraph:

1. **Memory Feasibility**: Given that a typical ESP32 has around 520 KB of total SRAM (with less available for the application heap) and several megabytes of Flash, is the memory footprint of both models (from Task 2.1) feasible for storage and execution on the device? Which model is more constrained by memory? (Hint: Models are typically stored in Flash and loaded into RAM for prediction).

2. **Time Efficiency**: If your application requires real-time predictions (e.g., classifying sensor data every 1 seconds), are the single inference times (from Task 2.2) fast enough for both models?

3. **Conclusion**: Which model (Logistic Regression or XGBoost) is the clear choice for this specific application based purely on efficiency and deployment constraints on the ESP32? Justify your choice by referencing the measured data and briefly discuss optimization techniques that would be needed to potentially deploy the less efficient model.