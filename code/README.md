# Firmware

ESP32 firmware for the Smart Laundry Auto-Ordering System, developed in the Arduino IDE.

## Files

| File | Description |
|---|---|
| `laundry_system/laundry_system.ino` | Main firmware: OLED menu UI, HX711 weighing and calibration, Wi-Fi scanning, threshold auto-orders and Telegram/NTP |
| `laundry_system/secrets.example.h` | Template for the Telegram token, chat ID and pickup address |

## Setup

1. Copy `laundry_system/secrets.example.h` to `laundry_system/secrets.h` and fill in your values (`secrets.h` is git-ignored).
2. Add your Wi-Fi networks to `KNOWN_NETWORKS` at the top of `laundry_system.ino`.
3. Install the required libraries: HX711, Adafruit SSD1306 and Adafruit GFX. WiFi, HTTPClient and Preferences come with the ESP32 board package.
4. Open `laundry_system/laundry_system.ino` in the Arduino IDE, select your ESP32 board and port, then upload.

See [`../circuit/wiring.md`](../circuit/wiring.md) for pin connections.
