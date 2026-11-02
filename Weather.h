#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Display.h"
#include <HTTPClient.h>
#include <ArduinoJson.h> 

class Weather {
public:
    void begin();
    void update(); 
    void fetch();  

private:
    float temp = 0.0;
    String condition = "--";
    unsigned long lastFetch = 0;
    bool hasData = false;
};

extern Weather weather;