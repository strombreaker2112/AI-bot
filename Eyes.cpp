#include "Eyes.h"
#include <math.h>

Eyes eyes;

// Center Positions
static constexpr int L_EYE_CX = (SCREEN_WIDTH - (2 * EYE_W + EYE_GAP)) / 2;
static constexpr int R_EYE_CX = L_EYE_CX + EYE_W + EYE_GAP;
static constexpr int EYE_CY   = (SCREEN_HEIGHT - EYE_H) / 2;

// =====================================================
// EASING FUNCTIONS for smooth, natural animations
// =====================================================
float Eyes::easeInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

float Eyes::easeOutBounce(float t) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;
    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

float Eyes::smooth(float a, float b, float t) {
    return a + (b - a) * t;
}

// =====================================================
// INITIALIZATION
// =====================================================
void Eyes::begin() {
    state = E_SLEEP;  // Start sleeping
    prevState = E_NEUTRAL;
    stateStart = millis();
    curX = curY = 0;
    tgtX = tgtY = 0;
    
    // Smooth lid tracking
    curLidL = curLidR = 1.0f; 
    
    nextSaccade = millis() + 2000;
    nextBlink = millis() + 3000;
    blinkActive = false;
    
    // Idle behavior tracking
    idleTimer = millis();
    lastIdleAction = millis();
    consecutiveNeutral = 0;
    currentIdleBehavior = 0;
    idleEmotionIndex = 0;
    idleEmotionCount = 0;
    shuffleIdleEmotions();
    
    clearDisplayFast();
    
    // Wake up after brief pause
    setState(E_WAKEUP);
}

void Eyes::setState(EyeState st) {
    if (state != st) {
        prevState = state;
        state = st;
        stateStart = millis();
        
        // Natural transition logic: certain states reset looking target
        if (st == E_NEUTRAL || st == E_SLEEP) {
            consecutiveNeutral++;
            tgtX = 0; 
            tgtY = 0;
        } else {
            consecutiveNeutral = 0;
        }
    }
    else if (st == E_PETTING) {
        stateStart = millis();  // Allow re-triggering petting
    }
}

// =====================================================
// DRAWING FUNCTIONS
// =====================================================
void Eyes::drawSquareEye(int x, int y, int w, int h, int r, float lidPct) {
    if (lidPct < 0) lidPct = 0;
    if (lidPct > 1) lidPct = 1;

    int lidH = (int)(lidPct * h); 

    // Draw eye base
    display.fillRoundRect(x, y, w, h, r, SSD1306_WHITE);

    if (lidH > 0) {
        // Smooth eyelid closing
        int topLid = (lidH / 2) + 1;
        int bottomLid = (lidH / 2) + 1;
        
        display.fillRect(x, y, w, topLid, SSD1306_BLACK);
        display.fillRect(x, y + h - bottomLid, w, bottomLid, SSD1306_BLACK);
        
        // Subtle eyelid edge for depth
        if (lidPct > 0.2f) {
            display.drawLine(x + 1, y + topLid, x + w - 2, y + topLid, SSD1306_WHITE);
            display.drawLine(x + 1, y + h - bottomLid, x + w - 2, y + h - bottomLid, SSD1306_WHITE);
        }
    }
}

float Eyes::computeBlink() {
    if (!blinkActive) return 0.0f;
    
    float t = (millis() - blinkStart) / 220.0f; // Slightly faster blink for realism
    
    if (t >= 1.0f) { 
        blinkActive = false; 
        return 0.0f; 
    }
    
    // Natural asymmetric blink curve (fast shut, slower open)
    if (t < 0.3f) {
        return easeInOutCubic(t / 0.3f);
    } 
    else {
        return 1.0f - easeInOutCubic((t - 0.3f) / 0.7f);
    }
}

// =====================================================
// EYE MOVEMENT SYSTEM
// =====================================================
void Eyes::scheduleSaccade() {
    int behavior = random(0, 100);
    
    // Saccades are sharper and more frequent in "alert" states
    if (state == E_SCAN || state == E_CURIOUS) {
        tgtX = random(-EYE_SHIFT_RANGE, EYE_SHIFT_RANGE + 1);
        tgtY = random(-EYE_SHIFT_RANGE, EYE_SHIFT_RANGE + 1);
        nextSaccade = millis() + random(100, 600);
    } 
    else if (behavior < 40) {
        // Micro-adjustment (very subtle tremor/jitter)
        tgtX += random(-1, 2); 
        tgtY += random(-1, 2);
        nextSaccade = millis() + random(200, 800); 
    } 
    else if (behavior < 75) {
        // Standard glance
        tgtX = random(-EYE_SHIFT_RANGE, EYE_SHIFT_RANGE + 1);
        tgtY = random(-EYE_SHIFT_RANGE, EYE_SHIFT_RANGE + 1);
        nextSaccade = millis() + random(1000, 3000); 
    }
    else {
        // Reset to center
        tgtX = 0;
        tgtY = 0;
        nextSaccade = millis() + random(1500, 4000);
    }
    
    tgtX = constrain(tgtX, -EYE_SHIFT_RANGE, EYE_SHIFT_RANGE);
    tgtY = constrain(tgtY, -EYE_SHIFT_RANGE, EYE_SHIFT_RANGE);
}

void Eyes::shuffleIdleEmotions() {
    EyeState allEmotions[] = {
        E_HAPPY,
        E_SAD,
        E_ANGRY,
        E_SURPRISED,
        E_SQUINT,
        E_SCAN,
        E_WINK_L,
        E_WINK_R,
        E_OFFENDED,
        E_CURIOUS
    };

    idleEmotionCount = sizeof(allEmotions) / sizeof(allEmotions[0]);

    for (uint8_t i = 0; i < idleEmotionCount; i++) {
        idleEmotionBag[i] = allEmotions[i];
    }

    for (uint8_t i = idleEmotionCount - 1; i > 0; i--) {
        uint8_t j = random(0, i + 1);
        EyeState tmp = idleEmotionBag[i];
        idleEmotionBag[i] = idleEmotionBag[j];
        idleEmotionBag[j] = tmp;
    }

    idleEmotionIndex = 0;
}

EyeState Eyes::nextIdleEmotion() {
    if (idleEmotionCount == 0 || idleEmotionIndex >= idleEmotionCount) {
        shuffleIdleEmotions();
    }

    EyeState next = idleEmotionBag[idleEmotionIndex++];

    if (idleEmotionIndex >= idleEmotionCount) {
        shuffleIdleEmotions();
        if (idleEmotionCount > 1 && idleEmotionBag[0] == next) {
            uint8_t swapIndex = random(1, idleEmotionCount);
            EyeState tmp = idleEmotionBag[0];
            idleEmotionBag[0] = idleEmotionBag[swapIndex];
            idleEmotionBag[swapIndex] = tmp;
        }
    }

    return next;
}

// =====================================================
// ADVANCED IDLE BEHAVIOR SYSTEM (Refined Shuffle)
// =====================================================
void Eyes::updateIdleBehavior() {
    unsigned long t = millis();
    
    if (state != E_NEUTRAL) {
        lastIdleAction = t;
        return;
    }
    
    unsigned long timeSinceAction = t - lastIdleAction;
    
    // Check periodically for "spontaneous" life
    if (t - idleTimer < 800) return;
    idleTimer = t;

    // Boredom multiplier: more likely to do something the longer we wait
    int boredom = min(35, (int)(timeSinceAction / 800));
    
    if (random(0, 100) < (5 + boredom)) {
        EyeState next = nextIdleEmotion();
        currentIdleBehavior = (int)next;

        if (next == E_WINK_L || next == E_WINK_R) {
            nextBlink = t + 900;
        }

        setState(next);
        lastIdleAction = t;
    }
}

// =====================================================
// MAIN STATE MACHINE
// =====================================================
void Eyes::doState(EyeState st) {
    unsigned long t = millis();
    unsigned long dt = t - stateStart;
    
    // Target parameters for this frame
    float targetLidL = 0.0f;
    float targetLidR = 0.0f;
    int leftX = 0, leftY = 0;
    int rightX = 0, rightY = 0;
    int widthMod = 0;
    int heightMod = 0;

    // --- LIFE TREMOR ---
    // Subtle constant movement to simulate muscle micro-vibrations
    float lifeTremor = sin(t / 40.0f) * 0.4f;
    
    // Update idle logic
    updateIdleBehavior();
    
    // --- STATE DURATIONS ---
    if (st == E_HAPPY && dt > 2500) setState(E_NEUTRAL);
    if (st == E_SQUINT && dt > 2000) setState(E_NEUTRAL);
    if (st == E_SCAN && dt > 4600) setState(E_NEUTRAL);
    if (st == E_WINK_L && dt > 1000) setState(E_NEUTRAL);
    if (st == E_WINK_R && dt > 1000) setState(E_NEUTRAL);
    if (st == E_SLEEP && dt > 4000) setState(E_WAKEUP);
    if (st == E_OFFENDED && dt > 2500) setState(E_NEUTRAL);
    if (st == E_SURPRISED && dt > 2400) setState(E_NEUTRAL);
    if (st == E_SAD && dt > 3800) setState(E_NEUTRAL);
    if (st == E_ANGRY && dt > 3200) setState(E_NEUTRAL);
    if (st == E_CURIOUS && dt > 3000) setState(E_NEUTRAL);

    // --- EYE MOVEMENT INTERPOLATION ---
    if (t > nextSaccade) scheduleSaccade();
    
    float moveSpeed = (st == E_SURPRISED || st == E_SCAN) ? 0.45f : (st == E_SAD ? 0.28f : 0.22f);
    curX = smooth(curX, tgtX, moveSpeed);
    curY = smooth(curY, tgtY, moveSpeed);
    
    leftX = rightX = (int)(curX + lifeTremor);
    leftY = rightY = (int)(curY + lifeTremor);

    // --- NATURAL BLINKING ---
    if (st != E_SLEEP && st != E_WAKEUP && st != E_WINK_L && st != E_WINK_R && st != E_PETTING) {
        if (!blinkActive && t > nextBlink) {
            blinkActive = true;
            blinkStart = t;
            // Animals blink in bursts; randomized intervals
            nextBlink = t + random(1500, 9000);
        }
        float b = computeBlink();
        targetLidL = targetLidR = b;
    }

    // =====================================================
    // STATE LOGIC
    // =====================================================
    switch (st) {
        case E_SLEEP:
            targetLidL = targetLidR = 1.0f;
            heightMod = -(int)(sin(t / 1800.0f) * 2.5f + 2.5f); // Breathing
            break;

        case E_WAKEUP:
            {
                // Multi-stage organic wake-up
                if (dt < 600) { targetLidL = targetLidR = 1.0f; }
                else if (dt < 1200) { // First peek
                    targetLidL = targetLidR = 0.85f;
                    leftX = rightX += (int)(sin(t/50.0f)*2); // Groggy shake
                }
                else if (dt < 1800) { // Settle back
                    targetLidL = targetLidR = 0.95f;
                }
                else if (dt < 3000) { // Open halfway
                    float p = (dt - 1800) / 1200.0f;
                    targetLidL = targetLidR = 0.95f - (easeInOutCubic(p) * 0.5f);
                }
                else if (dt < 4500) { // Fully open with a bounce
                    float p = (dt - 3000) / 1500.0f;
                    targetLidL = targetLidR = 0.45f - (easeOutBounce(p) * 0.45f);
                }
                else setState(E_NEUTRAL);
            }
            break;

        case E_PETTING:
            {
                leftY = rightY = -3;
                widthMod = 4;
                float pant = sin(t / 110.0f);
                targetLidL = targetLidR = 0.2f + (pant * 0.1f);
                if (dt > 4000) setState(E_NEUTRAL);
            }
            break;

        case E_HAPPY:
            {
                float bounce = sin(t / 150.0f);
                if (bounce > 0) {
                    leftY = rightY = (int)(bounce * bounce * -6.0f);
                    targetLidL = targetLidR = 0.35f;
                }
                widthMod = 2;
            }
            break;

        case E_WINK_L: 
            targetLidL = 1.0f; 
            break;
            
        case E_WINK_R: 
            targetLidR = 1.0f; 
            break;

        case E_SQUINT: 
            targetLidL = targetLidR = 0.65f + (sin(t / 400.0f) * 0.05f);
            break;

        case E_SCAN:
            {
                int phase = (dt / 120) % 6;
                int positions[] = {
                    -EYE_SHIFT_RANGE,
                    EYE_SHIFT_RANGE,
                    -2,
                    EYE_SHIFT_RANGE - 1,
                    -EYE_SHIFT_RANGE + 1,
                    0
                };

                leftX = rightX = positions[phase];
                leftY = rightY = ((dt / 180) % 2 == 0) ? -1 : 1;
                targetLidL = targetLidR = 0.08f;
            }
            break;

        case E_OFFENDED:
            leftX = rightX = 8;
            targetLidL = targetLidR = 0.5f;
            break;

        case E_SURPRISED:
            {
                int pop = (dt < 220) ? map(dt, 0, 220, 2, 8) : 8;
                widthMod = pop;
                heightMod = pop;
                leftY = rightY = -3 + (int)(sin(t / 65.0f) * 1.0f);
                targetLidL = targetLidR = -0.2f; // Eyes wider than neutral
            }
            break;
            
        case E_SAD:
            {
                int sigh = (int)(sin(t / 520.0f) * 1.4f);
                targetLidL = targetLidR = 0.48f + (sin(t / 820.0f) * 0.04f);
                leftY = rightY = 5 + sigh;
                leftX = -1;
                rightX = 1;
            }
            break;
            
        case E_ANGRY:
            {
                int shake = (int)(sin(t / 24.0f) * 1.6f);
                targetLidL = targetLidR = 0.62f;
                leftY = rightY = 1;
                leftX = 2 + shake;
                rightX = -2 - shake;
            }
            break;

        case E_CURIOUS:
            leftY = -2; rightY = 1;
            targetLidL = 0.1f; targetLidR = 0.2f;
            break;

        default: break;
    }

    // --- LID SMOOTHING (FLUENT TRANSITIONS) ---
    // This prevents the lids from "snapping" when switching states
    curLidL = smooth(curLidL, targetLidL, 0.25f);
    curLidR = smooth(curLidR, targetLidR, 0.25f);

    // =====================================================
    // RENDER
    // =====================================================
    int finalW = EYE_W + widthMod;
    int finalH = EYE_H + heightMod;
    int adjX = -(widthMod / 2);
    int adjY = -(heightMod / 2);

    int lx = constrain(L_EYE_CX + leftX + adjX, 0, SCREEN_WIDTH - finalW);
    int ly = constrain(EYE_CY + leftY + adjY, 0, SCREEN_HEIGHT - finalH);
    int rx = constrain(R_EYE_CX + rightX + adjX, 0, SCREEN_WIDTH - finalW);
    int ry = constrain(EYE_CY + rightY + adjY, 0, SCREEN_HEIGHT - finalH);

    if (st == E_PETTING) {
        display.fillRoundRect(lx, ly, finalW, finalH, 9, SSD1306_WHITE);
        display.fillRoundRect(rx, ry, finalW, finalH, 9, SSD1306_WHITE);
        display.fillRect(lx, ly, finalW, 9, SSD1306_WHITE);
        display.fillRect(rx, ry, finalW, 9, SSD1306_WHITE);
        display.fillCircle(lx + finalW/2, ly + finalH, 8, SSD1306_BLACK);
        display.fillCircle(rx + finalW/2, ry + finalH, 8, SSD1306_BLACK);
        
        float pant = sin(t / 130.0f); 
        int tLen = 22 + (int)(pant * 8.0f);
        display.fillRoundRect((SCREEN_WIDTH - 22) / 2, EYE_CY + EYE_H + 5, 22, tLen, 11, SSD1306_WHITE);
        display.fillRect((SCREEN_WIDTH - 22) / 2, EYE_CY + EYE_H + 5, 22, 11, SSD1306_WHITE);
    } 
    else {
        drawSquareEye(lx, ly, finalW, finalH, EYE_CORNER_RAD, curLidL);
        drawSquareEye(rx, ry, finalW, finalH, EYE_CORNER_RAD, curLidR);
    }

    switch (st) {
        case E_ANGRY:
            {
                int browY = max(0, ly - 5);
                display.drawLine(lx + 1, browY, lx + finalW - 3, browY + 3, SSD1306_WHITE);
                display.drawLine(rx + finalW - 2, browY, rx + 2, browY + 3, SSD1306_WHITE);
                display.drawPixel(lx - 3, ly + 6, SSD1306_WHITE);
                display.drawPixel(rx + finalW + 2, ry + 6, SSD1306_WHITE);
            }
            break;

        case E_SAD:
            {
                int tearStep = (dt / 160) % 18;
                int leftTearX = lx + finalW - 6;
                int rightTearX = rx + 5;
                int tearY = ly + finalH - 2 + tearStep;

                if (tearY < SCREEN_HEIGHT - 2) {
                    display.drawPixel(leftTearX, tearY, SSD1306_WHITE);
                    if (tearStep > 5) display.drawPixel(leftTearX, tearY + 1, SSD1306_WHITE);
                }

                if (tearStep > 7) {
                    int rightTearY = ry + finalH + tearStep - 8;
                    if (rightTearY < SCREEN_HEIGHT - 2) {
                        display.drawPixel(rightTearX, rightTearY, SSD1306_WHITE);
                    }
                }
            }
            break;

        case E_SCAN:
            break;

        case E_SURPRISED:
            {
                bool flash = (dt / 180) % 2 == 0;
                if (flash) {
                    display.drawFastVLine(max(0, lx - 5), ly + 5, 7, SSD1306_WHITE);
                    display.drawFastVLine(min(SCREEN_WIDTH - 1, rx + finalW + 4), ry + 5, 7, SSD1306_WHITE);
                    display.drawFastHLine(lx + finalW / 2 - 3, max(0, ly - 5), 7, SSD1306_WHITE);
                    display.drawFastHLine(rx + finalW / 2 - 3, max(0, ry - 5), 7, SSD1306_WHITE);
                }
            }
            break;

        default:
            break;
    }
}

void Eyes::update() {
    clearDisplayFast();
    doState(state);
    display.display();
}
