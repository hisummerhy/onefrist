#include "DisplayManager.h"
#include <Arduino.h>
#if __has_include(<TFT_eSPI.h>)
#include <TFT_eSPI.h>
static TFT_eSPI tft = TFT_eSPI();
#define HAS_TFT 1
#else
#define HAS_TFT 0
#endif

void DisplayManager::begin(){
#if HAS_TFT
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(4);
  tft.setTextColor(TFT_WHITE);
#else
  Serial.println("Display: TFT_eSPI not available, falling back to Serial");
#endif
}

void DisplayManager::showIP(const String &ip){
#if HAS_TFT
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 20);
  tft.print("IP:");
  tft.setCursor(0, 44);
  tft.print(ip);
#else
  Serial.printf("Display IP: %s\n", ip.c_str());
#endif
}

void DisplayManager::showNowPlaying(const String &title, uint32_t pos_ms, uint32_t dur_ms, uint8_t percent){
#if HAS_TFT
  // draw a simple progress area at the bottom
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // title (truncate to fit)
  String t = title;
  if(t.length() > 28) t = t.substring(0, 28) + "...";
  tft.setCursor(4, 8);
  tft.print(t);
  // progress bar
  int barX = 4, barY = 40, barW = tft.width() - 8, barH = 12;
  tft.drawRect(barX-1, barY-1, barW+2, barH+2, TFT_WHITE);
  int fillW = (barW * percent) / 100;
  if(fillW > 0) tft.fillRect(barX, barY, fillW, barH, TFT_GREEN);
  // time
  auto fmt = [](uint32_t ms)->String{ uint32_t s = ms/1000; uint32_t m = s/60; s = s%60; char buf[16]; sprintf(buf, "%u:%02u", (unsigned)m, (unsigned)s); return String(buf); };
  tft.setCursor(4, 60);
  tft.print(fmt(pos_ms));
  tft.setCursor(tft.width()-60, 60);
  tft.print(fmt(dur_ms));
#else
  Serial.printf("NowPlaying: %s %u/%u (%u%%)\n", title.c_str(), pos_ms, dur_ms, percent);
#endif
}
