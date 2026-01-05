#include <Arduino.h>
#include "DHT.h"

#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// 🔹 المعاملات من النموذج
const int N_FEATURES = 2;
const float MEAN[N_FEATURES] = { 25.000000f, 46.466667f };
const float STD[N_FEATURES] = { 4.320494f, 11.032477f };
const float WEIGHTS[N_FEATURES] = { 1.191529f, 1.256455f };
const float BIAS = -0.169730f;

// 🔹 دوال النموذج
float standardize(float x_raw, int idx) {
  return (x_raw - MEAN[idx]) / STD[idx];
}

float sigmoid(float z) {
  return 1.0 / (1.0 + exp(-z));
}

float predict(float features[]) {
  float z = BIAS;
  for (int i = 0; i < N_FEATURES; i++) {
    z += WEIGHTS[i] * features[i];
  }
  return sigmoid(z);
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("=== Logistic Regression Model with Live Simulation ==="));
  dht.begin();
  delay(2000);
  randomSeed(analogRead(0));
}

void loop() {
  delay(2000);

  // 🔹 محاكاة بيانات حية مع قيم متغيرة
  float h, t;
  static unsigned long counter = 0;
  
  // توليد قيم عشوائية لمحاكاة البيانات الحية
  h = 40.0 + random(-15, 25);  // رطوبة متغيرة: 25-65%
  t = 24.0 + random(-8, 12);   // حرارة متغيرة: 16-36°C
  
  // طباعة القيم الأصلية
  Serial.print("Sample "); Serial.print(++counter);
  Serial.print(": Temp="); Serial.print(t, 1); 
  Serial.print("°C, Hum="); Serial.print(h, 1); Serial.println("%");

  // 🔹 الخطوة 1: توحيد البيانات
  float x_scaled[N_FEATURES];
  x_scaled[0] = standardize(t, 0);  // Temperature
  x_scaled[1] = standardize(h, 1);  // Humidity

  // 🔹 الخطوة 2: التنبؤ
  float y_pred = predict(x_scaled);

  // 🔹 الخطوة 3: عرض النتائج مع تفسير
  Serial.print("  Standardized: [");
  Serial.print(x_scaled[0], 3); Serial.print(", "); 
  Serial.print(x_scaled[1], 3); Serial.print("]");
  
  Serial.print(" → Probability: ");
  Serial.print(y_pred, 4);
  
  // التصنيف النهائي مع تفسير
  if (y_pred >= 0.5) {
    Serial.println(" → 🔴 Class 1 (High Risk)");
  } else {
    Serial.println(" → 🟢 Class 0 (Normal)");
  }

  Serial.println("-------------------");
}