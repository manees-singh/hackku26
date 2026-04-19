#include <M5Unified.h>
#include <guardyou_inferencing.h>
#include <WiFi.h>
#include <esp_now.h>
#include <math.h>

// ── ESP-NOW MAC addresses ─────────────────────────────────
uint8_t mac1[] = {0x2C, 0xBC, 0xBB, 0x94, 0x34, 0xC8};
uint8_t mac2[] = {0x2C, 0xBC, 0xBB, 0x93, 0x7E, 0xC4};
uint8_t peerAddress[6];

// ── ESP-NOW data struct ───────────────────────────────────
typedef struct struct_message {
    int level;
    bool taunt;
} struct_message;

struct_message myData;
struct_message peerData;

// ── Forward declarations ──────────────────────────────────
bool friendLevelChanged = false;
int myLevel             = 0;
int friendLevel         = 0;

// ── ESP-NOW receive callback ──────────────────────────────
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    memcpy(&peerData, incomingData, sizeof(peerData));
    Serial.printf("Received level %d taunt %d from peer\n", peerData.level, peerData.taunt);

    if (peerData.taunt) {
        // Play sound immediately in the callback
        M5.Speaker.setVolume(32);
        M5.Speaker.tone(400, 200);
        delay(220);
        M5.Speaker.tone(300, 400);
    } else {
        friendLevelChanged = true;
    }
}

// ── TinyML ────────────────────────────────────────────────
#define PICKUP_THRESHOLD 0.60f
static float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
bool bufferReady     = false;
bool phoneLifted     = false;
bool lastPhoneLifted = false;

// ── Timer state ───────────────────────────────────────────
int lastRotation          = -1;
unsigned long startTime   = 0;
int timerMinutes          = 0;
long lastRemainingSeconds = -1;
uint16_t themeColor       = TFT_ORANGE;
String currentLabel       = "";
bool sessionCompleted     = false;

// ── Pause state ───────────────────────────────────────────
bool timerPaused            = false;
unsigned long pausedAt      = 0;
unsigned long totalPausedMs = 0;

// ── Alarm buffer state ────────────────────────────────────
bool inAlarmBuffer             = false;
unsigned long alarmBufferStart = 0;
#define ALARM_BUFFER_MS   2500

// ── Colors ────────────────────────────────────────────────
#define POKEBALL_RED    M5.Display.color565(220, 30,  30)
#define POKEBALL_WHITE  M5.Display.color565(240, 240, 240)
#define POKEBALL_BLACK  M5.Display.color565(20,  20,  20)

// ── Work mode check ───────────────────────────────────────
bool isWorkMode() {
    return (currentLabel == "FOCUS" || currentLabel == "DEEP WORK");
}

// ── Draw a single pokeball ────────────────────────────────
void drawPokeball(int cx, int cy, int r) {
    M5.Display.fillCircle(cx, cy, r, POKEBALL_RED);
    M5.Display.fillArc(cx, cy, r, 0, 180, 360, POKEBALL_WHITE);
    M5.Display.drawFastHLine(cx - r, cy, r * 2, POKEBALL_BLACK);
    M5.Display.fillCircle(cx, cy, r / 3, POKEBALL_WHITE);
    M5.Display.drawCircle(cx, cy, r / 3, POKEBALL_BLACK);
    M5.Display.drawCircle(cx, cy, r, POKEBALL_BLACK);
}

// ── Draw pokeball bar ─────────────────────────────────────
void drawPokeballBar(int level, int corner) {
    int r       = 8;
    int spacing = 20;
    int maxShow = 5;

    int w = M5.Display.width();
    int h = M5.Display.height();

    if (corner == 0) {
        M5.Display.fillRect(w - (maxShow * spacing + 10), 2,
                            maxShow * spacing + 10, r * 2 + 6, TFT_BLACK);
        for (int i = 0; i < maxShow; i++) {
            int cx = w - 12 - (i * spacing);
            int cy = r + 6;
            if (i < level) {
                drawPokeball(cx, cy, r);
            } else {
                M5.Display.fillCircle(cx, cy, r, M5.Display.color565(30, 30, 30));
                M5.Display.drawCircle(cx, cy, r, M5.Display.color565(60, 60, 60));
            }
        }
    } else {
        M5.Display.fillRect(0, h - (r * 2 + 14),
                            maxShow * spacing + 10, r * 2 + 14, TFT_BLACK);
        for (int i = 0; i < maxShow; i++) {
            int cx = 12 + (i * spacing);
            int cy = h - r - 6;
            if (i < level) {
                drawPokeball(cx, cy, r);
            } else {
                M5.Display.fillCircle(cx, cy, r, M5.Display.color565(30, 30, 30));
                M5.Display.drawCircle(cx, cy, r, M5.Display.color565(60, 60, 60));
            }
        }
    }
}

// ── Redraw both bars ──────────────────────────────────────
void drawBothBars() {
    drawPokeballBar(myLevel,     0);
    drawPokeballBar(friendLevel, 1);
}

// ── Draw angry face ───────────────────────────────────────
void drawAngryFace() {
    M5.Display.fillScreen(TFT_RED);
    int cx = M5.Display.width() / 2;
    int cy = M5.Display.height() / 2;

    M5.Display.fillTriangle(cx - 75, cy - 65, cx - 15, cy - 30, cx - 80, cy - 55, TFT_BLACK);
    M5.Display.fillTriangle(cx + 75, cy - 65, cx + 15, cy - 30, cx + 80, cy - 55, TFT_BLACK);

    M5.Display.fillRect(cx - 60, cy - 25, 30, 20, TFT_RED);
    M5.Display.fillRect(cx - 50, cy - 20, 10, 10, TFT_BLACK);
    M5.Display.drawRect(cx - 60, cy - 25, 30, 20, TFT_BLACK);

    M5.Display.fillRect(cx + 30, cy - 25, 30, 20, TFT_RED);
    M5.Display.fillRect(cx + 40, cy - 20, 10, 10, TFT_BLACK);
    M5.Display.drawRect(cx + 30, cy - 25, 30, 20, TFT_BLACK);

    M5.Display.fillArc(cx, cy + 30, 20, 15, 200, 340, TFT_BLACK);

    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextColor(TFT_WHITE, TFT_RED);
    M5.Display.drawString("TIMER PAUSED!", cx, cy + 65);
    M5.Display.setTextSize(2);
    M5.Display.drawString("PUT IT DOWN!", cx, cy + 95);
    M5.Display.setTextSize(1);
}

// ── Draw tick ring ────────────────────────────────────────
void drawTicks(long remaining, long total, uint16_t color) {
    int cx     = M5.Display.width() / 2;
    int cy     = M5.Display.height() / 2;
    int innerR = 85;
    int outerR = 105;
    int activeTicks = map(remaining, 0, total, 0, 60);

    for (int i = 0; i < 60; i++) {
        float angle = (i * 6 - 90) * (M_PI / 180.0);
        int x0 = cx + cos(angle) * innerR;
        int y0 = cy + sin(angle) * innerR;
        int x1 = cx + cos(angle) * outerR;
        int y1 = cy + sin(angle) * outerR;
        uint16_t drawColor = (i < activeTicks) ? color : M5.Display.color565(30, 30, 30);
        M5.Display.drawLine(x0, y0, x1, y1, drawColor);
    }
}

// ── Draw timer screen ─────────────────────────────────────
void drawTimerScreen(long remainingSeconds, long totalSecs, bool forceRedraw) {
    if (forceRedraw) {
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.setFont(&fonts::Font2);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextColor(themeColor, TFT_BLACK);
        M5.Display.drawString(currentLabel,
            M5.Display.width() / 2,
            M5.Display.height() / 2 - 55);
        lastRemainingSeconds = -1;
        drawBothBars();
    }

    if (remainingSeconds != lastRemainingSeconds) {
        lastRemainingSeconds = remainingSeconds;
        drawTicks(remainingSeconds, totalSecs, themeColor);

        int displayMins = remainingSeconds / 60;
        int displaySecs = remainingSeconds % 60;
        char timeString[10];
        sprintf(timeString, "%02d:%02d", displayMins, displaySecs);

        bool timerDone = (remainingSeconds == 0);

        if (timerDone && !sessionCompleted) {
            sessionCompleted = true;
            M5.Display.fillScreen(TFT_BLACK);

            if (currentLabel == "RELAX") {
                myLevel++;
                myData.level = myLevel;
                myData.taunt = false;
                esp_now_send(peerAddress, (uint8_t *)&myData, sizeof(myData));

                M5.Speaker.setVolume(32);
                M5.Speaker.tone(1200, 150);
                delay(180);
                M5.Speaker.tone(1500, 150);
                delay(180);
                M5.Speaker.tone(1800, 300);

                M5.Display.setFont(&fonts::Font4);
                M5.Display.setTextDatum(middle_center);
                M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
                M5.Display.drawString("LEVEL UP!",
                    M5.Display.width() / 2,
                    M5.Display.height() / 2 - 20);
                M5.Display.setFont(&fonts::Font2);
                M5.Display.drawString("Lv " + String(myLevel),
                    M5.Display.width() / 2,
                    M5.Display.height() / 2 + 20);
                delay(2000);

                M5.Display.fillScreen(TFT_BLACK);
                M5.Display.setFont(&fonts::Font2);
                M5.Display.setTextColor(themeColor, TFT_BLACK);
                M5.Display.setTextDatum(middle_center);
                M5.Display.drawString(currentLabel,
                    M5.Display.width() / 2,
                    M5.Display.height() / 2 - 55);
                lastRemainingSeconds = -1;
                drawBothBars();
                return;
            } else {
                M5.Speaker.setVolume(32);
                M5.Speaker.tone(800, 400);
            }
        }

        M5.Display.setFont(&fonts::Font7);
        M5.Display.setTextSize(1);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextColor(TFT_WHITE, timerDone ? TFT_RED : TFT_BLACK);
        if (timerDone) M5.Display.fillScreen(TFT_RED);
        M5.Display.drawString(timeString,
            M5.Display.width() / 2,
            M5.Display.height() / 2 + 15);

        drawBothBars();
    }
}

// ── TinyML sliding window inference ───────────────────────
void runInference() {
    size_t samples_required =
        EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE / EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;

    size_t quarter = (samples_required / 4) * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
    memmove(buffer, buffer + quarter,
            (EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - quarter) * sizeof(float));

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

    float idle_score = 0.0f;
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (String(ei_classifier_inferencing_categories[i]) == "idle") {
            idle_score = result.classification[i].value;
        }
    }

    phoneLifted = (idle_score > PICKUP_THRESHOLD);
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Imu.begin();
    Serial.begin(115200);
    M5.Speaker.setVolume(32);

    // ── ESP-NOW init ──────────────────────────────────────
    WiFi.mode(WIFI_STA);

    uint8_t baseMac[6];
    WiFi.macAddress(baseMac);
    if (memcmp(baseMac, mac1, 6) == 0) {
        memcpy(peerAddress, mac2, 6);
    } else {
        memcpy(peerAddress, mac1, 6);
    }

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
    } else {
        esp_now_register_recv_cb(OnDataRecv);
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, peerAddress, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        esp_now_add_peer(&peerInfo);
        Serial.println("ESP-NOW ready");

        // ── Startup sync ──────────────────────────────────
        delay(500);
        myData.level = myLevel;
        myData.taunt = false;
        esp_now_send(peerAddress, (uint8_t *)&myData, sizeof(myData));
    }

    myData.level   = 0;
    myData.taunt   = false;
    peerData.level = 0;
    peerData.taunt = false;

    // ── Display init ──────────────────────────────────────
    M5.Display.setTextDatum(middle_center);
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString("Loading AI...",
        M5.Display.width() / 2,
        M5.Display.height() / 2);

    // ── Pre-fill inference buffer ─────────────────────────
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
    M5.Display.fillScreen(TFT_BLACK);
}

void loop() {
    M5.update();

    // ── Reset button — BtnA resets level to 0 ────────────
    if (M5.BtnA.wasPressed()) {
        myLevel       = 0;
        myData.level  = 0;
        myData.taunt  = false;
        esp_now_send(peerAddress, (uint8_t *)&myData, sizeof(myData));
        drawBothBars();
    }

    // ── Friend level update from ESP-NOW ──────────────────
    if (friendLevelChanged) {
        friendLevel        = peerData.level;
        friendLevelChanged = false;
        drawPokeballBar(friendLevel, 1);
    }

    // ── 1. Inference ──────────────────────────────────────
    if (bufferReady) {
        runInference();
    }

    // ── 2. Orientation ────────────────────────────────────
    M5.Imu.update();
    auto ImuData = M5.Imu.getImuData();
    float ax = ImuData.accel.x;
    float ay = ImuData.accel.y;

    int currentRotation = lastRotation;
    String newLabel     = currentLabel;
    int minutes         = 0;

    if      (ay >  0.5) { currentRotation = 1; newLabel = "FOCUS";     minutes = 25; themeColor = TFT_RED;     }
    else if (ay < -0.5) { currentRotation = 3; newLabel = "BREAK";     minutes = 5;  themeColor = TFT_GREEN;   }
    else if (ax >  0.5) { currentRotation = 2; newLabel = "RELAX";     minutes = 2;  themeColor = TFT_BLUE;    }
    else if (ax < -0.5) { currentRotation = 0; newLabel = "DEEP WORK"; minutes = 45; themeColor = TFT_MAGENTA; }

    bool rotationChanged = (currentRotation != lastRotation && currentRotation != -1);

    if (rotationChanged) {
        lastRotation     = currentRotation;
        currentLabel     = newLabel;
        timerMinutes     = minutes;
        startTime        = millis();
        totalPausedMs    = 0;
        timerPaused      = false;
        inAlarmBuffer    = false;
        sessionCompleted = false;
        lastPhoneLifted  = !phoneLifted;
        M5.Display.setRotation(currentRotation);
        drawTimerScreen(timerMinutes * 60, timerMinutes * 60, true);
    }

    // ── 3. Alarm buffer expiry ────────────────────────────
    if (inAlarmBuffer && (millis() - alarmBufferStart >= ALARM_BUFFER_MS)) {
        inAlarmBuffer  = false;
        totalPausedMs += ALARM_BUFFER_MS;
        timerPaused    = false;

        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.setFont(&fonts::Font2);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextColor(themeColor, TFT_BLACK);
        M5.Display.drawString(currentLabel,
            M5.Display.width() / 2,
            M5.Display.height() / 2 - 55);
        lastRemainingSeconds = -1;
        drawBothBars();
    }

    // ── 4. Pause / resume ─────────────────────────────────
    if (isWorkMode() && !inAlarmBuffer) {
        bool justLifted   = phoneLifted && !lastPhoneLifted;
        bool justReturned = !phoneLifted && lastPhoneLifted;

        if (justLifted && !timerPaused) {
            timerPaused      = true;
            pausedAt         = millis();
            inAlarmBuffer    = true;
            alarmBufferStart = millis();
            drawAngryFace();
            M5.Speaker.setVolume(32);
            M5.Speaker.tone(1000, 300);
        }

        if (justReturned && timerPaused && !inAlarmBuffer) {
            totalPausedMs += (millis() - pausedAt);
            timerPaused    = false;

            M5.Display.fillScreen(TFT_BLACK);
            M5.Display.setFont(&fonts::Font2);
            M5.Display.setTextDatum(middle_center);
            M5.Display.setTextColor(themeColor, TFT_BLACK);
            M5.Display.drawString(currentLabel,
                M5.Display.width() / 2,
                M5.Display.height() / 2 - 55);
            lastRemainingSeconds = -1;
            drawBothBars();
        }
    }

    lastPhoneLifted = phoneLifted;

    // ── 5. Timer math ─────────────────────────────────────
    long totalSecs = timerMinutes * 60;
    unsigned long effectiveElapsed = millis() - startTime - totalPausedMs;

    if (timerPaused || inAlarmBuffer) {
        effectiveElapsed = pausedAt - startTime - totalPausedMs;
    }

    long remainingSeconds = totalSecs - (long)(effectiveElapsed / 1000);
    if (remainingSeconds < 0) remainingSeconds = 0;

    // ── 6. Draw ───────────────────────────────────────────
    if (lastRotation != -1 && !timerPaused && !inAlarmBuffer) {
        drawTimerScreen(remainingSeconds, totalSecs, false);
    }

    delay(100);
}