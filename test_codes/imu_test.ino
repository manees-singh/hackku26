#include <M5Unified.h>
#include <math.h>

int lastRotation = -1;
unsigned long startTime = 0;
int timerMinutes = 0;
long lastRemainingSeconds = -1;
uint16_t themeColor = TFT_ORANGE; // Default

void drawTicks(int remaining, int total, uint16_t color) {
  // Use dynamic width/height so it stays centered on tilt
  int cx = M5.Display.width() / 2;
  int cy = M5.Display.height() / 2;
  
  // Adjusted radii to push them "outward" from the text
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

void setup(void) {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setTextDatum(middle_center);
}

void loop(void) {
  M5.update();
  if (M5.Imu.update()) {
    auto ImuData = M5.Imu.getImuData();
    float ax = ImuData.accel.x;
    float ay = ImuData.accel.y;

    int currentRotation = lastRotation;
    String label = "";
    int minutes = 0;

    // Association Colors: Focus(Red), Break(Green), Relax(Blue), Deep(Purple)
    if (ay > 0.5)      { currentRotation = 1; label = "FOCUS";     minutes = 25; themeColor = TFT_RED; } 
    else if (ay < -0.5) { currentRotation = 3; label = "BREAK";     minutes = 5;  themeColor = TFT_GREEN; } 
    else if (ax > 0.5)  { currentRotation = 2; label = "RELAX";     minutes = 2;  themeColor = TFT_BLUE; } 
    else if (ax < -0.5) { currentRotation = 0; label = "DEEP WORK"; minutes = 45; themeColor = TFT_MAGENTA; }

    if (currentRotation != lastRotation && currentRotation != -1) {
      lastRotation = currentRotation;
      timerMinutes = minutes;
      startTime = millis(); 
      M5.Display.setRotation(currentRotation);
      M5.Display.fillScreen(TFT_BLACK);
      
      // Smaller font for label
      M5.Display.setFont(&fonts::Font2); 
      M5.Display.setTextColor(themeColor, TFT_BLACK);
      M5.Display.drawString(label, M5.Display.width() / 2, M5.Display.height() / 2 - 55);
      
      lastRemainingSeconds = -1;
    }

    unsigned long elapsedMillis = millis() - startTime;
    long totalSecs = timerMinutes * 60;
    long remainingSeconds = totalSecs - (elapsedMillis / 1000);
    if (remainingSeconds < 0) remainingSeconds = 0;

    if (remainingSeconds != lastRemainingSeconds) {
      lastRemainingSeconds = remainingSeconds;
      drawTicks(remainingSeconds, totalSecs, themeColor);

      int displayMins = remainingSeconds / 60;
      int displaySecs = remainingSeconds % 60;
      char timeString[10];
      sprintf(timeString, "%02d:%02d", displayMins, displaySecs);

      // Use Font 7 for a clean "Digital" look
      M5.Display.setFont(&fonts::Font7);
      M5.Display.setTextSize(1); // Font 7 is already large
      M5.Display.setTextColor(TFT_WHITE, (remainingSeconds == 0) ? TFT_RED : TFT_BLACK);
      M5.Display.drawString(timeString, M5.Display.width() / 2, M5.Display.height() / 2 + 15);
    }
  }
  delay(100);
}