#include "DHT.h"
#include <math.h> // for exp()
#include <Arduino.h>

#define DHTPIN 2      // Pin where DHT sensor is connected
#define DHTTYPE DHT22 // Using DHT22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

// ====== MODEL CONSTANTS ======
const int N_FEATURES = 15;

// mean and std (scaler parameters)
const float MEAN[N_FEATURES] = {
    31314.5f, 1654792070.0f, 15.9704f, 48.5395f, 1942.06f,
    670.021f, 12942.5f, 19754.3f, 938.627f, 100.594f,
    184.467f, 491.463f, 203.586f, 80.049f, 10511.4f};

const float STD[N_FEATURES] = {
    18079.7f, 110001.6f, 14.359f, 8.865f, 7811.53f,
    1905.87f, 272.462f, 609.508f, 1.331f, 922.516f,
    1976.29f, 4265.63f, 2214.72f, 1083.37f, 7597.81f};

// Model weights and bias
const float WEIGHTS[N_FEATURES] = {
    -1.2203f, -0.3955f, -0.2912f, 1.1930f, -23.7886f,
    1.7593f, 0.0894f, -3.6160f, -2.8001f, 1.3027f,
    0.5672f, 2.0138f, 0.5136f, -0.0871f, 19.9811f};

const float BIAS = 16.3585f;

// Input feature vector (15 values)
float X[N_FEATURES] = {
    1.0f, 1654733332.0f, 20.015f, 56.67f, 0.0f, 400.0f,
    12345.0f, 18651.0f, 939.744f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f
};

// ----- helper: standardize one value -----
float normalize(float raw, int i)
{
  return (raw - MEAN[i]) / STD[i];
}

// ----- helper: sigmoid activation -----
float sigmoid(float z)
{
  return 1.0f / (1.0f + exp(-z));
}

void setup()
{
  Serial.begin(9600);
  Serial.println(F("Starting Fire Alarm Prediction..."));
  dht.begin();
}

void loop()
{
  delay(2000);

  float hum = dht.readHumidity();
  float temp = dht.readTemperature();

  if (isnan(hum) || isnan(temp))
  {
    Serial.println(F("Sensor error! Skipping reading."));
    return;
  }

  // update live sensor features
  X[2] = temp;
  X[3] = hum;

  // normalize inputs
  float scaled[N_FEATURES];
  for (int i = 0; i < N_FEATURES; i++)
  {
    scaled[i] = normalize(X[i], i);
  }

  // compute linear combination (z = w·x + b)
  float z = BIAS;
  for (int i = 0; i < N_FEATURES; i++)
  {
    z += WEIGHTS[i] * scaled[i];
  }

  // apply sigmoid
  float prob = sigmoid(z);

  // output results
  Serial.print(F("Temp: "));
  Serial.print(temp);
  Serial.print(F(" °C | Humidity: "));
  Serial.print(hum);
  Serial.println(F(" %"));

  Serial.print(F("Fire Probability: "));
  Serial.println(prob, 4);

  if (prob >= 0.5)
  {
    Serial.println(F("Fire Alarm Triggered!"));
  }
  else
  {
    Serial.println(F("Environment Normal."));
  }

  Serial.println("-----------------------");
}
