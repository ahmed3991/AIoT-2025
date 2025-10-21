#include "DHT.h"
#include <math.h>

#define DHTPIN 2       
#define DHTTYPE DHT22  
DHT dht(DHTPIN, DHTTYPE);

#define MQ2_PIN A0     
#define FLAME_PIN 3    

const int N_FEATURES = 12;

const float MEAN[N_FEATURES] = {
  31297.363165, 1654791844.021415, 15.953115, 48.574302,
  1922.884341, 667.743913, 12942.300974, 19754.250459,
  938.631948, 100.887285, 186.073921, 490.209271
};

const float STD[N_FEATURES] = {
  18071.764509, 109669.276420, 14.358038, 8.821188,
  7755.895054, 1903.782213, 271.168609, 606.975952,
  1.324514, 925.303530, 1991.527889, 4259.415670
};

const float WEIGHTS[N_FEATURES] = {
  -1.373408, -0.342746, -0.212932, 1.180184,
  -22.874995, 0.849484, -0.068718, -3.561514,
  -2.865623, 1.926110, 0.944893, 2.861934
};

const float BIAS = 17.515389;

float X[N_FEATURES] = {20.0, 57.36, 0, 400, 12306, 18520, 939.735, 0.0, 0.0, 0.0, 0.0, 0.0}; // Input features

/*
20.0,57.36,0,400
,12306,18520,939.735,0.0
,0.0,0.0,0.0,0.0
*/

void setup()
{
  Serial.begin(9600);
  Serial.println(F("🔥 Fire Detection System — Based on Professor's Model"));
  dht.begin();
  pinMode(MQ2_PIN, INPUT);
  pinMode(FLAME_PIN, INPUT);
}

void loop() {
  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("⚠️ Failed to read from DHT sensor, using previous values."));
  } else {
    X[0] = t;  
    X[1] = h;  
  }

  int mq2_value = analogRead(MQ2_PIN);
  int flame_value = digitalRead(FLAME_PIN);

  X[2] = mq2_value;    
  X[3] = flame_value * 500; 
 

  float prob = predict(X);

  bool alarm = false;
  if (prob >= 0.6) {
    if (mq2_value > 400 || flame_value == 1 || t > 45.0) {
      alarm = true;
    }
  }

  Serial.println("🌡️ Reading Sensors:");
  Serial.print("Temperature: "); Serial.print(t); Serial.print(" °C | ");
  Serial.print("Humidity: "); Serial.print(h); Serial.println(" %");
  Serial.print("MQ2: "); Serial.print(mq2_value);
  Serial.print(" | Flame: "); Serial.println(flame_value);
  Serial.print("Predicted probability: "); Serial.println(prob, 6);

  if (alarm)
    Serial.println("🔥 FIRE ALARM: ON");
  else
    Serial.println("✅ FIRE ALARM: OFF");

  Serial.println("--------------------------------------------");
}
