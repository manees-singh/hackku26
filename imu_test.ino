#include <M5Unified.h>

void setup(void) {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  // Set text alignment to middle-center to make rotation cleaner
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

    // Determine rotation based on gravity (accelerometer)
    // 0: Portrait, 1: Landscape, 2: Portrait Inverted, 3: Landscape Inverted
    int newRotation = 1; // Default
    String label = "SCREEN UP";

    if (ay > 0.5) {
      newRotation = 1; // Standard Landscape
      label = "RIGHT SIDE UP";
    } else if (ay < -0.5) {
      newRotation = 3; // Inverted Landscape
      label = "LEFT SIDE UP";
    } else if (ax > 0.5) {
      newRotation = 2; // Inverted Portrait
      label = "FRONT UP";
    } else if (ax < -0.5) {
      newRotation = 0; // Standard Portrait
      label = "BACK UP";
    }

    // Apply the rotation to the display
    M5.Display.setRotation(newRotation);

    // Clear and draw
    M5.Display.fillScreen(TFT_WHITE);
    M5.Display.setTextColor(TFT_BLACK);
    
    // Draw text in the center of the current rotation
    int centerX = M5.Display.width() / 2;
    int centerY = M5.Display.height() / 2;

    M5.Display.setTextSize(1);
    M5.Display.drawString("Orientation:", centerX, centerY - 30);
    
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_BLUE);
    M5.Display.drawString(label, centerX, centerY + 10);
  }
  delay(200); // Reduced delay for more responsive rotation
}