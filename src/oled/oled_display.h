//
// Created by Ruehl_Wyatt on 19.04.2024.
//

#ifndef ESP32RAUMKLIMAMESSER_OLED_DISPLAY_H
#define ESP32RAUMKLIMAMESSER_OLED_DISPLAY_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET_PIN = -1;

Adafruit_SSD1306 oled_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

void oled_display_print(const String& label, const String& data)
{
    oled_display.clearDisplay();
    oled_display.setTextSize(1);
    oled_display.setTextColor(SSD1306_WHITE);
    oled_display.setCursor(0, 0);
    oled_display.println(label);
    oled_display.println(data);
    oled_display.display();
}

void oled_display_print(const String& text)
{
    oled_display.clearDisplay();
    oled_display.setTextSize(1);
    oled_display.setTextColor(SSD1306_WHITE);
    oled_display.setCursor(0, 0);
    oled_display.println(text);
}

void oled_display_clear()
{
    oled_display.clearDisplay();
}

void display_data()
{
    switch (current_display_value) {
        case CO2:
            if (scd41_error || scd41_exists <= 0) {
                oled_display_print("CO2", "Faulty measurement");
                break;
            } else {
                oled_display_print("CO2", String(current_scd41_co2));
                break;
            }
        case TEMPERATURE:
            if (sht41_error || sht41_exists <= 0) {
                oled_display_print("Temperatur", "Faulty measurement");
                break;
            } else {
                oled_display_print("Temperatur", String(current_sht41_temperature));
                break;
            }
        case HUMIDITY:
            if (sht41_error || sht41_exists <= 0) {
                oled_display_print("Luftfeuchtigkeit", "Faulty measurement");
                break;
            } else {
                oled_display_print("Luftfeuchtigkeit", String(current_sht41_humidity));
                break;
            }
        case VOC:
            if (sgp40_error || sgp40_exists <= 0) {
                oled_display_print("VOC", "Faulty measurement");
                break;
            } else {
                oled_display_print("VOC Index", String(current_sgp40_voc_index));
                break;
            }
        case POLLEN:
            if (dwd_pollen_response_json.size() == 0) {
                oled_display_print("Pollen", "Faulty request or parsing");
                break;
            } else {
                oled_display_print("Pollen", dwd_pollen_response_string);
                break;
            }
    }
}

#endif //ESP32RAUMKLIMAMESSER_OLED_DISPLAY_H
