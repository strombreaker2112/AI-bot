#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>
#include "Config.h"

class WebControl {
public:
    void begin();
    void update();
    String getIP(); 

private:
    WebServer server;       
    Preferences preferences;

    // Connection Monitoring
    unsigned long lastStatusCheck = 0;
    bool isConnected = false;
    String targetSSID = "";
    String targetPass = "";

    void handleRoot();
    void handleSaveWiFi();
    void handleSetMode();
    void handleTrainPage();
    void handleReminderPage();
    void handleSaveReminders();
    String getHTML();
    String getTrainHTML();
    String getReminderHTML();
};

extern WebControl webControl;
