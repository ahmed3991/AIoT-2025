#include <Arduino.h>
#include <DHT.h>
#include <math.h>

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

#define N_FEATURES 15

const float MEAN[N_FEATURES] = {
  31314.5f, 1654792070.0f, 15.9704f, 48.5395f, 1942.06f,
  670.021f, 12942.5f, 19754.3f, 938.627f, 100.594f,
  184.467f, 491.463f, 203.586f, 80.049f, 10511.4f
};

const float STD[N_FEATURES] = {
  18079.7f, 110001.6f, 14.359f, 8.865f, 7811.53f,
  1905.87f, 272.462f, 609.508f, 1.331f, 922.516f,
  1976.29f, 4265.63f, 2214.72f, 1083.37f, 7597.81f
};

const float WEIGHTS[N_FEATURES] = {
  -1.2203f, -0.3955f, -0.2912f, 1.1930f, -23.7886f,
  1.7593f, 0.0894f, -3.6160f, -2.8001f, 1.3027f,
  0.5672f, 2.0138f, 0.5136f, -0.0871f, 19.9811f
};

const float BIAS = 16.3585f;

float X[N_FEATURES] = {
  1.0f, 1654733332.0f, 20.015f, 56.67f, 0.0f, 400.0f,
  12345.0f, 18651.0f, 939.744f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f
};

static inline float zscore(float v, float m, float s) {
  if (s == 0.0f) return 0.0f;
  return (v - m) / s;
}

static inline float sigmoidf_fast(float x) {
  return 1.0f / (1.0f + expf(-x));
}

static float predict_proba(const float *x_raw) {
  float sum = BIAS;
  for (int i = 0; i < N_FEATURES; ++i) {
    float xi = zscore(x_raw[i], MEAN[i], STD[i]);
    sum += WEIGHTS[i] * xi;
  }
  return sigmoidf_fast(sum);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(t)) X[2] = t;
  if (!isnan(h)) X[3] = h;

  float p = predict_proba(X);
  int cls = (p >= 0.5f) ? 1 : 0;

  Serial.print("T=");
  Serial.print(X[2], 2);
  Serial.print("C, H=");
  Serial.print(X[3], 2);
  Serial.print("%, p=");
  Serial.print(p, 6);
  Serial.print(", alarm=");
  Serial.println(cls);

  delay(1000);
}
