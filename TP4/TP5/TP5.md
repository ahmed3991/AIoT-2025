# 🏆 Practical Work: Fashion-MNIST Image Classification Challenge (TensorFlow/Keras)

This practical work is designed to reinforce your understanding of **Multilayer Perceptrons (MLP)** and **Convolutional Neural Networks (CNN)** by applying them to the **Fashion-MNIST** image classification task using the TensorFlow 2.x and Keras API.

## 🎯 Objective

The primary goal is to:

1. Implement a simple **MLP model** and a simple **CNN model** in TensorFlow/Keras.
2. Train both models on the Fashion-MNIST dataset.
3. Compare the **performance metrics** (accuracy, loss) and **computational metrics** (number of parameters, memory usage) of the two architectures.
4. Conclude on the suitability of each architecture for image classification.

***

## 1. Environment Setup and Data Loading

The first step is to prepare your environment and load the dataset.

### 📝 Task 1.1: Setup and Data Loading

Write the TensorFlow/Keras code to:

* Import necessary libraries (`tensorflow`, `keras.datasets`, `keras.models`, `keras.layers`).
* Load the Fashion-MNIST dataset using `tf.keras.datasets.fashion_mnist.load_data()`.
* **Normalize** the image data to a range of $[0,1]$ by dividing the pixel values by $255.0$.
* Reshape/Expand Dimensions for both train and test images:
    * For the **MLP**: Flatten the $28\times 28$ image into a $784$-element vector (this is often handled implicitly by the `Flatten` layer).
    * For the **CNN**: Reshape the data from $(N,28,28)$ to $(N,28,28,1)$ to explicitly include the channel dimension required by Keras's convolutional layers.
* Print the new shapes of the training images for both the MLP and CNN models.

***

## 2. Model Implementation

Implement the two required models using the Keras Sequential API.

### 📝 Task 2.1: Implement and Compile the MLP Model

Define a Keras `Sequential` model named `mlp_model`:

* **Input Layer**: `keras.layers.Flatten(input_shape=(28, 28))` to flatten the image.
* **Hidden Layers**: Include two `keras.layers.Dense` hidden layers. Use **256** neurons for the first and **128** neurons for the second, both with the `relu` activation function.
* **Output Layer**: A final `keras.layers.Dense` layer with **10** units and the `softmax` activation function.
* Compile the model using the `adam` optimizer, `sparse_categorical_crossentropy` loss function, and `accuracy` as the metric.
* Print the model summary (`mlp_model.summary()`).

### 📝 Task 2.2: Implement and Compile the CNN Model

Define a Keras `Sequential` model named `cnn_model`:

* **Convolutional Block 1**: `Conv2D` (`filters=16`, `kernel_size=3`, `activation='relu'`, `input_shape=(28, 28, 1)`) $\rightarrow$ `MaxPooling2D` (`pool_size=2`).
* **Convolutional Block 2**: `Conv2D` (`filters=32`, `kernel_size=3`, `activation='relu'`) $\rightarrow$ `MaxPooling2D` (`pool_size=2`).
* **Classifier**: `Flatten()` $\rightarrow$ `Dense` (`units=64`, `activation='relu'`) $\rightarrow$ `Dense` (`units=10`, `activation='softmax'`).
* Compile the model using the same settings as the MLP model.
* Print the model summary (`cnn_model.summary()`).

***

## 3. Training and Evaluation

Train both models and evaluate their performance.

### 📝 Task 3.1: Train the MLP

Train the `mlp_model` using the $784$-element vector training data (`x_train_mlp`).

* Train for **5 epochs**.
* Use a **batch size of 64**.
* Store the training history.

### 📝 Task 3.2: Train the CNN

Train the `cnn_model` using the reshaped $(N,28,28,1)$ training data (`x_train_cnn`).

* Train for **5 epochs**.
* Use a **batch size of 64**.
* Store the training history.

### 📝 Task 3.3: Evaluate and Report

Use the `model.evaluate()` method for both models on their respective test datasets:

* Report the final test accuracy and loss for the MLP model.
* Report the final test accuracy and loss for the CNN model.

***

## 4. Resource Usage Comparison

Analyze and compare the model complexity and memory usage.

### 📝 Task 4.1: Count Trainable Parameters

Retrieve the total number of **trainable parameters** directly from the model summaries generated in Task 2.1 and 2.2.

### 📝 Task 4.2: Estimate Memory Footprint (Model Size)

Report the size of the saved model file (in Megabytes) for both models.

* Save the trained models (e.g., `mlp_model.save('mlp_model.h5')`).
* Report the file size of `mlp_model.h5` and `cnn_model.h5`.
    * *Note: This represents the memory needed to store the model for deployment.*

### 📝 Task 4.3: Estimate Computational Resources

**How many FLOPs (Floating-point operations) does each model use for a single:**

* **Training step (forward and backward pass)?**
* **Inference pass (forward pass only)?**

**What is the estimated total memory (in MB or GB) required to store the model's parameters, optimizer state, and gradients during training for each model?**

***

## 5. Final Report and Conclusion

Summarize your findings in a brief report.

### 📝 Task 5.1: Write the Conclusion

Create a table summarizing your results:

| Model | Test Accuracy | Trainable Parameters | Saved Model Size (MB) | FLOPs (Training) | FLOPs (Inference) | Training Memory (MB/GB) |
| :---: | :-----------: | :------------------: | :-------------------: | :--------------: | :---------------: | :---------------------: |
| **MLP** | $Acc_{MLP}$ | $Params_{MLP}$ | $Size_{MLP}$ | $FLOPs_{Train, MLP}$ | $FLOPs_{Inf, MLP}$ | $Mem_{Train, MLP}$ |
| **CNN** | $Acc_{CNN}$ | $Params_{CNN}$ | $Size_{CNN}$ | $FLOPs_{Train, CNN}$ | $FLOPs_{Inf, CNN}$ | $Mem_{Train, CNN}$ |

Based on the table, answer the following questions:

1.  Which model achieved a higher accuracy?
2.  Which model had a smaller number of parameters (lower memory footprint)?
3.  Explain the trade-off between the two models in the context of image classification (i.e., why did the winner in accuracy likely win, and what is the key advantage of the other model?). Why is a CNN generally superior for image tasks?