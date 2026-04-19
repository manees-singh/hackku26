#include <M5Unified.h>

// Settings
#define SAMPLING_FREQ_HZ    100
#define SAMPLING_PERIOD_MS  (1000 / SAMPLING_FREQ_HZ)
#define NUM_SAMPLES         150       // 1.5 second of data
#define CONVERT_G_TO_MS2    9.80665f  // Standard gravity conversion

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // Setup Display
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextFont(&fonts::Font2);
  M5.Display.drawString("READY: Press BTN A", M5.Display.width() / 2, M5.Display.height() / 2);

  Serial.begin(115200);
}

void loop() {
  M5.update();

  // Wait for Button A (Left side) to be pressed
  if (M5.BtnA.wasPressed()) {
    
    // UI: Turn screen Red to indicate recording
    M5.Display.fillScreen(TFT_RED);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawString("RECORDING...", M5.Display.width() / 2, M5.Display.height() / 2);

    // Print CSV Header - Crucial for Edge Impulse CSV Uploader
    Serial.println("timestamp,accX,accY,accZ,gyrX,gyrY,gyrZ");

    unsigned long start_timestamp = millis();

    for (int i = 0; i < NUM_SAMPLES; i++) {
      unsigned long sample_start_time = millis();

      // Update IMU and get latest data
      M5.Imu.update();
      auto imuData = M5.Imu.getImuData();

      // Get Timestamp relative to start of recording
      unsigned long current_timestamp = millis() - start_timestamp;

      // Print data in CSV format
      // Note: Shawn's script uses m/s^2 for acceleration
      Serial.print(current_timestamp);
      Serial.print(",");
      Serial.print(imuData.accel.x * CONVERT_G_TO_MS2);
      Serial.print(",");
      Serial.print(imuData.accel.y * CONVERT_G_TO_MS2);
      Serial.print(",");
      Serial.print(imuData.accel.z * CONVERT_G_TO_MS2);
      Serial.print(",");
      Serial.print(imuData.gyro.x);
      Serial.print(",");
      Serial.print(imuData.gyro.y);
      Serial.print(",");
      Serial.println(imuData.gyro.z);

      // Wait to maintain 100Hz sampling rate
      while (millis() - sample_start_time < SAMPLING_PERIOD_MS);
    }

    // Print empty line to signal end of block (for some python helper scripts)
    Serial.println();

    // UI: Return to Ready state
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.drawString("DONE! Ready for next.", M5.Display.width() / 2, M5.Display.height() / 2);
  }
}