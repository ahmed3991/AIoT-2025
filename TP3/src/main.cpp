// TP3/src/main.cpp
#include <Arduino.h>
#include "DHT.h"
#include <math.h>

// تعديل حسب نوع الحساس في المشروع (DHT11 أو DHT22)
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

const int N_FEATURES = 2;

// --- الصق القيم المستخرجة هنا (من extract_params.py) ---
// الترتيب: [ Temperature, Humidity ]
const float MEAN[N_FEATURES]    = { 25.212731f, 48.719855f };
const float STD[N_FEATURES]     = { 3.064268f, 9.432582f };
const float WEIGHTS[N_FEATURES] = { 2.703985f, 0.788680f };
const float BIAS = -1.303151f;

// عتبة التصنيف
const float THRESH = 0.5f;

float standardize(float x_raw, int idx) {
  if (STD[idx] == 0.0f) return 0.0f;
  return (x_raw - MEAN[idx]) / STD[idx];
}

float sigmoidf_float(float z) {
  // استخدام expf لعمليات على float أكثر كفاءة
  if (z > 50.0f) return 1.0f;
  if (z < -50.0f) return 0.0f;
  return 1.0f / (1.0f + expf(-z));
}

float predict_from_raw(float temp_raw, float hum_raw) {
  float x0 = standardize(temp_raw, 0); // Temperature
  float x1 = standardize(hum_raw, 1);  // Humidity
  float z = WEIGHTS[0]*x0 + WEIGHTS[1]*x1 + BIAS;
  return sigmoidf_float(z);
}

void setup() {
  Serial.begin(9600);
  unsigned long start = millis();
  while (!Serial && millis() - start < 2000) { ; } // انتظار قصير
  Serial.println("TP3: Logistic Regression on Arduino - Ready");
  dht.begin();
}

void loop() {
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(2000);
    return;
  }

  // --- (اختياري) فكّ التعليق لطباعة القيم الخام عند التصحيح ---
  // Serial.print("Raw Temp: "); Serial.print(temp, 2);
  // Serial.print("  Raw Hum: "); Serial.println(hum, 2);

  float y_pred = predict_from_raw(temp, hum);

  Serial.print("Predicted Probability: ");
  Serial.println(y_pred, 4);

  int predicted_class = (y_pred >= THRESH) ? 1 : 0;
  Serial.print("Predicted Class: ");
  Serial.println(predicted_class);

  Serial.println("-----------------------------");
  delay(2000);
}
