#include "DHT.h"

#define DHTPIN 2      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

const int N_FEATURES = 2; // فقط الحرارة والرطوبة في المثال

// إحصائيات البيانات (قيم تجريبية افتراضية)
const float MEAN[N_FEATURES] = {25.0, 60.0}; // المتوسط
const float STD[N_FEATURES] = {5.0, 10.0};   // الانحراف المعياري

// الأوزان والانحياز (افتراضية كمثال)
const float WEIGHTS[N_FEATURES] = {0.3, -0.2};
const float BIAS = 0.5;

float X[N_FEATURES];

void setup()
{
  Serial.begin(9600);
  Serial.println(F("DHT22 test + ML model!"));
  dht.begin();
}

void loop()
{
  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  X[0] = t;
  X[1] = h;

  // 1️⃣ توحيد القيم (Standardization)
  float X_std[N_FEATURES];
  for (int i = 0; i < N_FEATURES; i++)
  {
    X_std[i] = (X[i] - MEAN[i]) / STD[i];
  }

  // 2️⃣ حساب wx + b
  float linear = BIAS;
  for (int i = 0; i < N_FEATURES; i++)
  {
    linear += WEIGHTS[i] * X_std[i];
  }

  // 3️⃣ تطبيق دالة sigmoid
  float sigmoid = 1.0 / (1.0 + exp(-linear));

  // 4️⃣ طباعة النتائج
  Serial.print("Temp: ");
  Serial.print(t);
  Serial.print("°C  Humidity: ");
  Serial.print(h);
  Serial.print("%  -> Model Output: ");
  Serial.println(sigmoid, 4); // 4 منازل عشرية
}
