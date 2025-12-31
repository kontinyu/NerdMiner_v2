
#include "displayDriver.h"

#ifdef OLED_042_DISPLAY

#include <U8g2lib.h>
#include "monitor.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#ifdef USE_LED
#include <FastLED.h>
#define MAX_BRIGHTNESS 16

CRGB leds(0, 0, 0);

#ifndef RGB_LED_CLASS
#define RGB_LED_CLASS WS2812B
#endif
#ifndef RGB_LED_ORDER
#define RGB_LED_ORDER GRB
#endif

#endif // USE_LED

bool oledLedOn = true;
static bool oledPowerOn = true;
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
// API data
static double apiDifficultyRaw = 0;
static bool apiDataFetched = false;
static unsigned long lastApiCall = 0;
static String minerMotivation = "";

// Format total hash attempts in human-readable form
String formatTotalAttempts(uint64_t totalAttempts) {
  if (totalAttempts >= 1e15) {
    return String((long)(totalAttempts / 1e15)) + "Q";
  } else if (totalAttempts >= 1e12) {
    return String((long)(totalAttempts / 1e12)) + "T";
  } else if (totalAttempts >= 1e9) {
    return String((long)(totalAttempts / 1e9)) + "B";
  } else if (totalAttempts >= 1e6) {
    return String((long)(totalAttempts / 1e6)) + "M";
  } else if (totalAttempts >= 1e3) {
    return String((long)(totalAttempts / 1e3)) + "K";
  } else {
    return String((long)totalAttempts);
  }
}

// Format large numbers for display (1 in X trillion/billion/million)
String formatLuckOdds(double odds) {
  if (odds >= 1e21) {
    double sextillions = odds / 1e21;
    return "1 in " + String((long)sextillions) + "Sx";
  } else if (odds >= 1e18) {
    double quintillions = odds / 1e18;
    return "1 in " + String((long)quintillions) + "Qi";
  } else if (odds >= 1e15) {
    double quadrillions = odds / 1e15;
    return "1 in " + String((long)quadrillions) + "Q";
  } else if (odds >= 1e12) {
    double trillions = odds / 1e12;
    return "1 in " + String((long)trillions) + "T";
  } else if (odds >= 1e9) {
    double billions = odds / 1e9;
    return "1 in " + String((long)billions) + "B";
  } else if (odds >= 1e6) {
    double millions = odds / 1e6;
    return "1 in " + String((long)millions) + "M";
  } else {
    return "1 in " + String((long)odds);
  }
}

// Fetch API data from kontinyu server
void fetchKontinyuAPI() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  // Only fetch once per 30 minutes
  if (millis() - lastApiCall < 30 * 60 * 1000 && apiDataFetched) return;
  
  HTTPClient http;
  http.setTimeout(5000);
  http.begin("http://miner.kontinyu.com/api.php");
  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      if (doc.containsKey("difficultyRaw")) {
        apiDifficultyRaw = doc["difficultyRaw"].as<double>();
        apiDataFetched = true;
        lastApiCall = millis();
      }
      if (doc.containsKey("minermotivation")) {
        minerMotivation = doc["minermotivation"].as<String>();
      }
    }
    doc.clear();
  }
  http.end();
}


static uint8_t btc_icon[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xF0, 0x03, 0x00, 0x00, 0xFE, 0x1F, 0x00, 0x00, 0xFF, 0x3F, 0x00, 
  0x80, 0xFF, 0x7F, 0x00, 0xC0, 0xFF, 0xFF, 0x00, 0xE0, 0x3F, 0xFF, 0x01, 
  0xF0, 0x3F, 0xFF, 0x03, 0xF0, 0x0F, 0xFC, 0x03, 0xF0, 0x0F, 0xF8, 0x03, 
  0xF8, 0xCF, 0xF9, 0x07, 0xF8, 0xCF, 0xF9, 0x07, 0xF8, 0x0F, 0xFC, 0x07, 
  0xF8, 0x0F, 0xF8, 0x07, 0xF8, 0xCF, 0xF9, 0x07, 0xF8, 0xCF, 0xF9, 0x07, 
  0xF0, 0x0F, 0xF8, 0x03, 0xF0, 0x0F, 0xFC, 0x03, 0xF0, 0x3F, 0xFF, 0x03, 
  0xE0, 0x3F, 0xFF, 0x01, 0xC0, 0xFF, 0xFF, 0x00, 0x80, 0xFF, 0x7F, 0x00, 
  0x00, 0xFF, 0x3F, 0x00, 0x00, 0xFE, 0x1F, 0x00, 0x00, 0xF0, 0x03, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

static uint8_t setup_icon[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x01, 0x00, 0x00, 0xE0, 0x01, 0x00, 
  0x00, 0xF0, 0x03, 0x00, 0xC0, 0xF0, 0xC3, 0x00, 0xE0, 0xF9, 0xE3, 0x01, 
  0xF0, 0xFF, 0xFF, 0x03, 0xF0, 0xFF, 0xFF, 0x03, 0xE0, 0xFF, 0xFF, 0x01, 
  0xC0, 0xFF, 0xFF, 0x00, 0xC0, 0xFF, 0xFF, 0x00, 0xE0, 0x1F, 0xFE, 0x00, 
  0xF8, 0x0F, 0xFC, 0x07, 0xFE, 0x07, 0xF8, 0x1F, 0xFE, 0x07, 0xF8, 0x1F, 
  0xFE, 0x07, 0xF8, 0x1F, 0xFE, 0x07, 0xF8, 0x1F, 0xF8, 0x0F, 0xFC, 0x07, 
  0xE0, 0x1F, 0xFE, 0x00, 0xC0, 0xFF, 0xFF, 0x00, 0xC0, 0xFF, 0xFF, 0x00, 
  0xE0, 0xFF, 0xFF, 0x01, 0xF0, 0xFF, 0xFF, 0x03, 0xF0, 0xFF, 0xFF, 0x03, 
  0xE0, 0xF9, 0xE3, 0x01, 0xC0, 0xF0, 0xC3, 0x00, 0x00, 0xF0, 0x03, 0x00, 
  0x00, 0xE0, 0x01, 0x00, 0x00, 0xE0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

extern monitor_data mMonitor;

void clearScreen(void) {
    u8g2.clearBuffer();
    u8g2.sendBuffer();
}

void serialPrint(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);

  // Print hashrate to serial
  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Print extended data to serial for no display devices
  Serial.printf(">>> Valid blocks: %s\n", data.valids.c_str());
  Serial.printf(">>> Block templates: %s\n", data.templates.c_str());
  Serial.printf(">>> Best difficulty: %s\n", data.bestDiff.c_str());
  Serial.printf(">>> 32Bit shares: %s\n", data.completedShares.c_str());
  Serial.printf(">>> Temperature: %s\n", data.temp.c_str());
  Serial.printf(">>> Total MHashes: %s\n", data.totalMHashes.c_str());
  Serial.printf(">>> Time mining: %s\n", data.timeMining.c_str());
}

void oledDisplay_Init(void)
{
  Serial.println("OLED 0.42 display driver initialized");
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  u8g2.clear();

  u8g2.setFlipMode(1);
  clearScreen();


  #ifdef USE_LED
  // Initialize single RGB LED (same pattern as other display drivers)
  FastLED.addLeds<RGB_LED_CLASS, RGB_LED_PIN, RGB_LED_ORDER>(&leds, 1);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.show();
  #endif

}

void oledDisplay_AlternateScreenState(void)
{
  Serial.println("Switching display state");

#ifdef USE_LED
  // toggle led display state when user switches screens
  oledLedOn = !oledLedOn;
  if (!oledLedOn) FastLED.clear(true);
  
#endif
  oledPowerOn = !oledPowerOn;
  u8g2.setPowerSave(!oledPowerOn);
}

void oledDisplay_AlternateRotation(void)
{
}

void oledDisplay_Screen1(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  u8g2.clearBuffer();
  // Use inversion state set during screen switch
  extern bool currentScreenInversion;
  bool invertScreen = currentScreenInversion;
  
  
  u8g2.setDrawColor(invertScreen ? 0 : 1);
  
  // Fill screen if inverted
  if (invertScreen) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 128, 64);
    u8g2.setDrawColor(0);
  }
  
  // Draw border line at bottom of yellow region (y=15, full width)
  u8g2.drawLine(0, 15, 127, 15);
  
  // "Qminer" title in top left (yellow region, line 0-15)
  u8g2.setFont(u8g2_font_mercutio_basic_nbp_tf);
  u8g2.drawStr(2, 12, "Qminer");
  
  // KH/s label - top right (same font as Qminer)
  u8g2.setFont(u8g2_font_helvB10_tf);
  u8g2.drawStr(92, 13, "KH/s");
  
  // Main hashrate value - seven-segment LCD style font, moved 20 pixels higher
  u8g2.setFont(u8g2_font_7_Seg_41x21_mn); // Classic digital display look
  u8g2.drawStr(5, 20, data.currentHashRate.c_str());
  
  u8g2.sendBuffer();

  serialPrint(mElapsed);
}

void oledDisplay_Screen2(unsigned long mElapsed)
{
   mining_data data = getMiningData(mElapsed);

  // Use inversion state set during screen switch
  extern bool currentScreenInversion;
  bool invertScreen = currentScreenInversion;
  
  u8g2.clearBuffer();
  u8g2.setDrawColor(invertScreen ? 0 : 1);
  
  // Fill screen if inverted
  if (invertScreen) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 128, 64);
    u8g2.setDrawColor(0);
  }
  
  // Draw border line at bottom of yellow region (y=15, full width)
  u8g2.drawLine(0, 15, 127, 15);
  
  // "Qminer" text in top left (yellow region, line 0-15) - Habsburg Chancery font
  u8g2.setFont(u8g2_font_mercutio_basic_nbp_tf);
  u8g2.drawStr(2, 12, "Qminer");
  
  // KH/s value (number only) - top right (same size as original)
  u8g2.setFont(u8g2_font_helvB10_tn);
  u8g2.drawStr(90, 13, data.currentHashRate.c_str());
  
  // Current time - seven-segment LCD style font, centered in blue region
  // Screen width=128px, time string width ~60px, centered: (128-60)/2 = 34px
  u8g2.setFont(u8g2_font_7_Seg_41x21_mn);
  u8g2.drawStr(14, 20, data.currentTime.c_str());
  
  u8g2.sendBuffer();

  serialPrint(mElapsed);
}

void oledDisplay_Screen3(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  
  // Fetch API data once when screen loads
  static bool screen3Loaded = false;
  if (!screen3Loaded) {
    fetchKontinyuAPI();
    screen3Loaded = true;
  }

  // Use inversion state set during screen switch
  extern bool currentScreenInversion;
  bool invertScreen = currentScreenInversion;
  
  u8g2.clearBuffer();
  u8g2.setDrawColor(invertScreen ? 0 : 1);
  
  // Fill screen if inverted
  if (invertScreen) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 128, 64);
    u8g2.setDrawColor(0);
  }
  
  // Draw border line at bottom of yellow region (y=15, full width)
  u8g2.drawLine(0, 15, 127, 15);
  
  // "Qminer" title in top left (yellow region, line 0-15)
  u8g2.setFont(u8g2_font_mercutio_basic_nbp_tf);
  u8g2.drawStr(2, 12, "Qminer");
  
  // Temperature - top right (same font/position as KH/s in screen 1)
  u8g2.setFont(u8g2_font_helvB10_tf);
  u8g2.drawStr(103, 13, data.temp.c_str());
  // Draw degree symbol as small circle
  int tempWidth = u8g2.getStrWidth(data.temp.c_str());
  u8g2.drawCircle(103 + tempWidth + 3, 5, 2);
  
   // Luck calculation - show odds of finding block with best diff
  String luckNumber = "";
  if (apiDataFetched && apiDifficultyRaw > 0) {
    // Parse bestDiff to get numeric value
    String bestDiffStr = data.bestDiff;
    double bestDiff = bestDiffStr.toDouble();
    if (bestDiff > 0) {
      double odds = apiDifficultyRaw / bestDiff;
      luckNumber = formatLuckOdds(odds);
      // Remove "1 in " prefix from the formatted string
      luckNumber.replace("1 in ", "");
    } else {
      luckNumber = "--";
    }
  } else {
    luckNumber = "--";
  }
  
  // Display luck in two centered lines with larger font
  u8g2.setFont(u8g2_font_helvB12_tf);
  
  // Line 1: "1 in" - centered
  int line1Width = u8g2.getStrWidth("1 in");
  int line1X = (128 - line1Width) / 2;
  u8g2.drawStr(line1X, 31, "1 in");
   u8g2.setFont(u8g2_font_helvB18_tf);
  // Line 2: luck number - centered
  int line2Width = u8g2.getStrWidth(luckNumber.c_str());
  int line2X = (128 - line2Width) / 2;
  u8g2.drawStr(line2X, 56, luckNumber.c_str());
  
  u8g2.sendBuffer();

  serialPrint(mElapsed);
}

void oledDisplay_Screen4(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  // Use inversion state set during screen switch
  extern bool currentScreenInversion;
  bool invertScreen = currentScreenInversion;
  
  u8g2.clearBuffer();
  u8g2.setDrawColor(invertScreen ? 0 : 1);
  
  // Fill screen if inverted
  if (invertScreen) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 128, 64);
    u8g2.setDrawColor(0);
  }
  
  // Draw border line at bottom of yellow region (y=15, full width)
  u8g2.drawLine(0, 15, 127, 15);
  
  // "Qminer" title in top left (yellow region, line 0-15)
  u8g2.setFont(u8g2_font_mercutio_basic_nbp_tf);
  u8g2.drawStr(2, 12, "Qminer");
  
  // Calculate total attempts from Mhashes
  uint64_t totalAttempts = (uint64_t)data.totalMHashes.toInt() * 1000000ULL;
  String attemptsNum = formatTotalAttempts(totalAttempts);

  // Total attempts - top right, right-aligned with smaller "Tries" text
  u8g2.setFont(u8g2_font_8x13_tf);
  int numWidth = u8g2.getStrWidth(attemptsNum.c_str());
  u8g2.setFont(u8g2_font_6x10_tf);
  int triesWidth = u8g2.getStrWidth(" Tries");
  int totalWidth = numWidth + triesWidth;
  int startX = 128 - totalWidth - 2; // 2px margin from right edge

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.drawStr(startX, 13, attemptsNum.c_str());
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(startX + numWidth, 13, " Tries");
  
  // Parse uptime string - format is "D  HH:MM:SS" (two spaces separator)
  String uptime = data.timeMining;
  String daysText = "";
  String timeText = "";
  
  int doubleSpacePos = uptime.indexOf("  ");
  if (doubleSpacePos > 0) {
    // Split at double space
    daysText = uptime.substring(0, doubleSpacePos) + " days";
    timeText = uptime.substring(doubleSpacePos + 2); // Skip both spaces
  }
  
  // Display uptime in large font, centered, two lines
  u8g2.setFont(u8g2_font_helvB12_tf);
  
  // Line 1: Days - centered
  int line1Width = u8g2.getStrWidth(daysText.c_str());
  int line1X = (128 - line1Width) / 2;
  u8g2.drawStr(line1X, 36, daysText.c_str());
  
  // Line 2: Time - centered
  int line2Width = u8g2.getStrWidth(timeText.c_str());
  int line2X = (128 - line2Width) / 2;
  u8g2.drawStr(line2X, 54, timeText.c_str());
  
  u8g2.sendBuffer();

  serialPrint(mElapsed);
}

void oledDisplay_Screen5(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  
  // Fetch API data once when screen loads
  static bool screen5Loaded = false;
  if (!screen5Loaded) {
    fetchKontinyuAPI();
    screen5Loaded = true;
  }

  // Scrolling state
  static int scrollOffset = 0;
  static unsigned long lastScrollTime = 0;
  const int scrollSpeed = 100; // milliseconds per pixel scroll (matches screen refresh rate)

  // Use inversion state set during screen switch
  extern bool currentScreenInversion;
  bool invertScreen = currentScreenInversion;
  
  u8g2.clearBuffer();
  u8g2.setDrawColor(invertScreen ? 0 : 1);
  
  // Fill screen if inverted
  if (invertScreen) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 128, 64);
    u8g2.setDrawColor(0);
  }
  
  // Draw border line at bottom of yellow region (y=15, full width)
  u8g2.drawLine(0, 15, 127, 15);
  
  // "Qminer" title in top left (yellow region, line 0-15)
  u8g2.setFont(u8g2_font_mercutio_basic_nbp_tf);
  u8g2.drawStr(2, 12, "Qminer");
  
  // KH/s value - top right
  u8g2.setFont(u8g2_font_helvB10_tf);
  String khsText = data.currentHashRate;
  int khsWidth = u8g2.getStrWidth(khsText.c_str());
  int khsX = 128 - khsWidth - 2;
  u8g2.drawStr(khsX, 13, khsText.c_str());
  
  // Set clipping to blue region only (hide text that scrolls into yellow region)
  u8g2.setClipWindow(0, 16, 127, 63);
  
  // Display miner motivation quote with vertical scrolling
  u8g2.setFont(u8g2_font_8x13_tf);
  
  // Manual text wrapping for the quote
  String quote = minerMotivation;
  if (quote.length() == 0) {
    quote = "Keep mining!";
  }
  
  // Remove all newlines - we'll do our own word wrapping
  quote.replace("\\n", " ");
  quote.replace("\n", " ");
  quote.trim();
  
  int lineHeight = 15;
  int blueRegionTop = 26; // Start of blue region
  int blueRegionBottom = 64; // End of display
  int blueRegionHeight = blueRegionBottom - blueRegionTop; // 38 pixels
  int maxWidth = 118; // Max width for text (128 - margins)
  
  // Build wrapped lines array with word wrapping by pixel width
  String wrappedLines[50]; // Max 50 lines
  int lineCount = 0;
  
  // Word wrap the entire quote
  String currentLine = "";
  int wordStart = 0;
  
  for (int i = 0; i <= quote.length(); i++) {
    if (i == quote.length() || quote.charAt(i) == ' ') {
      String word = quote.substring(wordStart, i);
      word.trim();
      
      if (word.length() == 0) {
        wordStart = i + 1;
        continue;
      }
      
      String testLine = currentLine.length() > 0 ? currentLine + " " + word : word;
      
      if (u8g2.getStrWidth(testLine.c_str()) <= maxWidth) {
        currentLine = testLine;
      } else {
        // Current line is full, save it and start new line with this word
        if (currentLine.length() > 0) {
          wrappedLines[lineCount++] = currentLine;
        }
        currentLine = word;
      }
      
      wordStart = i + 1;
      
      if (lineCount >= 50) break;
    }
  }
  
  // Add remaining line
  if (currentLine.length() > 0 && lineCount < 50) {
    wrappedLines[lineCount++] = currentLine;
  }
  
  // Calculate total content height
  int totalContentHeight = lineCount * lineHeight;
  
  // Update scroll offset (smooth pixel-by-pixel scrolling)
  if (millis() - lastScrollTime > scrollSpeed) {
    scrollOffset++; // Scroll 1 pixel per update
    lastScrollTime = millis();
    
    // Reset scroll when we've scrolled past all content
    if (scrollOffset > totalContentHeight - blueRegionHeight + lineHeight) {
      scrollOffset = 0;
      delay(2000); // Pause at the top before restarting
    }
  }
  
  // Draw wrapped lines with scroll offset
  for (int i = 0; i < lineCount; i++) {
    int yPos = blueRegionTop + (i * lineHeight) - scrollOffset;
    
    // Draw if in visible range (clipping handles the rest)
    if (yPos > 0 && yPos < 80 && wrappedLines[i].length() > 0) {
      u8g2.drawStr(5, yPos, wrappedLines[i].c_str());
    }
  }
  
  // Remove clipping
  u8g2.setMaxClipWindow();
  
  u8g2.sendBuffer();

  serialPrint(mElapsed);
}

void oledDisplay_LoadingScreen(void)
{
 Serial.println("Initializing...");
  u8g2.clearBuffer();
  
  // Two-line text centered in blue region
  // QMiner - larger font
  u8g2.setFont(u8g2_font_mercutio_basic_nbp_tf);
  u8g2.drawStr(35, 36, "QMiner");
  
  // 2026 - smaller font
  u8g2.setFont(u8g2_font_helvB10_tf);
  u8g2.drawStr(45, 52, "2026");
  
  u8g2.sendBuffer();
}

void oledDisplay_SetupScreen(void)
{
  Serial.println("Setup...");
  u8g2.clearBuffer();
  u8g2.drawXBMP(20,0,30,30, setup_icon);
  u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.drawUTF8(20, 38, "Setup");
  u8g2.sendBuffer();
}

void oledDisplay_DoLedStuff(unsigned long frame)
{
  #ifdef USE_LED
  if (!oledLedOn) {
    Serial.println("[LED] OLED LED is OFF, clearing LED");
    FastLED.clear(true);
    return;
  }
/*
  Serial.print("[LED] NerdStatus: ");
  Serial.println(mMonitor.NerdStatus);
*/
  // --- Persistent blink state for NM_accepted always takes priority ---
  static bool blink_in_progress = false;
  static unsigned long blink_start_frame = 0;
  // Start blink if NM_accepted is detected and not already blinking
  if (mMonitor.NerdStatus == NM_accepted && !blink_in_progress) {
    blink_in_progress = true;
    blink_start_frame = frame;
  }
  // If blink in progress, override LED output regardless of status
  if (blink_in_progress) {
    // Blink 3 times: ON 100ms, OFF 200ms, repeat 3x (total 0.9s)
    // Each frame = 100ms, so ON = 1 frame, OFF = 2 frames, 3 frames per blink, 9 frames total
    unsigned long blink_frame = frame - blink_start_frame;
    unsigned long blink_cycle = blink_frame % 3;
    if (blink_frame < 9) {
      if (blink_cycle < 1) {
        leds.setRGB(128, 0, 128); // Purple ON
      } else {
        leds.setRGB(0, 0, 0); // OFF
      }
    } else {
      blink_in_progress = false;
      mMonitor.NerdStatus = NM_hashing;
    }
  } else {
    // Only set LED color for other statuses if not blinking
    switch (mMonitor.NerdStatus) {
      case NM_waitingConfig:
        leds.setRGB(255, 255, 0); // Yellow
        break;
      case NM_Connecting:
        leds.setRGB(0, 0, 255); // Blue
        break;
      case NM_hashing: {
        // Fade from Blue to White
        uint8_t val = beatsin8(78); // 30 BPM
        leds.setRGB(val, val, 255);
        break;
      }
      case NM_accepted: // handled above
        break;
      default:
        FastLED.clear(true);
        return;
    }
  }

  FastLED.show();

  #endif
}

void oledDisplay_AnimateCurrentScreen(unsigned long frame)
{
}

CyclicScreenFunction oledDisplayCyclicScreens[] = {oledDisplay_Screen1, oledDisplay_Screen2, oledDisplay_Screen3, oledDisplay_Screen4, oledDisplay_Screen5};

DisplayDriver oled042DisplayDriver = {
    oledDisplay_Init,
    oledDisplay_AlternateScreenState,
    oledDisplay_AlternateRotation,
    oledDisplay_LoadingScreen,
    oledDisplay_SetupScreen,
    oledDisplayCyclicScreens,
    oledDisplay_AnimateCurrentScreen,
    oledDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(oledDisplayCyclicScreens),
    0,
    0,
    0,
};

#endif
