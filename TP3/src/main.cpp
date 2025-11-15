#include "DHT.h"
#include <math.h>

#define DHTPIN 2      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

// === MODEL PARAMETERS ===
const int N_FEATURES = 2;
const float MEAN[N_FEATURES] = {15.92865516, 48.56115819};
const float STD[N_FEATURES] = {14.36205192, 8.84192962};
const float WEIGHTS[N_FEATURES] = {-0.20914643, 0.97220885};
const float BIAS = 0.9716077270501117;

// === FUNCTIONS ===
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

// === MAIN PROGRAM ===
void setup()
{
  Serial.begin(9600);
  Serial.println(F("=== DHTxx Logistic Regression Debug Test ==="));
  dht.begin();
  randomSeed(analogRead(0)); // for simulated variation if needed
}

void loop()
{
  delay(2000);

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // In Wokwi, DHT may not give real readings, so simulate small changes if needed
  if (isnan(humidity) || isnan(temperature) || humidity == 0.0 || temperature == 0.0)
  {
    temperature = 20.0 + random(-50, 50) * 0.1; // random 15-25C
    humidity = 40.0 + random(-100, 100) * 0.1;  // random 30-50%
    Serial.println(F("[DEBUG] Using simulated values (Wokwi mode)"));
  }

  // === DEBUG OUTPUT ===
  Serial.println(F("\n--- SENSOR INPUT ---"));
  Serial.print("Raw Temperature: ");
  Serial.println(temperature);
  Serial.print("Raw Humidity: ");
  Serial.println(humidity);

  float x_scaled[N_FEATURES];
  x_scaled[0] = standardize(temperature, 0);
  x_scaled[1] = standardize(humidity, 1);

  Serial.println(F("--- NORMALIZED FEATURES ---"));
  Serial.print("Norm Temperature: ");
  Serial.println(x_scaled[0]);
  Serial.print("Norm Humidity: ");
  Serial.println(x_scaled[1]);

  float z = WEIGHTS[0] * x_scaled[0] + WEIGHTS[1] * x_scaled[1] + BIAS;
  Serial.print("Linear Combination (z): ");
  Serial.println(z, 6);

  float y_pred = predict(x_scaled);

  Serial.println(F("--- MODEL OUTPUT ---"));
  Serial.print("Predicted Probability: ");
  Serial.println(y_pred, 4);
  Serial.print("Predicted Class: ");
  Serial.println(y_pred >= 0.5 ? "1" : "0");

  // Warning if probability is always stuck
 if (fabs(y_pred - 0.02) < 0.001)
{
  Serial.println(F("[WARNING] Probability stuck near 0.02 - check input variation."));
}
}
