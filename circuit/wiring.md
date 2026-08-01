# Wiring / Pin Connections

Complete wiring schematic and component list for the Smart Laundry Auto-Ordering System.

## Components

| Component | Spec |
|---|---|
| Microcontroller | ESP32 Development Board |
| Display | 0.96" SSD1306 OLED (I2C) |
| Weight Sensor | 5kg/10kg Load Cell + HX711 Amplifier |
| Input | 3x Tactile Push Buttons (UP, OK, DOWN) |
| Power Source | 3.7V Lithium Battery via TP4056 Charger (or USB Power Bank) |
| Optional Alerts | 5V Active Buzzer, Red LED |

## Master Pinout Table

| ESP32 Pin | Component | Description / Function |
|---|---|---|
| VIN (or 5V) | Power Source | Connects to Battery OUT+ (via TP4056) or USB power |
| 3V3 | OLED & HX711 | Powers OLED VCC and HX711 VCC (3.3V out) |
| GND | All Components | Common ground for the entire circuit |
| D21 | OLED (SDA) | I2C serial data for the display |
| D22 | OLED (SCL) | I2C serial clock for the display |
| D32 | HX711 (DOUT) | Serial data from the load cell amplifier |
| D33 | HX711 (SCK) | Serial clock to pulse the load cell amplifier |
| D25 | Button (UP) | Tactile button, wired to GND, internal pull-up |
| D26 | Button (OK) | Tactile button, wired to GND, internal pull-up |
| D27 | Button (DOWN) | Tactile button, wired to GND, internal pull-up |
| D34 | Battery Sensor | Reads battery voltage (requires 10k/10k voltage divider) |
| D18 | Buzzer (Optional) | Triggers active buzzer |
| D23 | LED (Optional) | Triggers red threshold warning LED |

## Wiring Instructions

### 1. Power Distribution
- TP4056 `OUT+` → ESP32 `VIN` (or `5V`)
- TP4056 `OUT-` → ESP32 `GND`
- ESP32 `3V3` → OLED `VCC` and HX711 `VCC`/`VDD`

> **Note:** If using the "direct bypass" approach due to a weak battery, the OLED VCC, HX711 VCC, and ESP32 3V3 are all wired directly to TP4056 `OUT+`.

### 2. HX711 Load Cell Amplifier
- `VCC`/`VDD` → ESP32 `3V3`
- `GND` → ESP32 `GND`
- `DOUT` → ESP32 `D32`
- `SCK` → ESP32 `D33`
- Load cell 4-wire (Red, Black, White, Green) → HX711 `E+`, `E-`, `A-`, `A+` respectively

### 3. SSD1306 OLED Display
- `VCC` → ESP32 `3V3`
- `GND` → ESP32 `GND`
- `SDA` → ESP32 `D21`
- `SCL` → ESP32 `D22`

### 4. Push Buttons (Menu Controls)
- **UP:** one leg → `D25`, other leg → `GND`
- **OK:** one leg → `D26`, other leg → `GND`
- **DOWN:** one leg → `D27`, other leg → `GND`

No external resistors needed — firmware enables the ESP32's internal pull-up resistors.
