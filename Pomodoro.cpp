#include "Pomodoro.h"

Pomodoro pomodoro;

void Pomodoro::begin() {
    state = P_STOPPED;
    setMinutes = 25; 
    stop();
}

void Pomodoro::setDuration(int min) {
    if (min < 1) min = 1;
    if (min > 99) min = 99;
    setMinutes = min;
    minutes = min;
    seconds = 0;
    state = P_STOPPED;
}

void Pomodoro::stop() {
    state = P_STOPPED;
    minutes = setMinutes;
    seconds = 0;
    // FIX: Ensure screen is definitely back to normal
    display.invertDisplay(false); 
}

void Pomodoro::start() {
    if (state == P_STOPPED) {
        state = P_RUNNING;
        lastTick = millis();
        // FIX: Ensure screen is normal before starting
        display.invertDisplay(false);
    }
}

void Pomodoro::addTime() {
    if (state == P_STOPPED) {
        setMinutes += 5;
        if (setMinutes > 60) setMinutes = 5; 
        minutes = setMinutes;
        seconds = 0;
    }
}

bool Pomodoro::isRunning() {
    return state == P_RUNNING;
}

void Pomodoro::update() {
    // Timer Logic
    if (state == P_RUNNING) {
        if (millis() - lastTick >= 1000) {
            lastTick = millis();
            if (seconds == 0) {
                if (minutes == 0) {
                    state = P_DONE; // Timer finished
                } else {
                    minutes--;
                    seconds = 59;
                }
            } else {
                seconds--;
            }
        }
    }

    clearDisplayFast();
    display.setTextSize(1);
    
    if (state == P_STOPPED) {
        drawCenteredText("SET TIME", 5);
        drawCenteredText("Start via APP/Btn", 55);
        drawTimer();
    }
    else if (state == P_RUNNING) {
        drawCenteredText("FOCUSING...", 5);
        drawTimer();

        // --- LOADING BAR ---
        int totalSeconds = setMinutes * 60;
        int remainingSeconds = minutes * 60 + seconds;
        
        display.drawRect(0, 54, SCREEN_WIDTH, 8, SSD1306_WHITE);
        
        if (totalSeconds > 0) {
            int innerW = map(totalSeconds - remainingSeconds, 0, totalSeconds, 0, SCREEN_WIDTH - 4);
            display.fillRect(2, 56, innerW, 4, SSD1306_WHITE);
        }
    }
    else if (state == P_DONE) {
        // --- CELEBRATION (FIXED) ---
        
        // 1. Force Inversion OFF (Fixes the "Blue Screen/Black Eyes" bug)
        display.invertDisplay(false);

        // 2. Blink the TEXT instead of the whole screen
        bool visible = (millis() / 250) % 2 == 0;
        
        if (visible) {
            display.setTextSize(3);
            display.setCursor(5, 20);
            display.print("DONE!!");
        }
        
        // 3. Random Confetti
        for(int i=0; i<20; i++) {
            display.drawPixel(random(SCREEN_WIDTH), random(SCREEN_HEIGHT), SSD1306_WHITE);
        }
        
        display.setTextSize(1);
        display.setCursor(25, 55);
        display.print("Press Btn");
    }

    display.display();
}

void Pomodoro::drawTimer() {
    display.setTextSize(3);
    int x = 20; 
    int y = 20;
    display.setCursor(x, y);

    if (minutes < 10) display.print("0");
    display.print(minutes);
    
    // Blink colon only when running
    if (state == P_RUNNING) {
        if ((millis() / 500) % 2 == 0) {
            display.print(":");
        } else {
            display.print(" ");
        }
    } else {
        display.print(":");
    }
    
    if (seconds < 10) display.print("0");
    display.print(seconds);
}
