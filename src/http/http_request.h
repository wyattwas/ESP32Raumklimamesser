//
// Created by Ruehl_Wyatt on 24.04.2024.
//

#ifndef ESP32RAUMKLIMAMESSER_HTTP_REQUEST_H
#define ESP32RAUMKLIMAMESSER_HTTP_REQUEST_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "http/http_keys.h"

WiFiClient wifiClient;
HttpClient httpClient(wifiClient, dwd_adress, dwd_endpoint_port);

String httpGETRequest() {
    httpClient.beginRequest();
    httpClient.get(dwd_endpoint);
    httpClient.endRequest();

    int httpResponseCode = httpClient.responseStatusCode();
    String response = "{}";

    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        response = httpClient.responseBody();
    } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }

    return response;
}

#endif //ESP32RAUMKLIMAMESSER_HTTP_REQUEST_H
