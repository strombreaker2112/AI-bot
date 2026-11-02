#include "DesktopApp.h"

DesktopApp desktopApp;

void DesktopApp::begin() {
    shuffle();
}

void DesktopApp::shuffle() {
    // Range 0-7
    patternType = random(0, 8); 
    lastShuffle = millis();
}

void DesktopApp::update() {
    // Shuffle pattern every 6 seconds
    if (millis() - lastShuffle > 6000) {
        shuffle();
    }

    clearDisplayFast();

    switch (patternType) {
        case 0: drawLines(); break;
        case 1: drawStars(); break;
        case 2: drawRects(); break;
        case 3: drawFireworks(); break;
        case 4: drawAmoeba(); break;   // Changed
        case 5: drawBioCells(); break; // Updated
        case 6: drawNeonTunnel(); break;
        case 7: drawCometOrbit(); break;
    }

    display.display();
}

// --- PATTERNS ---

void DesktopApp::drawLines() {
    for (int i = 0; i < SCREEN_WIDTH; i += 8) {
        int len = (int)(sin((millis() + i * 10) / 200.0) * 10.0) + 10;
        int y = (millis() / 10 + i) % (SCREEN_HEIGHT + 20) - 20;
        display.drawFastVLine(i, y, len, SSD1306_WHITE);
    }
}

void DesktopApp::drawStars() {
    for (int i = 0; i < 30; i++) {
        int x = (i * 137) % SCREEN_WIDTH;
        int y = (i * 59) % SCREEN_HEIGHT;
        bool show = ((millis() / (100 + i * 10)) % 2 == 0);
        if (show) display.drawPixel(x, y, SSD1306_WHITE);
    }
}

void DesktopApp::drawRects() {
    int size = 10;
    int x = (int)(sin(millis() / 500.0) * (SCREEN_WIDTH/2 - size)) + SCREEN_WIDTH/2;
    int y = (int)(cos(millis() / 400.0) * (SCREEN_HEIGHT/2 - size)) + SCREEN_HEIGHT/2;
    display.drawRect(x - size, y - size, size * 2, size * 2, SSD1306_WHITE);
    display.drawRect(SCREEN_WIDTH - x - size, SCREEN_HEIGHT - y - size, size * 2, size * 2, SSD1306_WHITE);
}

void DesktopApp::drawFireworks() {
    int explosionCount = 3; 
    int duration = 1500;

    for (int e = 0; e < explosionCount; e++) {
        unsigned long t = millis() + (e * duration / explosionCount);
        int phase = t % duration;
        int cycle = t / duration;

        int cx = (cycle * 53 + e * 97) % (SCREEN_WIDTH - 20) + 10;
        int cy = (cycle * 29 + e * 43) % (SCREEN_HEIGHT - 20) + 10;
        int maxRadius = 25;
        int r = (phase * maxRadius) / duration;

        if (phase < duration - 200) {
            for (int i = 0; i < 8; i++) {
                float angle = (i * 45) * 3.14159 / 180.0;
                int px = cx + (int)(cos(angle) * r);
                int py = cy + (int)(sin(angle) * r) + (r / 5);
                display.drawPixel(px, py, SSD1306_WHITE);
            }
        }
    }
}

// ==========================================
// GIANT AMOEBA (Replaces Grid)
// One large, undulating organic cell
// ==========================================
void DesktopApp::drawAmoeba() {
    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;
    
    // We draw the shape by connecting vertices
    int prevX = 0, prevY = 0;
    int firstX = 0, firstY = 0;
    int steps = 24; // Number of vertices for the shape
    
    for (int i = 0; i <= steps; i++) {
        float angle = (i * 360.0 / steps) * PI / 180.0;
        
        // Dynamic radius: Base size + combined sine waves for wobble
        // We use different multipliers for time and angle to make it look "gloopy"
        float t = millis() / 1000.0;
        float r = 22.0 
                  + sin(angle * 3 + t) * 4.0 
                  + sin(angle * 5 - t * 2.0) * 2.0;
        
        int x = cx + (int)(cos(angle) * r);
        int y = cy + (int)(sin(angle) * r);
        
        if (i > 0) {
            display.drawLine(prevX, prevY, x, y, SSD1306_WHITE);
        } else {
            firstX = x;
            firstY = y;
        }
        prevX = x;
        prevY = y;
    }
    // Close the loop
    display.drawLine(prevX, prevY, firstX, firstY, SSD1306_WHITE);
    
    // Draw Nucleus (inner floating blob)
    int nx = cx + (int)(sin(millis()/800.0) * 8);
    int ny = cy + (int)(cos(millis()/900.0) * 8);
    display.fillCircle(nx, ny, 3, SSD1306_WHITE);
    
    // Draw a vacuole (empty circle)
    int vx = cx + (int)(sin(millis()/1100.0 + 2) * 12);
    int vy = cy + (int)(cos(millis()/1200.0 + 2) * 12);
    display.drawCircle(vx, vy, 4, SSD1306_WHITE);
}

// ==========================================
// BIOTECH CELLS (Updated)
// Drift across screen with random-looking paths
// ==========================================
void DesktopApp::drawBioCells() {
    for(int i = 0; i < 7; i++) {
        // Use prime numbers to create chaotic, non-repeating paths
        // x moves linearly with wrap-around
        // y oscillates
        
        float speed = 10.0 + (i % 3) * 5.0; // Different speeds
        
        // Calculate X: linear drift
        // We add large offsets (i*5000) so they don't all start at 0
        int x = (int)((millis() / (30.0 + i*5) + i * 50)) % (SCREEN_WIDTH + 30) - 15;
        
        // Calculate Y: complex sine wave
        int y = 32 + (int)(sin(millis()/(800.0 + i*100)) * 25);
        
        // Size varies slightly
        int r = 4 + (i % 3);
        
        // Draw Cell
        display.drawCircle(x, y, r, SSD1306_WHITE);
        
        // Nucleus
        display.drawPixel(x, y, SSD1306_WHITE);
        
        // Connections: occasionally draw lines between close cells
        if (i > 0) {
             // Peek previous cell's position (simplified math for speed)
             int px = (int)((millis() / (30.0 + (i-1)*5) + (i-1) * 50)) % (SCREEN_WIDTH + 30) - 15;
             int py = 32 + (int)(sin(millis()/(800.0 + (i-1)*100)) * 25);
             
             // If close, draw a neural-like connection
             if (abs(x - px) < 30 && abs(y - py) < 30) {
                 display.drawLine(x, y, px, py, SSD1306_WHITE);
             }
        }
    }
}

// ==========================================
// NEON TUNNEL
// Expanding perspective rectangles with star streaks
// ==========================================
void DesktopApp::drawNeonTunnel() {
    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;
    int phase = (millis() / 70) % 14;

    for (int i = 0; i < 5; i++) {
        int inset = (i * 14 + phase) % 58;
        int x = inset;
        int y = inset / 2;
        int w = SCREEN_WIDTH - inset * 2;
        int h = SCREEN_HEIGHT - inset;

        if (w > 5 && h > 5) {
            display.drawRect(x, y, w, h, SSD1306_WHITE);
        }
    }

    for (int i = 0; i < 10; i++) {
        int angle = i * 36;
        float rad = angle * PI / 180.0;
        int len = 18 + ((millis() / 80 + i * 3) % 18);
        int x1 = cx + (int)(cos(rad) * 5);
        int y1 = cy + (int)(sin(rad) * 3);
        int x2 = cx + (int)(cos(rad) * len);
        int y2 = cy + (int)(sin(rad) * (len / 2));
        display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
    }

    display.fillCircle(cx, cy, 2, SSD1306_WHITE);
}

// ==========================================
// COMET ORBIT
// Swirling comets around a pulsing center
// ==========================================
void DesktopApp::drawCometOrbit() {
    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;
    float t = millis() / 650.0;

    display.drawCircle(cx, cy, 5 + ((millis() / 350) % 2), SSD1306_WHITE);
    display.drawCircle(cx, cy, 18, SSD1306_WHITE);
    display.drawCircle(cx, cy, 27, SSD1306_WHITE);

    for (int i = 0; i < 3; i++) {
        float angle = t + i * 2.094;
        int radius = 18 + i * 5;
        int x = cx + (int)(cos(angle) * radius);
        int y = cy + (int)(sin(angle) * (radius / 2));

        display.fillCircle(x, y, 2, SSD1306_WHITE);

        for (int tail = 1; tail <= 5; tail++) {
            float tailAngle = angle - tail * 0.18;
            int tx = cx + (int)(cos(tailAngle) * radius);
            int ty = cy + (int)(sin(tailAngle) * (radius / 2));

            if (tail < 3) {
                display.drawPixel(tx, ty, SSD1306_WHITE);
                display.drawPixel(tx + 1, ty, SSD1306_WHITE);
            } else {
                display.drawPixel(tx, ty, SSD1306_WHITE);
            }
        }
    }
}
