#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Display.h"

class ClockApp {
public:
    void begin();
    void update();
    
private:
    // Main drawing functions
    void drawTime(int h, int m, int s, bool isPM);
    
    // Time-of-day specific styles
    void drawMorningClock(int h, int m, int s, bool isPM);    // 6 AM - 12 PM
    void drawNoonClock(int h, int m, int s, bool isPM);       // 12 PM
    void drawAfternoonClock(int h, int m, int s, bool isPM);  // 1 PM - 6 PM
    void drawEveningClock(int h, int m, int s, bool isPM);    // 6 PM - 9 PM
    void drawNightClock(int h, int m, int s, bool isPM);      // 9 PM - 6 AM
    
    // Helper functions
    void drawCloud(int x, int y);
    
    // State tracking
    unsigned long lastUpdate;
    int lastSecond;
    int animPhase;
};

extern ClockApp clockApp;