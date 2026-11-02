#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"
#include "Display.h"

class Reminders {
public:
    void begin();
    void update();
    bool updateAutoNotification();

    void setFromText(const String &text);
    void startManualList();

    uint8_t getItemCount() const;
    String getItem(uint8_t index) const;
    String getItemsAsText() const;

private:
    static const uint8_t MAX_ITEMS = 8;
    static const uint8_t MAX_ITEM_LENGTH = 64;
    static const unsigned long AUTO_INTERVAL_MS = 60UL * 60UL * 1000UL;
    static const unsigned long AUTO_FLASH_MS = 8000UL;
    static const unsigned long MANUAL_ITEM_MS = 10000UL;

    Preferences preferences;
    String items[MAX_ITEMS];
    uint8_t itemCount = 0;

    bool manualActive = false;
    uint8_t manualIndex = 0;
    unsigned long lastManualSwitch = 0;

    bool autoActive = false;
    uint8_t autoIndex = 0;
    unsigned long lastAutoReminder = 0;
    unsigned long autoStarted = 0;

    void loadFromStorage();
    void saveToStorage();
    void drawEmptyScreen();
    void drawManualItem();
    void drawNotification(bool visible);
    void drawBellIcon(int x, int y, uint16_t color);
    void drawProgressDots(uint8_t activeIndex, int y);
    void drawWrappedText(const String &text, int x, int y, uint8_t charsPerLine, uint8_t maxLines);
};

extern Reminders reminders;
