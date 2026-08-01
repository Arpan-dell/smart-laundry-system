// ============================================================================
// SMART LAUNDRY AUTO-ORDERING SYSTEM - STEP 1 (Fixed Baseline)
// - Auto-Sleep is completely removed (Screen will never turn off)
// - Proper edge detection added (Buttons will never double-click)
// - NO WI-FI OR TELEGRAM YET.
// ============================================================================

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ---------------- Config ----------------
#define WIFI_SSID          "Airtel_1375"
#define WIFI_PASSWORD      "Air@7292"
#define TELEGRAM_BOT_TOKEN "8480704456:AAFaOD6nhyzNNRTadmcJiYITHlKEpg_pXXI"
#define TELEGRAM_CHAT_ID   "1734936965"
#define PICKUP_ADDRESS     "Apt 4B, Smart Laundry HQ, Delhi, India"
#define REMINDER_DELAY_MS  86400000UL // 24 hours before reminder is sent

#define NTP_SERVER          "pool.ntp.org"
#define GMT_OFFSET_SEC       (5 * 3600 + 1800)  
#define DAYLIGHT_OFFSET_SEC  0
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define I2C_SDA       21
#define I2C_SCL       22
#define OLED_I2C_ADDR 0x3C

#define LOADCELL_DOUT_PIN 32
#define LOADCELL_SCK_PIN  33
#define LOADCELL_CALIBRATION_FACTOR -17500.0f

#define BTN_UP_PIN   25
#define BTN_OK_PIN   26
#define BTN_DOWN_PIN 27

#define BATT_PIN     34
#define BATT_RAW_MIN 1980
#define BATT_RAW_MAX 2600

#define BUZZER_PIN  18   
#define LED_RED_PIN 23   

#define DEFAULT_TARGET_KG 5.0f

class ButtonManager {
public:
  struct Presses {
    bool up, ok, down;
    bool any() const { return up || ok || down; }
  };

  void begin(uint8_t upPin, uint8_t okPin, uint8_t downPin) {
    _upPin = upPin;
    _okPin = okPin;
    _downPin = downPin;
    pinMode(_upPin, INPUT_PULLUP);
    pinMode(_okPin, INPUT_PULLUP);
    pinMode(_downPin, INPUT_PULLUP);
  }

  Presses poll() {
    Presses p = {false, false, false};
    unsigned long now = millis();

    // Check UP (250ms debounce to prevent double-clicks from cheap springs)
    bool upState = digitalRead(_upPin) == LOW;
    if (upState && !_lastUpState && now - _lastUpTime > 250) {
      p.up = true; _lastUpTime = now;
    }
    _lastUpState = upState;

    // Check OK
    bool okState = digitalRead(_okPin) == LOW;
    if (okState && !_lastOkState && now - _lastOkTime > 250) {
      p.ok = true; _lastOkTime = now;
    }
    _lastOkState = okState;

    // Check DOWN
    bool downState = digitalRead(_downPin) == LOW;
    if (downState && !_lastDownState && now - _lastDownTime > 250) {
      p.down = true; _lastDownTime = now;
    }
    _lastDownState = downState;

    return p;
  }

private:
  uint8_t _upPin = 0, _okPin = 0, _downPin = 0;
  bool _lastUpState = false, _lastOkState = false, _lastDownState = false;
  unsigned long _lastUpTime = 0, _lastOkTime = 0, _lastDownTime = 0;
};


// ---------------- BatteryMonitor ----------------
class BatteryMonitor {
public:
  void begin(uint8_t pin) { _pin = pin; }
  int readPercentage() {
    int raw = analogRead(_pin);
    int pct = map(raw, BATT_RAW_MIN, BATT_RAW_MAX, 0, 100);
    return constrain(pct, 0, 100);
  }
private:
  uint8_t _pin = 0;
};


// ---------------- DisplayManager ----------------
class DisplayManager {
public:
  bool begin() {
    Wire.begin(I2C_SDA, I2C_SCL);
    bool ok = _display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR);
    if (!ok) {
      delay(500);
      ok = _display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR);
    }
    Wire.setClock(100000);
    _display.setTextColor(SSD1306_WHITE);
    _display.cp437(true);
    return ok;
  }

  void showBootScreen(const char* title, const char* subtitle) {
    _display.clearDisplay();
    _display.drawRoundRect(4, 4, SCREEN_WIDTH - 8, SCREEN_HEIGHT - 8, 8, SSD1306_WHITE);
    printCentered(title, 18, 2);
    printCentered(subtitle, 36, 1);
    _display.display();
  }

  void showStatusLine(const char* line1, const char* line2, int dots) {
    _display.clearDisplay();
    printCentered(line1, 10, 1);
    printCentered(line2, 22, 1);
    for (int i = 0; i < dots && i < 6; i++) {
      _display.setCursor(10 + i * 8, 40);
      _display.print(".");
    }
    _display.display();
  }

  void showToast(const char* msg, uint16_t holdMs = 800) {
    _display.clearDisplay();
    printCentered(msg, 24, 2);
    _display.display();
    delay(holdMs);
  }

  void drawMainScreen(float weightKg, float targetKg, int batteryPct, int wifiBars) {
    _display.clearDisplay();
    drawHeader(batteryPct, wifiBars);
    char buf[16];
    dtostrf(weightKg, 4, 2, buf);
    String weightStr = String(buf) + " kg";
    printCentered(weightStr.c_str(), 20, 2);
    drawTargetBar(weightKg, targetKg, 42, 10);
    _display.display();
  }

  void drawMenuList(const char* const* items, int count, int selectedIndex, int scrollOffset) {
    _display.clearDisplay();
    printCentered("MENU", 0, 1);
    _display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

    const int visibleRows = 3;
    for (int row = 0; row < visibleRows; row++) {
      int itemIdx = scrollOffset + row;
      if (itemIdx >= count) break;
      int y = 14 + row * 16;
      bool selected = (itemIdx == selectedIndex);

      if (selected) {
        _display.fillRect(0, y - 1, SCREEN_WIDTH, 14, SSD1306_WHITE);
        _display.setTextColor(SSD1306_BLACK);
      } else {
        _display.setTextColor(SSD1306_WHITE);
      }
      _display.setTextSize(1);
      _display.setCursor(6, y + 3);
      _display.print(items[itemIdx]);
    }
    _display.setTextColor(SSD1306_WHITE);
    _display.display();
  }

  void drawEditThreshold(float targetKg) {
    _display.clearDisplay();
    printCentered("SET TARGET", 2, 1);
    char buf[16];
    dtostrf(targetKg, 4, 2, buf);
    String s = String(buf) + " kg";
    printCentered(s.c_str(), 20, 2);
    printCentered("UP/DOWN +-0.5", 44, 1);
    printCentered("OK = Save", 54, 1);   
    _display.display();
  }

  void drawBatteryInfo(int pct) {
    _display.clearDisplay();
    printCentered("BATTERY", 2, 1);
    drawBattery(56, 18, pct, SSD1306_WHITE);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", pct);
    printCentered(buf, 32, 2);
    printCentered("OK = Back", 54, 1);   
    _display.display();
  }

  void drawConfirmOrder() {
    _display.clearDisplay();
    printCentered("ORDER PICKUP?", 6, 1);
    printCentered("OK = Confirm", 28, 1);
    printCentered("UP/DOWN = Cancel", 42, 1);
    _display.display();
  }

private:
  Adafruit_SSD1306 _display{SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET};

  void printCentered(const char* text, int y, int size) {
    _display.setTextSize(size);
    int16_t x1, y1;
    uint16_t w, h;
    _display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
    int x = (SCREEN_WIDTH - (int)w) / 2;
    if (x < 0) x = 0;
    _display.setCursor(x, y);
    _display.print(text);
  }

  void drawHeader(int batteryPct, int wifiBars) {
    _display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
    _display.setTextColor(SSD1306_BLACK);
    _display.setTextSize(1);
    _display.setCursor(4, 2);
    _display.print("LAUNDRY");
    drawWifiBars(SCREEN_WIDTH - 46, 2, wifiBars, SSD1306_BLACK);
    drawBattery(SCREEN_WIDTH - 20, 2, batteryPct, SSD1306_BLACK);
    _display.setTextColor(SSD1306_WHITE);
  }

  void drawBattery(int x, int y, int pct, uint16_t color) {
    _display.drawRect(x, y, 16, 8, color);
    _display.fillRect(x + 16, y + 2, 2, 4, color);
    int fillWidth = map(constrain(pct, 0, 100), 0, 100, 0, 14);
    if (fillWidth > 0) {
      _display.fillRect(x + 1, y + 1, fillWidth, 6, color);
    }
  }

  void drawWifiBars(int x, int y, int wifiBars, uint16_t color) {
    if (wifiBars < 0) {
      _display.drawLine(x, y, x + 8, y + 8, color);
      _display.drawLine(x, y + 8, x + 8, y, color);
      return;
    }
    for (int i = 0; i < 4; i++) {
      int barHeight = 2 + i * 2;
      int barX = x + i * 3;
      int barY = y + 8 - barHeight;
      if (i < wifiBars) {
        _display.fillRect(barX, barY, 2, barHeight, color);
      } else {
        _display.drawRect(barX, barY, 2, barHeight, color);
      }
    }
  }

  void drawTargetBar(float weightKg, float targetKg, int y, int height) {
    int barX = 8;
    int barW = SCREEN_WIDTH - 16;
    _display.drawRect(barX, y, barW, height, SSD1306_WHITE);
    if (targetKg > 0.0f) {
      float ratio = weightKg / targetKg;
      ratio = constrain(ratio, 0.0f, 1.0f);
      int fillW = (int)(ratio * (barW - 2));
      if (fillW > 0) {
        _display.fillRect(barX + 1, y + 1, fillW, height - 2, SSD1306_WHITE);
      }
    }
    char buf[24];
    snprintf(buf, sizeof(buf), "Target: %.2f kg", targetKg);
    int labelY = min(y + height + 2, SCREEN_HEIGHT - 8);
    printCentered(buf, labelY, 1);
  }
};


// ---------------- LoadCellManager ----------------
class LoadCellManager {
public:
  bool begin(unsigned int timeoutMs = 5000) { // Increased to 5 seconds to guarantee boot tare
    // At low voltages (2.6V), the HX711 struggles to wake up from deep sleep.
    // Instead of forcing it to sleep, we just let it stabilize naturally during boot.
    digitalWrite(LOADCELL_SCK_PIN, LOW);
    delay(500);

    _scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    _scale.set_scale(LOADCELL_CALIBRATION_FACTOR);

    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
      if (_scale.is_ready()) {
        _scale.tare();
        _ready = true;
        _needsTare = false;
        return true;
      }
      delay(10);
    }
    _ready = false;
    _needsTare = true; // Mark that we STILL need to tare when it finally wakes up!
    return false;
  }

  bool tare(unsigned int timeoutMs = 2000) {
    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
      if (_scale.is_ready()) {
        _scale.tare();
        _lastWeight = 0.0f;
        _needsTare = false;
        return true;
      }
      delay(10);
    }
    return false;
  }
  bool isReady() { return _scale.is_ready(); }
  void ignoreSpikesFor(unsigned int ms) {
    _ignoreSpikesUntil = millis() + ms;
  }

  float readKg() {
    if (_scale.is_ready()) {
      if (_needsTare) {
        // It finally woke up late! Tare it instantly before reading to prevent a massive fake weight spike.
        _scale.tare();
        _needsTare = false;
        _lastWeight = 0.0f;
        return 0.0f;
      }

      float raw = _scale.get_units(1);
      if (raw < 0.0f) raw = 0.0f;
      
      if (millis() > _ignoreSpikesUntil) {
        _lastWeight = raw;
      }
    }
    return _lastWeight;
  }

private:
  HX711 _scale;
  bool _ready = false;
  bool _needsTare = true;
  float _lastWeight = 0.0f;
  unsigned long _ignoreSpikesUntil = 0;
};


// ---------------- AlertManager ----------------
class AlertManager {
public:
  void begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_RED_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_RED_PIN, LOW);
  }
  void beep(unsigned int durationMs) { delay(durationMs); }
  void updateThresholdLed(bool overThreshold) {
    digitalWrite(LED_RED_PIN, overThreshold ? ((millis() / 500) % 2) : LOW);
  }
};


// ---------------- LaundryNetwork (Phase 3) ----------------
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

class LaundryNetwork {
public:
  bool isConnected() { return WiFi.status() == WL_CONNECTED; }

  int wifiBars() {
    if (WiFi.status() != WL_CONNECTED) return -1;
    long rssi = WiFi.RSSI();
    if (rssi >= -55) return 4;
    if (rssi >= -65) return 3;
    if (rssi >= -75) return 2;
    if (rssi >= -85) return 1;
    return 0;
  }

  int sendPickupAlert(float weightKg, bool isReminder = false) {
    if (!isConnected()) return -1;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    struct tm timeinfo;
    String timeString = "Time unavailable";
    if (getLocalTime(&timeinfo, 1000)) {
      char buf[32];
      strftime(buf, sizeof(buf), "%d-%b-%Y %I:%M %p", &timeinfo);
      timeString = String(buf);
    }

    uint32_t orderNum = esp_random() % 90000 + 10000;
    String message = isReminder ? "⚠️ [REMINDER] Smart Laundry Alert\n" : "Smart Laundry Alert\n";
    message += "Order ID: #ORD-" + String(orderNum) + "\n";
    message += "Total Weight: " + String(weightKg, 2) + " kg\n";
    message += "Request Time: " + timeString + "\n";
    message += "Pickup Address: " + String(PICKUP_ADDRESS);

    String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) +
                 "/sendMessage?chat_id=" + String(TELEGRAM_CHAT_ID) +
                 "&text=" + urlEncode(message);

    http.begin(client, url);
    int code = http.GET();
    http.end();
    return code;
  }

private:
  String urlEncode(const String& s) {
    String out;
    out.reserve(s.length());
    for (size_t i = 0; i < s.length(); i++) {
      char c = s[i];
      if (c == ' ') out += "%20";
      else if (c == '\n') out += "%0A";
      else out += c;
    }
    return out;
  }
};


// ---------------- AppController ----------------
// ---------------- AppController ----------------
enum class UIState {
  MAIN_SCREEN,
  MENU_LIST,
  EDIT_THRESHOLD,
  BATTERY_INFO,
  CONFIRM_ORDER
};

class AppController {
public:
  using TareFn  = void  (*)();
  using OrderFn = void  (*)(bool);
  using FloatFn = float (*)();
  using IntFn   = int   (*)();
  using IgnoreSpikesFn = void (*)(unsigned int);

  void begin(DisplayManager* display, ButtonManager* buttons,
             TareFn tareFn, OrderFn orderFn,
             FloatFn getWeightFn, IntFn getBatteryFn, IntFn getWifiBarsFn,
             IgnoreSpikesFn ignoreSpikesFn,
             float initialTargetKg) {
    _display = display;
    _buttons = buttons;
    _tareFn = tareFn;
    _orderFn = orderFn;
    _getWeightFn = getWeightFn;
    _getBatteryFn = getBatteryFn;
    _getWifiBarsFn = getWifiBarsFn;
    _ignoreSpikesFn = ignoreSpikesFn;
    _targetKg = initialTargetKg;
    _needsRedraw = true;
  }

  float targetKg() const { return _targetKg; }

  void update() {
    ButtonManager::Presses p = _buttons->poll();
    UIState prevState = _state;

    if (p.any()) {
      handleInput(p);
      _needsRedraw = true;
    }

    // FIX: only (re-)arm the anti-jitter freeze on an actual transition into
    // a menu screen, or on a press while already on the main screen - not
    // every single loop tick. Re-arming every tick (the old version) meant
    // the freeze window kept sliding forward for as long as you were
    // anywhere off the main screen, and nothing cleared it on return - so
    // the live weight could sit stuck for a while (or indefinitely, if you
    // kept navigating) even after you were back looking at the main screen.
    if (_ignoreSpikesFn) {
      if (_state != UIState::MAIN_SCREEN && prevState != _state) {
        _ignoreSpikesFn(1500);      // just entered a menu screen - brief freeze
      } else if (_state == UIState::MAIN_SCREEN && prevState != _state) {
        _ignoreSpikesFn(0);         // just returned to main - clear the freeze now
      } else if (p.any() && _state == UIState::MAIN_SCREEN) {
        _ignoreSpikesFn(1500);      // press while already on main - brief freeze
      }
    }

    // Force redraw whenever the weight changes by even 0.01kg so it stays live.
    float currentWeight = _getWeightFn ? _getWeightFn() : 0.0f;
    if (_state == UIState::MAIN_SCREEN && fabs(currentWeight - _lastDrawnWeight) >= 0.01f) {
      _lastDrawnWeight = currentWeight;
      _needsRedraw = true;
    }

    if (_needsRedraw) {
      redraw();
      _needsRedraw = false;
    }
  }

private:
  DisplayManager* _display = nullptr;
  ButtonManager* _buttons = nullptr;
  TareFn _tareFn = nullptr;
  OrderFn _orderFn = nullptr;
  FloatFn _getWeightFn = nullptr;
  IntFn _getBatteryFn = nullptr;
  IntFn _getWifiBarsFn = nullptr;
  IgnoreSpikesFn _ignoreSpikesFn = nullptr;

  UIState _state = UIState::MAIN_SCREEN;
  float _targetKg = 5.0f;
  float _lastDrawnWeight = -1.0f;
  int _menuIndex = 0;
  int _menuScroll = 0;
  bool _needsRedraw = true;

  static constexpr int NUM_MENU_ITEMS = 5;
  const char* _menuItems[NUM_MENU_ITEMS] = {
    "Tare Scale", "Set Target", "Customer Order", "Battery Info", "Exit Menu"
  };

  void handleInput(ButtonManager::Presses p) {
    switch (_state) {
      case UIState::MAIN_SCREEN:
        if (p.ok) {
          _state = UIState::MENU_LIST;
          _menuIndex = 0;
          _menuScroll = 0;
        }
        break;

      case UIState::MENU_LIST:
        if (p.down) {
          _menuIndex = (_menuIndex + 1) % NUM_MENU_ITEMS;
          if (_menuIndex >= _menuScroll + 3) _menuScroll = _menuIndex - 2;
          if (_menuIndex == 0) _menuScroll = 0;
        } else if (p.up) {
          _menuIndex = (_menuIndex - 1 + NUM_MENU_ITEMS) % NUM_MENU_ITEMS;
          if (_menuIndex < _menuScroll) _menuScroll = _menuIndex;
          if (_menuIndex == NUM_MENU_ITEMS - 1) _menuScroll = NUM_MENU_ITEMS - 3;
        } else if (p.ok) {
          selectMenuItem();
        }
        break;

      case UIState::EDIT_THRESHOLD:
        if (p.up) {
          _targetKg += 0.5f;
        } else if (p.down) {
          _targetKg = max(0.0f, _targetKg - 0.5f);
        } else if (p.ok) {
          _display->showToast("SAVED!");
          _state = UIState::MENU_LIST;
        }
        break;

      case UIState::BATTERY_INFO:
        if (p.ok) _state = UIState::MENU_LIST;
        break;

      case UIState::CONFIRM_ORDER:
        if (p.ok) {
          if (_getWifiBarsFn && _getWifiBarsFn() < 0) {
            _display->showToast("NO WIFI", 1500);
          } else {
            _display->showToast("SENDING...", 500);
            if (_orderFn) _orderFn(false);
            _display->showToast("SENT!", 800);
          }
          _state = UIState::MAIN_SCREEN;
        } else if (p.up || p.down) {
          _state = UIState::MAIN_SCREEN;
        }
        break;
    }
  }

  void selectMenuItem() {
    switch (_menuIndex) {
      case 0:
        _display->showToast("Taring...");
        if (_tareFn) _tareFn();
        _display->showToast("TARED!");
        _state = UIState::MAIN_SCREEN;
        break;
      case 1:
        _state = UIState::EDIT_THRESHOLD;
        break;
      case 2:
        _state = UIState::CONFIRM_ORDER;
        break;
      case 3:
        _state = UIState::BATTERY_INFO;
        break;
      case 4:
        _state = UIState::MAIN_SCREEN;
        break;
    }
  }

  void redraw() {
    int batteryPct = _getBatteryFn ? _getBatteryFn() : 0;
    int wifiBars = _getWifiBarsFn ? _getWifiBarsFn() : -1;
    float weight = _getWeightFn ? _getWeightFn() : 0.0f;

    switch (_state) {
      case UIState::MAIN_SCREEN:
        _display->drawMainScreen(weight, _targetKg, batteryPct, wifiBars);
        break;
      case UIState::MENU_LIST:
        _display->drawMenuList(_menuItems, NUM_MENU_ITEMS, _menuIndex, _menuScroll);
        break;
      case UIState::EDIT_THRESHOLD:
        _display->drawEditThreshold(_targetKg);
        break;
      case UIState::BATTERY_INFO:
        _display->drawBatteryInfo(batteryPct);
        break;
      case UIState::CONFIRM_ORDER:
        _display->drawConfirmOrder();
        break;
    }
  }
};



// ---------------- setup() / loop() ----------------
DisplayManager display;
ButtonManager buttons;
BatteryMonitor battery;
LoadCellManager loadCell;
AlertManager alerts;
LaundryNetwork network;
AppController app;

bool autoOrderLatched = false; 
bool reminderSent = false;
unsigned long autoOrderTime = 0;
unsigned long overThresholdStart = 0;
unsigned long underThresholdStart = 0;

float getWeight()       { return loadCell.readKg(); }
int   getBatteryPct()   { return battery.readPercentage(); }
int   getWifiBars()     { return network.wifiBars(); }   
void  doIgnoreSpikes(unsigned int ms) { loadCell.ignoreSpikesFor(ms); }

void doTare() {
  loadCell.tare();
}

void doOrder(bool isReminder = false) {
  float weight = loadCell.readKg();
  int code = network.sendPickupAlert(weight, isReminder);
  if (code > 0) {
    Serial.print("Telegram sent, HTTP "); Serial.println(code);
  } else {
    Serial.print("Telegram send failed, code "); Serial.println(code);
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(LED_RED_PIN, OUTPUT);
  digitalWrite(LED_RED_PIN, LOW);
  // Removed holdPowerDown() so the amplifier can warm up while Wi-Fi connects
  
  Serial.begin(115200);
  delay(100);

  battery.begin(BATT_PIN);
  buttons.begin(BTN_UP_PIN, BTN_OK_PIN, BTN_DOWN_PIN);
  alerts.begin();

  if (!display.begin()) {
    Serial.println("ERROR: OLED not detected.");
  }
  display.showBootScreen("SMART", "LAUNDRY");
  delay(1200);

  // STEP 2: WI-FI CONNECTION TEST
  display.showStatusLine("Connecting", "WiFi...", 0);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long start = millis();
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    delay(500);
    dots = (dots + 1) % 6;
    display.showStatusLine("Connecting", "WiFi...", dots);
  }

  if (WiFi.status() == WL_CONNECTED) {
    display.showStatusLine("Syncing", "Time...", 0);
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    
    struct tm timeinfo;
    int retries = 0;
    while (!getLocalTime(&timeinfo, 1000) && retries < 10) {
      retries++;
      display.showStatusLine("Syncing", "Time...", retries % 6);
    }
    
    if (retries < 10) {
      display.showToast("TIME SYNCED!");
    } else {
      display.showToast("SYNC FAILED");
    }
  } else {
    display.showToast("WIFI FAILED");
    WiFi.mode(WIFI_OFF);
  }

  display.showStatusLine("Waking", "Load Cell...", 0);
  if (!loadCell.begin()) {
    Serial.println("ERROR: HX711 not responding.");
  }

  app.begin(&display, &buttons, doTare, doOrder,
            getWeight, getBatteryPct, getWifiBars,
            doIgnoreSpikes, DEFAULT_TARGET_KG);
}

void loop() {
  float weight = loadCell.readKg();
  // ---- TEMPORARY DIAGNOSTIC - add this near the top of loop(), after
// "float weight = loadCell.readKg();" - remove once we've found the issue ----
  static unsigned long _dbgLast = 0;
  if (millis() - _dbgLast > 1000) {
    _dbgLast = millis();
    Serial.print("[DEBUG] is_ready=");
    Serial.print(loadCell.isReady() ? "true" : "false");
    Serial.print("  readKg()=");
    Serial.print(weight, 3);
    Serial.print("  app.targetKg()=");
    Serial.println(app.targetKg());
  }

  float target = app.targetKg();
  bool overThreshold = (target > 0.0f) && (weight >= target);

  alerts.updateThresholdLed(overThreshold);

  if (overThreshold) {
    if (overThresholdStart == 0) overThresholdStart = millis();
    if (millis() - overThresholdStart > 3000 && !autoOrderLatched) {
      autoOrderLatched = true;
      autoOrderTime = millis();
      doOrder(false); // First order
    }
    
    // Reminder Logic: Wait REMINDER_DELAY_MS after the first order
    if (autoOrderLatched && !reminderSent) {
      if (millis() - autoOrderTime > REMINDER_DELAY_MS) {
        reminderSent = true;
        doOrder(true); // Send Reminder
      }
    }
  } else {
    overThresholdStart = 0;
  }

  if (weight < 1.0f) {
    if (underThresholdStart == 0) underThresholdStart = millis();
    if (millis() - underThresholdStart > 5000 && autoOrderLatched) {
      autoOrderLatched = false;
      reminderSent = false;
    }
  } else {
    underThresholdStart = 0;
  }

  app.update();
  yield();
}


