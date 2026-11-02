#include "Weather.h"
#include <WiFi.h>

Weather weather;

namespace {
const uint8_t SNOW_COUNT = 8;
const uint8_t HEAT_COUNT = 5;
const uint8_t BREEZE_COUNT = 4;
const int CONTENT_X = 0;
const int CONTENT_Y = 12;
const int CONTENT_W = 88;
const int CONTENT_H = 36;
const int FOOTER_Y = 54;
const int ANIM_X_MIN = 92;

int snowY[SNOW_COUNT];
int snowX[SNOW_COUNT];
int snowSpeed[SNOW_COUNT];

int heatY[HEAT_COUNT];
int heatX[HEAT_COUNT];
int heatSpeed[HEAT_COUNT];
int heatPhase[HEAT_COUNT];

int breezeX[BREEZE_COUNT];
int breezeY[BREEZE_COUNT];
int breezeLen[BREEZE_COUNT];

int roundedTemp(float value) {
    return value >= 0 ? (int)(value + 0.5f) : (int)(value - 0.5f);
}

void resetSnowflake(uint8_t i, bool startAboveScreen) {
    snowX[i] = random(ANIM_X_MIN + 2, SCREEN_WIDTH - 2);
    snowY[i] = startAboveScreen ? random(-20, 0) : random(0, FOOTER_Y);
    snowSpeed[i] = random(1, 3);
}

void resetHeatParticle(uint8_t i) {
    heatX[i] = random(ANIM_X_MIN, SCREEN_WIDTH - 4);
    heatY[i] = random(28, FOOTER_Y);
    heatSpeed[i] = 1;
    heatPhase[i] = random(0, 20);
}

void resetBreeze(uint8_t i) {
    breezeX[i] = random(ANIM_X_MIN, SCREEN_WIDTH);
    breezeY[i] = random(20, 48);
    breezeLen[i] = random(8, 15);
}

void clearReadoutBits() {
    display.fillRect(CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H, SSD1306_BLACK);
    display.fillRect(0, FOOTER_Y, SCREEN_WIDTH, SCREEN_HEIGHT - FOOTER_Y, SSD1306_BLACK);
}

void drawSmallSnowflake(int x, int y, bool bright) {
    if (bright) {
        display.drawPixel(x, y, SSD1306_WHITE);
        display.drawPixel(x - 1, y, SSD1306_WHITE);
        display.drawPixel(x + 1, y, SSD1306_WHITE);
        display.drawPixel(x, y - 1, SSD1306_WHITE);
        display.drawPixel(x, y + 1, SSD1306_WHITE);
    } else {
        display.drawPixel(x, y, SSD1306_WHITE);
    }
}

void drawTinySparkle(int x, int y, uint8_t phase) {
    display.drawPixel(x, y, SSD1306_WHITE);

    if (phase == 0) {
        display.drawPixel(x - 1, y, SSD1306_WHITE);
        display.drawPixel(x + 1, y, SSD1306_WHITE);
        display.drawPixel(x, y - 1, SSD1306_WHITE);
        display.drawPixel(x, y + 1, SSD1306_WHITE);
    } else if (phase == 1) {
        display.drawPixel(x - 1, y - 1, SSD1306_WHITE);
        display.drawPixel(x + 1, y + 1, SSD1306_WHITE);
    }
}

void drawCloud(int x, int y) {
    display.drawCircle(x + 5, y + 4, 3, SSD1306_WHITE);
    display.drawCircle(x + 11, y + 3, 4, SSD1306_WHITE);
    display.drawCircle(x + 17, y + 4, 3, SSD1306_WHITE);
    display.drawFastHLine(x + 4, y + 7, 15, SSD1306_WHITE);
}

void drawColdAnimation() {
    static unsigned long lastSnowUpdate = 0;
    bool move = millis() - lastSnowUpdate > 80;
    uint8_t phase = (millis() / 260) % 3;

    if (move) {
        lastSnowUpdate = millis();
    }

    drawCloud(99, 7);

    for (int x = ANIM_X_MIN; x < SCREEN_WIDTH; x += 14) {
        int length = 3 + ((millis() / 350 + x) % 3);
        display.drawLine(x, 0, x - 2, length, SSD1306_WHITE);
        display.drawLine(x, 0, x + 2, length, SSD1306_WHITE);
    }

    for (uint8_t i = 0; i < SNOW_COUNT; i++) {
        bool bright = (i % 4 == 0);
        drawSmallSnowflake(snowX[i], snowY[i], bright);

        if (move) {
            snowY[i] += snowSpeed[i];

            if (snowY[i] >= FOOTER_Y) {
                resetSnowflake(i, true);
            }
        }
    }

    drawTinySparkle(123, 27, phase);
    drawTinySparkle(95, 45, (phase + 1) % 3);
    display.drawFastHLine(ANIM_X_MIN + 3, FOOTER_Y - 1, 30, SSD1306_WHITE);
}

void drawSun(int cx, int cy, int radius) {
    uint8_t phase = (millis() / 240) % 2;

    display.drawCircle(cx, cy, radius, SSD1306_WHITE);
    display.drawPixel(cx, cy, SSD1306_WHITE);
    display.drawFastHLine(cx - radius - 4, cy, 3, SSD1306_WHITE);
    display.drawFastHLine(cx + radius + 2, cy, 3, SSD1306_WHITE);
    display.drawFastVLine(cx, cy - radius - 4, 3, SSD1306_WHITE);
    display.drawFastVLine(cx, cy + radius + 2, 3, SSD1306_WHITE);

    if (phase == 0) {
        display.drawLine(cx - radius - 3, cy - radius - 3, cx - radius - 1, cy - radius - 1, SSD1306_WHITE);
        display.drawLine(cx + radius + 1, cy + radius + 1, cx + radius + 3, cy + radius + 3, SSD1306_WHITE);
    } else {
        display.drawLine(cx + radius + 1, cy - radius - 1, cx + radius + 3, cy - radius - 3, SSD1306_WHITE);
        display.drawLine(cx - radius - 3, cy + radius + 3, cx - radius - 1, cy + radius + 1, SSD1306_WHITE);
    }
}

void drawHeatSquiggle(int x, int y, uint8_t phase) {
    for (uint8_t i = 0; i < 5; i++) {
        int wiggle = ((phase + i) % 3) - 1;
        display.drawPixel(x + wiggle, y - i * 3, SSD1306_WHITE);
    }
}

void drawHotAnimation() {
    static unsigned long lastHeatUpdate = 0;
    bool move = millis() - lastHeatUpdate > 90;
    uint8_t phase = (millis() / 180) % 3;

    if (move) {
        lastHeatUpdate = millis();
    }

    drawSun(113, 12, 6);

    for (int x = ANIM_X_MIN; x < SCREEN_WIDTH; x += 10) {
        int flameHeight = 4 + ((millis() / 180 + x) % 3);
        display.drawLine(x, FOOTER_Y - 1, x + 3, FOOTER_Y - 1 - flameHeight, SSD1306_WHITE);
        display.drawLine(x + 3, FOOTER_Y - 1 - flameHeight, x + 6, FOOTER_Y - 1, SSD1306_WHITE);
    }

    for (uint8_t i = 0; i < HEAT_COUNT; i++) {
        int x = heatX[i];
        int y = heatY[i];

        drawHeatSquiggle(x, y, (phase + heatPhase[i]) % 3);

        if (move) {
            heatY[i] -= heatSpeed[i];

            if (heatY[i] < 20) {
                resetHeatParticle(i);
                heatY[i] = FOOTER_Y - 1;
            }
        }
    }

    drawTinySparkle(97, 22, phase);
}

void drawSprout(int x, int y, uint8_t phase) {
    display.drawFastVLine(x, y - 5, 5, SSD1306_WHITE);

    if (phase == 0) {
        display.drawLine(x, y - 3, x - 4, y - 5, SSD1306_WHITE);
        display.drawLine(x, y - 4, x + 4, y - 6, SSD1306_WHITE);
    } else {
        display.drawLine(x, y - 3, x - 3, y - 6, SSD1306_WHITE);
        display.drawLine(x, y - 4, x + 3, y - 5, SSD1306_WHITE);
    }
}

void drawPleasantAnimation() {
    static unsigned long lastBreezeUpdate = 0;
    bool move = millis() - lastBreezeUpdate > 110;
    uint8_t phase = (millis() / 320) % 3;
    int cloudX = ANIM_X_MIN + 1 + ((millis() / 260) % 8);

    if (move) {
        lastBreezeUpdate = millis();
    }

    drawSun(113, 11, 5);
    drawCloud(cloudX, 24);

    for (uint8_t i = 0; i < BREEZE_COUNT; i++) {
        int y = breezeY[i] + ((phase + i) % 2);
        int len = breezeLen[i];
        if (breezeX[i] + len > SCREEN_WIDTH) len = SCREEN_WIDTH - breezeX[i];
        if (len > 0) display.drawFastHLine(breezeX[i], y, len, SSD1306_WHITE);
        if (len > 6) display.drawPixel(breezeX[i] + len - 2, y - 1, SSD1306_WHITE);

        if (move) {
            breezeX[i] += 1;

            if (breezeX[i] > SCREEN_WIDTH + 4) {
                breezeX[i] = -breezeLen[i];
                if (breezeX[i] < ANIM_X_MIN) breezeX[i] = ANIM_X_MIN;
                breezeY[i] = random(24, 48);
                breezeLen[i] = random(9, 20);
            }
        }
    }

    for (uint8_t i = 0; i < 3; i++) {
        int x = ANIM_X_MIN + 5 + i * 11;
        int y = 48 + ((millis() / 400 + i) % 2);
        display.drawPixel(x, y, SSD1306_WHITE);
    }

    drawSprout(122, FOOTER_Y - 2, phase % 2);
    drawTinySparkle(96, 18, phase);
}

void drawTemperatureAnimation(float temp) {
    if (temp < 20.0f) {
        drawColdAnimation();
    } else if (temp > 27.0f) {
        drawHotAnimation();
    } else {
        drawPleasantAnimation();
    }
}

void drawWeatherReadout(float temp, const String &condition) {
    int shownTemp = roundedTemp(temp);

    clearReadoutBits();

    display.setTextSize(3);
    display.setCursor(8, 16);
    display.print(shownTemp);

    int cursorX = display.getCursorX();
    int cursorY = display.getCursorY();
    display.drawCircle(cursorX + 4, cursorY + 4, 3, SSD1306_WHITE);

    display.setCursor(cursorX + 12, cursorY);
    display.print("C");

    display.setTextSize(1);
    display.setCursor(0, 55);
    display.print(WEATHER_CITY);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(condition, 0, 0, &x1, &y1, &w, &h);
    int conditionX = SCREEN_WIDTH - w;
    if (conditionX < 70) conditionX = 70;

    display.setCursor(conditionX, 55);
    display.print(condition);
}
}

void Weather::begin() {
    for(uint8_t i = 0; i < SNOW_COUNT; i++) {
        resetSnowflake(i, false);
    }

    for(uint8_t i = 0; i < HEAT_COUNT; i++) {
        resetHeatParticle(i);
    }

    for(uint8_t i = 0; i < BREEZE_COUNT; i++) {
        resetBreeze(i);
    }
}

void Weather::fetch() {
    if(WiFi.status() != WL_CONNECTED) return; 

    HTTPClient http;
    http.setTimeout(3000); 
    
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + String(WEATHER_CITY) + "," + String(WEATHER_COUNTRY) + "&units=metric&appid=" + String(WEATHER_API_KEY);

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
        String payload = http.getString();
        JsonDocument doc; 
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            temp = doc["main"]["temp"];
            const char* desc = doc["weather"][0]["main"];
            condition = String(desc);
            hasData = true;
        }
    }
    http.end();
}

void Weather::update() {
    // Retry fetch logic
    bool timeToUpdate = (millis() - lastFetch > 900000);
    bool retryFailed = (!hasData && millis() - lastFetch > 5000);

    if (WiFi.status() == WL_CONNECTED && (timeToUpdate || retryFailed)) {
        clearDisplayFast();
        drawCenteredText("Updating...", 30);
        display.display();
        fetch();
        lastFetch = millis();
    }

    clearDisplayFast();
    
    if (!hasData) {
        display.setTextSize(1);
        drawCenteredText("No Weather Data", 20);
        if(WiFi.status() != WL_CONNECTED) drawCenteredText("Waiting for WiFi...", 40);
        else drawCenteredText("Checking API...", 40);
    } else {
        drawTemperatureAnimation(temp);
        drawWeatherReadout(temp, condition);
    }
    display.display();
}
