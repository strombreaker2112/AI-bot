#include "Reminders.h"

Reminders reminders;

void Reminders::begin() {
    preferences.begin("reminders", false);
    loadFromStorage();
    lastAutoReminder = millis();
}

void Reminders::loadFromStorage() {
    String saved = preferences.getString("items", "");
    setFromText(saved);
}

void Reminders::saveToStorage() {
    preferences.putString("items", getItemsAsText());
}

void Reminders::setFromText(const String &text) {
    itemCount = 0;
    manualActive = false;
    autoActive = false;

    int start = 0;
    while (start < text.length() && itemCount < MAX_ITEMS) {
        int end = text.indexOf('\n', start);
        if (end < 0) end = text.length();

        String line = text.substring(start, end);
        line.replace("\r", "");
        line.trim();

        if (line.length() > 0) {
            if (line.length() > MAX_ITEM_LENGTH) {
                line = line.substring(0, MAX_ITEM_LENGTH);
            }
            items[itemCount++] = line;
        }

        start = end + 1;
    }

    saveToStorage();
    lastAutoReminder = millis();
}

uint8_t Reminders::getItemCount() const {
    return itemCount;
}

String Reminders::getItem(uint8_t index) const {
    if (index >= itemCount) return "";
    return items[index];
}

String Reminders::getItemsAsText() const {
    String text = "";
    for (uint8_t i = 0; i < itemCount; i++) {
        if (i > 0) text += "\n";
        text += items[i];
    }
    return text;
}

void Reminders::startManualList() {
    manualActive = true;
    manualIndex = 0;
    lastManualSwitch = millis();
}

void Reminders::update() {
    if (itemCount == 0) {
        drawEmptyScreen();
        return;
    }

    if (!manualActive) {
        startManualList();
    }

    if (millis() - lastManualSwitch >= MANUAL_ITEM_MS) {
        lastManualSwitch = millis();
        manualIndex++;
        if (manualIndex >= itemCount) manualIndex = 0;
    }

    drawManualItem();
}

bool Reminders::updateAutoNotification() {
    if (itemCount == 0) return false;

    unsigned long now = millis();
    if (!autoActive && now - lastAutoReminder >= AUTO_INTERVAL_MS) {
        autoActive = true;
        autoStarted = now;
        lastAutoReminder = now;
    }

    if (!autoActive) return false;

    if (now - autoStarted >= AUTO_FLASH_MS) {
        autoActive = false;
        autoIndex++;
        if (autoIndex >= itemCount) autoIndex = 0;
        clearDisplayFast();
        display.display();
        return false;
    }

    bool visible = (now / 350) % 2 == 0;
    drawNotification(visible);
    return true;
}

void Reminders::drawEmptyScreen() {
    clearDisplayFast();
    display.setTextSize(1);
    display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 5, SSD1306_WHITE);
    display.fillRect(0, 0, SCREEN_WIDTH, 14, SSD1306_WHITE);
    drawBellIcon(6, 3, SSD1306_BLACK);

    display.setTextColor(SSD1306_BLACK);
    display.setCursor(21, 4);
    display.print("REMINDERS");
    display.setTextColor(SSD1306_WHITE);

    display.drawRoundRect(12, 23, 104, 25, 4, SSD1306_WHITE);
    drawCenteredText("No reminders", 29);
    drawCenteredText("Open APP", 51);
    display.display();
}

void Reminders::drawManualItem() {
    clearDisplayFast();
    display.setTextSize(1);
    display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 5, SSD1306_WHITE);
    display.fillRect(0, 0, SCREEN_WIDTH, 14, SSD1306_WHITE);
    drawBellIcon(6, 3, SSD1306_BLACK);

    display.setTextColor(SSD1306_BLACK);
    display.setCursor(21, 4);
    display.print("REMINDER");

    display.setCursor(106, 4);
    display.print(manualIndex + 1);
    display.print("/");
    display.print(itemCount);
    display.setTextColor(SSD1306_WHITE);

    display.drawRoundRect(4, 18, 120, 31, 4, SSD1306_WHITE);
    drawWrappedText(items[manualIndex], 9, 24, 18, 2);

    unsigned long elapsed = millis() - lastManualSwitch;
    if (elapsed > MANUAL_ITEM_MS) elapsed = MANUAL_ITEM_MS;
    int progressWidth = map(elapsed, 0, MANUAL_ITEM_MS, 0, SCREEN_WIDTH - 16);
    display.drawRect(8, 53, SCREEN_WIDTH - 16, 4, SSD1306_WHITE);
    if (progressWidth > 0) {
        display.fillRect(9, 54, progressWidth, 2, SSD1306_WHITE);
    }

    drawProgressDots(manualIndex, 61);
    display.display();
}

void Reminders::drawNotification(bool visible) {
    clearDisplayFast();

    if (visible) {
        display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 6, SSD1306_WHITE);
        display.drawRoundRect(3, 3, SCREEN_WIDTH - 6, SCREEN_HEIGHT - 6, 4, SSD1306_WHITE);
        display.fillRect(4, 4, SCREEN_WIDTH - 8, 15, SSD1306_WHITE);
        drawBellIcon(10, 7, SSD1306_BLACK);

        display.setTextColor(SSD1306_BLACK);
        display.setTextSize(1);
        display.setCursor(29, 8);
        display.print("REMINDER");
        display.setTextColor(SSD1306_WHITE);

        drawWrappedText(items[autoIndex], 9, 28, 18, 2);

        for (uint8_t i = 0; i < 5; i++) {
            int x = 11 + i * 26;
            int y = 53 + ((millis() / 220 + i) % 2);
            display.drawPixel(x, y, SSD1306_WHITE);
            display.drawPixel(x + 1, y, SSD1306_WHITE);
        }
    }

    display.display();
}

void Reminders::drawBellIcon(int x, int y, uint16_t color) {
    display.drawRoundRect(x + 2, y + 2, 8, 8, 3, color);
    display.drawFastHLine(x + 1, y + 10, 10, color);
    display.drawPixel(x + 6, y + 12, color);
    display.drawFastVLine(x + 6, y, 2, color);
}

void Reminders::drawProgressDots(uint8_t activeIndex, int y) {
    if (itemCount == 0) return;

    int totalWidth = itemCount * 8 - 3;
    int startX = (SCREEN_WIDTH - totalWidth) / 2;

    for (uint8_t i = 0; i < itemCount; i++) {
        int x = startX + i * 8;
        if (i == activeIndex) {
            display.fillCircle(x, y, 2, SSD1306_WHITE);
        } else {
            display.drawPixel(x, y, SSD1306_WHITE);
        }
    }
}

void Reminders::drawWrappedText(const String &text, int x, int y, uint8_t charsPerLine, uint8_t maxLines) {
    int offset = 0;

    for (uint8_t line = 0; line < maxLines && offset < text.length(); line++) {
        while (offset < text.length() && text[offset] == ' ') offset++;

        int end = offset + charsPerLine;
        if (end > text.length()) end = text.length();

        if (end < text.length()) {
            int split = end;
            while (split > offset && text[split] != ' ') split--;
            if (split > offset + 6) end = split;
        }

        String part = text.substring(offset, end);
        part.trim();

        if (line == maxLines - 1 && end < text.length() && part.length() > 3) {
            part = part.substring(0, part.length() - 3) + "...";
        }

        display.setCursor(x, y + line * 10);
        display.print(part);
        offset = end;
        if (offset < text.length() && text[offset] == ' ') offset++;
    }
}
