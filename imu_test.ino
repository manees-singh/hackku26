#include <M5Unified.h>

int lastRotation = -1;
unsigned long startTime = 0;
int timerMinutes = 0;
long lastRemainingSeconds = -1;

void setup(void) {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextFont(&fonts::Font4);
  
  // Padding ensures that new text "overwrites" old text perfectly
  // We set it to the width of the screen to be safe
  M5.Display.setTextPadding(M5.Display.width());
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

    // 1. Orientation Detection
    if (ay > 0.5)      { currentRotation = 1; label = "RELAX";     minutes = 2;  } 
    else if (ay < -0.5) { currentRotation = 3; label = "BREAK";     minutes = 5;  } 
    else if (ax > 0.5)  { currentRotation = 2; label = "FOCUS";     minutes = 25; } 
    else if (ax < -0.5) { currentRotation = 0; label = "DEEP WORK"; minutes = 45; }

    // 2. Only Redraw everything on a Flip
    if (currentRotation != lastRotation && currentRotation != -1) {
      lastRotation = currentRotation;
      timerMinutes = minutes;
      startTime = millis(); 
      
      M5.Display.setRotation(currentRotation);
      M5.Display.fillScreen(TFT_WHITE); // Full clear ONLY on rotation change
      
      // Draw static label once
      M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
      M5.Display.setTextSize(1);
      M5.Display.drawString(label, M5.Display.width() / 2, M5.Display.height() / 2 - 40);
      
      lastRemainingSeconds = -1; // Force timer redraw
    }

    // 3. Timer Math
    unsigned long elapsedMillis = millis() - startTime;
    long remainingSeconds = (timerMinutes * 60) - (elapsedMillis / 1000);
    if (remainingSeconds < 0) remainingSeconds = 0;

    // 4. Only Update the numbers if the second has changed
    if (remainingSeconds != lastRemainingSeconds) {
      lastRemainingSeconds = remainingSeconds;

      int displayMins = remainingSeconds / 60;
      int displaySecs = remainingSeconds % 60;
      char timeString[10];
      sprintf(timeString, "%02d:%02d", displayMins, displaySecs);

      // Change background color to Red only when time is up
      uint16_t bgColor = (remainingSeconds == 0) ? TFT_RED : TFT_WHITE;
      if (remainingSeconds == 0) M5.Display.fillScreen(TFT_RED);

      M5.Display.setTextColor(TFT_BLACK, bgColor); // (Text, Background)
      M5.Display.setTextSize(3);
      
      // drawString with Padding will only overwrite the number area
      M5.Display.drawString(timeString, M5.Display.width() / 2, M5.Display.height() / 2 + 10);
    }
  }
  delay(100); // Faster polling for orientation, but drawing is throttled by the "if" statement
}