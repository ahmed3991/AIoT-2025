# retrain_for_iot.py
# Retrain model with Temperature & Humidity only for IoT deployment

import pandas as pd
import numpy as np
import pickle
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import accuracy_score, classification_report

print("="*60)
print("Retraining Model for IoT (2 features only)")
print("="*60)

# Load data
df = pd.read_csv('smoke_detection_iot.csv')

# Use ONLY Temperature and Humidity
X = df[['Temperature[C]', 'Humidity[%]']]
y = df['Fire Alarm']

print(f"\nDataset: {X.shape[0]} samples")
print(f"Features: {list(X.columns)}")
print(f"Fire: {(y==1).sum()}, No Fire: {(y==0).sum()}")

# Split data
X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=42, stratify=y
)

# Scale
scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# Train model
lr_model = LogisticRegression(max_iter=1000, random_state=42)
lr_model.fit(X_train_scaled, y_train)

# Evaluate
y_pred = lr_model.predict(X_test_scaled)
accuracy = accuracy_score(y_test, y_pred)

print(f"\nModel Performance:")
print(f"Accuracy: {accuracy:.4f} ({accuracy*100:.2f}%)")
print("\nClassification Report:")
print(classification_report(y_test, y_pred, target_names=['No Fire', 'Fire']))

# Create a simple pipeline object
class SimplePipeline:
    def __init__(self, scaler, model):
        self.scaler = scaler
        self.model = model
    
    def predict(self, X):
        X_scaled = self.scaler.transform(X)
        return self.model.predict(X_scaled)
    
    def predict_proba(self, X):
        X_scaled = self.scaler.transform(X)
        return self.model.predict_proba(X_scaled)

# Save the 2-feature pipeline
iot_pipeline = SimplePipeline(scaler, lr_model)

with open('lr_iot_pipeline.pkl', 'wb') as f:
    pickle.dump(iot_pipeline, f)

print(f"\n✅ IoT Pipeline saved as: lr_iot_pipeline.pkl")
print(f"This pipeline expects 2 features: Temperature, Humidity")
