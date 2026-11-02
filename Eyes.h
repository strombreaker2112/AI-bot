#pragma once
#include <Arduino.h>
#include "Display.h"
#include "Config.h"

class Eyes {
public:
    void begin();
    void update();
    void setState(EyeState st);
    
    EyeState state;
    
private:
    // State management
    EyeState prevState;
    unsigned long stateStart;
    
    // Eye movement
    float curX, curY;
    float tgtX, tgtY;
    unsigned long nextSaccade;
    
    // Blinking & Lid Smoothing
    bool blinkActive;
    unsigned long blinkStart;
    unsigned long nextBlink;
    float curLidL, curLidR; // Added to fix compilation error
    
    // Idle behavior tracking
    unsigned long idleTimer;
    unsigned long lastIdleAction;
    int consecutiveNeutral;
    int currentIdleBehavior;
    EyeState idleEmotionBag[10];
    uint8_t idleEmotionIndex;
    uint8_t idleEmotionCount;
    
    // Core animation functions
    void doState(EyeState st);
    void scheduleSaccade();
    void updateIdleBehavior();
    void shuffleIdleEmotions();
    EyeState nextIdleEmotion();
    
    // Drawing functions
    void drawSquareEye(int x, int y, int w, int h, int r, float lidPct);
    
    // Animation helpers
    float computeBlink();
    float smooth(float a, float b, float t);
    
    // Easing functions
    float easeInOutCubic(float t);
    float easeOutBounce(float t);
};

extern Eyes eyes;
