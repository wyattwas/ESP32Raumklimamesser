#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <SensirionI2cSht4x.h>
#include <SensirionI2CSgp40.h>
#include <VOCGasIndexAlgorithm.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include "http/http_keys.h"
#include "http/http_request.h"
#include "Adafruit_SSD1306.h"
#include "oled/oled_display.h"

#define ADDRESS_SHT41 (0x44)
#define ADDRESS_SGP40 (0x59)
#define ADDRESS_SCD41 (0x62)
#define ADDRESS_OLED (0x3C)

int scd41_exists = 0;
int sgp40_exists = 0;
int sht41_exists = 0;
int oled_exists = 0;

const int BUTTON_PIN = 15;

int BUTTON_STATE = 0;
int LAST_BUTTON_STATE = 0;
int BUTTON_COUNT = 0;

//Declaring all sensors
SensirionI2CScd4x scd41;
SensirionI2cSht4x sht41;
SensirionI2CSgp40 sgp40;
VOCGasIndexAlgorithm vocGasIndexAlgorithm;

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

uint16_t scd41_error;
uint16_t sht41_error;
uint16_t sgp40_error;

//State for switching display value on OLED
enum State {
    CO2,
    TEMPERATURE,
    HUMIDITY,
    VOC,
    POLLEN
};

State current_display_value = CO2;

String dwd_pollen_response;
JSONVar dwd_pollen_response_json;

void setup() {
    Serial.begin(9600);
    pinMode(BUTTON_PIN, INPUT);

    WiFi.begin(ssid, password);
    Wire.begin();

    if (!oled_display.begin(SSD1306_SWITCHCAPVCC, ADDRESS_OLED)) {
        Serial.println(F("SSD1306 allocation failed"));
        oled_display.clearDisplay();
        while (true);
    }

    byte error, address;
    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            if (address == ADDRESS_SCD41) {
                scd41_exists = address;
            } else if (address == SHT40_I2C_ADDR_44) {
                sht41_exists = address;
            } else if (address == ADDRESS_SGP40) {
                sgp40_exists = address;
            } else if (address == ADDRESS_OLED) {
                oled_exists = address;
            }
        } else if (error == 4) {
            Serial.println("Wire error");
        }
    }

    if (scd41_exists > 0) {
        scd41.begin(Wire);
        scd41.startPeriodicMeasurement();
        scd41.setAutomaticSelfCalibration(1);
        Serial.println("SCD41 does exist");
    } else {
        Serial.println("SCD41 doesn't exist");
    }

    if (sht41_exists > 0) {
        sht41.begin(Wire, SHT40_I2C_ADDR_44);
        Serial.println("SHT41 does exist");
    } else {
        Serial.println("SHT41 doesn't exist");
    }

    if (sgp40_exists > 0) {
        sgp40.begin(Wire);
        Serial.println("SGP40 does exist");
    } else {
        Serial.println("SGP40 doesn't exist");
    }

    if (oled_exists > 0) {
        oled_display.clearDisplay();
        Serial.println("OLED does exist");
    } else {
        Serial.println("OLED doesn't exist");
    }
}

void loop() {
    Serial.println("hi");

    BUTTON_STATE = digitalRead(BUTTON_PIN);
    Serial.println(BUTTON_STATE);

    if (BUTTON_STATE != LAST_BUTTON_STATE && BUTTON_STATE == 1) {
        BUTTON_COUNT++;
    }

    if (BUTTON_COUNT > 4) {
        BUTTON_COUNT = 0;
    }

    LAST_BUTTON_STATE = BUTTON_STATE;

    Serial.println(BUTTON_COUNT);

    if (BUTTON_COUNT == 0) {
        current_display_value = CO2;
    } else if (BUTTON_COUNT == 1) {
        current_display_value = TEMPERATURE;
    } else if (BUTTON_COUNT == 2) {
        current_display_value = HUMIDITY;
    } else if (BUTTON_COUNT == 3) {
        current_display_value = VOC;
    } else if (BUTTON_COUNT == 4) {
        current_display_value = POLLEN;
    }

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
            if (JSON.typeof(dwd_pollen_response_json) == "undefined" || dwd_pollen_response_json.length() == 0) {
                oled_display_print("Pollen", "Faulty request or parsing");
                break;
            } else {
                oled_display_print("Pollen", dwd_pollen_response);
                break;
            }
    }
}

void getData() {
    if (scd41_exists > 0) {
        scd41_error = scd41.readMeasurement(current_scd41_co2, current_scd41_temperature, current_scd41_humidity);
        sht41_error = sht41.measureHighPrecision(current_sht41_temperature, current_sht41_humidity);
    }

    if (scd41_exists && sht41_error) {
        current_sgp40_relative_humidity = default_sgp40_relative_humidity;
        current_sgp40_temperature = default_sgp40_temperature;
    } else {
        current_sgp40_relative_humidity = static_cast<uint16_t>(current_sht41_humidity * 65535 / 100);
        current_sgp40_temperature = static_cast<uint16_t>((current_sht41_temperature + 45) * 65535 / 175);
    }

    if (sgp40_exists > 0) {
        sgp40_error = sgp40.measureRawSignal(current_sgp40_relative_humidity, current_sgp40_temperature,
                                             current_sgp40_sraw_voc);
        current_sgp40_voc_index = vocGasIndexAlgorithm.process(current_sgp40_sraw_voc);
    }

    if (scd41_exists > 0) {
        scd41_error = scd41.readMeasurement(current_scd41_co2, current_scd41_temperature, current_scd41_humidity);
        sht41_error = sht41.measureHighPrecision(current_sht41_temperature, current_sht41_humidity);
    }

    if (scd41_exists && sht41_error) {
        current_sgp40_relative_humidity = default_sgp40_relative_humidity;
        current_sgp40_temperature = default_sgp40_temperature;
    } else {
        current_sgp40_relative_humidity = static_cast<uint16_t>(current_sht41_humidity * 65535 / 100);
        current_sgp40_temperature = static_cast<uint16_t>((current_sht41_temperature + 45) * 65535 / 175);
    }

    if (sgp40_exists > 0) {
        sgp40_error = sgp40.measureRawSignal(current_sgp40_relative_humidity, current_sgp40_temperature,
                                             current_sgp40_sraw_voc);
        current_sgp40_voc_index = vocGasIndexAlgorithm.process(current_sgp40_sraw_voc);
    }

    dwd_pollen_response = httpGETRequest();
    dwd_pollen_response_json = JSON.parse(dwd_pollen_response);
    std::string dwd_pollen_response_content{dwd_pollen_response_json["content"]};

    for (int i = 0; i < dwd_pollen_response_content.length(); i++) {
        std::string dwd_pollen_region_id_string{dwd_pollen_region_id};
        std::string dwd_pollen_response_content_region_id{"region_id"[dwd_pollen_response_content[i]]};
        if (dwd_pollen_response_content_region_id == dwd_pollen_region_id_string) {
            dwd_pollen_response = dwd_pollen_response_content[i];
        }
    }
}