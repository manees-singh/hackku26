#include <M5Unified.h>

String detectOrientation(float ax, float ay, float az) {
  if (az > 0.7)  return "SCREEN UP";
  if (az < -0.7) return "SCREEN DOWN";
  if (ay > 0.7)  return "RIGHT SIDE UP";
  if (ay < -0.7) return "LEFT SIDE UP";
  if (ax > 0.7)  return "FRONT UP";
  if (ax < -0.7) return "BACK UP";
  return "TILTED";
}

void setup(void) {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setTextFont(&fonts::Font4);
  M5.Display.setTextSize(1);
}

void loop(void) {
  auto imu_update = M5.Imu.update();
  if (imu_update) {
    M5.Display.clear(TFT_WHITE);
    M5.Display.setTextColor(TFT_BLACK);

    auto ImuData = M5.Imu.getImuData();
    float ax = ImuData.accel.x;
    float ay = ImuData.accel.y;
    float az = ImuData.accel.z;

    // Raw accel values
    M5.Display.setCursor(0, 10);
    M5.Display.printf("ax:%6.2f\r\n", ax);
    M5.Display.printf("ay:%6.2f\r\n", ay);
    M5.Display.printf("az:%6.2f\r\n", az);

    // Orientation label
    String orientation = detectOrientation(ax, ay, az);
    M5.Display.setCursor(0, 130);
    M5.Display.setTextSize(2);

    if (orientation == "SCREEN UP")       M5.Display.setTextColor(TFT_GREEN);
    else if (orientation == "SCREEN DOWN") M5.Display.setTextColor(TFT_RED);
    else if (orientation == "LEFT SIDE UP") M5.Display.setTextColor(TFT_BLUE);
    else if (orientation == "RIGHT SIDE UP") M5.Display.setTextColor(TFT_ORANGE);
    else if (orientation == "FRONT UP")   M5.Display.setTextColor(TFT_PURPLE);
    else if (orientation == "BACK UP")    M5.Display.setTextColor(TFT_CYAN);
    else                                   M5.Display.setTextColor(TFT_DARKGREY);

    M5.Display.println(orientation);
  }
  delay(500);
}