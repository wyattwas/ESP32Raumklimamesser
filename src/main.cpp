#include <Arduino.h>
#include <SensirionCore.h>
#include <SensirionI2CScd4x.h>
#include <SensirionI2cSht4x.h>
#include <SensirionI2CSgp40.h>
#include <VOCGasIndexAlgorithm.h>
#include <Adafruit_SSD1306.h>
#include "oled/oled_display.h"

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET_PIN = 4;

const int BUTTON_PIN = 15;

int BUTTON_STATE = 0;
int LAST_BUTTON_STATE = 0;

//Declaring all sensors
SensirionI2CScd4x scd41;
SensirionI2cSht4x sht41;
SensirionI2CSgp40 sgp40;
VOCGasIndexAlgorithm vocGasIndexAlgorithm;

//Declaring OLED display SSD1306
Adafruit_SSD1306 oled_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

//SCD41 current values
uint16_t current_scd41_co2 = 0;
float_t current_scd41_temperature = 0.0f;
float_t current_scd41_humidity = 0.0f;

//SHT41 current values
float_t current_sht41_temperature = 0.0f;
float_t current_sht41_humidity = 0.0f;

//SGP40 current values
uint16_t default_sgp40_relative_humidity = 0x8000;
uint16_t default_sgp40_temperature = 0x6666;
uint16_t current_sgp40_relative_humidity = 0;
uint16_t current_sgp40_temperature = 0;
uint32_t current_sgp40_voc_index = 0;
uint16_t current_sgp40_sraw_voc = 0;

//State for switching display value on OLED
enum State {
    CO2,
    TEMPERATURE,
    HUMIDITY,
    VOC,
    POLLEN
};

State current_display_value = CO2;

//Declaring function for use before initialization
void oled_display_print(const String &label, const String &data);

void setup() {
    Serial.begin(9600);
    pinMode(BUTTON_PIN, INPUT);

    if (!oled_display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }

    Wire.begin();
    scd41.begin(Wire);
    sht41.begin(Wire, SHT40_I2C_ADDR_44);
    sgp40.begin(Wire);

    scd41.startPeriodicMeasurement();
    scd41.setAutomaticSelfCalibration(1);
}

void loop() {
    uint16_t scd41_error;
    uint16_t sht41_error;
    uint16_t sgp40_error;

    BUTTON_STATE = digitalRead(BUTTON_PIN);

    scd41_error = scd41.readMeasurement(current_scd41_co2, current_scd41_temperature, current_scd41_humidity);
    sht41_error = sht41.measureHighPrecision(current_sht41_temperature, current_sht41_humidity);
    if (sht41_error) {
        current_sgp40_relative_humidity = default_sgp40_relative_humidity;
        current_sgp40_temperature = default_sgp40_temperature;
    } else {
        current_sgp40_relative_humidity = static_cast<uint16_t>(current_sht41_humidity * 65535 / 100);
        current_sgp40_temperature = static_cast<uint16_t>((current_sht41_temperature + 45) * 65535 / 175);
    }
    sgp40_error = sgp40.measureRawSignal(current_sgp40_relative_humidity, current_sgp40_temperature,
                                         current_sgp40_sraw_voc);
    current_sgp40_voc_index = vocGasIndexAlgorithm.process(current_sgp40_sraw_voc);

    if (BUTTON_STATE != LAST_BUTTON_STATE) {
        if (BUTTON_STATE) {
            if (current_display_value == CO2) {
                current_display_value = TEMPERATURE;
            } else if (current_display_value == TEMPERATURE) {
                current_display_value = HUMIDITY;
            } else if (current_display_value == HUMIDITY) {
                current_display_value = VOC;
            } else if (current_display_value == VOC) {
                current_display_value = POLLEN;
            } else if (current_display_value == POLLEN) {
                current_display_value = CO2;
            }
        }
    }

    switch (current_display_value) {
        case CO2:
            if (scd41_error) {
                oled_display_print("CO²", "Faulty measurement");
                break;
            } else {
                oled_display_print("CO²", String(current_scd41_co2));
                break;
            }
        case TEMPERATURE:
            if (sht41_error) {
                oled_display_print("Temperatur", "Faulty measurement");
                break;
            } else {
                oled_display_print("Temperatur", String(current_sht41_temperature));
                break;
            }
        case HUMIDITY:
            if (sht41_error) {
                oled_display_print("Luftfeuchtigkeit", "Faulty measurement");
                break;
            } else {
                oled_display_print("Luftfeuchtigkeit", String(current_sht41_humidity));
                break;
            }
        case VOC:
            if (sgp40_error) {
                oled_display_print("VOC", "Faulty measurement");
                break;
            } else {
                oled_display_print("VOC Index", String(current_sgp40_voc_index));
                break;
            }
        case POLLEN:
            oled_display_print("POLLEN", String(current_sht41_humidity));
            break;
    }
}