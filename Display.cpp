#include "Display.h"
#include <Wire.h>
#include <Arduino.h>

namespace {
bool displayReady = false;
}

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

bool initDisplay() {
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    Wire.setClock(OLED_I2C_CLOCK_HZ);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        displayReady = false;
        Serial.println(F("OLED init failed at I2C address 0x3C."));
        return false;
    }

    display.setRotation(0);
    display.invertDisplay(false);
    display.dim(false);
    clearDisplayFast();
    display.display();

    displayReady = true;
    Serial.println(F("OLED initialized at I2C address 0x3C"));
    return true;
}

bool isDisplayReady() {
    return displayReady;
}

void resetDisplayDrawState() {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
}

void clearDisplayFast() {
    display.clearDisplay();
    resetDisplayDrawState();
}

void drawCenteredText(const String &txt, int y) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, y);
    display.print(txt);
}
