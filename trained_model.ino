#include <M5Stack.h>
#include <guardyou_inferencing.h> // Your AI library

// Settings
#define CONVERT_G_TO_MS2    9.80665f
static const bool debug_nn = false; 

void setup() {
    M5.begin();
    M5.Power.begin();
    M5.IMU.Init(); // Initialize the M5GO's internal accelerometer
    
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("GUARDIAN ACTIVE");
    M5.Lcd.println("Status: Stable");
}

void loop() {
    // 1. Create a buffer to hold the 1.5s of data the AI expects
    float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };

    // 2. Fill the buffer with live data from the M5GO IMU
    for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME) {
        int64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);

        float ax, ay, az, gx, gy, gz;
        M5.IMU.getAccelData(&ax, &ay, &az);
        M5.IMU.getGyroData(&gx, &gy, &gz);

        // Fill buffer (Must match the order you used in Edge Impulse: accX, accY, accZ, gyrX...)
        buffer[ix + 0] = ax * CONVERT_G_TO_MS2;
        buffer[ix + 1] = ay * CONVERT_G_TO_MS2;
        buffer[ix + 2] = az * CONVERT_G_TO_MS2;
        buffer[ix + 3] = gx;
        buffer[ix + 4] = gy;
        buffer[ix + 5] = gz;

        int64_t wait_time = next_tick - micros();
        if(wait_time > 0) delayMicroseconds(wait_time);
    }

    // 3. Run the AI Classifier
    signal_t signal;
    numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    ei_impulse_result_t result = { 0 };
    run_classifier(&signal, &result, debug_nn);

    // 4. Logic: What to do if the phone is stolen?
    // Based on your 100% accuracy, 'pickup' is at index [0] or [1]
    // Check Serial Monitor to see which index 'pickup' is!
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        if (strcmp(result.classification[ix].label, "pickup") == 0) {
            if (result.classification[ix].value > 0.85) {
                triggerAlarm();
            } else {
                resetDisplay();
            }
        }
    }
}

void triggerAlarm() {
    M5.Lcd.fillScreen(RED);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(60, 100);
    M5.Lcd.setTextSize(4);
    M5.Lcd.println("STOLEN!");
    M5.Speaker.beep(); // Annoy the thief
}

void resetDisplay() {
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.setTextColor(GREEN, BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Status: Stable");
}