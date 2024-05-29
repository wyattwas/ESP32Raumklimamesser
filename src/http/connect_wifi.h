//
// Created by Ruehl_Wyatt on 29.05.2024.
//

#ifndef ESP32RAUMKLIMAMESSER_CONNECT_WIFI_H
#define ESP32RAUMKLIMAMESSER_CONNECT_WIFI_H

#include <WiFi.h>
#include "http/http_keys.h"

void connect_wifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("\nConnecting to WiFi Network .");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }

    Serial.println("Wifi connected: " + static_cast<String>(WiFi.isConnected() ? "true" : "false"));
}

#endif //ESP32RAUMKLIMAMESSER_CONNECT_WIFI_H
