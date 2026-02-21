/*
 * Weather Station Firmware for ESP8266 + ST7735 Display
 * (c) Jasu-tech / MIT
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// --- CONFIGURATION ---
const char* ssid     = "xxx"; // Your wifi SSID
const char* password = "xxx"; // Your wifi password
const char* apiKey   = "xxx"; //Your OpenWeather API code
const char* city     = "xxx"; // Your sity/location

// Check interval in minutes (convert to ms)
const unsigned long interval = 2 * 60 * 1000;

// Pin Definitions for ESP8266
// CS = D1, A0(DC) = D2, RST = RST (hardware reset)
// SDA = D7 (GPIO13/MOSI), SCK = D5 (GPIO14/SCK)
#define TFT_CS     D1
#define TFT_RST    -1   // -1 = tied to ESP RST pin
#define TFT_DC     D2    // A0 on display module

// SDA(MOSI) = D7 (GPIO13), SCK = D5 (GPIO14) - ESP8266 hardware SPI
// LED/BL (backlight) = D8
#define TFT_LED    D8

// Thin wrapper to expose offset adjustment for Chinese ST7735 modules
class ST7735_Offset : public Adafruit_ST7735 {
  public:
    ST7735_Offset(int8_t cs, int8_t dc, int8_t rst)
      : Adafruit_ST7735(cs, dc, rst) {}
    void fixOffset(int8_t col, int8_t row) {
      setColRowStart(col, row);
    }
};

ST7735_Offset tft = ST7735_Offset(TFT_CS, TFT_DC, TFT_RST);

// Pixel offset adjustment for Chinese ST7735 modules
// Change these values if display content is shifted
int8_t colOffset = 0;
int8_t rowOffset = 0;

unsigned long lastCheckTime = 0;

// Forward declarations
void fetchWeather();
float getMoonPhase(long unixTime);
void drawMoonIcon(int cx, int cy, int r, float phase);
void centerText(const char* text, int y, int textSize);
void drawWeatherIcon(const char* weather, int cx, int cy);
void updateDisplay(float temp, float feelsLike, int humidity, float windSpeed, float moonPhase, const char* weather);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Weather Station starting...");

  // Turn on backlight
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);

  // Initialize SPI explicitly
  SPI.begin();
  delay(100);

  // Initialize Display
  Serial.println("Init display...");
  tft.initR(INITR_GREENTAB);
  delay(100);
  
  // Apply pixel offset for Chinese modules (adjust colOffset/rowOffset above if needed)
  tft.fixOffset(colOffset, rowOffset);
  tft.setRotation(0); // Portrait mode (128 wide x 160 tall)
  
  // Test pattern - fill screen RED to verify display works
  Serial.println("Drawing test pattern...");
  tft.fillScreen(ST7735_RED);
  delay(500);
  tft.fillScreen(ST7735_GREEN);
  delay(500);
  tft.fillScreen(ST7735_BLUE);
  delay(500);
  tft.fillScreen(ST7735_BLACK);
  
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("Display OK!");
  Serial.println("Display OK!");
  delay(500);
  tft.println("Connecting to WiFi...");

  // Connect to WiFi
  // Force WPA2 mode for WPA2/WPA3-Personal routers
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.setPhyMode(WIFI_PHY_MODE_11N);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 60) {
    delay(500);
    Serial.print(".");
    tft.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi FAILED!");
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(0, 0);
    tft.setTextColor(ST7735_RED);
    tft.println("WiFi FAILED!");
    tft.setTextColor(ST7735_WHITE);
    tft.println("Check SSID/password");
    tft.println("or try rebooting");
    while(1) { delay(1000); } // Halt
  }

  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(10, 10);
  tft.println("WiFi Connected!");
  delay(1000);
  
  fetchWeather();
}

void loop() {
  if (millis() - lastCheckTime >= interval) {
    fetchWeather();
  }
  yield();
  delay(100);
}

void fetchWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + String(city) + "&appid=" + String(apiKey) + "&units=metric";
    
    http.begin(client, url);
    int httpCode = http.GET();

    Serial.print("HTTP code: ");
    Serial.println(httpCode);

    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println(payload);
      
      // Parse JSON
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (error) {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        tft.fillScreen(ST7735_BLACK);
        tft.setCursor(0, 0);
        tft.setTextColor(ST7735_RED);
        tft.println("JSON Error!");
        return;
      }

      float temp = doc["main"]["temp"].as<float>();
      float feelsLike = doc["main"]["feels_like"].as<float>();
      int humidity = doc["main"]["humidity"].as<int>();
      float windSpeed = doc["wind"]["speed"].as<float>();
      long dt = doc["dt"].as<long>();
      String weather = doc["weather"][0]["main"].as<String>();

      Serial.print("Temp: "); Serial.println(temp);
      Serial.print("Feels like: "); Serial.println(feelsLike);
      Serial.print("Humidity: "); Serial.println(humidity);
      Serial.print("Wind: "); Serial.print(windSpeed); Serial.println(" m/s");
      Serial.print("Weather: "); Serial.println(weather);

      // Calculate moon phase (0.0 - 1.0)
      float moonPhase = getMoonPhase(dt);
      Serial.print("Moon phase: "); Serial.println(moonPhase);

      // Update Display
      updateDisplay(temp, feelsLike, humidity, windSpeed, moonPhase, weather.c_str());
    } else {
      Serial.print("HTTP Error: ");
      Serial.println(httpCode);
      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(0, 0);
      tft.setTextColor(ST7735_RED);
      tft.print("HTTP Error: ");
      tft.println(httpCode);
    }
    http.end();
  }
  lastCheckTime = millis();
}

// Calculate moon phase from unix timestamp
// Returns 0.0 to 1.0 (0=new moon, 0.25=first quarter, 0.5=full, 0.75=last quarter)
float getMoonPhase(long unixTime) {
  double days = (unixTime / 86400.0) - 10000.5;
  double lunations = days / 29.53058867;
  double phase = lunations - floor(lunations);
  return (float)phase;
}

// Draw moon phase icon
// cx,cy = center, r = radius
void drawMoonIcon(int cx, int cy, int r, float phase) {
  uint16_t moonColor = ST7735_YELLOW;
  uint16_t bg = ST7735_BLACK;

  if (phase < 0.03 || phase > 0.97) {
    // New moon - dark circle with outline
    tft.drawCircle(cx, cy, r, 0x4208);
  } else if (phase < 0.22) {
    // Waxing crescent - right sliver lit
    tft.drawCircle(cx, cy, r, 0x4208);
    for (int y2 = -r; y2 <= r; y2++) {
      int x2 = sqrt(r*r - y2*y2);
      int edge = x2 * (1.0 - phase * 4.0);
      if (edge < x2) tft.drawFastHLine(cx + edge, cy + y2, x2 - edge, moonColor);
    }
  } else if (phase < 0.28) {
    // First quarter - right half lit
    tft.fillCircle(cx, cy, r, bg);
    for (int y2 = -r; y2 <= r; y2++) {
      int x2 = sqrt(r*r - y2*y2);
      tft.drawFastHLine(cx, cy + y2, x2, moonColor);
    }
    tft.drawCircle(cx, cy, r, 0x4208);
  } else if (phase < 0.47) {
    // Waxing gibbous
    tft.fillCircle(cx, cy, r, moonColor);
    float cut = (phase - 0.25) * 4.0;
    for (int y2 = -r; y2 <= r; y2++) {
      int x2 = sqrt(r*r - y2*y2);
      int edge = x2 * (1.0 - cut);
      tft.drawFastHLine(cx - x2, cy + y2, x2 - edge, bg);
    }
  } else if (phase < 0.53) {
    // Full moon
    tft.fillCircle(cx, cy, r, moonColor);
  } else if (phase < 0.72) {
    // Waning gibbous
    tft.fillCircle(cx, cy, r, moonColor);
    float cut = (phase - 0.5) * 4.0;
    for (int y2 = -r; y2 <= r; y2++) {
      int x2 = sqrt(r*r - y2*y2);
      int edge = x2 * (1.0 - cut);
      tft.drawFastHLine(cx + edge, cy + y2, x2 - edge, bg);
    }
  } else if (phase < 0.78) {
    // Last quarter - left half lit
    tft.fillCircle(cx, cy, r, bg);
    for (int y2 = -r; y2 <= r; y2++) {
      int x2 = sqrt(r*r - y2*y2);
      tft.drawFastHLine(cx - x2, cy + y2, x2, moonColor);
    }
    tft.drawCircle(cx, cy, r, 0x4208);
  } else {
    // Waning crescent - left sliver lit
    tft.drawCircle(cx, cy, r, 0x4208);
    for (int y2 = -r; y2 <= r; y2++) {
      int x2 = sqrt(r*r - y2*y2);
      int edge = x2 * (1.0 - (1.0 - phase) * 4.0);
      if (edge < x2) tft.drawFastHLine(cx - x2, cy + y2, x2 - edge, moonColor);
    }
  }
}

// Helper: center text horizontally on 128px wide screen
void centerText(const char* text, int y, int textSize) {
  int16_t len = strlen(text);
  int16_t pixelWidth = len * 6 * textSize;
  int16_t x = (128 - pixelWidth) / 2;
  if (x < 0) x = 0;
  tft.setTextSize(textSize);
  tft.setCursor(x, y);
  tft.print(text);
}

// --- Weather Icons (drawn with GFX primitives) ---

// Sun icon: yellow circle with rays
void drawSunIcon(int cx, int cy, int r) {
  tft.fillCircle(cx, cy, r, ST7735_YELLOW);
  for (int a = 0; a < 360; a += 45) {
    float rad = a * 3.14159 / 180.0;
    int x1 = cx + cos(rad) * (r + 3);
    int y1 = cy + sin(rad) * (r + 3);
    int x2 = cx + cos(rad) * (r + 7);
    int y2 = cy + sin(rad) * (r + 7);
    tft.drawLine(x1, y1, x2, y2, ST7735_YELLOW);
  }
}

// Cloud icon: overlapping circles
void drawCloudIcon(int cx, int cy) {
  uint16_t c = 0xC618;
  tft.fillCircle(cx - 8, cy, 6, c);
  tft.fillCircle(cx, cy - 4, 8, c);
  tft.fillCircle(cx + 8, cy, 6, c);
  tft.fillRoundRect(cx - 14, cy, 28, 8, 3, c);
}

// Rain icon: cloud + rain drops
void drawRainIcon(int cx, int cy) {
  drawCloudIcon(cx, cy - 4);
  uint16_t blue = 0x04DF;
  tft.fillCircle(cx - 6, cy + 10, 1, blue);
  tft.drawLine(cx - 6, cy + 10, cx - 8, cy + 14, blue);
  tft.fillCircle(cx, cy + 12, 1, blue);
  tft.drawLine(cx, cy + 12, cx - 2, cy + 16, blue);
  tft.fillCircle(cx + 6, cy + 10, 1, blue);
  tft.drawLine(cx + 6, cy + 10, cx + 4, cy + 14, blue);
}

// Drizzle icon: cloud + small dots
void drawDrizzleIcon(int cx, int cy) {
  drawCloudIcon(cx, cy - 4);
  uint16_t blue = 0x04DF;
  tft.fillCircle(cx - 6, cy + 11, 1, blue);
  tft.fillCircle(cx, cy + 13, 1, blue);
  tft.fillCircle(cx + 6, cy + 11, 1, blue);
}

// Snow icon: cloud + snowflakes (small asterisks)
void drawSnowIcon(int cx, int cy) {
  drawCloudIcon(cx, cy - 4);
  uint16_t w = ST7735_WHITE;
  for (int i = -1; i <= 1; i++) {
    int sx = cx + i * 10;
    int sy = cy + 12;
    tft.drawLine(sx - 2, sy, sx + 2, sy, w);
    tft.drawLine(sx, sy - 2, sx, sy + 2, w);
    tft.drawPixel(sx - 1, sy - 1, w);
    tft.drawPixel(sx + 1, sy + 1, w);
    tft.drawPixel(sx + 1, sy - 1, w);
    tft.drawPixel(sx - 1, sy + 1, w);
  }
}

// Thunderstorm icon: cloud + lightning bolt
void drawThunderIcon(int cx, int cy) {
  drawCloudIcon(cx, cy - 4);
  uint16_t y = ST7735_YELLOW;
  tft.fillTriangle(cx - 2, cy + 6, cx + 4, cy + 6, cx, cy + 12, y);
  tft.fillTriangle(cx - 4, cy + 11, cx + 2, cy + 11, cx - 2, cy + 17, y);
}

// Mist/Fog icon: horizontal lines
void drawMistIcon(int cx, int cy) {
  uint16_t c = 0x8410;
  for (int i = -2; i <= 2; i++) {
    int w = 20 - abs(i) * 3;
    tft.drawFastHLine(cx - w/2, cy + i * 4, w, c);
  }
}

// Draw weather icon based on condition string
void drawWeatherIcon(const char* weather, int cx, int cy) {
  if (strcmp(weather, "Clear") == 0) {
    drawSunIcon(cx, cy, 8);
  } else if (strcmp(weather, "Clouds") == 0) {
    drawCloudIcon(cx, cy);
  } else if (strcmp(weather, "Rain") == 0) {
    drawRainIcon(cx, cy);
  } else if (strcmp(weather, "Drizzle") == 0) {
    drawDrizzleIcon(cx, cy);
  } else if (strcmp(weather, "Snow") == 0) {
    drawSnowIcon(cx, cy);
  } else if (strcmp(weather, "Thunderstorm") == 0) {
    drawThunderIcon(cx, cy);
  } else if (strcmp(weather, "Mist") == 0 || strcmp(weather, "Fog") == 0 || strcmp(weather, "Haze") == 0) {
    drawMistIcon(cx, cy);
  } else {
    drawCloudIcon(cx, cy);
  }
}

void updateDisplay(float temp, float feelsLike, int humidity, float windSpeed, float moonPhase, const char* weather) {
  tft.fillScreen(ST7735_BLACK);
  
  // Screen: 128 x 160 pixels (portrait)

  // Header - city name left-aligned + WiFi icon top-right
  tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 6);
  tft.print(city);
  // Moon phase icon top-right (left of WiFi icon)
  drawMoonIcon(100, 8, 6, moonPhase);

  // WiFi icon top-right (drawn as arcs using quarter pixels)
  if (WiFi.status() == WL_CONNECTED) {
    uint16_t wc = ST7735_GREEN;
    int wx = 118, wy = 14;
    tft.fillCircle(wx, wy, 1, wc);
    // Arc 1
    for (int a = 200; a <= 340; a += 5) {
      float r1 = a * 3.14159 / 180.0;
      tft.drawPixel(wx + cos(r1)*4, wy + sin(r1)*4, wc);
    }
    // Arc 2
    for (int a = 200; a <= 340; a += 4) {
      float r2 = a * 3.14159 / 180.0;
      tft.drawPixel(wx + cos(r2)*7, wy + sin(r2)*7, wc);
    }
    // Arc 3
    for (int a = 200; a <= 340; a += 3) {
      float r3 = a * 3.14159 / 180.0;
      tft.drawPixel(wx + cos(r3)*10, wy + sin(r3)*10, wc);
    }
  }
  tft.drawFastHLine(0, 18, 128, ST7735_BLUE);

  // Weather icon centered
  drawWeatherIcon(weather, 64, 38);

  // Temperature - large, centered with degree symbol using getTextBounds
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(3);
  char tempStr[16];
  dtostrf(temp, 0, 1, tempStr);
  char tempFull[20];
  snprintf(tempFull, sizeof(tempFull), "%s C", tempStr);
  int16_t tbx, tby; uint16_t tbw, tbh;
  tft.getTextBounds(tempFull, 0, 0, &tbx, &tby, &tbw, &tbh);
  int16_t tX = (128 - (int16_t)tbw) / 2;
  if (tX < 0) tX = 0;
  tft.setCursor(tX, 58);
  tft.print(tempStr);
  int16_t dx = tft.getCursorX();
  tft.drawCircle(dx + 3, 60, 3, ST7735_WHITE);
  tft.setCursor(dx + 8, 58);
  tft.print("C");

  // Divider
  tft.drawFastHLine(4, 84, 120, ST7735_BLUE);

  // Feels like - centered with degree symbol using getTextBounds
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  centerText("Feels like", 90, 1);
  tft.setTextSize(2);
  char feelsStr[16];
  dtostrf(feelsLike, 0, 1, feelsStr);
  char feelsFull[20];
  snprintf(feelsFull, sizeof(feelsFull), "%s C", feelsStr);
  int16_t fbx, fby; uint16_t fbw, fbh;
  tft.getTextBounds(feelsFull, 0, 0, &fbx, &fby, &fbw, &fbh);
  int16_t fX = (128 - (int16_t)fbw) / 2;
  if (fX < 0) fX = 0;
  tft.setCursor(fX, 104);
  tft.print(feelsStr);
  int16_t fx = tft.getCursorX();
  tft.drawCircle(fx + 2, 105, 2, 0xC618);
  tft.setCursor(fx + 6, 104);
  tft.print("C");

  // Divider
  tft.drawFastHLine(4, 122, 120, ST7735_BLUE);

  // Humidity (left) and Wind (right) side by side
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(6, 126);
  tft.print("Humidity");
  tft.setCursor(78, 126);
  tft.print("Wind");
  tft.setTextSize(2);
  char humStr[8];
  snprintf(humStr, sizeof(humStr), "%d%%", humidity);
  tft.setCursor(6, 140);
  tft.print(humStr);
  char windStr[10];
  dtostrf(windSpeed, 0, 1, windStr);
  tft.setCursor(72, 140);
  tft.print(windStr);
  tft.setTextSize(1);
  tft.setCursor(tft.getCursorX() + 1, 146);
  tft.print("m/s");
}