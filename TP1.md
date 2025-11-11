# 💡 AI for IoT Practical 1: Fire Alarm Detection

📌 **Important:** Please run this practical work on **[Kaggle Notebooks](https://www.kaggle.com/code)**.  
Kaggle provides free GPU/CPU resources, pre-installed libraries (scikit-learn, XGBoost, etc.), and easy dataset integration.

---

## 1. Objective
The goal of this practical is to build a **binary classification model** to predict the state of an IoT-connected smoke detector based on environmental sensor readings.  
You'll start with the fundamental **Logistic Regression** model and have the opportunity to implement the high-performance **XGBoost** model as a bonus.

---

## 2. Dataset Overview
This dataset simulates real-world conditions encountered by an IoT fire detection device.

- **Source**: [Kaggle - Smoke Detection Dataset](https://www.kaggle.com/datasets/deepcontractor/smoke-detection-dataset)  
- **Key Features (Input/X):**  
  Time-series readings from sensors like:
  - Temperature  
  - Humidity  
  - Gas concentrations (CO, LPG, Methane)  
  - Other environmental factors  

- **Target Feature (Output/y):**  
  - `1` → Fire Alarm is **ON** (Fire/Smoke detected)  
  - `0` → Normal operational conditions (No Fire/Smoke)  

---

## 3. Core Task: Logistic Regression Classifier

Logistic Regression is an excellent starting point for binary classification, providing a good performance baseline for an AIoT application.

### A. Setup and Preprocessing
1. **Import Libraries**  
   - `pandas`, `numpy`, `matplotlib` / `seaborn`  
   - From `sklearn`: `train_test_split`, `LogisticRegression`, `metrics`  

2. **Load and Inspect Data**  
   - Load the dataset from Kaggle  
   - Use `.info()` and `.isnull().sum()` to check for missing values  
   - Handle missing data (imputation or removal)  

3. **Define Features and Target**  
   - Separate features (`X`) and target variable (`y`)  

4. **Feature Scaling (MANDATORY)**  
   - Apply `StandardScaler` from `sklearn.preprocessing`  
   - Logistic Regression requires scaled features  

5. **Split Data**  
   - Use `train_test_split` with 80% training and 20% testing  

---

### B. Training and Evaluation
1. **Train Model**  
   - Initialize and train a `LogisticRegression` model  

2. **Make Predictions**  
   - Predict outcomes on the test set  

3. **Evaluate Performance**  
   - Calculate metrics:  
     - Accuracy  
     - Precision  
     - Recall  
     - F1-Score  

4. **Visualize Results**  
   - Generate and display a **Confusion Matrix**  

---

## 🏆 Bonus Challenge: Building a Robust XGBoost Model

XGBoost (Extreme Gradient Boosting) is one of the most powerful ensemble techniques used in industry.

1. **Implement XGBoost**  
   - Use `XGBClassifier` from the `xgboost` library  
   - Note: Tree-based models like XGBoost are not highly sensitive to feature scaling  

2. **Train and Evaluate**  
   - Train on the same train/test split  
   - Calculate the same metrics as Logistic Regression  

3. **Compare and Analyze**  
   - Which model has a higher **Recall** score?  
   - Why is minimizing **False Negatives** crucial in fire detection?  
   - Discuss trade-offs:  
     - Logistic Regression → simplicity & speed  
     - XGBoost → higher performance but more complexity  

---

## 4. Deliverables

Submit a **single Kaggle Notebook (`.ipynb`)** containing:

- **Code and Documentation**  
  - Well-commented code with all preprocessing and modeling steps  

- **Core Task Results**  
  - Accuracy, Precision, Recall, F1-Score, Confusion Matrix for Logistic Regression  

- **Conclusion**  
  - Interpret the Recall score of Logistic Regression  
  - Discuss implications for IoT fire alarm reliability  

- **Bonus Results (if completed)**  
  - XGBoost metrics  
  - Comparative analysis between Logistic Regression and XGBoost  
