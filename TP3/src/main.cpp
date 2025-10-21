#include "DHT.h"
#define DHTPIN 2      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

const int N_FEATURES = 13;
const float MEAN[N_FEATURES] = { 15.953115080632283, 48.57430185214753, 1922.8843405716111, 667.7439126616638, 12942.300973974134, 19754.250459045186, 938.6319484871467, 100.88728524668689, 186.0739208446432, 490.20927071690875, 205.45779255149284, 81.31975353265209, 10520.92878812071 };
const float STD[N_FEATURES]  = { 14.358038477662703, 8.82118774162391, 7755.895054463778, 1903.7822129584472, 271.16860881062263, 606.9759523422715, 1.3245144335765542, 925.3035296966241, 1991.52788902251, 4259.415669501792, 2232.484760401612, 1095.2900289597956, 7604.5956649702775 };
const float WEIGHTS[N_FEATURES] = { -0.46544457249057913, 1.022852158816893, -29.074778234970264, 4.640371616627808, 0.4125583219918355, -4.348073381069025, -1.8530283632145381, 0.6868801244477326, 0.21310455197534284, 1.165998454335439, 0.1791877930692523, -0.1949601909160403, 16.461431704695936 };
const float BIAS =  11.842098895256767; /* b */

float X[N_FEATURES] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0}; // Input features


// دالة التقييس
float standardize(float x_raw, int idx) {
  return (x_raw - MEAN[idx]) / STD[idx];
}

// دالة Sigmoid
float sigmoid(float z) {
  return 1.0 / (1.0 + exp(-z));
}

// دالة التوقع
float predict(float features[]) {
  float z = 0.0;
  for (int i = 0; i < N_FEATURES; i++) {
    z += WEIGHTS[i] * features[i];
  }
  z += BIAS;
  return sigmoid(z);
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("DHT22 + Logistic Regression Fire Alarm"));
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

  X[0] = t;  // Temperature
  X[1] = h;  // Humidity

  // Standardize all features
  float X_scaled[N_FEATURES];
  for (int i = 0; i < N_FEATURES; i++) {
    X_scaled[i] = standardize(X[i], i);
  }

  // Predict probability
  float y_pred = predict(X_scaled);

  Serial.print("Predicted Probability: ");
  Serial.println(y_pred, 4);

  if (y_pred >= 0.5)
    Serial.println("Predicted Class: 1 (Fire Alarm ON)");
  else
    Serial.println("Predicted Class: 0 (Normal)");

  Serial.print("Humidity: "); Serial.print(h);
  Serial.print("%  Temperature: "); Serial.print(t); Serial.println("°C");
}