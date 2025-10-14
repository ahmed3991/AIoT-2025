
#include "DHT.h"
#define DHTPIN 2      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

const int N_FEATURES = 12;
const float MEAN[N_FEATURES] = {/* μ_Temperature, μ_Humidity */};
const float STD[N_FEATURES] = {/* σ_Temperature, σ_Humidity */};
const float WEIGHTS[N_FEATURES] = {/* W_Temperature, W_Humidity */};
const float BIAS = 0; /* b */

float X[N_FEATURES] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0}; // Input features

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