#include <M5Unified.h>
#include <guardyou_inferencing.h>

// ── TinyML settings ──────────────────────────────────────
#define PICKUP_THRESHOLD     0.60f
static float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

// ── Timer state ───────────────────────────────────────────
int lastRotation        = -1;
unsigned long startTime = 0;
int timerMinutes        = 0;
long lastRemainingSeconds = -1;

// ── Phone detection state ─────────────────────────────────
bool phoneLifted        = false;
bool lastPhoneLifted    = false;

// ── Sliding window state ──────────────────────────────────
bool bufferReady        = false;

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Imu.begin();
    Serial.begin(115200);

    // Lower volume
    M5.Speaker.setVolume(96); // 0-255, default is 128

    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextFont(&fonts::Font4);
    M5.Display.setTextPadding(M5.Display.width());
    M5.Display.fillScreen(TFT_WHITE);

    // Pre-fill inference buffer
    M5.Display.setTextFont(&fonts::Font2);
    M5.Display.drawString("Loading...",
        M5.Display.width() / 2,
        M5.Display.height() / 2);

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

    bufferReady = true;
    M5.Display.fillScreen(TFT_WHITE);
}

// ── Run one sliding window inference ─────────────────────
void runInference() {
    size_t samples_required =
        EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE / EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;

    // Slide 25% — drop oldest quarter
    size_t quarter = (samples_required / 4) * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
    memmove(buffer, buffer + quarter,
            (EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - quarter) * sizeof(float));

    // Fill last 25% with fresh samples
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

    signal_t signal;
    numpy::signal_from_buffer(buffer,
        EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);

    ei_impulse_result_t result = { 0 };
    run_classifier(&signal, &result, false);

    // Find idle score (your pick-up class)
    float idle_score = 0.0f;
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.print(ei_classifier_inferencing_categories[i]);
        Serial.print(": ");
        Serial.println(result.classification[i].value, 4);
        if (String(ei_classifier_inferencing_categories[i]) == "idle") {
            idle_score = result.classification[i].value;
        }
    }
    Serial.println("---");

    phoneLifted = (idle_score > PICKUP_THRESHOLD);
}

// ── Draw timer screen ─────────────────────────────────────
void drawTimerScreen(long remainingSeconds, String label,
                     bool lifted, bool forceRedraw) {

    uint16_t bgColor = lifted ? TFT_RED : TFT_WHITE;
    uint16_t textColor = lifted ? TFT_WHITE : TFT_BLACK;

    // Full redraw on rotation change, lift state change, or forced
    if (forceRedraw || lifted != lastPhoneLifted) {
        M5.Display.fillScreen(bgColor);

        M5.Display.setTextFont(&fonts::Font4);
        M5.Display.setTextColor(textColor, bgColor);
        M5.Display.setTextSize(1);
        M5.Display.drawString(label,
            M5.Display.width() / 2,
            M5.Display.height() / 2 - 40);

        // Show warning text when lifted
        if (lifted) {
            M5.Display.setTextFont(&fonts::Font2);
            M5.Display.setTextSize(1);
            M5.Display.drawString("PUT IT DOWN!",
                M5.Display.width() / 2,
                M5.Display.height() / 2 + 55);
            M5.Display.setTextFont(&fonts::Font4);
        }

        lastPhoneLifted = lifted;
        lastRemainingSeconds = -1; // force timer number redraw
    }

    // Update timer numbers only when second changes
    if (remainingSeconds != lastRemainingSeconds) {
        lastRemainingSeconds = remainingSeconds;

        int displayMins = remainingSeconds / 60;
        int displaySecs = remainingSeconds % 60;
        char timeString[10];
        sprintf(timeString, "%02d:%02d", displayMins, displaySecs);

        // Timer done — solid red regardless of lift state
        if (remainingSeconds == 0) {
            M5.Display.fillScreen(TFT_RED);
            bgColor = TFT_RED;
            textColor = TFT_WHITE;
            M5.Speaker.tone(800, 500);
        }

        M5.Display.setTextFont(&fonts::Font4);
        M5.Display.setTextPadding(M5.Display.width());
        M5.Display.setTextColor(textColor, bgColor);
        M5.Display.setTextSize(3);
        M5.Display.drawString(timeString,
            M5.Display.width() / 2,
            M5.Display.height() / 2 + 10);
    }
}

void loop() {
    M5.update();

    // ── 1. Run inference (handles its own IMU reads) ──────
    if (bufferReady) {
        runInference();
    }

    // ── 2. Orientation detection ──────────────────────────
    M5.Imu.update();
    auto ImuData = M5.Imu.getImuData();
    float ax = ImuData.accel.x;
    float ay = ImuData.accel.y;

    int currentRotation = lastRotation;
    String label = "";
    int minutes = 0;

    if      (ay >  0.5) { currentRotation = 1; label = "FOCUS";     minutes = 25; }
    else if (ay < -0.5) { currentRotation = 3; label = "BREAK";     minutes = 5;  }
    else if (ax >  0.5) { currentRotation = 2; label = "RELAX";     minutes = 2;  }
    else if (ax < -0.5) { currentRotation = 0; label = "DEEP WORK"; minutes = 45; }

    bool rotationChanged = (currentRotation != lastRotation && currentRotation != -1);

    if (rotationChanged) {
        lastRotation  = currentRotation;
        timerMinutes  = minutes;
        startTime     = millis();
        lastPhoneLifted = !phoneLifted; // force redraw on rotation change
        M5.Display.setRotation(currentRotation);
    }

    // ── 3. Timer math ─────────────────────────────────────
    unsigned long elapsedMillis = millis() - startTime;
    long remainingSeconds = (timerMinutes * 60) - (elapsedMillis / 1000);
    if (remainingSeconds < 0) remainingSeconds = 0;

    // ── 4. Draw ───────────────────────────────────────────
    if (lastRotation != -1) {
        drawTimerScreen(remainingSeconds, label,
                        phoneLifted, rotationChanged);
    }

    // ── 5. Beep on pick-up trigger ────────────────────────
    if (phoneLifted && !lastPhoneLifted) {
        M5.Speaker.tone(1000, 200); // short low beep
    }
}