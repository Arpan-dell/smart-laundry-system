# Firmware

ESP32 firmware for the Smart Laundry Auto-Ordering System, developed in the Arduino IDE.

## Files

| File | Description |
|---|---|
| `laundry_system.ino` | Main firmware — handles OLED menu UI, HX711 load cell reading, threshold alerts, and Wi-Fi/NTP/Telegram notifications |

## Setup

1. Open `laundry_system.ino` in the Arduino IDE.
2. Install required libraries (HX711, Adafruit SSD1306, Adafruit GFX, WiFi/HTTPClient).
3. Update Wi-Fi credentials and Telegram Bot token/chat ID in the config section at the top of the file.
4. Select the correct ESP32 board and port, then upload.

See [`../circuit/wiring.md`](../circuit/wiring.md) for pin connections.
