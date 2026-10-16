// TP3 - Logistic Regression fire alarm on the Arduino Uno (Wokwi)
// The model is the LR-Pipeline trained in TP2 (StandardScaler + LogisticRegression on the 12 sensor features,
// models/LR-Pipeline.pkl). Its parameters are printed by tools/export_lr_params.py.
#include <math.h>
#include "DHT.h"
#define DHTPIN 2      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

// ---- Model parameters, in the training order of the features:
// Temperature_C, Humidity_%, TVOC_ppb, eCO2_ppm, Raw H2, Raw Ethanol, Pressure_hPa, PM1.0, PM2.5, NC0.5, NC1.0, NC2.5
const int N_FEATURES = 12;
const float MEAN[N_FEATURES] = {15.9531155, 48.5743027, 1922.8844, 667.743896, 12942.3008, 19754.25, 938.631958, 100.887283, 186.073914, 490.209259, 205.457794, 81.3197556};
const float STD[N_FEATURES] = {14.3580389, 8.82118797, 7755.89502, 1903.78223, 271.16861, 606.975952, 1.32451439, 925.303528, 1991.52783, 4259.41553, 2232.48486, 1095.29004};
const float WEIGHTS[N_FEATURES] = {-1.10630012, 1.37815046, -18.2296009, 5.91924238, 3.35154843, -7.89639568, -2.42789745, -0.330777943, -0.19455418, -0.454262137, -0.184319511, -0.0682086498};
const float BIAS = -0.384436756;
const float THRESHOLD = 0.5; // same decision rule as Pipeline.predict()

// Input features: the DHT22 replaces X[0] (Temperature) and X[1] (Humidity); the 10 other sensors keep these values
float X[N_FEATURES] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0};

// x_scaled = (x_raw - mu) / sigma
float standardize(float x_raw, int idx)
{
  return (x_raw - MEAN[idx]) / STD[idx];
}

// y = 1 / (1 + e^-z)
float sigmoid(float z)
{
  return 1.0 / (1.0 + exp(-z));
}

// z = sum_i W_i * x_scaled_i + b, then P(fire) = sigmoid(z)
float predict(float features[])
{
  float z = 0.0;
  for (int i = 0; i < N_FEATURES; i++)
  {
    z += WEIGHTS[i] * features[i];
  }
  z += BIAS;
  return sigmoid(z);
}

void setup()
{
  Serial.begin(9600);
  Serial.println(F("TP3 - Logistic Regression fire alarm (Arduino Uno + DHT22)"));
  dht.begin();
}

void loop()
{
  delay(2000);

  // 1. Read the DHT sensor (about 250 ms; the values may be up to 2 s old)
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Celsius

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // add data to input array (feature 0 = Temperature, feature 1 = Humidity)
  X[0] = t;
  X[1] = h;

  // 2. Standardize every feature with the mean and std of the TP2 scaler
  float x_scaled[N_FEATURES];
  for (int i = 0; i < N_FEATURES; i++)
  {
    x_scaled[i] = standardize(X[i], i);
  }

  // 3. Predict P(fire)
  float y_pred = predict(x_scaled);

  // 4. Print the readings, the probability and the class
  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.print(F(" C | Humidity: "));
  Serial.print(h);
  Serial.print(F(" % | Predicted Probability: "));
  Serial.print(y_pred, 4);
  if (y_pred >= THRESHOLD)
    Serial.println(F(" | Predicted Class: 1 (FIRE)"));
  else
    Serial.println(F(" | Predicted Class: 0 (normal)"));
}
