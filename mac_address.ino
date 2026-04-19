#include <M5Unified.h>
#include <WiFi.h>

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    // 1. Initialize the Wi-Fi radio (Required to read the MAC)
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); 

    // 2. Get the MAC Address string
    String mac = WiFi.macAddress();

    // 3. Print to Serial Monitor (for easy copy-pasting into your code)
    Serial.begin(115200);
    Serial.print("DEVICE MAC ADDRESS: ");
    Serial.println(mac);

    // 4. Print to M5GO LCD Screen
    M5.Display.setTextSize(2);
    M5.Display.setCursor(10, 10);
    M5.Display.println("ESP-NOW MAC Finder");
    M5.Display.setCursor(10, 50);
    M5.Display.setTextColor(YELLOW);
    M5.Display.print("MAC: ");
    M5.Display.println(mac);
}

void loop() {
    // Nothing to do here
}