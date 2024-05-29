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

#endif //ESP32RAUMKLIMAMESSER_OLED_DISPLAY_H
