#include "DHT.h"
#include <math.h>

#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

const int N_FEATURES = 2;
const float MEAN[N_FEATURES] = {25.0, 50.0};
const float STD[N_FEATURES]  = {5.0, 20.0};
const float WEIGHTS[N_FEATURES] = {-1.10434114, 1.37397825};
const float BIAS = -0.3836474233535921;

float sigmoid(float x) {
  return 1.0 / (1.0 + exp(-x));
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("DHT22 AI Comfort Model (Temperature & Humidity)"));
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

  float t_scaled = (t - MEAN[0]) / STD[0];
  float h_scaled = (h - MEAN[1]) / STD[1];
  float z = WEIGHTS[0] * t_scaled + WEIGHTS[1] * h_scaled + BIAS;
  float y = sigmoid(z);

  Serial.print("Temperature: ");
  Serial.print(t, 2);
  Serial.print(" °C, Humidity: ");
  Serial.print(h, 2);
  Serial.print("% => Model Output: ");
  Serial.println(y, 6);

  if (y > 0.7)
    Serial.println("Comfortable environment");
  else if (y > 0.4)
    Serial.println("Slightly uncomfortable");
  else
    Serial.println("Uncomfortable environment");

  Serial.println("-----------------------------");
}
