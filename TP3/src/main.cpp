#include "DHT.h"
#include <math.h>

#define SENSOR_PIN 2
#define SENSOR_TYPE DHT22

DHT climate(SENSOR_PIN, SENSOR_TYPE);

const int FIRE_LED = 13;
const int FEATURE_COUNT = 12;

const float AVG[FEATURE_COUNT]    = {-0.3464, -0.2900, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const float SCALE[FEATURE_COUNT]  = {18079.723677, 110001.609881, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
const float COEFFS[FEATURE_COUNT] = {1.900675, -1.407203, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const float OFFSET = 0;

float inputVec[FEATURE_COUNT] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0};

void normalizeData(float data[], int n) {
  for (int i = 0; i < n; i++) {
    if (SCALE[i] != 0) data[i] = (data[i] - AVG[i]) / SCALE[i];
  }
}

float weightedSum(float data[], float weights[], int n, float bias) {
  float sum = bias;
  for (int i = 0; i < n; i++) sum += weights[i] * data[i];
  return sum;
}

float sigmoidFunc(float x) {
  if (x < -20.0) return 0.0;
  if (x > 20.0) return 1.0;
  return 1.0 / (1.0 + exp(-x));
}

float infer(float data[], int n) {
  float z = weightedSum(data, COEFFS, n, OFFSET);
  return sigmoidFunc(z);
}

void ledControl(float prob) {
  digitalWrite(FIRE_LED, (prob >= 0.5) ? HIGH : LOW);
}

void setup() {
  Serial.begin(9600);
  climate.begin();
  pinMode(FIRE_LED, OUTPUT);
  digitalWrite(FIRE_LED, LOW);
  delay(1000);
}

void loop() {
  delay(2000);

  float humidity = climate.readHumidity();
  float tempC = climate.readTemperature();
  float tempF = climate.readTemperature(true);

  inputVec[0] = tempC;
  inputVec[1] = humidity;

  if (isnan(humidity) || isnan(tempC) || isnan(tempF)) {
    Serial.println("Sensor error!");
    for (int i = 0; i < 2; i++) {
      digitalWrite(FIRE_LED, HIGH); delay(200);
      digitalWrite(FIRE_LED, LOW);  delay(200);
    }
    return;
  }

  float standardized[FEATURE_COUNT];
  for (int i = 0; i < FEATURE_COUNT; i++) standardized[i] = inputVec[i];
  normalizeData(standardized, FEATURE_COUNT);

  float zVal = weightedSum(standardized, COEFFS, FEATURE_COUNT, OFFSET);
  float prediction = infer(standardized, FEATURE_COUNT);
  ledControl(prediction);

  Serial.println("\nFire Detection Model Output");
  Serial.print("Temp: "); Serial.print(tempC);
  Serial.print("°C, Humidity: "); Serial.print(humidity); Serial.println("%");
  Serial.print("Z-value: "); Serial.println(zVal, 4);
  Serial.print("Probability: "); Serial.println(prediction, 4);

  if (prediction >= 0.5) {
    Serial.println("Alert: FIRE DETECTED!");
    Serial.println("LED: ON");
  } else {
    Serial.println("Status: SAFE");
    Serial.println("LED: OFF");
  }

  Serial.print("Sensor Values -> ");
  Serial.print(tempC); Serial.print("°C (");
  Serial.print(tempF); Serial.print("°F), ");
  Serial.print(humidity); Serial.println("%");

  Serial.print("Standardized -> T: ");
  Serial.print(standardized[0], 6);
  Serial.print(", H: ");
  Serial.println(standardized[1], 6);
}
