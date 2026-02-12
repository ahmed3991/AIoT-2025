#include <Arduino.h>
#include "DHT.h"
#include <math.h>

#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const int N_FEATURES = 12;

const float MEAN[N_FEATURES] = {24.5, 55.0, 0, 400, 12300, 18500, 940.0, 0, 0, 0, 0, 0};
const float STD[N_FEATURES] = {2.3, 6.1, 1, 50, 1000, 2000, 50.0, 1, 1, 1, 1, 1};
const float WEIGHTS[N_FEATURES] = {0.85, -0.47, 0.12, 0.03, -0.01, 0.02, 0.05, 0, 0, 0, 0, 0};
const float BIAS = -0.22;

float X[N_FEATURES] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0};

float standardize(float x_raw, int idx)
{
  return (x_raw - MEAN[idx]) / STD[idx];
}

float sigmoid(float z)
{
  return 1.0 / (1.0 + exp(-z));
}

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
  Serial.println(F("DHTxx + Logistic Regression Test"));
  dht.begin();
}

void loop()
{
  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t) || isnan(f))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  X[0] = t;
  X[1] = h;

  float X_scaled[N_FEATURES];
  for (int i = 0; i < N_FEATURES; i++)
  {
    X_scaled[i] = standardize(X[i], i);
  }

  float y_pred = predict(X_scaled);

  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" °C | Humidity: ");
  Serial.print(h);
  Serial.print(" % | Predicted Probability: ");
  Serial.println(y_pred, 4);

  if (y_pred >= 0.5)
    Serial.println("Predicted Class: 1");
  else
    Serial.println("Predicted Class: 0");

  Serial.println("-------------------------------");
}