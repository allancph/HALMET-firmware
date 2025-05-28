
#include "halmet_display.h"

#include <WiFi.h>

namespace halmet {

// OLED display width and height, in pixels
const int kScreenWidth = 128;
const int kScreenHeight = 64;

// OLED display parameters
const int kDisplayInitDelayMs = 100;
const int kDisplayRotation = 2;  // Set rotation (0-3)
const int kDisplayDefaultTextSize = 1;
const int kDisplayRowHeight = 8;  // Height of a single text row in pixels

bool InitializeSSD1306(sensesp::SensESPBaseApp* sensesp_app,
                       Adafruit_SSD1306** display, TwoWire* i2c) {
  *display = new Adafruit_SSD1306(kScreenWidth, kScreenHeight, i2c, -1);
  bool init_successful = (*display)->begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!init_successful) {
    debugD("SSD1306 allocation failed");
    return false;
  }
  delay(kDisplayInitDelayMs);
  (*display)->setRotation(kDisplayRotation);
  (*display)->clearDisplay();
  (*display)->setTextSize(kDisplayDefaultTextSize);
  (*display)->setTextColor(SSD1306_WHITE);
  (*display)->setCursor(0, 0);
  (*display)->printf("Host: %s\n", sensesp_app->get_hostname().c_str());
  (*display)->display();

  return true;
}

/// Clear a text row on an Adafruit graphics display
void ClearRow(Adafruit_SSD1306* display, int row) {
  display->fillRect(0, kDisplayRowHeight * row, kScreenWidth, kDisplayRowHeight,
                    SSD1306_BLACK);
}

void PrintValue(Adafruit_SSD1306* display, int row, String title, float value) {
  ClearRow(display, row);
  display->setCursor(0, kDisplayRowHeight * row);
  display->printf("%s: %.1f", title.c_str(), value);
  display->display();
}

void PrintValue(Adafruit_SSD1306* display, int row, String title,
                String value) {
  ClearRow(display, row);
  display->setCursor(0, kDisplayRowHeight * row);
  display->printf("%s: %s", title.c_str(), value.c_str());
  display->display();
}

}  // namespace halmet
