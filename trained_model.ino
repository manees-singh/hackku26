#include <M5Unified.h>
#include <guardyou_inferencing.h>

#define PICKUP_THRESHOLD 0.50f

static float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Imu.begin();
    Serial.begin(115200);

    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextFont(&fonts::Font2);
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawString("Initializing...",
        M5.Display.width() / 2,
        M5.Display.height() / 2);

    // Pre-fill entire buffer before first inference
    size_t samples_required =
        EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE / EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;

    for (size_t i = 0; i < samples_required; i++) {
        M5.Imu.update();
        auto imu = M5.Imu.getImuData();
        size_t ix = i * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
        buffer[ix + 0] = imu.accel.x;
        buffer[ix + 1] = imu.accel.y;
        buffer[ix + 2] = imu.accel.z;
        buffer[ix + 3] = imu.gyro.x;
        buffer[ix + 4] = imu.gyro.y;
        buffer[ix + 5] = imu.gyro.z;
        delay(EI_CLASSIFIER_INTERVAL_MS);
    }
}

void loop() {
    size_t samples_required =
        EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE / EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;

    // Slide buffer left by 25% — drop oldest quarter of samples
    size_t quarter = (samples_required / 4) * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
    memmove(buffer, buffer + quarter,
            (EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - quarter) * sizeof(float));

    // Fill the last 25% with fresh samples
    size_t fill_start = (samples_required * 3) / 4;
    for (size_t i = fill_start; i < samples_required; i++) {
        M5.Imu.update();
        auto imu = M5.Imu.getImuData();
        size_t ix = i * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
        buffer[ix + 0] = imu.accel.x;
        buffer[ix + 1] = imu.accel.y;
        buffer[ix + 2] = imu.accel.z;
        buffer[ix + 3] = imu.gyro.x;
        buffer[ix + 4] = imu.gyro.y;
        buffer[ix + 5] = imu.gyro.z;
        delay(EI_CLASSIFIER_INTERVAL_MS);
    }

    // Run classifier
    signal_t signal;
    numpy::signal_from_buffer(buffer,
        EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE,
        &signal);

    ei_impulse_result_t result = { 0 };
    run_classifier(&signal, &result, false);

    // Get scores
    float idle_score = 0.0f;
    float stable_score = 0.0f;

    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        String label = String(ei_classifier_inferencing_categories[i]);
        if (label == "idle")   idle_score   = result.classification[i].value;
        if (label == "stable") stable_score = result.classification[i].value;

        Serial.print(label);
        Serial.print(": ");
        Serial.println(result.classification[i].value, 4);
    }
    Serial.println("---");

    // Update screen immediately
    if (idle_score > PICKUP_THRESHOLD) {
        M5.Display.fillScreen(TFT_RED);
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.drawString("PUT IT DOWN!",
            M5.Display.width() / 2,
            M5.Display.height() / 2 - 20);
        String score = "conf: " + String(idle_score, 2);
        M5.Display.drawString(score,
            M5.Display.width() / 2,
            M5.Display.height() / 2 + 20);
        M5.Speaker.tone(1000, 300);
        Serial.println("*** ALARM ***");
    } else {
        M5.Display.fillScreen(TFT_DARKGREEN);
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.drawString("FOCUSING",
            M5.Display.width() / 2,
            M5.Display.height() / 2 - 20);
        String score = "conf: " + String(stable_score, 2);
        M5.Display.drawString(score,
            M5.Display.width() / 2,
            M5.Display.height() / 2 + 20);
    }
}