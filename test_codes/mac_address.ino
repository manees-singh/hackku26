#include <M5Unified.h>
#include <esp_mac.h>

void setup() {
    Serial.begin(115200);

    auto cfg = M5.config();
    M5.begin(cfg);

    delay(500);

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    Serial.print("DEVICE MAC ADDRESS: ");
    Serial.println(macStr);

    M5.Display.setTextSize(2);
    M5.Display.setCursor(10, 10);
    M5.Display.println("ESP-NOW MAC Finder");
    M5.Display.setCursor(10, 50);
    M5.Display.setTextColor(YELLOW);
    M5.Display.print("MAC: ");
    M5.Display.println(macStr);
}

void loop() {}