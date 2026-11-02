# Simba ESP32 / ESP32-C3 AI Desktop Companion & Native Flutter App

This repository contains the complete firmware for the **Simba ESP32 / ESP32-C3 AI Desktop Companion** and the companion **Native Flutter Android Application**.

---

## 📂 Project Layout

```
botnew/
├── botnew.ino               # Main Arduino C++ firmware sketch file
├── src/                     # Firmware source code (Config, WebControl, Display, Eyes, etc.)
│   ├── Config.h             # Hardware configuration & ESP32-C3 pin mapping
│   ├── Display.h / .cpp     # SSD1306 OLED driver
│   ├── Eyes.h / .cpp        # Simba 14 eye animation expressions
│   ├── ClockApp.h / .cpp    # Timepiece app
│   ├── Pomodoro.h / .cpp    # Focus timer app
│   ├── Weather.h / .cpp     # Weather station app
│   ├── Reminders.h / .cpp   # Task reminder list app
│   ├── DesktopApp.h / .cpp  # OLED screensaver app
│   └── WebControl.h / .cpp  # REST HTTP web server & NVS configuration
│
└── simba_companion_app/     # SEPARATE FOLDER for Native Flutter Android Application
    ├── lib/                 # Flutter Dart code (Clean Architecture, Provider)
    ├── assets/              # App images and icons
    ├── android/             # Android project manifest & gradle files
    ├── pubspec.yaml         # Dependencies manifest
    └── README.md            # Mobile App build and usage guide
```

---

## ⚡ ESP32 & ESP32-C3 Firmware Setup

### Supported Microcontrollers
- **ESP32-C3** (ESP32-C3 SuperMini, DevKitM-1, NodeMCU-ESP32-C3)
- **Standard ESP32** (DevKit v1, WROOM-32, WROVER)

### Hardware Connections

| Component | Standard ESP32 Pin | ESP32-C3 Pin |
| :--- | :--- | :--- |
| **OLED SDA** | GPIO 6 (or GPIO 21) | **GPIO 8** (or GPIO 4) |
| **OLED SCL** | GPIO 4 (or GPIO 22) | **GPIO 9** (or GPIO 6) |
| **Touch Sensor (TTP223)** | GPIO 5 | **GPIO 5** |
| **Power (VCC)** | 3.3V / 5V | 3.3V / 5V |
| **GND** | GND | GND |

### How to Upload to ESP32 / ESP32-C3 (Arduino IDE)

1. **Open Arduino IDE**:
   - Install **ESP32 Board Support**: Go to `File -> Preferences` and add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` under *Additional Boards Manager URLs*.
   - Open `Tools -> Board Manager`, search for `esp32` by Espressif Systems and click **Install**.

2. **Required Libraries**:
   Go to `Sketch -> Include Library -> Manage Libraries...` and install:
   - `Adafruit GFX Library`
   - `Adafruit SSD1306`
   - `ArduinoJson` (v6 or v7)

3. **Select Board**:
   - For ESP32-C3: Select **`ESP32C3 Dev Module`** (or `XIAO_ESP32C3` / `ESP32-C3 SuperMini`).
   - For Classic ESP32: Select **`ESP32 Dev Module`**.

4. **Upload**:
   - Open `botnew.ino`.
   - Connect board via USB-C, select Port, and click **Upload**.

---

## 📱 Mobile App Setup (Flutter Android)

The mobile app is located in the `simba_companion_app/` folder.

```bash
cd simba_companion_app
flutter pub get
flutter run
```

For full details on building the release APK and configuring connection parameters, check `simba_companion_app/README.md`.
