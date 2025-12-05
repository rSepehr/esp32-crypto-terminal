# 🚀 Quick Start Guide

Get your Crypto Terminal Pro up and running in 15 minutes!

## ⚡ What You Need

### Hardware Checklist
- [ ] ESP32-WROOM-32 board
- [ ] 1.8" TFT Display (ST7735, 128x160)
- [ ] 3 push buttons
- [ ] 1 LED
- [ ] 1x 220Ω resistor (for LED)
- [ ] Breadboard and jumper wires
- [ ] USB cable

### Software Checklist
- [ ] Arduino IDE installed
- [ ] ESP32 board support added
- [ ] TFT_eSPI library installed
- [ ] ArduinoJson library installed

## 📝 Step-by-Step Setup

### Step 1: Install Arduino IDE (5 min)

1. Download from [arduino.cc](https://www.arduino.cc/en/software)
2. Install for your operating system
3. Launch Arduino IDE

### Step 2: Add ESP32 Support (3 min)

1. Open Arduino IDE
2. Go to `File` → `Preferences`
3. In "Additional Board Manager URLs", paste:
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```
4. Click OK
5. Go to `Tools` → `Board` → `Boards Manager`
6. Search for "ESP32"
7. Install "ESP32 by Espressif Systems"

### Step 3: Install Libraries (2 min)

1. Go to `Sketch` → `Include Library` → `Manage Libraries`
2. Search and install:
   - **TFT_eSPI** (by Bodmer) - version 2.5.0 or newer
   - **ArduinoJson** (by Benoît Blanchon) - version 6.21.0 or newer

### Step 4: Configure TFT_eSPI (3 min)

1. Find your Arduino libraries folder:
   - **Windows**: `Documents\Arduino\libraries\`
   - **Mac**: `~/Documents/Arduino/libraries/`
   - **Linux**: `~/Arduino/libraries/`

2. Navigate to `TFT_eSPI` folder

3. Open `User_Setup.h` in a text editor

4. Find and uncomment (remove `//` from):
   ```cpp
   #define ST7735_DRIVER
   ```

5. Set display size:
   ```cpp
   #define TFT_WIDTH  128
   #define TFT_HEIGHT 160
   ```

6. Set ESP32 pins:
   ```cpp
   #define TFT_MOSI 23
   #define TFT_SCLK 18
   #define TFT_CS   2
   #define TFT_DC   15
   #define TFT_RST  4
   ```

7. Save and close

### Step 5: Wire the Hardware (5 min)

#### Display Connections
```
ESP32          TFT Display
━━━━━━━━━━━━━━━━━━━━━━━━━
GPIO 23   →    MOSI
GPIO 18   →    SCK
GPIO 2    →    CS
GPIO 15   →    DC
GPIO 4    →    RST
3.3V      →    VCC
GND       →    GND
3.3V      →    LED (backlight)
```

#### Button Connections
```
ESP32          Button
━━━━━━━━━━━━━━━━━━━━━━
GPIO 12   →    SCROLL (to GND)
GPIO 14   →    SELECT (to GND)
GPIO 27   →    MODE (to GND)
```

#### LED Connection
```
ESP32 GPIO 4 → [220Ω resistor] → LED (+) → GND
```

### Step 6: Download & Configure Code (2 min)

1. Download the code:
   ```bash
   git clone https://github.com/rSepehr/esp32-crypto-terminal.git
   ```
   
   Or download ZIP from GitHub and extract

2. Open `esp32-crypto-terminal.ino` in Arduino IDE

3. Edit WiFi credentials (around line 29):
   ```cpp
   const char* WIFI_SSID = "YourWiFiName";
   const char* WIFI_PASSWORD = "YourPassword";
   ```

4. **IMPORTANT**: Use 2.4GHz WiFi (ESP32 doesn't support 5GHz)

### Step 7: Upload to ESP32 (2 min)

1. Connect ESP32 to computer via USB

2. In Arduino IDE:
   - `Tools` → `Board` → `ESP32 Dev Module`
   - `Tools` → `Port` → Select your ESP32 port
     - **Windows**: Usually `COM3`, `COM4`, etc.
     - **Mac/Linux**: Usually `/dev/ttyUSB0` or `/dev/cu.usbserial-*`

3. Click the Upload button (→)

4. Wait for "Done uploading" message

### Step 8: First Run! 🎉

1. Open Serial Monitor (`Tools` → `Serial Monitor`)
2. Set baud rate to `115200`
3. Reset ESP32 (press RST button)
4. You should see:
   ```
   ╔════════════════════════════════════╗
   ║  CRYPTO TERMINAL PRO - v3.0        ║
   ╚════════════════════════════════════╝
   
   ✓ Hardware initialized
   ✓ Display initialized
   → Connecting to WiFi...
   ✓ WiFi connected
   ✓ Data loaded
   ✓ System ready
   ```

5. Watch the display come to life!

## 🎮 Using Your Terminal

### Button Functions

| Button | Function |
|--------|----------|
| **SCROLL** | Navigate through lists |
| **SELECT** | View details / Change chart |
| **MODE** | Switch between views |

### View Modes

Press **MODE** to cycle through:
1. **Market** - Live prices and mini charts
2. **Portfolio** - Your holdings and P&L
3. **Heatmap** - Color-coded market overview
4. **Trends** - Top gainers and losers

In **Detail** view, press **SELECT** to change chart type:
- Line Chart
- Area Chart
- Candle Chart

## 🔧 Troubleshooting Quick Fixes

### Display Not Working?
1. Check all connections
2. Verify `User_Setup.h` configuration
3. Try different `TFT_RGB_ORDER` setting (RGB vs BGR)

### WiFi Won't Connect?
1. Use 2.4GHz WiFi only
2. Check password (case-sensitive)
3. Move closer to router
4. Check serial monitor for error details

### No Data Loading?
1. Verify internet connection
2. Wait 30 seconds for first update
3. Check serial monitor for API errors

### Buttons Not Responding?
1. Verify button wiring to correct GPIO pins
2. Check buttons connect to GND when pressed
3. Look for loose connections

## 📚 Next Steps

Now that your terminal is running:

1. **Customize Portfolio**: Edit the portfolio array to track your real holdings
2. **Adjust Update Interval**: Change how often prices update
3. **Add More Coins**: Modify the cryptos array
4. **Read Full Documentation**: Check out the complete README

## 🆘 Need Help?

- 📖 Read the [Full Documentation](README.md)
- 🐛 [Report an Issue](https://github.com/rSepehr/esp32-crypto-terminal/issues)
- 💬 [Ask in Discussions](https://github.com/rSepehr/esp32-crypto-terminal/discussions)
- 📧 Email: your.email@example.com

## 🎉 Success!

Congratulations! You now have a professional crypto monitoring terminal. 

**Star the repo** if you found this helpful! ⭐

---

**Total Time**: ~15 minutes  
**Difficulty**: Beginner-Friendly  
**Cost**: ~$15-20 in parts

Happy monitoring! 🚀