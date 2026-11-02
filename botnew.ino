#include <Arduino.h>
#include <WiFi.h>
#include "src/Config.h"
#include "src/Display.h"
#include "src/Eyes.h"
#include "src/ClockApp.h"
#include "src/Pomodoro.h"
#include "src/Weather.h"
#include "src/Reminders.h"
#include "src/WebControl.h"
#include "src/DesktopApp.h" 
// --- Global Variables ---
int currentMode = MODE_EYES;

// --- Interrupt Variables (These run in the background) ---
volatile bool isTouchedISR = false;
volatile unsigned long touchStartTime = 0;
volatile unsigned long touchDuration = 0;
volatile bool newReleaseReady = false;

// --- Touch Logic Trackers ---
unsigned long lastTapTime = 0;
bool waitingSecondTap = false;
bool longPressTriggered = false;

// --- Idle Mood Timings ---
unsigned long lastPetTime = 0;
const unsigned long TIME_TO_OFFEND = 30UL * 60UL * 1000UL; 
const unsigned long TIME_TO_SLEEP = 50UL * 60UL * 1000UL; 

// ==========================================
// BACKGROUND INTERRUPT SERVICE ROUTINE (ISR)
// ==========================================
void IRAM_ATTR touchInterrupt() {
  if (digitalRead(PIN_TOUCH) == HIGH) {
    isTouchedISR = true;
    touchStartTime = millis();
  } else {
    isTouchedISR = false;
    touchDuration = millis() - touchStartTime;
    newReleaseReady = true; // Signal the main loop that a tap finished
  }
}

void setup() {
  Serial.begin(115200);
  initDisplay();
  
  // Initialize Services
  webControl.begin(); 
  eyes.begin();
  clockApp.begin();
  pomodoro.begin();
  weather.begin();
  reminders.begin();
  desktopApp.begin(); 
  
  eyes.setState(E_WAKEUP);
  lastPetTime = millis(); 

  // Setup Touch Pin and Attach the Background Interrupt
  pinMode(PIN_TOUCH, INPUT); 
  attachInterrupt(digitalPinToInterrupt(PIN_TOUCH), touchInterrupt, CHANGE);
}

void handleTouchInteraction() {
  unsigned long now = millis();

  // --- 1. WHILE HOLDING (Immediate "Petting" feedback) ---
  if (isTouchedISR && !longPressTriggered && (now - touchStartTime > LONG_PRESS)) {
    // Force switch to eyes mode to show reaction
    if (currentMode != MODE_EYES) {
        currentMode = MODE_EYES;
        clearDisplayFast();
    }
    eyes.setState(E_PETTING);
    lastPetTime = now;
    longPressTriggered = true; // Prevent animation freezing
  }

  // --- 2. TOUCH RELEASED (Process the tap) ---
  if (newReleaseReady) {
    newReleaseReady = false; // Acknowledge we saw the release

    if (longPressTriggered) {
      // If we just finished petting, reset and ignore this release
      longPressTriggered = false; 
      waitingSecondTap = false;
    } 
    // Uses the TAP_MIN and TAP_MAX from your updated Config.h
    else if (touchDuration > TAP_MIN && touchDuration < TAP_MAX) { 
      
      // Check if this completes a Double Tap
      if (waitingSecondTap && (now - lastTapTime < DOUBLE_TAP_GAP)) {
        
        if (currentMode == MODE_POMODORO) {
          if (pomodoro.isRunning()) {
            pomodoro.stop();
            Serial.println("Double Tap: Pomodoro Stopped");
          } else {
            pomodoro.start();
            Serial.println("Double Tap: Pomodoro Started");
          }
        } else if (currentMode == MODE_REMINDERS) {
          reminders.startManualList();
          Serial.println("Double Tap: Reminders Restarted");
        } else {
          currentMode = MODE_REMINDERS;
          reminders.startManualList();
          clearDisplayFast();
          Serial.println("Double Tap: Reminders Opened");
        }
        waitingSecondTap = false;
        
      } else {
        // First tap detected, wait to see if a second one happens
        waitingSecondTap = true;
        lastTapTime = now;
      }
    }
  }

  // --- 3. SINGLE TAP CONFIRMATION ---
  // If the gap time passes and no second tap happened, it's a single tap
  if (waitingSecondTap && (now - lastTapTime > DOUBLE_TAP_GAP)) {
    
    // ACTION: SINGLE TAP -> CHANGE MODE
    currentMode++;
    if (currentMode >= MODE_COUNT) currentMode = 0;
    if (currentMode == MODE_REMINDERS) {
      reminders.startManualList();
    }
    clearDisplayFast();
    Serial.println("Single Tap: Mode Changed");
    
    waitingSecondTap = false;
  }

  // --- 4. IDLE MOOD LOGIC (Only in Eyes Mode) ---
  if (currentMode == MODE_EYES && !isTouchedISR) {
    unsigned long elapsed = now - lastPetTime;
    if (elapsed > TIME_TO_SLEEP) {
      eyes.setState(E_SLEEP);
    } else if (elapsed > TIME_TO_OFFEND) {
      eyes.setState(E_OFFENDED);
    }
  }
}

void loop() {
  // Always update web interface
  webControl.update();
  
  // Check the interrupt flags
  handleTouchInteraction();

  // Run the active application
  switch (currentMode) {
    case MODE_EYES:     eyes.update(); break;
    case MODE_CLOCK:    clockApp.update(); break;
    case MODE_POMODORO: pomodoro.update(); break;
    case MODE_WEATHER:  weather.update(); break;
    case MODE_DESKTOP:  desktopApp.update(); break;
    case MODE_REMINDERS: reminders.update(); break;
  }

  if (currentMode != MODE_REMINDERS) {
    reminders.updateAutoNotification();
  }
}
