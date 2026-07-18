# Autonomous IoT Laundry Auto-Ordering System

Battery-optimized ESP32-based smart laundry system that tracks detergent/consumable weight via a load cell, displays status on an OLED screen, and sends automated reorder alerts over Telegram when supplies run low.

## Overview

This project automates a simple but common household problem: knowing when your detergent (or similar laundry consumable) is running low, and getting notified before it runs out — without manual tracking. An ESP32 continuously monitors weight via a load cell, shows live status on an OLED display, and pushes a Telegram alert when the weight crosses a configured threshold.

## Features

- **Weight sensing** — HX711 load cell amplifier for precise consumable weight tracking
- **On-device UI** — SSD1306 OLED display with a menu system navigated via tactile buttons
- **Automated alerts** — Wi-Fi + NTP time sync and Telegram Bot API integration for real-time low-stock notifications
- **Power-aware design** — brownout protection, safe boot-time pin states, and staged peripheral initialization for stable operation on the ESP32's 3.3V rail

## Hardware Used

| Component | Purpose |
|---|---|
| ESP32 | Main microcontroller |
| HX711 + Load Cell | Weight sensing |
| SSD1306 OLED (I2C) | Status display and menu UI |
| Tactile Buttons | Menu navigation / input |
| Wi-Fi | NTP time sync + Telegram HTTP notifications |

## Project Structure

The system is built in three phases:

1. **Phase 1 — UI / Menu**: OLED display setup, menu navigation via tactile buttons
2. **Phase 2 — Load Cell & Alerts**: HX711 integration, weight thresholds, local alert logic
3. **Phase 3 — Connectivity**: Wi-Fi, NTP time sync, and Telegram Bot notifications

## How It Works

1. On boot, the ESP32 initializes peripherals in a safe, staged order (brownout detector disabled, Wi-Fi held off during I2C/load-cell init to avoid rail sag).
2. The load cell continuously measures the weight of the tracked consumable.
3. The OLED displays current weight, status, and menu options via button input.
4. When weight drops below a set threshold, the ESP32 connects to Wi-Fi, syncs time via NTP, and sends a Telegram message alerting that a reorder is needed.

## Getting Started

1. Flash the `.ino` sketch to an ESP32 using the Arduino IDE.
2. Wire the HX711, load cell, SSD1306 OLED (I2C), and tactile buttons as per the pin definitions in the sketch.
3. Update Wi-Fi credentials and Telegram Bot token/chat ID in the config section.
4. Power on — the system will calibrate the load cell and boot into the main menu.

## Future Improvements

- Deep-sleep power optimization for longer battery life
- Multi-item tracking (detergent, fabric softener, etc.)
- Historical usage graphs via a companion app or dashboard

## Author

**Arpan Ailawadi**
B.Tech, Electronics and Communication Engineering, Delhi Technological University
[LinkedIn](https://www.linkedin.com/in/arpan-ailawadi/)
