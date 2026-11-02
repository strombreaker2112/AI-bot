#pragma once
#include "Config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Global I2C display object -- actual instance created in Display.cpp
extern Adafruit_SSD1306 display;

bool initDisplay();
bool isDisplayReady();
void resetDisplayDrawState();
void clearDisplayFast();
void drawCenteredText(const String &txt, int y);
