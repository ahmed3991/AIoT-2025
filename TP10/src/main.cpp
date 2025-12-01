/* TP10: MQTT-Based Audio Streaming for Edge Impulse Keyword Spotting */

#define EIDSP_QUANTIZE_FILTERBANK   0

#include <ahmed3991-Keyword-Spotting_inferencing.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>

// Configuration
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define MQTT_BROKER "broker.mqtt.cool"
#define MQTT_PORT 1883
#define MQTT_TOPIC "ei/audio/raw"
#define MQTT_CLIENT_ID "esp32_kws_simulator"
#define FIFO_BUFFER_SIZE 8192

// Global Objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Data Structures
typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

typedef struct {
    signed short *data;
    volatile uint32_t write_idx;
    volatile uint32_t read_idx;
    uint32_t capacity;
    volatile uint32_t samples_dropped;
} fifo_buffer_t;

// Global Buffers
static signed short fifo_data[FIFO_BUFFER_SIZE];
static fifo_buffer_t audio_fifo = { fifo_data, 0, 0, FIFO_BUFFER_SIZE, 0 };
static inference_t inference;
static bool debug_nn = false;
static int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
static unsigned long chunks_received = 0;

// Function Declarations
void setup_wifi();
void setup_mqtt();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void reconnect_mqtt();
static void audio_inference_callback(uint32_t n_bytes);
static bool microphone_inference_start(uint32_t n_samples);
static bool microphone_inference_record(void);
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr);

// FIFO Helper Functions
inline uint32_t fifo_available(fifo_buffer_t *fifo) {
    uint32_t w = fifo->write_idx;
    uint32_t r = fifo->read_idx;
    return (w >= r) ? (w - r) : (fifo->capacity - (r - w));
}

inline uint32_t fifo_free_space(fifo_buffer_t *fifo) {
    return fifo->capacity - fifo_available(fifo) - 1;
}

uint32_t fifo_write(fifo_buffer_t *fifo, const int16_t *samples, uint32_t count) {
    uint32_t written = 0;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t next_write = (fifo->write_idx + 1) % fifo->capacity;
        if (next_write == fifo->read_idx) {
            fifo->samples_dropped += (count - i);
            break;
        }
        fifo->data[fifo->write_idx] = samples[i];
        fifo->write_idx = next_write;
        written++;
    }
    return written;
}

uint32_t fifo_read(fifo_buffer_t *fifo, int16_t *dest, uint32_t count) {
    uint32_t read = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (fifo->read_idx == fifo->write_idx) break;
        dest[i] = fifo->data[fifo->read_idx];
        fifo->read_idx = (fifo->read_idx + 1) % fifo->capacity;
        read++;
    }
    return read;
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("TP10: MQTT-Based Edge Impulse KWS");

    // Init LCD
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("TP10: KWS");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");

    // Init Network
    setup_wifi();
    setup_mqtt();
    lcd.setCursor(0, 1);
    lcd.print("Connected!      ");

    // Init Classifier
    run_classifier_init();
    
    // Init Audio Buffer
    if (!microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE)) {
        Serial.println("ERR: Could not allocate audio buffer");
        while(1) delay(1000);
    }

    Serial.printf("Waiting for audio data via MQTT topic: %s\n", MQTT_TOPIC);
    Serial.printf("[MEM] Setup complete, free heap: %d bytes\n", ESP.getFreeHeap());
    
    lcd.setCursor(0, 1);
    lcd.print("Chunks: 0       ");
}

void loop() {
    if (!mqttClient.connected()) reconnect_mqtt();
    mqttClient.loop();

    // Wait for sufficient audio data
    if (!microphone_inference_record()) {
        inference.buf_ready = 0;
        return;
    }

    // Run Inference
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
    signal.get_data = &microphone_audio_signal_get_data;
    
    ei_impulse_result_t result = {0};
    
    Serial.printf("[MEM] Before inference: %d bytes\n", ESP.getFreeHeap());
    EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);
    Serial.printf("[MEM] After inference: %d bytes\n", ESP.getFreeHeap());
    
    if (r != EI_IMPULSE_OK) {
        Serial.printf("ERR: Failed to run classifier (%d)\n", r);
        return;
    }

    // Display Results
    if (++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)) {
        float max_confidence = 0.0;
        int max_index = 0;
        
        Serial.println("Predictions:");
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            Serial.print("    ");
            Serial.print(result.classification[ix].label);
            Serial.print(": ");
            Serial.println(result.classification[ix].value, 5);
            
            if (result.classification[ix].value > max_confidence) {
                max_confidence = result.classification[ix].value;
                max_index = ix;
            }
        }

        // Update LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Class: ");
        lcd.print(result.classification[max_index].label);
        lcd.setCursor(0, 1);
        lcd.print("Conf:");
        lcd.print((int)(max_confidence * 100));
        lcd.print("% Ch:");
        lcd.print(chunks_received);

        print_results = 0;
    }
}

void setup_wifi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println(WiFi.status() == WL_CONNECTED ? "\nWiFi connected!" : "\nWiFi failed!");
}

void setup_mqtt() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqtt_callback);
    mqttClient.setBufferSize(4096); // Ensure buffer is large enough
    reconnect_mqtt();
}

void reconnect_mqtt() {
    while (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT... ");
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            Serial.println("Connected!");
            mqttClient.subscribe(MQTT_TOPIC);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5s");
            delay(5000);
        }
    }
}

// MQTT Callback: Receives audio data and writes to FIFO
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    if (strcmp(topic, MQTT_TOPIC) != 0) return;

    unsigned int num_samples = length / 2;
    static int16_t temp_samples[2048];
    
    if (num_samples > 2048) num_samples = 2048;

    for (unsigned int i = 0; i < num_samples; i++) {
        temp_samples[i] = (int16_t)(payload[i*2] | (payload[i*2+1] << 8));
    }
    
    fifo_write(&audio_fifo, temp_samples, num_samples);
    chunks_received++;
    
    if (chunks_received % 10 == 0) {
        lcd.setCursor(13, 1); // Update chunk count on LCD
        lcd.print(chunks_received);
    }
}

// Reads from FIFO into inference buffer
static void audio_inference_callback(uint32_t n_bytes) {
    unsigned int samples_needed = n_bytes / 2;
    static int16_t temp_buffer[4096];
    
    if (samples_needed > 4096) samples_needed = 4096;
    
    uint32_t samples_read = fifo_read(&audio_fifo, temp_buffer, samples_needed);
    
    for (unsigned int i = 0; i < samples_read; i++) {
        inference.buffers[inference.buf_select][inference.buf_count++] = temp_buffer[i];
        if(inference.buf_count >= inference.n_samples) {
            inference.buf_select ^= 1;
            inference.buf_count = 0;
            inference.buf_ready = 1;
        }
    }
    
    // Fill remaining with silence
    for (unsigned int i = samples_read; i < samples_needed; i++) {
        inference.buffers[inference.buf_select][inference.buf_count++] = 0;
        if(inference.buf_count >= inference.n_samples) {
            inference.buf_select ^= 1;
            inference.buf_count = 0;
            inference.buf_ready = 1;
        }
    }
}

static bool microphone_inference_start(uint32_t n_samples) {
    inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));
    inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[0] == NULL || inference.buffers[1] == NULL) {
        free(inference.buffers[0]);
        free(inference.buffers[1]);
        return false;
    }

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;
    return true;
}

static bool microphone_inference_record(void) {
    if (inference.buf_ready == 1) {
        Serial.println("Warn: Buffer overrun");
        inference.buf_ready = 0;
        return false;
    }

    uint32_t available = fifo_available(&audio_fifo);
    unsigned long start_wait = millis();
    
    while (available < EI_CLASSIFIER_SLICE_SIZE) {
        mqttClient.loop();
        delay(1);
        available = fifo_available(&audio_fifo);
        if (millis() - start_wait > 5000) return false;
    }
    
    audio_inference_callback(EI_CLASSIFIER_SLICE_SIZE * 2);
    return true;
}

static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);
    return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif
