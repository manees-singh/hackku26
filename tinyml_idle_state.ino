#include <M5Unified.h>

#define SAMPLING_FREQ_HZ    100
#define SAMPLING_PERIOD_MS  (1000 / SAMPLING_FREQ_HZ)
#define NUM_SAMPLES         150
#define CONVERT_G_TO_MS2    9.80665f
#define TOTAL_SAMPLES       30
#define GAP_BETWEEN_MS      2000  // 2 second gap between samples

void collectSample(int sampleNum) {
  // UI: show sample number and recording state
  M5.Display.fillScreen(TFT_RED);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextFont(&fonts::Font2);
  
  String msg = "REC " + String(sampleNum) + "/" + String(TOTAL_SAMPLES);
  M5.Display.drawString(msg, M5.Display.width() / 2, M5.Display.height() / 2);

  // Print CSV header
  Serial.println("timestamp,accX,accY,accZ,gyrX,gyrY,gyrZ");

  unsigned long start_timestamp = millis();

  for (int i = 0; i < NUM_SAMPLES; i++) {
    unsigned long sample_start_time = millis();

    M5.Imu.update();
    auto imuData = M5.Imu.getImuData();
    unsigned long current_timestamp = millis() - start_timestamp;

    Serial.print(current_timestamp); Serial.print(",");
    Serial.print(imuData.accel.x * CONVERT_G_TO_MS2); Serial.print(",");
    Serial.print(imuData.accel.y * CONVERT_G_TO_MS2); Serial.print(",");
    Serial.print(imuData.accel.z * CONVERT_G_TO_MS2); Serial.print(",");
    Serial.print(imuData.gyro.x); Serial.print(",");
    Serial.print(imuData.gyro.y); Serial.print(",");
    Serial.println(imuData.gyro.z);

    while (millis() - sample_start_time < SAMPLING_PERIOD_MS);
  }

  // Two empty lines — signal end of sample to Python script
  Serial.println();
  Serial.println();

  // UI: show done + countdown to next
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE);
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextFont(&fonts::Font2);
  M5.Display.setTextColor(TFT_WHITE);
  Serial.begin(115200);

  // 3 second countdown before starting so you can get ready
  for (int i = 3; i > 0; i--) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.drawString("Starting in " + String(i), M5.Display.width() / 2, M5.Display.height() / 2);
    delay(1000);
  }
}

void loop() {
  for (int s = 1; s <= TOTAL_SAMPLES; s++) {

    // Countdown before each sample
    for (int c = 2; c > 0; c--) {
      M5.Display.fillScreen(TFT_BLACK);
      M5.Display.drawString("Get ready: " + String(c), M5.Display.width() / 2, M5.Display.height() / 2);
      delay(1000);
    }

    // Collect the sample
    collectSample(s);

    // Brief pause after sample
    M5.Display.drawString("Done " + String(s) + "/" + String(TOTAL_SAMPLES), 
                           M5.Display.width() / 2, M5.Display.height() / 2);
    delay(500);
  }

  // All done
  M5.Display.fillScreen(TFT_GREEN);
  M5.Display.setTextColor(TFT_BLACK);
  M5.Display.drawString("ALL DONE!", M5.Display.width() / 2, M5.Display.height() / 2);

  // Stop forever
  while (true) delay(1000);
}