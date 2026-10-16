// AI for IoT - Fire alarm on ESP32
// Runs a model of Practical 2 (exported by tools/export_model.py) on real test samples,
// one sample per second like the real sensor, and drives a red LED + buzzer (fire) or a green LED (normal).
#include <Arduino.h>
#include "lr_model.h"
#include "xgb_model.h"
#include "test_samples.h"

// 1 = compressed XGBoost (20 trees, depth 3): detects 86% of the fires in unseen sessions
// 0 = Logistic Regression: detects 49% of them
#define USE_XGB 1

#if USE_XGB
#define MODEL_NAME "XGBoost (20 trees, depth 3)"
#define predict_proba xgb_predict_proba
#define PYTHON_PROBA PYTHON_PROBA_XGB
const size_t MODEL_BYTES = sizeof(XGB_NODES) + sizeof(XGB_TREE_ROOT) + sizeof(XGB_BASE_MARGIN);
#else
#define MODEL_NAME "Logistic Regression"
#define predict_proba lr_predict_proba
#define PYTHON_PROBA PYTHON_PROBA_LR
const size_t MODEL_BYTES = sizeof(LR_MEAN) + sizeof(LR_STD) + sizeof(LR_WEIGHTS) + sizeof(LR_BIAS);
#endif

const int PIN_LED_FIRE = 25;   // red LED
const int PIN_LED_OK   = 26;   // green LED
const int PIN_BUZZER   = 27;
const float THRESHOLD  = 0.5f; // same decision rule as Pipeline.predict()
const uint32_t PERIOD_MS = 1000;

volatile float sink;           // keeps the benchmark loop from being optimised away

void benchmark() {
  const int REPS = 1000;
  uint32_t t0 = micros();
  for (int r = 0; r < REPS; r++)
    for (int i = 0; i < N_SAMPLES; i++) sink = predict_proba(SAMPLES[i]);
  uint32_t dt = micros() - t0;

  Serial.printf("CPU: %u MHz | free heap: %u bytes\r\n", getCpuFrequencyMhz(), ESP.getFreeHeap());
  Serial.printf("Model parameters: %u bytes (%d features)\r\n", (unsigned)MODEL_BYTES, N_FEATURES);
  Serial.printf("Inference time: %.3f us per sample (mean of %d calls)\r\n\r\n",
                (float)dt / (REPS * N_SAMPLES), REPS * N_SAMPLES);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED_FIRE, OUTPUT);
  pinMode(PIN_LED_OK, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  delay(200);
  Serial.println("\n=== ESP32 Fire Alarm - " MODEL_NAME " ===");
  benchmark();
  Serial.println(" #  | P(fire) | ESP32  | true   | Python  | match");
  Serial.println("----+---------+--------+--------+---------+------");
}

void loop() {
  static int i = 0, correct = 0, same_as_python = 0;
  static float max_diff = 0.0f;

  // One prediction, timed on its own
  uint32_t t0 = micros();
  float p = predict_proba(SAMPLES[i]);
  uint32_t dt = micros() - t0;
  bool fire = p >= THRESHOLD;

  // Actuators
  digitalWrite(PIN_LED_FIRE, fire);
  digitalWrite(PIN_LED_OK, !fire);
  if (fire) tone(PIN_BUZZER, 1000, 150);

  // Compare with the true label and with the probability computed by scikit-learn
  bool truth = TRUE_LABEL[i] == 1;
  bool py_fire = PYTHON_PROBA[i] >= THRESHOLD;
  correct += (fire == truth);
  same_as_python += (fire == py_fire);
  max_diff = max(max_diff, fabsf(p - PYTHON_PROBA[i]));

  Serial.printf("%3d | %7.4f | %-6s | %-6s | %7.4f | %s%s  (%u us)\r\n", i + 1, p, fire ? "FIRE" : "normal",
                truth ? "FIRE" : "normal", PYTHON_PROBA[i], fire == truth ? "ok" : "WRONG",
                (!truth || fire) ? "" : " (missed fire)", (unsigned)dt);

  if (++i == N_SAMPLES) {
    Serial.printf("\r\nSummary: %d/%d correct | same decision as Python: %d/%d | max |P_esp32 - P_python| = %.2e\r\n\r\n",
                  correct, N_SAMPLES, same_as_python, N_SAMPLES, max_diff);
    i = correct = same_as_python = 0;
    max_diff = 0.0f;
  }
  delay(PERIOD_MS);
}
