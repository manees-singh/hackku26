#include <M5Unified.h>

int lastRotation = -1;
unsigned long startTime = 0;
int timerMinutes = 0;

void setup(void) {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextFont(&fonts::Font4);
}

void loop(void) {
  M5.update();
  auto imu_update = M5.Imu.update();

  if (imu_update) {
    auto ImuData = M5.Imu.getImuData();
    float ax = ImuData.accel.x;
    float ay = ImuData.accel.y;

    int currentRotation = lastRotation;
    String label = "";
    int minutes = 0;

    // Determine orientation and assign timer values
    if (ay > 0.5) {
      currentRotation = 1; 
      label = "RELAX";
      minutes = 2;
    } else if (ay < -0.5) {
      currentRotation = 3; 
      label = "BREAK";
      minutes = 5;
    } else if (ax > 0.5) {
      currentRotation = 2; 
      label = "FOCUS";
      minutes = 25;
    } else if (ax < -0.5) {
      currentRotation = 0; 
      label = "DEEP WORK";
      minutes = 45;
    }

    // Check if the device was flipped to a NEW side
    if (currentRotation != lastRotation && currentRotation != -1) {
      lastRotation = currentRotation;
      timerMinutes = minutes;
      startTime = millis(); // Reset the timer
      M5.Display.setRotation(currentRotation);
    }

    // Calculate remaining time
    unsigned long elapsedMillis = millis() - startTime;
    long remainingSeconds = (timerMinutes * 60) - (elapsedMillis / 1000);

    if (remainingSeconds < 0) remainingSeconds = 0;

    int displayMins = remainingSeconds / 60;
    int displaySecs = remainingSeconds % 60;

    // Drawing
    M5.Display.startWrite(); // Faster UI updates
    M5.Display.fillScreen(remainingSeconds == 0 ? TFT_RED : TFT_WHITE);
    M5.Display.setTextColor(TFT_BLACK);
    
    int centerX = M5.Display.width() / 2;
    int centerY = M5.Display.height() / 2;

    M5.Display.setTextSize(1);
    M5.Display.drawString(label, centerX, centerY - 40);
    
    M5.Display.setTextSize(3);
    M5.Display.printf("%02d:%02d", displayMins, displaySecs);
    M5.Display.endWrite();
  }
  delay(200); 
}