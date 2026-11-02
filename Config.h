#pragma once

// ===================== DISPLAY SETTINGS ======================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define EYE_W 24
#define EYE_H 24
#define EYE_CORNER_RAD 4
#define EYE_GAP 15   

// ===================== PIN DEFINITIONS =========================
// OLED Display (4-pin I2C Mode: GND, VCC, SCL, SDA)
// Set these GPIOs to match your board wiring.

// ===================== PIN DEFINITIONS =========================
// Force ESP32-C3 Pins
#define PIN_OLED_SCL         9        // Clock (OLED SCL) - GPIO 9 on ESP32-C3
#define PIN_OLED_SDA         8        // Data  (OLED SDA) - GPIO 8 on ESP32-C3

#define PIN_TOUCH            5        // Touch Sensor (TTP223 / Digital Touch) - GPIO 5
#define PIN_BTN_MODE         3        // Physical button (if used)
#define PIN_BTN_POMO         2        // Physical button (if used)

#define OLED_I2C_ADDRESS     0x3C     // Most 128x64 SSD1306 I2C modules
#define OLED_I2C_CLOCK_HZ    400000UL // Fast mode keeps animations responsive

#if PIN_OLED_SDA == PIN_OLED_SCL
#error "OLED SDA and SCL must use different GPIO pins."
#endif

#if PIN_OLED_SDA == PIN_TOUCH || PIN_OLED_SCL == PIN_TOUCH
#error "OLED I2C pins must not overlap the touch sensor pin."
#endif

// ===================== TOUCH CALIBRATION ====================
// TTP223 pulses can be very short - 30 ms captures light taps reliably.
#define TAP_MIN 50         // Minimum duration for a valid tap (ms)
#define TAP_MAX 400        // Maximum duration for a single tap (ms)

// 500 ms gives comfortable room for human double-taps and avoids
// single-tap misdetection even when webControl.update() is slow.
#define DOUBLE_TAP_GAP 450 // Max time window between two tap releases (ms)

#define LONG_PRESS 1200     // Duration to trigger "Petting" hold (ms)

// ===================== WIFI & WEATHER API ===================
#define WIFI_SSID_DEFAULT      "CARDILAC"
#define WIFI_PASS_DEFAULT      "Viraj@141205"

#define WEATHER_API_KEY "586ad27aba041f1223cd8701e1140bf7"
#define WEATHER_CITY    "Kolhapur"
#define WEATHER_COUNTRY "IN"

// Timezone (Indian Standard Time: GMT + 5:30)
#define GMT_OFFSET_SEC 19800 
#define DST_OFFSET_SEC 0

// ===================== SYSTEM CONSTANTS =======================
#define BLINK_HEIGHT 4
#define EYE_SHIFT_RANGE 4 

// Application Modes
enum RobotMode {
    MODE_EYES = 0,
    MODE_CLOCK,
    MODE_POMODORO,
    MODE_WEATHER,
    MODE_DESKTOP,
    MODE_REMINDERS,
    MODE_COUNT
};

// Simba's Expressions
enum EyeState { 
    E_NEUTRAL,      // Default resting state
    E_BLINK,        // Normal blink
    E_WINK_L,       // Left eye wink
    E_WINK_R,       // Right eye wink
    E_HAPPY,        // Bouncy happy expression
    E_SAD,          // Droopy sad look
    E_ANGRY,        // Intense narrow stare
    E_SURPRISED,    // Wide-eyed surprise
    E_SQUINT,       // Relaxed squint
    E_SCAN,         // Looking around horizontally
    E_SLEEP,        // Sleeping with breathing
    E_WAKEUP,       // Natural wake-up sequence
    E_PETTING,      // Happy dog-style eyes (triggered by long hold)
    E_OFFENDED,     // Side-eye look (idle mood)
    E_CURIOUS       // Inquisitive tilted look
};
