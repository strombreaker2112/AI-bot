#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Display.h"

class DesktopApp {
public:
    void begin();
    void update();

private:
    unsigned long lastShuffle = 0;
    int patternType = 0; 
    
    // Pattern Drawing Functions
    void drawLines();
    void drawStars();
    void drawRects();
    void drawFireworks();
    void drawAmoeba();   // REPLACED Grid with Giant Amoeba
    void drawBioCells(); // IMPROVED: Random movement
    void drawNeonTunnel();
    void drawCometOrbit();
    
    void shuffle();
};

extern DesktopApp desktopApp;
