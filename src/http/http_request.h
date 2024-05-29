//
// Created by Ruehl_Wyatt on 24.04.2024.
//

#ifndef ESP32RAUMKLIMAMESSER_HTTP_REQUEST_H
#define ESP32RAUMKLIMAMESSER_HTTP_REQUEST_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "http/http_keys.h"

HTTPClient httpClient;
JsonDocument jsonDocument;

JsonDocument httpGETRequestDWDasJSON()
{
    httpClient.begin("http://opendata.dwd.de/climate_environment/health/alerts/s31fg.json");
    int httpResponseCode = httpClient.GET();
    httpClient.end();

    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        deserializeJson(jsonDocument, httpClient.getString());
    } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        String httpCode = String(httpResponseCode);
        deserializeJson(jsonDocument, httpCode);
    }

    return jsonDocument;
}

#endif //ESP32RAUMKLIMAMESSER_HTTP_REQUEST_H
