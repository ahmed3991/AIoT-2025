
#include "DHT.h"
#define DHTPIN 2      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

const int N_FEATURES = 12;

const float MEAN[N_FEATURES] = {
  15.9531, 48.5743, 1922.8843, 667.7439, 12942.301, 19754.2505,
  938.6319, 100.8873, 186.0739, 490.2093, 205.4578, 81.3198
};

const float STD[N_FEATURES] = {
  14.358, 8.8212, 7755.8951, 1903.7822, 271.1686, 606.976,
  1.3245, 925.3035, 1991.5279, 4259.4157, 2232.4848, 1095.29
};

const float WEIGHTS[N_FEATURES] = {
  -1.1063, 1.3782, -18.2296, 5.9192, 3.3515, -7.8964,
  -2.4279, -0.3308, -0.1946, -0.4543, -0.1843, -0.0682
};

const float BIAS = -0.3844;

float X[N_FEATURES] = {20.073, 54.12,	0,	400,	12419,	18998,	939.725,	0,	0,	0,	0,	0}; // Input features

void setup()
{
  Serial.begin(9600);
  Serial.println(F("DHTxx test!"));
  dht.begin();
}

void loop()
{
  delay(2000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // add data to input array
  X[0] = t;
  X[1] = h;

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // TODO: Add code to standardize the inputs

  // TODO: Add code to compute the output of wx + b

  // TODO: Add code to apply the sigmoid function

  // TODO: Add code to print the result to the serial monitor

  // Compute heat index in Fahrenheit (the default)
  // float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  // float hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print("%  Tempeature: ");
  Serial.print(t);
  Serial.print("°C ");
  Serial.println(f);
  // Serial.print(F("°F  Heat index: "));
  // Serial.print(hic);
  // Serial.print(F("°C "));
  // Serial.print(hif);
  // Serial.println(F("°F"));
}