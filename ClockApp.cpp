#include "ClockApp.h"
#include <time.h>

ClockApp clockApp;

void ClockApp::begin() {
    lastUpdate = 0;
    lastSecond = -1;
    animPhase = 0;
    clearDisplayFast();

    // =======================================================
    // KOLHAPUR / INDIA CONFIGURATION
    // =======================================================
    // Indian Standard Time (IST) is UTC + 5:30
    // Offset = (5 * 3600) + (30 * 60) = 19800 seconds
    // India does not observe Daylight Saving Time (DST)
    configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
}

void ClockApp::update() {
    unsigned long now = millis();
    
    // Update every 100ms for smooth animations
    if (now - lastUpdate < 100) return;
    lastUpdate = now;
    
    // Get current time
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        clearDisplayFast();
        display.setTextSize(1);
        display.setCursor(10, 20);
        display.print("Syncing...");
        display.setCursor(10, 35);
        display.print("Kolhapur, MH");
        display.display();
        return;
    }

    int h = timeinfo.tm_hour;
    int m = timeinfo.tm_min;
    int s = timeinfo.tm_sec;
    
    // Convert to 12-hour format
    bool isPM = (h >= 12);
    if (h == 0) h = 12;
    else if (h > 12) h -= 12;
    
    // Detect second change for animations
    if (s != lastSecond) {
        lastSecond = s;
        animPhase = 0;
    }
    
    // Draw the clock
    clearDisplayFast();
    drawTime(h, m, s, isPM);
    display.display();
    
    animPhase++;
}

void ClockApp::drawTime(int h, int m, int s, bool isPM) {
    // Choose style based on time of day
    int style = 0;
    
    if (h >= 6 && h < 12 && !isPM) {
        style = 0; // Morning 
    } else if (h == 12 && isPM) {
        style = 1; // Noon
    } else if (h >= 1 && h < 6 && isPM) {
        style = 2; // Afternoon
    } else if (h >= 6 && h < 9 && isPM) {
        style = 3; // Evening
    } else {
        style = 4; // Night
    }
    
    switch(style) {
        case 0: drawMorningClock(h, m, s, isPM); break;
        case 1: drawNoonClock(h, m, s, isPM); break;
        case 2: drawAfternoonClock(h, m, s, isPM); break;
        case 3: drawEveningClock(h, m, s, isPM); break;
        case 4: drawNightClock(h, m, s, isPM); break;
    }
}

// ========================================
// MORNING STYLE (6 AM - 12 PM)
// Peeking rising sun & fluffy clouds
// ========================================
void ClockApp::drawMorningClock(int h, int m, int s, bool isPM) {
    // 1. Rising sun that slowly bobs up and down
    int sunY = 35 - (int)(sin(animPhase * 0.05f) * 6);
    display.fillCircle(105, sunY, 12, SSD1306_WHITE);
    
    // Cute dotted sun rays
    for (int i = 0; i < 5; i++) {
        float angle = PI + (i * PI / 4); // Top half only
        int rx = 105 + (int)(cos(angle) * 18);
        int ry = sunY + (int)(sin(angle) * 18);
        display.fillCircle(rx, ry, 1, SSD1306_WHITE);
    }

    // 2. Fluffy cloud covering the bottom of the sun
    display.fillCircle(85, 55, 8, SSD1306_WHITE);
    display.fillCircle(105, 48, 14, SSD1306_WHITE);
    display.fillCircle(125, 55, 10, SSD1306_WHITE);
    display.fillRect(85, 52, 40, 12, SSD1306_WHITE); // Solid cloud base

    // 3. Bouncy time on the left
    display.setTextSize(2);
    char timeStr[6];
    sprintf(timeStr, "%2d:%02d", h, m);
    int bounce = (int)(sin(animPhase * 0.15f) * 4); // Soft bounce effect
    
    display.setCursor(10, 24 + bounce);
    display.print(timeStr);
    
    // AM Tag
    display.setTextSize(1);
    display.setCursor(10, 45 + bounce);
    display.print("AM");
}

// ========================================
// NOON STYLE (12 PM)
// Bright sun & cute rounded frame
// ========================================
void ClockApp::drawNoonClock(int h, int m, int s, bool isPM) {
    // 1. Big bright sun at the top center
    display.fillCircle(64, 0, 16, SSD1306_WHITE); 
    
    // Spinning dotted rays
    for (int i = 0; i < 8; i++) {
        float angle = (i * 45 + animPhase * 2) * PI / 180.0f;
        int rx = 64 + (int)(cos(angle) * 24);
        int ry = (int)(sin(angle) * 24);
        if(ry > 0) display.fillCircle(rx, ry, 2, SSD1306_WHITE); // Keep rays on screen
    }
    
    // 2. Cute rounded bubble frame for the time
    display.fillRoundRect(14, 30, 100, 32, 12, SSD1306_BLACK); // Clear background
    display.drawRoundRect(14, 30, 100, 32, 12, SSD1306_WHITE); // White border
    
    // 3. Centered Time inside the bubble
    display.setTextSize(2);
    char timeStr[6];
    sprintf(timeStr, "%2d:%02d", h, m);
    int x = (128 - (strlen(timeStr) * 12)) / 2;
    
    display.setCursor(x, 38);
    display.print(timeStr);
}

// ========================================
// AFTERNOON STYLE (1 PM - 6 PM)
// Floating hearts
// ========================================
void ClockApp::drawAfternoonClock(int h, int m, int s, bool isPM) {
    // 1. Animated floating hearts bubbling up
    for(int i = 0; i < 3; i++) {
        // Sway back and forth using sine, move up over time
        int hX = 25 + (i * 35) + (sin((animPhase + i * 10) * 0.15) * 6);
        int hY = 70 - ((animPhase * 2 + i * 25) % 80);
        
        if (hY > -10 && hY < 64) {
            // Draw a tiny cute heart using two circles and a triangle
            display.fillCircle(hX - 2, hY, 2, SSD1306_WHITE);
            display.fillCircle(hX + 2, hY, 2, SSD1306_WHITE);
            display.fillTriangle(hX - 4, hY + 1, hX + 4, hY + 1, hX, hY + 5, SSD1306_WHITE);
        }
    }
    
    // 2. Cute rounded bubble frame for the time
    display.fillRoundRect(22, 16, 84, 32, 10, SSD1306_BLACK); // Clear background
    display.drawRoundRect(22, 16, 84, 32, 10, SSD1306_WHITE); // White border
    
    // 3. Centered Time
    display.setTextSize(2);
    char timeStr[6];
    sprintf(timeStr, "%2d:%02d", h, m);
    int x = (128 - (strlen(timeStr) * 12)) / 2;
    
    display.setCursor(x, 24);
    display.print(timeStr);
}

// ========================================
// EVENING STYLE (6 PM - 9 PM)
// Magic sparkles and dreamy moon
// ========================================
void ClockApp::drawEveningClock(int h, int m, int s, bool isPM) {
    // 1. Dreamy crescent moon
    display.fillCircle(105, 22, 14, SSD1306_WHITE);
    display.fillCircle(100, 20, 11, SSD1306_BLACK); // Cutout to make it a crescent
    
    // 2. Twinkling magic sparkles (four-point stars)
    int sparkles[4][2] = {{15, 12}, {35, 45}, {80, 52}, {115, 55}};
    for(int i = 0; i < 4; i++) {
        // Blink them on and off based on animation phase
        if((animPhase + i * 7) % 15 < 8) {
            int sx = sparkles[i][0];
            int sy = sparkles[i][1];
            display.drawLine(sx - 3, sy, sx + 3, sy, SSD1306_WHITE);
            display.drawLine(sx, sy - 3, sx, sy + 3, SSD1306_WHITE);
        }
    }
    
    // 3. Bouncy time
    display.setTextSize(3);
    char timeStr[6];
    sprintf(timeStr, "%2d:%02d", h, m);
    
    // Calculate a soft bounce effect
    int bounce = (int)(sin(animPhase * 0.15f) * 4);
    display.setCursor(12, 22 + bounce);
    display.print(timeStr);
    
    // 4. Little AM/PM tag
    display.setTextSize(1);
    display.setCursor(12, 50 + bounce);
    display.print(isPM ? "PM" : "AM");
}

// ========================================
// NIGHT STYLE (9 PM - 6 AM)
// Sleepy moon with closed eyes
// ========================================
void ClockApp::drawNightClock(int h, int m, int s, bool isPM) {
    // 1. Sleepy Crescent Moon
    display.fillCircle(95, 32, 18, SSD1306_WHITE);
    display.fillCircle(90, 28, 15, SSD1306_BLACK); // Crescent cutout
    
    // Cute closed, sleepy eye on the moon
    display.drawLine(97, 36, 103, 36, SSD1306_WHITE);
    display.drawLine(97, 36, 99, 38, SSD1306_WHITE); // Little eyelash
    
    // 2. Floating "Zzz"s
    float zFloat = sin(animPhase * 0.1f);
    display.setTextSize(1);
    display.setCursor(110, 10 + (int)(zFloat * 2)); display.print("Z");
    display.setCursor(118, 5 + (int)(zFloat * 3));  display.print("z");

    // 3. Magical four-point twinkle stars
    if (animPhase % 20 < 10) {
        display.drawLine(20, 10, 24, 10, SSD1306_WHITE); // Horizontal
        display.drawLine(22, 8, 22, 12, SSD1306_WHITE);  // Vertical
    }
    if ((animPhase + 10) % 20 < 10) {
        display.drawLine(60, 50, 64, 50, SSD1306_WHITE);
        display.drawLine(62, 48, 62, 52, SSD1306_WHITE);
    }

    // 4. Soft time display
    display.setTextSize(3);
    char timeStr[6];
    sprintf(timeStr, "%2d:%02d", h, m);
    display.setCursor(5, 24);
    display.print(timeStr);
}

// ========================================
// HELPER FUNCTIONS
// ========================================
void ClockApp::drawCloud(int x, int y) {
    if (x < -20 || x > 128) return;
    
    display.fillCircle(x + 5, y + 3, 3, SSD1306_WHITE);
    display.fillCircle(x + 10, y + 2, 4, SSD1306_WHITE);
    display.fillCircle(x + 15, y + 3, 3, SSD1306_WHITE);
    display.fillRect(x + 5, y + 3, 10, 3, SSD1306_WHITE);
}