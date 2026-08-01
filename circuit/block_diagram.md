# System Block Diagram

```mermaid
flowchart TD
    subgraph Power["Power Supply"]
        BAT["3.7V Li-ion Battery"] --> TP["TP4056 Charger"]
        TP --> ESP32
        USB["USB Power Bank"] -.-> ESP32
    end

    subgraph Sensing["Sensing"]
        LC["Load Cell (5kg/10kg)"] --> HX["HX711 Amplifier"]
        HX -->|DOUT / SCK| ESP32
    end

    subgraph Input["User Input"]
        UP["UP Button"] --> ESP32
        OK["OK Button"] --> ESP32
        DOWN["DOWN Button"] --> ESP32
    end

    ESP32["ESP32 Dev Board"]

    subgraph Output["Display & Alerts"]
        ESP32 -->|I2C: SDA/SCL| OLED["SSD1306 OLED Display"]
        ESP32 -->|Optional| LED["Red LED"]
    end

    subgraph Connectivity["Connectivity"]
        ESP32 -->|Wi-Fi| WIFI["Wi-Fi Network"]
        WIFI --> NTP["NTP Time Sync"]
        WIFI --> TG["Telegram Bot API"]
        TG --> USER["User's Phone (Alert)"]
    end
```

## How it works

1. **Power** — the battery feeds the ESP32 through the TP4056 charger (or USB power directly).
2. **Sensing** — the load cell measures weight, and the HX711 amplifies and digitizes that reading for the ESP32.
3. **Input** — three tactile buttons let the user navigate the on-device menu.
4. **Output** — the ESP32 drives the OLED display for status/menu, and optionally triggers a red LED for local alerts.
5. **Connectivity** — when the weight crosses a threshold, the ESP32 connects to Wi-Fi, syncs time via NTP, and sends a Telegram alert to the user's phone.
