# ESP8266 Weather Station (c) Jasu-tech / MIT 2026

A full-stack web application for building and managing ESP8266-based weather stations with ST7735 TFT displays.
![TFT](https://github.com/user-attachments/assets/711a9ad6-f8d5-4974-a091-fc0ada5409b6)

## Features

### Web Dashboard
- Add and monitor weather for multiple cities in real-time
- Displays temperature, humidity, weather conditions via OpenWeatherMap API
- Auto-refreshes weather data every 10 minutes
- Dark-themed interface with smooth animations

### Firmware Generator
- Generates ready-to-compile C++ Arduino code for ESP8266 + ST7735 TFT
- Configurable WiFi credentials, city, update interval, and pin assignments
- Support for multiple ST7735 display types (BLACKTAB, GREENTAB, REDTAB, etc.)
- Pixel offset adjustment for Chinese ST7735 modules
- Copy-to-clipboard for easy transfer to Arduino IDE

### Display Features (on ESP8266 hardware)
- Weather condition icons drawn with GFX primitives:
  - Sun (Clear), Cloud, Rain, Drizzle, Snow, Thunderstorm, Mist/Fog
- Moon phase indicator calculated from timestamp
- Temperature and "Feels like" with degree symbol
- Humidity and wind speed (m/s) side by side
- WiFi status icon
- All elements centered on 128x160 portrait display

### Icon Previews
- Weather icons and moon phase previews rendered on the web page
- Shows exactly how icons will appear on the physical display

## Hardware Requirements

- **Microcontroller**: ESP8266 (NodeMCU, Wemos D1 Mini, etc.)
- **Display**: ST7735 1.8" TFT 128x160 (Chinese red module recommended)
- **Wiring** (default pins):

| Display Pin | ESP8266 Pin | Description |
|-------------|-------------|-------------|
| CS          | D1          | Chip Select |
| DC / A0     | D2          | Data/Command |
| RST         | -1          | Hardware reset (tied to ESP RST) |
| SDA / MOSI  | D7 (GPIO13) | SPI Data  |
| SCK         | D5 (GPIO14) | SPI Clock |
| LED / BL    | D8          | Backlight |
| VCC         | 3.3V        | Power     |
| GND         | GND         | Ground    |

## Arduino IDE Setup

### Required Libraries
Install these via Arduino IDE Library Manager:

1. **Adafruit ST7735 and ST7789 Library** (by Adafruit)
2. **Adafruit GFX Library** (by Adafruit)
3. **ArduinoJson** (by Benoit Blanchon) - version 6.x
4. **ESP8266WiFi** (included with ESP8266 board package)
5. **ESP8266HTTPClient** (included with ESP8266 board package)

### Board Setup
1. Open Arduino IDE
2. Go to File > Preferences
3. Add to "Additional Board Manager URLs":
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
4. Go to Tools > Board > Board Manager
5. Search "ESP8266" and install **esp8266 by ESP8266 Community**
6. Select your board: Tools > Board > ESP8266 Boards > NodeMCU 1.0

### Upload Firmware
1. Open the web app and go to "Firmware Generator"
2. Enter your WiFi SSID and password
3. Enter your OpenWeatherMap API key (free from openweathermap.org)
4. Set your city name
5. Adjust pins if your wiring differs from defaults
6. Click "Copy Code"
7. Paste into Arduino IDE
8. Connect ESP8266 via USB
9. Click Upload

## Troubleshooting

### Display stays black
- Try a different Display Init Type (BLACKTAB, GREENTAB, REDTAB)
- Check wiring, especially CS, DC, and SDA/SCK connections
- Verify backlight pin (D8) is connected to LED on the display

### Display image is shifted
- Adjust `colOffset` and `rowOffset` values in the generated code (near the top)
- Common values for Chinese modules: `colOffset = 2`, `rowOffset = 1`

### WiFi won't connect
- The firmware forces WPA2 mode for compatibility with WPA2/WPA3 routers
- Check SSID and password (case-sensitive)
- Ensure router is broadcasting on 2.4 GHz (ESP8266 doesn't support 5 GHz)

### Serial Monitor shows garbage text
- Set Serial Monitor baud rate to **115200**
- If you see readable text at **74880 baud**, the ESP is crash-looping

### ESP crashes or reboots
- The firmware uses `DynamicJsonDocument(1024)` to save stack memory
- `yield()` and `delay(100)` in loop prevent watchdog resets
- Ensure stable 3.3V power supply to ESP8266

## API Key

Get a free OpenWeatherMap API key at:
https://openweathermap.org/api

The free tier allows 1,000 API calls per day, which is more than enough for a 2-minute update interval.

## Tech Stack

### Web Application
- **Frontend**: React, TypeScript, Tailwind CSS, Shadcn/ui, Framer Motion
- **Backend**: Node.js, Express, TypeScript
- **Database**: PostgreSQL with Drizzle ORM
- **API**: OpenWeatherMap (proxied through backend)

### Firmware
- **Platform**: ESP8266 (Arduino framework)
- **Display Library**: Adafruit GFX + Adafruit ST7735
- **JSON Parsing**: ArduinoJson 6.x
- **WiFi**: ESP8266WiFi with WPA2 forced mode

## License

MIT
