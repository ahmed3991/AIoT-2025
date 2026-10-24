// TP3 - Logistic Regression fire alarm on the Arduino Uno (Wokwi)
// The model is the TP2 LR-Pipeline (StandardScaler + LogisticRegression on the 12 sensor features) retrained without
// the two sessions whose temperature is artificial (TP1, D.4), models/LR-Pipeline-TP3.pkl.
// Its parameters and the X values are printed by tools/train_export_lr.py.
// Alarm outputs: green LED on D4 (normal), red LED on D5 and buzzer on D8 (fire).
#include <math.h>
#include "DHT.h"
#define DHTPIN 2      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

// ---- Model parameters, in the training order of the features:
// Temperature_C, Humidity_%, TVOC_ppb, eCO2_ppm, Raw H2, Raw Ethanol, Pressure_hPa, PM1.0, PM2.5, NC0.5, NC1.0, NC2.5
const int N_FEATURES = 12;
const float MEAN[N_FEATURES] = {25.4014378, 48.1189194, 1903.88574, 733.665344, 12937.0908, 19759.1543, 938.494873, 122.786888, 235.373734, 574.235352, 260.711121, 107.619476};
const float STD[N_FEATURES] = {7.14452696, 9.53073692, 7702.45215, 2172.82153, 278.878326, 608.977661, 1.65632236, 1036.80627, 2324.08179, 4574.74219, 2611.97388, 1312.55261};
const float WEIGHTS[N_FEATURES] = {2.80974889, 3.15949154, -25.2313862, 5.31723785, 1.9516865, -5.6789794, -2.07346129, 0.562522411, 0.112168983, 1.06645966, 0.0816110298, -0.244422168};
const float BIAS = -0.42784512;
const float THRESHOLD = 0.5; // same decision rule as Pipeline.predict()

// ---- Alarm outputs
const int LED_NORMAL = 4; // green LED: on while the air is normal
const int LED_FIRE = 5;   // red LED: blinks while fire is predicted
const int BUZZER = 8;     // buzzer: beeps while fire is predicted
const int BEEP_HZ = 2000; // buzzer tone (Hz)
bool fire = false;        // last predicted class

// Input features: the DHT22 replaces X[0] (Temperature) and X[1] (Humidity).
// The 10 other sensors are not on the Uno, so they keep a clean-air value: the median of the normal training samples.
// (The template used the first row of the dataset, a gas-sensor warm-up reading that every model calls "fire".)
float X[N_FEATURES] = {26.3700008, 46.1899986, 86.0, 400.0, 12839.0, 20157.0, 937.479004, 1.45000005, 1.51999998, 9.94999981, 1.57500005, 0.0390000008};

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

// Wait ms milliseconds. While fire is predicted, blink the red LED and beep: 250 ms on, 250 ms off
void waitWithAlarm(unsigned long ms)
{
  for (unsigned long t = 0; t < ms; t += 250)
  {
    bool on = fire && (t / 250) % 2 == 0;
    digitalWrite(LED_FIRE, on ? HIGH : LOW);
    if (on)
      tone(BUZZER, BEEP_HZ);
    else
      noTone(BUZZER);
    delay(250);
  }
  noTone(BUZZER); // silent while the DHT22 is read: its timing is sensitive
}

void setup()
{
  Serial.begin(9600);
  Serial.println(F("TP3 - Logistic Regression fire alarm (Arduino Uno + DHT22)"));
  pinMode(LED_NORMAL, OUTPUT);
  pinMode(LED_FIRE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  dht.begin();
}

void loop()
{
  waitWithAlarm(2000);

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

  // 2. Standardize every feature with the mean and std of the scaler
  float x_scaled[N_FEATURES];
  for (int i = 0; i < N_FEATURES; i++)
  {
    x_scaled[i] = standardize(X[i], i);
  }

  // 3. Predict P(fire)
  float y_pred = predict(x_scaled);
  fire = (y_pred >= THRESHOLD);

  // 4. Print the readings, the probability and the class
  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.print(F(" C | Humidity: "));
  Serial.print(h);
  Serial.print(F(" % | Predicted Probability: "));
  Serial.print(y_pred, 4);
  if (fire)
    Serial.println(F(" | Predicted Class: 1 (FIRE)"));
  else
    Serial.println(F(" | Predicted Class: 0 (normal)"));

  // 5. Lights and sound: green LED for normal air; the red LED and the buzzer run in waitWithAlarm()
  digitalWrite(LED_NORMAL, fire ? LOW : HIGH);
}
