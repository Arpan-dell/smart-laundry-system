# Smart Laundry Auto-Ordering System

An ESP32-based smart laundry basket that weighs your laundry and automatically sends a pickup order over Telegram once the basket is full.

> **Status: Phase 1 (working prototype).** The core loop works end to end: weigh, display, auto-order and remind. Planned Phase 2 improvements are listed under [Roadmap](#roadmap-phase-2).

## How It Works

1. **Weighing:** a load cell under the basket, read through an HX711 amplifier, measures the weight of the laundry continuously.
2. **Display:** a 0.96" SSD1306 OLED shows the live weight, a progress bar towards the target weight, the Wi-Fi signal and the battery level.
3. **Auto-order:** when the weight stays at or above the target (default **5 kg**) for **3 seconds**, the ESP32 sends a pickup request to Telegram:
   ```
   Smart Laundry Alert
   Order ID: #ORD-12345
   Total Weight: 5.12 kg
   Request Time: 04-Oct-2026 06:30 PM
   Pickup Address: <your address>
   ```
   The 3-second hold stops a quick bump or press on the lid from triggering an order.
4. **One order per load:** after an order is sent, the system latches so it doesn't send again. If the basket is still full **24 hours** later, it sends a single `[REMINDER]` message.
5. **Reset:** when the basket is emptied (under 1 kg for 5 seconds), the latch resets, ready for the next load.
6. **Local alert:** a red LED blinks whenever the weight is over the target.

### Boot sequence

On power-up the device shows a splash screen, connects to Wi-Fi (12-second timeout), syncs the clock over NTP (IST) so orders carry a real timestamp, then wakes and zeroes the load cell. If Wi-Fi fails, the scale and menu still work offline.

### On-device menu

Press **OK** on the main screen to open the menu. Use **UP/DOWN** to move and **OK** to select.

| Menu item | What it does |
|---|---|
| Tare Scale | Zeroes the scale (empty basket) |
| Set Target | Changes the order threshold in 0.5 kg steps |
| Customer Order | Sends a pickup order manually, with confirmation |
| Battery Info | Shows the battery percentage |
| Exit Menu | Returns to the main screen |

## Engineering Notes

Problems solved while building Phase 1:

- **Wi-Fi start-up resets:** Wi-Fi start-up draws a burst of current that sagged the battery rail and tripped the ESP32's brownout reset. The firmware disables the brownout detector and powers up peripherals in stages.
- **Late HX711 wake-up:** at low supply voltage the HX711 sometimes isn't ready at boot, so it can't be zeroed. The firmware remembers that it still needs zeroing and does it the moment the sensor responds, so a large fake weight never appears.
- **Button-press spikes:** pressing a button on the enclosure physically loads the scale. Weight updates pause for 1.5 seconds after menu actions, so a press can't cause a false reading or an accidental order.
- **Button debouncing:** edge detection plus a 250 ms lockout on each button stops a single press registering twice.

## Hardware

| Component | Purpose |
|---|---|
| ESP32 Dev Board | Main controller, Wi-Fi |
| Load Cell (5/10 kg) + HX711 | Weight sensing |
| 0.96" SSD1306 OLED (I2C) | Live weight and menu UI |
| 3 × Tactile Buttons | UP / OK / DOWN navigation |
| Red LED | Over-target indicator |
| 3.7 V Li-ion + TP4056 (or USB power) | Power |
| 3D-printed enclosure | Designed in Tinkercad (see [`cad/`](cad/)) |

Full pinout and wiring: [`circuit/wiring.md`](circuit/wiring.md). Block diagram: [`circuit/block_diagram.md`](circuit/block_diagram.md).

## Repository Structure

| Folder | Contents |
|---|---|
| [`code/`](code/) | ESP32 firmware (`laundry_system.ino`) |
| [`circuit/`](circuit/) | Circuit diagram, wiring table, block diagram |
| [`cad/`](cad/) | Enclosure STL files and renders |
| [`photos/`](photos/) | Photos of the build |
| [`videos/`](videos/) | Demo videos |

## Getting Started

1. Wire the components as described in [`circuit/wiring.md`](circuit/wiring.md).
2. In the Arduino IDE, install the libraries **HX711**, **Adafruit SSD1306** and **Adafruit GFX**. (WiFi and HTTPClient come with the ESP32 board package.)
3. Open [`code/laundry_system.ino`](code/laundry_system.ino) and **fill in the config section at the top with your own details**:
   ```cpp
   #define WIFI_SSID          "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD      "YOUR_WIFI_PASSWORD"
   #define TELEGRAM_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"   // from @BotFather
   #define TELEGRAM_CHAT_ID   "YOUR_TELEGRAM_CHAT_ID"
   #define PICKUP_ADDRESS     "YOUR_PICKUP_ADDRESS"
   ```
   > Never commit your real credentials to a public repository.
4. If needed, calibrate the scale by adjusting `LOADCELL_CALIBRATION_FACTOR` with a known weight.
5. Select your ESP32 board and port, then upload. The device boots, connects, zeroes the scale and opens the main screen.

## Gallery

### Photos

| | |
|---|---|
| <img src="photos/calibration%20phase.jpg" width="400"><br>**calibration phase** | <img src="photos/device.jpg" width="400"><br>**device** |
| <img src="photos/front%20face.jpg" width="400"><br>**front face** | <img src="photos/side%20face.jpg" width="400"><br>**side face** |

### Videos

- [connecting to wifi](videos/connecting%20to%20wifi.mp4)
- [custom order](videos/custom%20order.mp4)
- [menu](videos/menu.mp4)
- [weight verification](videos/weight%20verification.mp4)

## Roadmap (Phase 2)

- [ ] Set up Wi-Fi from a phone (captive portal) instead of hard-coded credentials
- [ ] Save the target weight and calibration to flash so they survive a reboot
- [ ] Filter weight readings (moving average or median) and calibrate from the menu
- [ ] Deep sleep with button/timer wake-up for longer battery life
- [ ] Accurate battery measurement through a calibrated voltage divider
- [ ] Two-way Telegram: confirm or cancel an order from the phone
- [ ] Verified TLS for API calls
- [ ] Custom PCB to replace the perfboard

## Author

**Arpan Ailawadi**
B.Tech, Electronics and Communication Engineering, Delhi Technological University
[LinkedIn](https://www.linkedin.com/in/arpan-ailawadi/)
