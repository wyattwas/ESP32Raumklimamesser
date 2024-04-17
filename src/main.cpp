#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Adafruit_SSD1306.h>

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET_PIN = 4;

//Declaring sensor Sensirion SCD41
SensirionI2CScd4x scd41;

//Declaring OLED display SSD1306
Adafruit_SSD1306 oled_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

//SCD41 current values
float_t current_scd41_temp = 0.0f;
float_t current_scd41_humid = 0.0f;
uint16_t current_scd41_co2 = 0;

void setup() {
    if (!oled_display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    scd41.begin(Wire);
    scd41.startPeriodicMeasurement();
    scd41.setAutomaticSelfCalibration(1);
}

void loop() {
    scd41.readMeasurement(current_scd41_co2, current_scd41_temp, current_scd41_humid);
}