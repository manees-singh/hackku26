#include <M5Unified.h>
#include <WiFi.h>
#include <esp_now.h>

void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    char msg[len + 1];
    memcpy(msg, data, len);
    msg[len] = '\0';

    Serial.print("Received: ");
    Serial.println(msg);

    M5.Display.fillScreen(BLACK);
    M5.Display.setCursor(0, 0);
    M5.Display.setTextSize(2);
    M5.Display.println("Received:");
    M5.Display.println(msg);
}

void setup() {
    Serial.begin(115200);
    auto cfg = M5.config();
    M5.begin(cfg);

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        M5.Display.println("ESP-NOW init failed!");
        return;
    }

    esp_now_register_recv_cb(onReceive);

    M5.Display.setTextSize(2);
    M5.Display.println("Receiver ready!\nWaiting...");
}

void loop() {}