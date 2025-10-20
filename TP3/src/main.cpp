#include "DHT.h"
#include <math.h>

#define DHTPIN 2      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

const int N_FEATURES = 12;

const float MEAN[N_FEATURES] = {
  15.9531, 48.5743, 1922.8843, 667.7439, 12942.301, 19754.2505,
  938.6319, 100.8873, 186.0739, 490.2093, 205.4578, 81.3198
};

const float STD[N_FEATURES] = {
  14.358, 8.8212, 7755.8951, 1903.7822, 271.1686, 606.976,
  1.3245, 925.3035, 1991.5279, 4259.4157, 2232.4848, 1095.29
};

const float WEIGHTS[N_FEATURES] = {
  -1.1063, 1.3782, -18.2296, 5.9192, 3.3515, -7.8964,
  -2.4279, -0.3308, -0.1946, -0.4543, -0.1843, -0.0682
};

const float BIAS = -0.3844;

float X[N_FEATURES] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0}; // Input features

// Standardization Function 
float standardize(float x_raw, int idx) {
  return (x_raw - MEAN[idx]) / STD[idx];
}

// Sigmoid Function 
float sigmoid(float z) {
  return 1.0 / (1.0 + exp(-z));
}

// Prediction Function 
float predict(float features[]) {
  float z = 0.0;
  for (int i = 0; i < N_FEATURES; i++) {
    z += WEIGHTS[i] * features[i];
  }
  z += BIAS;
  return sigmoid(z);
}

void setup()
{
  Serial.begin(9600);
  Serial.println(F("DHTxx test with Logistic Regression"));
  dht.begin();
}

void loop()
{
  delay(2000);

  // Reading temperature or humidity takes about 250 milliseconds!
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  // add data to input array
  X[0] = t; // Feature 0 → Temperature
  X[1] = h; // Feature 1 → Humidity

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  //  Standarization features
  float X_scaled[N_FEATURES];
  for (int i = 0; i < N_FEATURES; i++) {
    X_scaled[i] = standardize(X[i], i);
  }

  //  Prediction
  float y_pred = predict(X_scaled);

  //  Output Results
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print("%  Temperature: ");
  Serial.print(t);
  Serial.print("°C  ");
  Serial.print("Predicted Probability: ");
  Serial.println(y_pred, 4);

  if (y_pred >= 0.5) {
    Serial.println("Predicted Class: 1 (Positive)");
  } else {
    Serial.println("Predicted Class: 0 (Negative)");
  }

}