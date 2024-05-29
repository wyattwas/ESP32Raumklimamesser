#include <Arduino.h>
#include <WiFi.h>
#include <SensirionI2CScd4x.h>
#include <SensirionI2cSht4x.h>
#include <SensirionI2CSgp40.h>
#include <VOCGasIndexAlgorithm.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include "http/http_keys.h"
#include "http/http_request.h"
#include "Adafruit_SSD1306.h"
#include "oled/oled_display.h"

const int BUTTON_PIN = 15;

int BUTTON_STATE = 0;
int LAST_BUTTON_STATE = 0;
int BUTTON_COUNT = 0;

unsigned long previousTime = 0;
unsigned long interval = 60000;

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

String dwd_pollen_response;

//State for switching display value on OLED
enum State {
    CO2,
    TEMPERATURE,
    HUMIDITY,
    VOC,
    POLLEN
};

State current_display_value = CO2;

void setup() {
    Serial.begin(9600);
    pinMode(BUTTON_PIN, INPUT);

    WiFi.begin(ssid, password);

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

    dwd_pollen_response = httpGETRequest();
    JSONVar dwd_pollen_response_json = JSON.parse(dwd_pollen_response);
    std::string dwd_pollen_response_content{dwd_pollen_response_json["content"]};

    if (BUTTON_COUNT > 4) {
        BUTTON_COUNT = 0;
    }

    LAST_BUTTON_STATE = BUTTON_STATE;

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


    display_data();
}

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

void scanI2C() {
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
        oled_display_clear();
        Serial.println("OLED does exist");
    } else {
        Serial.println("OLED doesn't exist");
    }
}

void display_data() {
    switch (current_display_value) {
        case CO2:
            if (scd41_error) {
                oled_display_print("CO²", "Faulty measurement");
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

void getData() {
    if (millis() - previousTime >= interval) {
        previousTime = millis();

        if (scd41_exists > 0) {
            scd41_error = scd41.readMeasurement(current_scd41_co2, current_scd41_temperature,
                                                current_scd41_humidity);
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

        dwd_pollen_response_json = httpGETRequestDWDasJSON();

        for (int i = 0; i < dwd_pollen_response_json["content"].size(); i++) {
            if (dwd_pollen_response_json["content"][i]["region_id"] == 90 &&
                dwd_pollen_response_json["content"][i]["partregion_id"] == 92) {
                JsonDocument contentJson;
                String region_name = dwd_pollen_response_json["content"][i]["region_name"];
                String partregion_name = dwd_pollen_response_json["content"][i]["partregion_name"];
                String contentString = dwd_pollen_response_json["content"][i]["Pollen"];
                deserializeJson(contentJson, contentString);

                dwd_pollen_response_string = dwd_pollen_response_string + "Gräser: ";
                String pollen_graeser_today = contentJson["Graeser"]["today"];
                dwd_pollen_response_string = dwd_pollen_response_string + pollen_graeser_today + "\n";

                dwd_pollen_response_string = dwd_pollen_response_string + "Esche: ";
                String pollen_esche_today = contentJson["Esche"]["today"];
                dwd_pollen_response_string = dwd_pollen_response_string + pollen_esche_today + "\n";

                dwd_pollen_response_string = dwd_pollen_response_string + "Birke: ";
                String pollen_birke_today = contentJson["Birke"]["today"];
                dwd_pollen_response_string = dwd_pollen_response_string + pollen_birke_today + "\n";

                dwd_pollen_response_string = dwd_pollen_response_string + "Hasel: ";
                String pollen_hasel_today = contentJson["Hasel"]["today"];
                dwd_pollen_response_string = dwd_pollen_response_string + pollen_hasel_today + "\n";

                dwd_pollen_response_string = dwd_pollen_response_string + "Beifuss: ";
                String pollen_beifuss_today = contentJson["Beifuss"]["today"];
                dwd_pollen_response_string = dwd_pollen_response_string + pollen_beifuss_today + "\n";

                dwd_pollen_response_string = dwd_pollen_response_string + "Roggen: ";
                String pollen_roggen_today = contentJson["Roggen"]["today"];
                dwd_pollen_response_string = dwd_pollen_response_string + pollen_roggen_today + "\n";

                dwd_pollen_response_string = dwd_pollen_response_string + "Erle: ";
                String pollen_erle_today = contentJson["Erle"]["today"];
                dwd_pollen_response_string = dwd_pollen_response_string + pollen_erle_today + "\n";

                dwd_pollen_response_string = dwd_pollen_response_string + "Ambrosia: ";
                String pollen_ambrosia_today = contentJson["Ambrosia"]["today"];
                dwd_pollen_response_string = dwd_pollen_response_string + pollen_ambrosia_today + "\n";
            }
        }
    }
}