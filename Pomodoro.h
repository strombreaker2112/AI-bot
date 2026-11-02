#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Display.h"

enum PomoState { P_STOPPED, P_RUNNING, P_DONE };

class Pomodoro {
public:
    void begin();
    void update();
    
    // Controls
    void addTime();  
    void start();    
    void stop();     
    bool isRunning();
    
    // NEW: Set custom duration
    void setDuration(int min);

private:
    PomoState state = P_STOPPED;
    unsigned long lastTick = 0;
    
    int setMinutes = 25; 
    int minutes = 25;    
    int seconds = 0;
    
    void drawTimer();
};

extern Pomodoro pomodoro;