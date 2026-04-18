/*
 * TinyML Phone Guardian — M5Go
 *
 * TWO PHASES:
 *   Set MODE to DATA_COLLECTION, flash, collect training data via Edge Impulse.
 *   Set MODE to INFERENCE after importing the generated EI library.
 *
 * Edge Impulse setup:
 *   1. npm install -g edge-impulse-cli
 *   2. Flash with MODE = DATA_COLLECTION
 *   3. Run: edge-impulse-data-forwarder --frequency 100
 *      Name axes: accX,accY,accZ
 *   4. Record labeled samples in EI Studio:
 *        "idle"    — 2+ min normal desk activity / typing
 *        "pickup"  — 30+ reps lifting phone off desk
 *        "putdown" — 30+ reps setting phone on desk
 *   5. Create Impulse: Spectral Analysis block + Neural Network classifier
 *   6. Train, then Export > Arduino Library
 *   7. In Arduino IDE: Sketch > Include Library > Add .ZIP Library
 *   8. Change MODE to INFERENCE and reflash
 */

#include <M5Unified.h>

// ── Toggle between phases ────────────────────────────────────────────────────
#define DATA_COLLECTION 0
#define INFERENCE       1
#define MODE            DATA_COLLECTION   // <── change to INFERENCE after training
// ─────────────────────────────────────────────────────────────────────────────

#if MODE == INFERENCE
  // Replace with the actual header name Edge Impulse generates for your project
  #include <phone_guardian_inferencing.h>
#endif

#define SAMPLE_RATE_HZ       100
#define SAMPLE_INTERVAL_MS   (1000 / SAMPLE_RATE_HZ)

// Inference window: 2 seconds of 3-axis data = 600 floats
#define WINDOW_SAMPLES       200
#define FEATURE_SIZE         (WINDOW_SAMPLES * 3)

// Confidence threshold — raise to reduce false positives
#define CONFIDENCE_THRESHOLD 0.70f

// ── State machine ────────────────────────────────────────────────────────────
enum GuardianState { IDLE, DANGER };
GuardianState state = IDLE;

// ── Shared buffers ────────────────────────────────────────────────────────────
float   features[FEATURE_SIZE];
int     feature_idx    = 0;
unsigned long last_sample_ms = 0;

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setTextFont(&fonts::Font2);
  M5.Display.setTextSize(1);

#if MODE == DATA_COLLECTION
  M5.Display.clear(TFT_BLACK);
  M5.Display.setTextColor(TFT_GREEN);
  M5.Display.setCursor(0, 0);
  M5.Display.println("Data Collection");
  M5.Display.println("100 Hz  accX,Y,Z");
  M5.Display.println("");
  M5.Display.println("Run:");
  M5.Display.println("ei-data-forwarder");
#else
  M5.Display.clear(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.println("Guardian READY");
  M5.Display.println("Watching desk...");
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();
  if (now - last_sample_ms < SAMPLE_INTERVAL_MS) return;
  last_sample_ms = now;

  M5.Imu.update();
  auto imu = M5.Imu.getImuData();
  float ax = imu.accel.x;
  float ay = imu.accel.y;
  float az = imu.accel.z;

// ── Phase 1: stream CSV to Edge Impulse data forwarder ───────────────────────
#if MODE == DATA_COLLECTION
  Serial.print(ax, 4); Serial.print(',');
  Serial.print(ay, 4); Serial.print(',');
  Serial.println(az, 4);

// ── Phase 2: run inference on a sliding window ───────────────────────────────
#else
  // Fill the feature buffer
  features[feature_idx++] = ax;
  features[feature_idx++] = ay;
  features[feature_idx++] = az;

  if (feature_idx < FEATURE_SIZE) return;   // window not full yet

  // --- Run classifier ---
  signal_t signal;
  numpy::signal_from_buffer(features, FEATURE_SIZE, &signal);

  ei_impulse_result_t result = { 0 };
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
  if (err != EI_IMPULSE_OK) {
    Serial.printf("Classifier error: %d\n", err);
    goto slide_window;
  }

  // Find highest-confidence label
  {
    float    max_val = 0;
    uint32_t max_idx = 0;
    for (uint32_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
      if (result.classification[i].value > max_val) {
        max_val = result.classification[i].value;
        max_idx = i;
      }
    }

    const char* label = result.classification[max_idx].label;
    Serial.printf("[%s] %.2f\n", label, max_val);

    if (max_val >= CONFIDENCE_THRESHOLD) {
      if (strcmp(label, "pickup") == 0 && state == IDLE) {
        state = DANGER;
        showState();
      } else if (strcmp(label, "putdown") == 0 && state == DANGER) {
        state = IDLE;
        showState();
      }
    }
  }

  slide_window:
  // 50% overlap — slide window forward by half its length
  int half = FEATURE_SIZE / 2;
  memmove(features, features + half, half * sizeof(float));
  feature_idx = half;
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
#if MODE == INFERENCE
void showState() {
  M5.Display.clear(state == DANGER ? TFT_RED : TFT_WHITE);
  M5.Display.setTextColor(state == DANGER ? TFT_WHITE : TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(0, 50);
  M5.Display.println(state == DANGER ? "  PHONE" : "  PHONE");
  M5.Display.println(state == DANGER ? "  PICKED UP!" : "  RETURNED");
  Serial.println(state == DANGER ? "STATE → DANGER" : "STATE → IDLE");
}
#endif
