#include <M5Unified.h>

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // 1. Set background to solid Red
  M5.Display.fillScreen(TFT_RED);

  // Get center coordinates
  int cx = M5.Display.width() / 2;
  int cy = M5.Display.height() / 2;

  // 2. Draw Eyebrows (Slanted deeper and lower for more anger)
  // Left Eyebrow (Lowered y-coordinates of the inner points)
  M5.Display.fillTriangle(cx - 75, cy - 65, cx - 15, cy - 30, cx - 80, cy - 55, TFT_BLACK);
  // Right Eyebrow (Lowered y-coordinates of the inner points)
  M5.Display.fillTriangle(cx + 75, cy - 65, cx + 15, cy - 30, cx + 80, cy - 55, TFT_BLACK);

  // 3. Draw Eyes (Changed to a glowing, intense Red)
  // Left Eye
  M5.Display.fillRect(cx - 60, cy - 25, 30, 20, TFT_RED);
  M5.Display.fillRect(cx - 50, cy - 20, 10, 10, TFT_BLACK); // Pupil
  M5.Display.drawRect(cx - 60, cy - 25, 30, 20, TFT_BLACK); // Added border for definition
  
  // Right Eye
  M5.Display.fillRect(cx + 30, cy - 25, 30, 20, TFT_RED);
  M5.Display.fillRect(cx + 40, cy - 20, 10, 10, TFT_BLACK); // Pupil
  M5.Display.drawRect(cx + 30, cy - 25, 30, 20, TFT_BLACK); // Added border for definition

  // 4. Draw an angry downturned mouth arc
  M5.Display.fillArc(cx, cy + 30, 20, 15, 200, 340, TFT_BLACK);
}

void loop() {
  // Static image, nothing needed here
  M5.update();
}