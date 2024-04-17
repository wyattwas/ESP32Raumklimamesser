#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Adafruit_SSD1306.h>

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET_PIN = 4;

const int BUTTON_PIN = 15;

int BUTTON_STATE = 0;
int LAST_BUTTON_STATE = 0;

//Declaring sensor Sensirion SCD41
SensirionI2CScd4x scd41;

//Declaring OLED display SSD1306
Adafruit_SSD1306 oled_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

//SCD41 current values
float_t current_scd41_temp = 0.0f;
float_t current_scd41_humid = 0.0f;
uint16_t current_scd41_co2 = 0;

//State for switching display value on OLED
enum State {
    CO2,
    TEMPERATUR,
    HUMIDITY
};

State current_display_value = CO2;

//Declaring function for use before initialization
void oled_display_print(const String& label, const String& data);

void setup() {
    Serial.begin(9600);
    pinMode(BUTTON_PIN, INPUT);
    if (!oled_display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    scd41.begin(Wire);
    scd41.startPeriodicMeasurement();
    scd41.setAutomaticSelfCalibration(1);
}

void loop() {
    BUTTON_STATE = digitalRead(BUTTON_PIN);

    scd41.readMeasurement(current_scd41_co2, current_scd41_temp, current_scd41_humid);

    if(BUTTON_STATE != LAST_BUTTON_STATE) {
        if(BUTTON_STATE) {
            if(current_display_value == CO2) {
                current_display_value = TEMPERATUR;
            } else if(current_display_value == TEMPERATUR) {
                current_display_value = HUMIDITY;
            } else if(current_display_value == HUMIDITY) {
                current_display_value = CO2;
            }
        }
    }

    switch (current_display_value) {
        case CO2: oled_display_print("CO²", String(current_scd41_co2)); break;
        case TEMPERATUR: oled_display_print("Temperatur", String(current_scd41_temp)); break;
        case HUMIDITY: oled_display_print("Luftfeuchtigkeit", String(current_scd41_humid)); break;
    }
}

void oled_display_print(const String& label, const String& data) {
    oled_display.clearDisplay();
    oled_display.setTextSize(1);
    oled_display.setTextColor(SSD1306_WHITE);
    oled_display.setCursor(0, 0);
    oled_display.println(label);
    oled_display.println(data);
    oled_display.display();
}