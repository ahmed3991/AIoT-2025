
#include <Arduino.h>
#include <DHT.h>
#include <math.h>

// ==========================================
// 1. DHT22 Sensor Configuration
// ==========================================
#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// ==========================================
// 2. Logistic Regression Model Parameters
// Feature order: Temperature, Humidity
// IMPORTANT: Replace these values with the
// actual parameters extracted from Python.
// ==========================================

const int N_FEATURES = 2;

// StandardScaler mean_
const float MEAN[N_FEATURES] = {
    31.350938f,
    40.726445f
};

// StandardScaler scale_
// Use scale_, NOT var_ or sqrt(var_) incorrectly.
const float STD[N_FEATURES] = {
    9.210854f,
    16.888109f
};

// LogisticRegression coef_[0]
const float WEIGHTS[N_FEATURES] = {
    3.401914f,
   -1.696985f
};

// LogisticRegression intercept_[0]
const float BIAS = 13.062134f;

// Classification threshold
const float THRESHOLD = 0.5f;

// ==========================================
// 3. Standardization
// ==========================================
float standardize(float rawValue, int index)
{
    return (rawValue - MEAN[index]) / STD[index];
}

// ==========================================
// 4. Stable Sigmoid Function
// ==========================================
float sigmoid(float z)
{
    if (z >= 0.0f)
    {
        return 1.0f / (1.0f + expf(-z));
    }

    float ez = expf(z);
    return ez / (1.0f + ez);
}

// ==========================================
// 5. Logistic Regression Prediction
// ==========================================
float predictProbability(const float features[])
{
    float z = BIAS;

    for (int i = 0; i < N_FEATURES; i++)
    {
        z += WEIGHTS[i] * features[i];
    }

    return sigmoid(z);
}

// ==========================================
// 6. Setup
// ==========================================
void setup()
{
    Serial.begin(9600);
    dht.begin();

    delay(2000);

    Serial.println();
    Serial.println(F("=========================================="));
    Serial.println(F(" LOGISTIC REGRESSION - FIRE DETECTION"));
    Serial.println(F(" Sensor: DHT22"));
    Serial.println(F(" Board : Arduino Uno"));
    Serial.println(F("=========================================="));
    Serial.println(F("Features: Temperature, Humidity"));
    Serial.println(F("Starting sensor readings..."));
}

// ==========================================
// 7. Main Loop
// ==========================================
void loop()
{
    // DHT22 requires a reading interval
    delay(2000);

    // Read sensor
    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

    // Validate readings
    if (isnan(temp) || isnan(hum))
    {
        Serial.println(F("[ERROR] Failed to read DHT22."));
        Serial.println(F("Check wiring, power and sensor type."));
        return;
    }

    // Validate physical reading ranges
    if (temp < -40.0f || temp > 80.0f ||
        hum < 0.0f || hum > 100.0f)
    {
        Serial.println(F("[ERROR] Sensor reading out of range."));
        return;
    }

    // Standardize in the SAME order as Python:
    // Temperature first, Humidity second
    float x_scaled[N_FEATURES];

    x_scaled[0] = standardize(temp, 0);
    x_scaled[1] = standardize(hum, 1);

    // Calculate probability for class 1
    float probability = predictProbability(x_scaled);

    // Binary classification
    int predicted_class =
        (probability >= THRESHOLD) ? 1 : 0;

    // ======================================
    // 8. Display Results
    // ======================================
    Serial.println();
    Serial.println(F("------------------------------------------"));

    Serial.print(F("Temperature : "));
    Serial.print(temp, 2);
    Serial.println(F(" C"));

    Serial.print(F("Humidity    : "));
    Serial.print(hum, 2);
    Serial.println(F(" %"));

    Serial.print(F("Scaled Temp : "));
    Serial.println(x_scaled[0], 4);

    Serial.print(F("Scaled Hum  : "));
    Serial.println(x_scaled[1], 4);

    Serial.print(F("Probability : "));
    Serial.print(probability, 4);
    Serial.print(F(" ("));
    Serial.print(probability * 100.0f, 2);
    Serial.println(F("%)"));

    Serial.print(F("Predicted Class: "));
    Serial.println(predicted_class);

    if (predicted_class == 1)
    {
        Serial.println(F("Prediction  : FIRE ALARM"));
    }
    else
    {
        Serial.println(F("Prediction  : SAFE"));
    }

    Serial.println(F("------------------------------------------"));
}
