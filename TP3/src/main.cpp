#include "DHT.h"

// ------------------- Sensor Setup -------------------
#define DHTPIN 2      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

// ------------------- Model Parameters -------------------
#define N_FEATURES 2   // Two input features: temperature and humidity

const float MEAN[N_FEATURES]   = {25.125, 54.625};
const float STD[N_FEATURES]    = {4.56720648, 9.40661337};
const float WEIGHTS[N_FEATURES] = {0.90812293, -0.8877293};
const float BIAS = -0.03897119;

// ------------------- Variables -------------------
float X[N_FEATURES];  // Input vector

// ------------------- Helper Functions -------------------

// Standardize input features: (x - mean) / std
void standardize(float x[], const float mean[], const float std[], int size) {
  for (int i = 0; i < size; i++) {
    x[i] = (x[i] - mean[i]) / std[i];
  }
}

// Compute dot product: w · x
float dotProduct(const float a[], const float b[], int size) {
  float result = 0.0;
  for (int i = 0; i < size; i++) {
    result += a[i] * b[i];
  }
  return result;
}

// Sigmoid function
float sigmoid(float z) {
  return 1.0 / (1.0 + exp(-z));
}

// ------------------- Main Program -------------------
void setup() {
  Serial.begin(9600);
  Serial.println(F("DHT22 + Logistic Regression Test"));
  dht.begin();
}

void loop() {
  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Fill input features
  X[0] = t;
  X[1] = h;

  // Standardize inputs
  standardize(X, MEAN, STD, N_FEATURES);

  // Compute model output (z = w·x + b)
  float z = dotProduct(WEIGHTS, X, N_FEATURES) + BIAS;

  // Apply sigmoid to get prediction between 0 and 1
  float y_pred = sigmoid(z);

  // ------------------- Output -------------------
  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.print(F("°C | Humidity: "));
  Serial.print(h);
 Serial.print(F("% | Prediction: "));
Serial.print(y_pred * 100, 1);  // multiply by 100 and show 1 decimal
Serial.println(F("%"));


  delay(2000);
}
