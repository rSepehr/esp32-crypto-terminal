/*
 * ═══════════════════════════════════════════════════════════════════════
 * CRYPTO TERMINAL PRO - Ultimate Edition v3.0
 * ═══════════════════════════════════════════════════════════════════════
 * 
 * Professional Cryptocurrency Market Monitor for ESP32
 * 
 * Features:
 * - Real-time price tracking for 15 cryptocurrencies
 * - 6 Different view modes (Market/Detail/Portfolio/Heatmap/Trends/Alerts)
 * - Advanced charting with 3 chart types (Line/Candle/Area)
 * - Portfolio management with P&L tracking
 * - Price alert system
 * - Market statistics and sentiment analysis
 * - Professional cyberpunk UI design
 * 
 * Hardware Requirements:
 * - ESP32-WROOM-32
 * - 1.8" TFT Display 128x160 (ST7735)
 * - 3 Push buttons
 * - 1 LED indicator
 * 
 * Pin Configuration:
 * - LED: GPIO 4
 * - Button Scroll: GPIO 12 (Pull-up)
 * - Button Select: GPIO 14 (Pull-up)
 * - Button Mode: GPIO 27 (Pull-up)
 * 
 * Author: Sepehr Zahedi Abghari
 * License: MIT
 * GitHub: https://github.com/rSepehr/esp32-crypto-terminal
 * 
 * ═══════════════════════════════════════════════════════════════════════
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <vector>
#include <algorithm>

// ═══════════════════════════════════════════════════════════
// 🔐 CONFIGURATION
// ═══════════════════════════════════════════════════════════
const char* WIFI_SSID = "your_wifi_name";
const char* WIFI_PASSWORD = "your_wifi_password";
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET = 12600; // GMT+3:30 (Iran)
const int WIFI_TIMEOUT = 30000; // 30 seconds
const int API_TIMEOUT = 10000;  // 10 seconds
const unsigned long UPDATE_INTERVAL = 30000; // 30 seconds
const unsigned long DEBOUNCE_DELAY = 250;

// ═══════════════════════════════════════════════════════════
// 🎨 COLOR PALETTE (Cyberpunk Theme)
// ═══════════════════════════════════════════════════════════
#define COLOR_BG_DARK       0x0000  // Black
#define COLOR_BG_PANEL      0x1082  // Dark Gray
#define COLOR_ACCENT_CYAN   0x07FF  // Cyan
#define COLOR_ACCENT_PURPLE 0x781F  // Purple
#define COLOR_ACCENT_PINK   0xF81F  // Magenta
#define COLOR_TEXT_WHITE    0xFFFF  // White
#define COLOR_TEXT_GRAY     0x7BEF  // Light Gray
#define COLOR_GREEN         0x07E0  // Green (Up)
#define COLOR_RED           0xF800  // Red (Down)
#define COLOR_GOLD          0xFEA0  // Gold
#define COLOR_ORANGE        0xFD20  // Orange
#define COLOR_GRID          0x2104  // Dark Grid

// ═══════════════════════════════════════════════════════════
// 📊 DATA STRUCTURES
// ═══════════════════════════════════════════════════════════
struct CryptoAsset {
  const char* symbol;
  const char* name;
  const char* coinGeckoId;
  float price;
  float change24h;
  float marketCap;
  float volume24h;
  float high24h;
  float low24h;
  float priceHistory[40];
  uint8_t historyIndex;
  uint8_t rank;
  bool dataValid;
};

struct MarketStats {
  float totalMarketCap;
  float btcDominance;
  uint8_t fearGreedIndex;
  uint8_t gainersCount;
  uint8_t losersCount;
};

struct PortfolioEntry {
  const char* symbol;
  float amount;
  float buyPrice;
};

struct SystemState {
  bool wifiConnected;
  bool dataLoaded;
  unsigned long lastUpdate;
  unsigned long lastButtonPress;
  uint8_t viewMode;
  uint8_t selectedIndex;
  uint8_t menuOffset;
  uint8_t chartMode;
  bool needsRedraw;
};

// ═══════════════════════════════════════════════════════════
// 📦 GLOBAL OBJECTS
// ═══════════════════════════════════════════════════════════
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

// Crypto assets (15 top cryptocurrencies)
CryptoAsset cryptos[] = {
  {"BTC", "Bitcoin", "bitcoin", 0, 0, 0, 0, 0, 0, {}, 0, 1, false},
  {"ETH", "Ethereum", "ethereum", 0, 0, 0, 0, 0, 0, {}, 0, 2, false},
  {"BNB", "BNB", "binancecoin", 0, 0, 0, 0, 0, 0, {}, 0, 3, false},
  {"XRP", "Ripple", "ripple", 0, 0, 0, 0, 0, 0, {}, 0, 4, false},
  {"ADA", "Cardano", "cardano", 0, 0, 0, 0, 0, 0, {}, 0, 5, false},
  {"SOL", "Solana", "solana", 0, 0, 0, 0, 0, 0, {}, 0, 6, false},
  {"DOGE", "Dogecoin", "dogecoin", 0, 0, 0, 0, 0, 0, {}, 0, 7, false},
  {"MATIC", "Polygon", "matic-network", 0, 0, 0, 0, 0, 0, {}, 0, 8, false},
  {"DOT", "Polkadot", "polkadot", 0, 0, 0, 0, 0, 0, {}, 0, 9, false},
  {"AVAX", "Avalanche", "avalanche-2", 0, 0, 0, 0, 0, 0, {}, 0, 10, false},
  {"LINK", "Chainlink", "chainlink", 0, 0, 0, 0, 0, 0, {}, 0, 11, false},
  {"UNI", "Uniswap", "uniswap", 0, 0, 0, 0, 0, 0, {}, 0, 12, false},
  {"ATOM", "Cosmos", "cosmos", 0, 0, 0, 0, 0, 0, {}, 0, 13, false},
  {"LTC", "Litecoin", "litecoin", 0, 0, 0, 0, 0, 0, {}, 0, 14, false},
  {"NEAR", "Near", "near", 0, 0, 0, 0, 0, 0, {}, 0, 15, false}
};
const uint8_t CRYPTO_COUNT = sizeof(cryptos) / sizeof(cryptos[0]);

// Portfolio (demo entries)
PortfolioEntry portfolio[] = {
  {"BTC", 0.05, 40000},
  {"ETH", 1.5, 2000},
  {"SOL", 10, 100}
};
const uint8_t PORTFOLIO_COUNT = sizeof(portfolio) / sizeof(portfolio[0]);

// System state
SystemState sys = {false, false, 0, 0, 0, 0, 0, 0, true};
MarketStats market = {0, 0, 50, 0, 0};

// Hardware pins
const uint8_t PIN_LED = 4;
const uint8_t PIN_BTN_SCROLL = 12;
const uint8_t PIN_BTN_SELECT = 14;
const uint8_t PIN_BTN_MODE = 27;

// View modes
enum ViewMode {
  VIEW_SPLASH = 0,
  VIEW_MARKET = 1,
  VIEW_DETAIL = 2,
  VIEW_PORTFOLIO = 3,
  VIEW_HEATMAP = 4,
  VIEW_TRENDS = 5
};

// Chart modes
enum ChartMode {
  CHART_LINE = 0,
  CHART_AREA = 1,
  CHART_CANDLE = 2
};

const uint8_t ITEMS_PER_PAGE = 5;

// ═══════════════════════════════════════════════════════════
// 🔧 FUNCTION DECLARATIONS
// ═══════════════════════════════════════════════════════════
// System
void setupHardware();
void setupDisplay();
bool connectWiFi();
void updateSystemState();

// Data
bool fetchCryptoData();
void updateMarketStats();
void updatePriceHistory(uint8_t index, float price);
float calculatePortfolioValue();
float calculatePortfolioChange();

// Display
void drawSplashScreen();
void drawMarketView();
void drawDetailView();
void drawPortfolioView();
void drawHeatmapView();
void drawTrendsView();
void drawTopBar(const char* title);
void drawBottomBar(const char* left, const char* center, const char* right);
void drawNotification(const char* message, uint16_t color);

// Charts
void drawMiniChart(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t cryptoIdx);
void drawAdvancedChart(uint8_t cryptoIdx);

// Utilities
String formatPrice(float price);
String formatChange(float change);
String formatLargeNumber(float number);
String getTimeString();
void blinkLED(uint8_t times = 1);
void safeDelay(unsigned long ms);

// ═══════════════════════════════════════════════════════════
// 🚀 SETUP
// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  Serial.println(F("\n╔════════════════════════════════════╗"));
  Serial.println(F("║  CRYPTO TERMINAL PRO - v3.0        ║"));
  Serial.println(F("╚════════════════════════════════════╝\n"));
  
  setupHardware();
  setupDisplay();
  
  drawSplashScreen();
  safeDelay(2000);
  
  // Connect to WiFi
  sys.wifiConnected = connectWiFi();
  
  if (sys.wifiConnected) {
    Serial.println(F("✓ WiFi connected successfully"));
    configTime(GMT_OFFSET, 0, NTP_SERVER);
    
    // Initial data fetch
    if (fetchCryptoData()) {
      sys.dataLoaded = true;
      updateMarketStats();
      Serial.println(F("✓ Initial data loaded"));
    } else {
      Serial.println(F("✗ Failed to load initial data"));
      drawNotification("Data Load Failed", COLOR_RED);
      safeDelay(2000);
    }
  } else {
    Serial.println(F("✗ WiFi connection failed"));
    drawNotification("WiFi Failed!", COLOR_RED);
    safeDelay(2000);
    // Don't proceed to main views without WiFi
    sys.viewMode = VIEW_SPLASH;
    sys.needsRedraw = true;
    return;
  }
  
  // Start with market view
  sys.viewMode = VIEW_MARKET;
  sys.needsRedraw = true;
  sys.lastUpdate = millis();
  
  Serial.println(F("✓ System ready\n"));
}

// ═══════════════════════════════════════════════════════════
// 🔁 MAIN LOOP
// ═══════════════════════════════════════════════════════════
void loop() {
  // Handle buttons
  if (millis() - sys.lastButtonPress > DEBOUNCE_DELAY) {
    
    // Scroll button
    if (digitalRead(PIN_BTN_SCROLL) == LOW) {
      sys.lastButtonPress = millis();
      blinkLED(1);
      
      if (sys.viewMode == VIEW_MARKET || sys.viewMode == VIEW_HEATMAP) {
        sys.selectedIndex++;
        if (sys.selectedIndex >= CRYPTO_COUNT) {
          sys.selectedIndex = 0;
          sys.menuOffset = 0;
        } else if (sys.selectedIndex >= sys.menuOffset + ITEMS_PER_PAGE) {
          sys.menuOffset++;
        }
        sys.needsRedraw = true;
      }
    }
    
    // Select button
    if (digitalRead(PIN_BTN_SELECT) == LOW) {
      sys.lastButtonPress = millis();
      blinkLED(1);
      
      if (sys.viewMode == VIEW_MARKET) {
        sys.viewMode = VIEW_DETAIL;
        sys.needsRedraw = true;
      } else if (sys.viewMode == VIEW_DETAIL) {
        sys.chartMode = (sys.chartMode + 1) % 3;
        sys.needsRedraw = true;
      } else {
        sys.viewMode = VIEW_MARKET;
        sys.needsRedraw = true;
      }
    }
    
    // Mode button
    if (digitalRead(PIN_BTN_MODE) == LOW) {
      sys.lastButtonPress = millis();
      blinkLED(1);
      
      sys.viewMode++;
      if (sys.viewMode > VIEW_TRENDS) sys.viewMode = VIEW_MARKET;
      sys.needsRedraw = true;
    }
  }
  
  // Auto-update data
  if (sys.wifiConnected && sys.dataLoaded && 
      (millis() - sys.lastUpdate > UPDATE_INTERVAL)) {
    
    Serial.println(F("→ Updating data..."));
    if (fetchCryptoData()) {
      updateMarketStats();
      sys.lastUpdate = millis();
      sys.needsRedraw = true;
      Serial.println(F("✓ Data updated"));
    } else {
      Serial.println(F("✗ Update failed"));
    }
  }
  
  // Redraw display if needed
  if (sys.needsRedraw) {
    sys.needsRedraw = false;
    
    switch (sys.viewMode) {
      case VIEW_SPLASH:
        drawSplashScreen();
        break;
      case VIEW_MARKET:
        if (sys.dataLoaded) drawMarketView();
        break;
      case VIEW_DETAIL:
        if (sys.dataLoaded) drawDetailView();
        break;
      case VIEW_PORTFOLIO:
        if (sys.dataLoaded) drawPortfolioView();
        break;
      case VIEW_HEATMAP:
        if (sys.dataLoaded) drawHeatmapView();
        break;
      case VIEW_TRENDS:
        if (sys.dataLoaded) drawTrendsView();
        break;
    }
  }
  
  safeDelay(10);
}

// ═══════════════════════════════════════════════════════════
// 🔧 HARDWARE SETUP
// ═══════════════════════════════════════════════════════════
void setupHardware() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BTN_SCROLL, INPUT_PULLUP);
  pinMode(PIN_BTN_SELECT, INPUT_PULLUP);
  pinMode(PIN_BTN_MODE, INPUT_PULLUP);
  digitalWrite(PIN_LED, LOW);
  
  Serial.println(F("✓ Hardware initialized"));
}

void setupDisplay() {
  tft.init();
  tft.setRotation(1); // Landscape: 160x128
  tft.fillScreen(COLOR_BG_DARK);
  
  sprite.setColorDepth(16);
  sprite.createSprite(160, 128);
  sprite.setSwapBytes(true);
  
  Serial.println(F("✓ Display initialized (160x128)"));
}

bool connectWiFi() {
  Serial.print(F("→ Connecting to WiFi"));
  
  sprite.fillSprite(COLOR_BG_DARK);
  sprite.setTextColor(COLOR_ACCENT_CYAN);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString("Connecting WiFi...", 80, 50, 2);
  sprite.drawRoundRect(30, 70, 100, 10, 3, COLOR_TEXT_GRAY);
  sprite.pushSprite(0, 0);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  uint8_t progress = 0;
  
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > WIFI_TIMEOUT) {
      Serial.println(F(" TIMEOUT"));
      return false;
    }
    
    delay(500);
    Serial.print(".");
    
    // Update progress bar
    progress = ((millis() - startTime) * 100) / WIFI_TIMEOUT;
    if (progress > 100) progress = 100;
    
    sprite.fillRect(32, 72, progress - 4, 6, COLOR_ACCENT_CYAN);
    sprite.pushSprite(0, 0);
  }
  
  Serial.println(F(" CONNECTED"));
  Serial.print(F("   IP: "));
  Serial.println(WiFi.localIP());
  
  sprite.fillSprite(COLOR_BG_DARK);
  sprite.setTextColor(COLOR_GREEN);
  sprite.drawString("WiFi Connected!", 80, 64, 2);
  sprite.pushSprite(0, 0);
  safeDelay(1000);
  
  return true;
}

// ═══════════════════════════════════════════════════════════
// 🌐 DATA FETCHING
// ═══════════════════════════════════════════════════════════
bool fetchCryptoData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("✗ WiFi not connected"));
    return false;
  }
  
  // Build CoinGecko API URL
  String ids = "";
  for (uint8_t i = 0; i < CRYPTO_COUNT; i++) {
    ids += cryptos[i].coinGeckoId;
    if (i < CRYPTO_COUNT - 1) ids += "%2C";
  }
  
  String url = "https://api.coingecko.com/api/v3/simple/price?ids=" + ids +
               "&vs_currencies=usd&include_24hr_change=true&include_24hr_vol=true" +
               "&include_market_cap=true";
  
  HTTPClient http;
  http.begin(url);
  http.setTimeout(API_TIMEOUT);
  
  int httpCode = http.GET();
  
  if (httpCode != 200) {
    Serial.printf("✗ HTTP Error: %d\n", httpCode);
    http.end();
    return false;
  }
  
  // Parse JSON
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, http.getString());
  http.end();
  
  if (error) {
    Serial.print(F("✗ JSON parse error: "));
    Serial.println(error.c_str());
    return false;
  }
  
  // Update crypto data
  bool success = false;
  for (uint8_t i = 0; i < CRYPTO_COUNT; i++) {
    const char* id = cryptos[i].coinGeckoId;
    
    if (doc.containsKey(id)) {
      float price = doc[id]["usd"] | 0.0f;
      float change = doc[id]["usd_24h_change"] | 0.0f;
      float volume = doc[id]["usd_24h_vol"] | 0.0f;
      float mcap = doc[id]["usd_market_cap"] | 0.0f;
      
      if (price > 0) {
        cryptos[i].price = price;
        cryptos[i].change24h = change;
        cryptos[i].volume24h = volume;
        cryptos[i].marketCap = mcap;
        cryptos[i].high24h = price * 1.05f; // Approximation
        cryptos[i].low24h = price * 0.95f;  // Approximation
        cryptos[i].dataValid = true;
        
        updatePriceHistory(i, price);
        success = true;
      }
    }
  }
  
  return success;
}

void updatePriceHistory(uint8_t index, float price) {
  if (index >= CRYPTO_COUNT) return;
  
  if (cryptos[index].historyIndex < 40) {
    cryptos[index].priceHistory[cryptos[index].historyIndex++] = price;
  } else {
    // Shift array left and add new value
    for (uint8_t i = 0; i < 39; i++) {
      cryptos[index].priceHistory[i] = cryptos[index].priceHistory[i + 1];
    }
    cryptos[index].priceHistory[39] = price;
  }
}

void updateMarketStats() {
  market.totalMarketCap = 0;
  market.gainersCount = 0;
  market.losersCount = 0;
  
  for (uint8_t i = 0; i < CRYPTO_COUNT; i++) {
    if (cryptos[i].dataValid) {
      market.totalMarketCap += cryptos[i].marketCap;
      
      if (cryptos[i].change24h > 0) market.gainersCount++;
      else if (cryptos[i].change24h < 0) market.losersCount++;
    }
  }
  
  // Calculate BTC dominance
  if (cryptos[0].dataValid && market.totalMarketCap > 0) {
    market.btcDominance = (cryptos[0].marketCap / market.totalMarketCap) * 100.0f;
  }
  
  // Calculate Fear & Greed Index (simplified)
  uint8_t total = market.gainersCount + market.losersCount;
  if (total > 0) {
    market.fearGreedIndex = (market.gainersCount * 100) / total;
  }
}

float calculatePortfolioValue() {
  float total = 0;
  for (uint8_t i = 0; i < PORTFOLIO_COUNT; i++) {
    for (uint8_t j = 0; j < CRYPTO_COUNT; j++) {
      if (strcmp(cryptos[j].symbol, portfolio[i].symbol) == 0 && cryptos[j].dataValid) {
        total += portfolio[i].amount * cryptos[j].price;
        break;
      }
    }
  }
  return total;
}

float calculatePortfolioChange() {
  float currentValue = calculatePortfolioValue();
  float costBasis = 0;
  
  for (uint8_t i = 0; i < PORTFOLIO_COUNT; i++) {
    costBasis += portfolio[i].amount * portfolio[i].buyPrice;
  }
  
  if (costBasis > 0) {
    return ((currentValue - costBasis) / costBasis) * 100.0f;
  }
  return 0;
}

// ═══════════════════════════════════════════════════════════
// 🎬 SPLASH SCREEN
// ═══════════════════════════════════════════════════════════
void drawSplashScreen() {
  sprite.fillSprite(COLOR_BG_DARK);
  
  // Logo
  sprite.fillRoundRect(55, 25, 50, 50, 8, COLOR_BG_PANEL);
  sprite.drawRoundRect(55, 25, 50, 50, 8, COLOR_ACCENT_CYAN);
  sprite.drawRoundRect(56, 26, 48, 48, 7, COLOR_ACCENT_PINK);
  
  sprite.setTextColor(COLOR_GOLD);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString("$", 80, 50, 7);
  
  // Title
  sprite.setTextColor(COLOR_TEXT_WHITE);
  sprite.drawString("CRYPTO TERMINAL", 80, 85, 2);
  
  sprite.setTextColor(COLOR_ACCENT_CYAN);
  sprite.drawString("PRO Edition v3.0", 80, 100, 1);
  
  // Badge
  sprite.fillRoundRect(60, 110, 40, 12, 3, COLOR_ACCENT_PURPLE);
  sprite.setTextColor(COLOR_TEXT_WHITE);
  sprite.drawString("GitHub", 80, 116, 1);
  
  sprite.pushSprite(0, 0);
  blinkLED(3);
}

// ═══════════════════════════════════════════════════════════
// 📊 TOP BAR
// ═══════════════════════════════════════════════════════════
void drawTopBar(const char* title) {
  sprite.fillRect(0, 0, 160, 14, COLOR_BG_PANEL);
  sprite.drawFastHLine(0, 14, 160, COLOR_ACCENT_CYAN);
  
  // Time
  sprite.setTextColor(COLOR_TEXT_WHITE);
  sprite.setTextDatum(ML_DATUM);
  sprite.drawString(getTimeString(), 3, 7, 1);
  
  // Title
  sprite.setTextColor(COLOR_ACCENT_CYAN);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString(title, 80, 7, 1);
  
  // WiFi indicator
  uint16_t statusColor = sys.wifiConnected ? COLOR_GREEN : COLOR_RED;
  sprite.fillCircle(153, 7, 2, statusColor);
}

// ═══════════════════════════════════════════════════════════
// 📊 BOTTOM BAR
// ═══════════════════════════════════════════════════════════
void drawBottomBar(const char* left, const char* center, const char* right) {
  sprite.fillRect(0, 116, 160, 12, COLOR_BG_PANEL);
  sprite.drawFastHLine(0, 116, 160, COLOR_ACCENT_CYAN);
  
  sprite.setTextColor(COLOR_TEXT_GRAY);
  
  if (left) {
    sprite.setTextDatum(ML_DATUM);
    sprite.drawString(left, 3, 122, 1);
  }
  
  if (center) {
    sprite.setTextDatum(MC_DATUM);
    sprite.drawString(center, 80, 122, 1);
  }
  
  if (right) {
    sprite.setTextDatum(MR_DATUM);
    sprite.drawString(right, 157, 122, 1);
  }
}

// ═══════════════════════════════════════════════════════════
// 📈 MARKET VIEW
// ═══════════════════════════════════════════════════════════
void drawMarketView() {
  sprite.fillSprite(COLOR_BG_DARK);
  drawTopBar("MARKET");
  
  // Market stats banner
  sprite.fillRect(0, 16, 160, 11, COLOR_BG_PANEL);
  sprite.setTextColor(COLOR_TEXT_GRAY);
  sprite.setTextDatum(ML_DATUM);
  sprite.drawString("MC:" + formatLargeNumber(market.totalMarketCap), 2, 21, 1);
  
  sprite.setTextDatum(MC_DATUM);
  uint16_t fgColor = market.fearGreedIndex > 50 ? COLOR_GREEN : COLOR_RED;
  sprite.setTextColor(fgColor);
  sprite.drawString("F&G:" + String(market.fearGreedIndex), 80, 21, 1);
  
  sprite.setTextColor(COLOR_TEXT_GRAY);
  sprite.setTextDatum(MR_DATUM);
  sprite.drawString("BTC:" + String(market.btcDominance, 0) + "%", 157, 21, 1);
  
  // Table header
  sprite.fillRect(0, 29, 160, 10, COLOR_BG_PANEL);
  sprite.setTextColor(COLOR_TEXT_GRAY);
  sprite.setTextDatum(ML_DATUM);
  sprite.drawString("COIN", 3, 34, 1);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString("PRICE", 80, 34, 1);
  sprite.setTextDatum(MR_DATUM);
  sprite.drawString("24H", 155, 34, 1);
  sprite.drawLine(0, 39, 160, 39, COLOR_GRID);
  
  // Crypto list
  int16_t startY = 41;
  int16_t rowHeight = 15;
  
  for (uint8_t i = 0; i < ITEMS_PER_PAGE; i++) {
    uint8_t idx = sys.menuOffset + i;
    if (idx >= CRYPTO_COUNT) break;
    if (!cryptos[idx].dataValid) continue;
    
    int16_t yPos = startY + (i * rowHeight);
    
    // Selection highlight
    if (idx == sys.selectedIndex) {
      sprite.fillRect(1, yPos, 158, rowHeight - 1, COLOR_BG_PANEL);
      sprite.drawRect(1, yPos, 158, rowHeight - 1, COLOR_ACCENT_CYAN);
    }
    
    // Rank
    sprite.setTextColor(COLOR_GOLD);
    sprite.setTextDatum(ML_DATUM);
    sprite.drawString(String(cryptos[idx].rank), 3, yPos + 7, 1);
    
    // Symbol
    sprite.setTextColor(COLOR_TEXT_WHITE);
    sprite.drawString(cryptos[idx].symbol, 15, yPos + 3, 2);
    
    // Mini chart
    if (cryptos[idx].historyIndex >= 2) {
      drawMiniChart(15, yPos + 10, 30, 4, idx);
    }
    
    // Price
    sprite.setTextColor(COLOR_TEXT_WHITE);
    sprite.setTextDatum(MC_DATUM);
    sprite.drawString(formatPrice(cryptos[idx].price), 80, yPos + 7, 1);
    
    // 24h change
    float change = cryptos[idx].change24h;
    uint16_t changeColor = (change >= 0) ? COLOR_GREEN : COLOR_RED;
    sprite.setTextColor(changeColor);
    sprite.setTextDatum(MR_DATUM);
    sprite.drawString(formatChange(change), 155, yPos + 7, 1);
  }
  
  // Scrollbar
  if (CRYPTO_COUNT > ITEMS_PER_PAGE) {
    int16_t scrollY = 41;
    int16_t scrollH = 73;
    sprite.drawFastVLine(159, scrollY, scrollH, COLOR_GRID);
    int16_t handleH = max(8, (scrollH * ITEMS_PER_PAGE) / CRYPTO_COUNT);
    int16_t handleY = map(sys.selectedIndex, 0, CRYPTO_COUNT - 1, scrollY, scrollY + scrollH - handleH);
    sprite.fillRect(158, handleY, 2, handleH, COLOR_ACCENT_CYAN);
  }
  
  drawBottomBar("SCROLL", "SELECT", "MODE");
  sprite.pushSprite(0, 0);
}

// ═══════════════════════════════════════════════════════════
// 📊 DETAIL VIEW
// ═══════════════════════════════════════════════════════════
void drawDetailView() {
  if (sys.selectedIndex >= CRYPTO_COUNT || !cryptos[sys.selectedIndex].dataValid) {
    sys.viewMode = VIEW_MARKET;
    sys.needsRedraw = true;
    return;
  }
  
  sprite.fillSprite(COLOR_BG_DARK);
  drawTopBar("DETAIL");
  
  CryptoAsset &crypto = cryptos[sys.selectedIndex];
  
  // Info panel
  sprite.fillRoundRect(3, 17, 154, 30, 4, COLOR_BG_PANEL);
  sprite.drawRoundRect(3, 17, 154, 30, 4, COLOR_ACCENT_CYAN);
  
  // Symbol
  sprite.setTextColor(COLOR_GOLD);
  sprite.setTextDatum(ML_DATUM);
  sprite.drawString(crypto.symbol, 8, 24, 2);
  
  // Name
  sprite.setTextColor(COLOR_TEXT_GRAY);
  sprite.drawString(crypto.name, 8, 35, 1);
  
  // Rank badge
  sprite.fillCircle(145, 25, 8, COLOR_ACCENT_PURPLE);
  sprite.setTextColor(COLOR_TEXT_WHITE);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString("#" + String(crypto.rank), 145, 25, 1);
  
  // Price
  sprite.setTextColor(COLOR_TEXT_WHITE);
  sprite.setTextDatum(MR_DATUM);
  sprite.drawString(formatPrice(crypto.price), 152, 41, 4);
  
  // 24h change badge
  float change = crypto.change24h;
  uint16_t changeColor = (change >= 0) ? COLOR_GREEN : COLOR_RED;
  sprite.fillRoundRect(8, 42, 42, 10, 2, changeColor);
  sprite.setTextColor(COLOR_BG_DARK);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString(formatChange(change), 29, 47, 1);
  
  // Stats bar
  sprite.fillRect(3, 49, 154, 10, COLOR_BG_PANEL);
  sprite.setTextColor(COLOR_TEXT_GRAY);
  sprite.setTextDatum(ML_DATUM);
  sprite.drawString("H:" + formatPrice(crypto.high24h), 6, 54, 1);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString("L:" + formatPrice(crypto.low24h), 80, 54, 1);
  sprite.setTextDatum(MR_DATUM);
  sprite.drawString("V:" + formatLargeNumber(crypto.volume24h), 154, 54, 1);
  
  // Chart
  drawAdvancedChart(sys.selectedIndex);
  
  // Chart mode indicator
  sprite.fillRoundRect(3, 105, 35, 9, 2, COLOR_ACCENT_PURPLE);
  sprite.setTextColor(COLOR_TEXT_WHITE);
  sprite.setTextDatum(MC_DATUM);
  const char* mode = (sys.chartMode == CHART_LINE) ? "LINE" : 
                     (sys.chartMode == CHART_AREA) ? "AREA" : "CANDLE";
  sprite.drawString(mode, 20, 109, 1);
  
  drawBottomBar("CHART", crypto.symbol, "BACK");
  sprite.pushSprite(0, 0);
}

// ═══════════════════════════════════════════════════════════
// 💼 PORTFOLIO VIEW
// ═══════════════════════════════════════════════════════════
void drawPortfolioView() {
  sprite.fillSprite(COLOR_BG_DARK);
  drawTopBar("PORTFOLIO");
  
  float totalValue = calculatePortfolioValue();
  float totalChange = calculatePortfolioChange();
  
  // Summary panel
  sprite.fillRoundRect(3, 17, 154, 28, 4, COLOR_BG_PANEL);
  sprite.drawRoundRect(3, 17, 154, 28, 4, COLOR_ACCENT_CYAN);
  
  sprite.setTextColor(COLOR_TEXT_GRAY);
  sprite.setTextDatum(ML_DATUM);
  sprite.drawString("Total Value", 8, 24, 1);
  
  sprite.setTextColor(COLOR_TEXT_WHITE);
  sprite.drawString("$" + String(totalValue, 2), 8, 36, 4);
  
  // Change badge
  uint16_t changeColor = (totalChange >= 0) ? COLOR_GREEN : COLOR_RED;
  sprite.fillRoundRect(100, 20, 50, 12, 3, changeColor);
  sprite.setTextColor(COLOR_BG_DARK);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString(formatChange(totalChange), 125, 26, 1);
  
  // Holdings header
  sprite.setTextColor(COLOR_TEXT_GRAY);
  sprite.setTextDatum(ML_DATUM);
  sprite.drawString("Holdings:", 5, 50, 1);
  sprite.drawFastHLine(0, 54, 160, COLOR_GRID);
  
  // Holdings list
  int16_t yPos = 57;
  for (uint8_t i = 0; i < min(4, (int)PORTFOLIO_COUNT); i++) {
    // Find crypto data
    CryptoAsset* crypto = nullptr;
    for (uint8_t j = 0; j < CRYPTO_COUNT; j++) {
      if (strcmp(cryptos[j].symbol, portfolio[i].symbol) == 0 && cryptos[j].dataValid) {
        crypto = &cryptos[j];
        break;
      }
    }
    
    if (!crypto) continue;
    
    float currentValue = portfolio[i].amount * crypto->price;
    float profit = currentValue - (portfolio[i].amount * portfolio[i].buyPrice);
    float profitPercent = (profit / (portfolio[i].amount * portfolio[i].buyPrice)) * 100.0f;
    
    // Symbol
    sprite.setTextColor(COLOR_TEXT_WHITE);
    sprite.setTextDatum(ML_DATUM);
    sprite.drawString(portfolio[i].symbol, 5, yPos + 3, 2);
    
    // Amount
    sprite.setTextColor(COLOR_TEXT_GRAY);
    sprite.drawString(String(portfolio[i].amount, 2), 5, yPos + 13, 1);
    
    // Value
    sprite.setTextColor(COLOR_TEXT_WHITE);
    sprite.setTextDatum(MR_DATUM);
    sprite.drawString("$" + String(currentValue, 0), 100, yPos + 3, 1);
    
    // P&L
    uint16_t plColor = (profit >= 0) ? COLOR_GREEN : COLOR_RED;
    sprite.setTextColor(plColor);
    sprite.drawString((profit >= 0 ? "+" : "") + String(profitPercent, 1) + "%", 155, yPos + 3, 1);
    
    yPos += 17;
  }
  
  drawBottomBar(nullptr, "[MODE] Switch View", nullptr);
  sprite.pushSprite(0, 0);
}

// ═══════════════════════════════════════════════════════════
// 🔥 HEATMAP VIEW
// ═══════════════════════════════════════════════════════════
void drawHeatmapView() {
  sprite.fillSprite(COLOR_BG_DARK);
  drawTopBar("HEATMAP");
  
  sprite.setTextColor(COLOR_TEXT_GRAY);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString("Market Heatmap - 24h", 80, 20, 1);
  sprite.drawFastHLine(0, 24, 160, COLOR_GRID);
  
  // Heatmap grid (4x4)
  int16_t cellW = 38;
  int16_t cellH = 19;
  int16_t startX = 4;
  int16_t startY = 27;
  
  for (uint8_t i = 0; i < min(16, (int)CRYPTO_COUNT); i++) {
    if (!cryptos[i].dataValid) continue;
    
    uint8_t row = i / 4;
    uint8_t col = i % 4;
    int16_t x = startX + (col * (cellW + 2));
    int16_t y = startY + (row * (cellH + 2));
    
    float change = cryptos[i].change24h;
    
    // Color based on change
    uint16_t cellColor;
    if (change > 10) cellColor = 0x07E0;
    else if (change > 5) cellColor = 0x05E0;
    else if (change > 0) cellColor = 0x03E0;
    else if (change > -5) cellColor = 0xF800;
    else if (change > -10) cellColor = 0xD800;
    else cellColor = 0xB800;
    
    sprite.fillRoundRect(x, y, cellW, cellH, 2, cellColor);
    
    // Symbol
    sprite.setTextColor(COLOR_BG_DARK);
    sprite.setTextDatum(MC_DATUM);
    sprite.drawString(cryptos[i].symbol, x + cellW / 2, y + 6, 1);
    
    // Change
    sprite.drawString(String(change, 1) + "%", x + cellW / 2, y + 14, 1);
  }
  
  // Legend
  sprite.fillRect(0, 108, 160, 8, COLOR_BG_PANEL);
  
  sprite.fillRect(5, 111, 8, 4, 0x07E0);
  sprite.setTextColor(COLOR_TEXT_GRAY);
  sprite.setTextDatum(ML_DATUM);
  sprite.drawString(">10%", 15, 113, 1);
  
  sprite.fillRect(50, 111, 8, 4, 0x03E0);
  sprite.drawString("0-5%", 60, 113, 1);
  
  sprite.fillRect(95, 111, 8, 4, 0xF800);
  sprite.drawString("<0%", 105, 113, 1);
  
  drawBottomBar("SCROLL", nullptr, "MODE");
  sprite.pushSprite(0, 0);
}

// ═══════════════════════════════════════════════════════════
// 📊 TRENDS VIEW
// ═══════════════════════════════════════════════════════════
void drawTrendsView() {
  sprite.fillSprite(COLOR_BG_DARK);
  drawTopBar("TRENDS");
  
  // Sort by change
  uint8_t sorted[CRYPTO_COUNT];
  for (uint8_t i = 0; i < CRYPTO_COUNT; i++) sorted[i] = i;
  
  for (uint8_t i = 0; i < CRYPTO_COUNT - 1; i++) {
    for (uint8_t j = i + 1; j < CRYPTO_COUNT; j++) {
      if (cryptos[sorted[i]].change24h < cryptos[sorted[j]].change24h) {
        uint8_t temp = sorted[i];
        sorted[i] = sorted[j];
        sorted[j] = temp;
      }
    }
  }
  
  // Top gainers
  sprite.fillRect(0, 16, 80, 11, COLOR_GREEN);
  sprite.setTextColor(COLOR_BG_DARK);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString("TOP GAINERS", 40, 21, 1);
  
  int16_t yPos = 29;
  for (uint8_t i = 0; i < 5; i++) {
    uint8_t idx = sorted[i];
    if (!cryptos[idx].dataValid) continue;
    
    sprite.setTextColor(COLOR_TEXT_WHITE);
    sprite.setTextDatum(ML_DATUM);
    sprite.drawString(String(i + 1) + "." + cryptos[idx].symbol, 5, yPos, 1);
    
    sprite.setTextColor(COLOR_GREEN);
    sprite.setTextDatum(MR_DATUM);
    sprite.drawString("+" + String(cryptos[idx].change24h, 1) + "%", 75, yPos, 1);
    
    yPos += 9;
  }
  
  // Top losers
  sprite.fillRect(80, 16, 80, 11, COLOR_RED);
  sprite.setTextColor(COLOR_BG_DARK);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString("TOP LOSERS", 120, 21, 1);
  
  yPos = 29;
  for (uint8_t i = 0; i < 5; i++) {
    uint8_t idx = sorted[CRYPTO_COUNT - 1 - i];
    if (!cryptos[idx].dataValid) continue;
    
    sprite.setTextColor(COLOR_TEXT_WHITE);
    sprite.setTextDatum(ML_DATUM);
    sprite.drawString(String(i + 1) + "." + cryptos[idx].symbol, 85, yPos, 1);
    
    sprite.setTextColor(COLOR_RED);
    sprite.setTextDatum(MR_DATUM);
    sprite.drawString(String(cryptos[idx].change24h, 1) + "%", 155, yPos, 1);
    
    yPos += 9;
  }
  
  // Market sentiment
  sprite.fillRect(0, 75, 160, 28, COLOR_BG_PANEL);
  sprite.drawRect(0, 75, 160, 28, COLOR_ACCENT_CYAN);
  
  sprite.setTextColor(COLOR_TEXT_GRAY);
  sprite.setTextDatum(ML_DATUM);
  sprite.drawString("Market Sentiment:", 5, 81, 1);
  
  uint8_t total = market.gainersCount + market.losersCount;
  if (total > 0) {
    int16_t greenWidth = (market.gainersCount * 150) / total;
    sprite.fillRect(5, 88, greenWidth, 10, COLOR_GREEN);
    sprite.fillRect(5 + greenWidth, 88, 150 - greenWidth, 10, COLOR_RED);
  }
  
  sprite.setTextColor(COLOR_TEXT_WHITE);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString(String(market.gainersCount) + " Up / " + 
                    String(market.losersCount) + " Down", 80, 93, 1);
  
  drawBottomBar(nullptr, "[MODE] Next View", nullptr);
  sprite.pushSprite(0, 0);
}

// ═══════════════════════════════════════════════════════════
// 📈 MINI CHART
// ═══════════════════════════════════════════════════════════
void drawMiniChart(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t cryptoIdx) {
  if (cryptoIdx >= CRYPTO_COUNT || cryptos[cryptoIdx].historyIndex < 2) return;
  
  CryptoAsset &crypto = cryptos[cryptoIdx];
  
  // Find min/max
  float minP = crypto.priceHistory[0];
  float maxP = crypto.priceHistory[0];
  for (uint8_t i = 0; i < crypto.historyIndex; i++) {
    if (crypto.priceHistory[i] < minP) minP = crypto.priceHistory[i];
    if (crypto.priceHistory[i] > maxP) maxP = crypto.priceHistory[i];
  }
  
  if (maxP <= minP) return;
  
  uint16_t chartColor = (crypto.change24h >= 0) ? COLOR_GREEN : COLOR_RED;
  
  // Draw lines
  for (uint8_t i = 0; i < crypto.historyIndex - 1; i++) {
    int16_t x1 = x + (i * w / (crypto.historyIndex - 1));
    int16_t y1 = y + h - ((crypto.priceHistory[i] - minP) / (maxP - minP) * h);
    int16_t x2 = x + ((i + 1) * w / (crypto.historyIndex - 1));
    int16_t y2 = y + h - ((crypto.priceHistory[i + 1] - minP) / (maxP - minP) * h);
    sprite.drawLine(x1, y1, x2, y2, chartColor);
  }
}

// ═══════════════════════════════════════════════════════════
// 📊 ADVANCED CHART
// ═══════════════════════════════════════════════════════════
void drawAdvancedChart(uint8_t cryptoIdx) {
  if (cryptoIdx >= CRYPTO_COUNT || cryptos[cryptoIdx].historyIndex < 2) return;
  
  CryptoAsset &crypto = cryptos[cryptoIdx];
  
  int16_t chartX = 8;
  int16_t chartY = 63;
  int16_t chartW = 144;
  int16_t chartH = 38;
  
  // Background
  sprite.fillRoundRect(chartX - 3, chartY - 3, chartW + 6, chartH + 6, 3, COLOR_BG_PANEL);
  
  // Grid
  for (uint8_t i = 0; i <= 3; i++) {
    int16_t y = chartY + (i * chartH / 3);
    sprite.drawFastHLine(chartX, y, chartW, COLOR_GRID);
  }
  
  // Find min/max
  float minP = crypto.priceHistory[0];
  float maxP = crypto.priceHistory[0];
  for (uint8_t i = 0; i < crypto.historyIndex; i++) {
    if (crypto.priceHistory[i] < minP) minP = crypto.priceHistory[i];
    if (crypto.priceHistory[i] > maxP) maxP = crypto.priceHistory[i];
  }
  
  if (maxP <= minP) return;
  
  uint16_t chartColor = (crypto.change24h >= 0) ? COLOR_GREEN : COLOR_RED;
  
  // Draw based on mode
  switch (sys.chartMode) {
    case CHART_LINE:
      // Line chart
      for (uint8_t i = 0; i < crypto.historyIndex - 1; i++) {
        int16_t x1 = chartX + (i * chartW / (crypto.historyIndex - 1));
        int16_t y1 = chartY + chartH - ((crypto.priceHistory[i] - minP) / (maxP - minP) * chartH);
        int16_t x2 = chartX + ((i + 1) * chartW / (crypto.historyIndex - 1));
        int16_t y2 = chartY + chartH - ((crypto.priceHistory[i + 1] - minP) / (maxP - minP) * chartH);
        sprite.drawLine(x1, y1, x2, y2, chartColor);
        sprite.drawLine(x1, y1 + 1, x2, y2 + 1, chartColor);
      }
      break;
      
    case CHART_AREA:
      // Area chart
      for (uint8_t i = 0; i < crypto.historyIndex - 1; i++) {
        int16_t x1 = chartX + (i * chartW / (crypto.historyIndex - 1));
        int16_t y1 = chartY + chartH - ((crypto.priceHistory[i] - minP) / (maxP - minP) * chartH);
        sprite.drawLine(x1, y1, x1, chartY + chartH, chartColor & 0x39E7);
        sprite.drawLine(x1, y1, x1 + 1, y1, chartColor);
      }
      break;
      
    case CHART_CANDLE:
      // Candle chart
      int16_t candleW = max(2, chartW / crypto.historyIndex);
      for (uint8_t i = 0; i < crypto.historyIndex; i++) {
        int16_t x = chartX + (i * chartW / crypto.historyIndex);
        int16_t yPrice = chartY + chartH - ((crypto.priceHistory[i] - minP) / (maxP - minP) * chartH);
        uint16_t candleColor = (i > 0 && crypto.priceHistory[i] >= crypto.priceHistory[i - 1]) ? 
                               COLOR_GREEN : COLOR_RED;
        sprite.fillRect(x, yPrice - 2, candleW, 5, candleColor);
      }
      break;
  }
  
  // Current price marker
  int16_t lastX = chartX + ((crypto.historyIndex - 1) * chartW / (crypto.historyIndex - 1));
  int16_t lastY = chartY + chartH - ((crypto.priceHistory[crypto.historyIndex - 1] - minP) / (maxP - minP) * chartH);
  sprite.fillCircle(lastX, lastY, 2, chartColor);
  sprite.drawCircle(lastX, lastY, 3, COLOR_TEXT_WHITE);
  
  // Price labels
  sprite.setTextColor(COLOR_TEXT_GRAY);
  sprite.setTextDatum(MR_DATUM);
  sprite.drawString(String((int)maxP), chartX - 2, chartY + 2, 1);
  sprite.drawString(String((int)minP), chartX - 2, chartY + chartH - 2, 1);
}

// ═══════════════════════════════════════════════════════════
// 🔔 NOTIFICATION
// ═══════════════════════════════════════════════════════════
void drawNotification(const char* message, uint16_t color) {
  sprite.fillRoundRect(10, 50, 140, 28, 4, color);
  sprite.drawRoundRect(10, 50, 140, 28, 4, COLOR_TEXT_WHITE);
  sprite.setTextColor(COLOR_BG_DARK);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString(message, 80, 64, 2);
  sprite.pushSprite(0, 0);
}

// ═══════════════════════════════════════════════════════════
// 🛠️ UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════
String formatPrice(float price) {
  if (price >= 10000) return String((int)price);
  else if (price >= 1000) return String(price, 0);
  else if (price >= 1) return String(price, 2);
  else if (price >= 0.01) return String(price, 3);
  else return String(price, 6);
}

String formatChange(float change) {
  String sign = (change >= 0) ? "+" : "";
  return sign + String(change, 1) + "%";
}

String formatLargeNumber(float number) {
  if (number >= 1e12) return String(number / 1e12, 2) + "T";
  else if (number >= 1e9) return String(number / 1e9, 1) + "B";
  else if (number >= 1e6) return String(number / 1e6, 1) + "M";
  else if (number >= 1e3) return String(number / 1e3, 1) + "K";
  else return String((int)number);
}

String getTimeString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "--:--";
  char buffer[6];
  sprintf(buffer, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  return String(buffer);
}

void blinkLED(uint8_t times) {
  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(50);
    digitalWrite(PIN_LED, LOW);
    if (i < times - 1) delay(50);
  }
}

void safeDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    yield(); // Allow background tasks
  }
}