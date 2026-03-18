#include "DisplayManager.h"
#include <Arduino.h>
#if __has_include(<TFT_eSPI.h>)
#include <TFT_eSPI.h>
#define HAS_TFT 1
#include <SPI.h>
static TFT_eSPI *tft = nullptr;
static TFT_eSprite *sprite = nullptr;
#else
#define HAS_TFT 0
#endif

void DisplayManager::begin(){
#if HAS_TFT
  if(!tft){
    tft = new TFT_eSPI();
    tft->init();
    tft->setRotation(1);
    tft->fillScreen(TFT_BLACK);
    tft->setTextSize(4);
    tft->setTextColor(TFT_WHITE);
  }
  // create a sprite for the top info area to avoid full-screen redraws
  if(!sprite){
    sprite = new TFT_eSprite(tft);
    // reserve ~72 pixels height for title+progress
    sprite->createSprite(tft->width(), 72);
  }
#else
  Serial.println("Display: TFT_eSPI not available, falling back to Serial");
#endif
}

void DisplayManager::showIP(const String &ip){
#if HAS_TFT
  tft->fillScreen(TFT_BLACK);
  tft->setCursor(0, 20);
  tft->print("IP:");
  tft->setCursor(0, 44);
  tft->print(ip);
#else
  Serial.printf("Display IP: %s\n", ip.c_str());
#endif
}

void DisplayManager::setNowPlayingEnabled(bool en){
  nowPlayingEnabled = en;
}

bool DisplayManager::isNowPlayingEnabled(){
  return nowPlayingEnabled;
}

void DisplayManager::showNowPlaying(const String &title, uint32_t pos_ms, uint32_t dur_ms, uint8_t percent){
#if HAS_TFT
#if 0
  // verbose logging disabled to reduce serial output frequency
  // Serial.printf("[Display] showNowPlaying title='%s' pos=%u dur=%u pct=%u\n", title.c_str(), pos_ms, dur_ms, percent);
#endif
#endif
#if HAS_TFT
  if(!nowPlayingEnabled) return;
  // draw into sprite and push the region to avoid flicker
  if(sprite){
    // prepare formatted strings
    auto fmt = [](uint32_t ms)->String{ uint32_t s = ms/1000; uint32_t m = s/60; s = s%60; char buf[16]; sprintf(buf, "%u:%02u", (unsigned)m, (unsigned)s); return String(buf); };
    String posStr = fmt(pos_ms);
    String durStr = (dur_ms==0) ? String("--:--") : fmt(dur_ms);
    String t = title;
    if(t.length() > 28) t = t.substring(0, 28) + "...";

    bool titleChanged = (t != lastTitle);
    bool percentChanged = (percent != lastPercent);
    bool timeChanged = (posStr != lastPosStr) || (durStr != lastDurStr);

    if(titleChanged || percentChanged){
      // redraw full top sprite (title + progress + time)
      sprite->fillSprite(TFT_BLACK);
      sprite->setTextSize(2);
      sprite->setTextColor(TFT_WHITE, TFT_BLACK);
      // title
      sprite->setCursor(4, 6);
      sprite->print(t);
      // progress bar
      int barX = 4, barY = 28, barW = sprite->width() - 8, barH = 12;
      sprite->drawRect(barX-1, barY-1, barW+2, barH+2, TFT_WHITE);
      int fillW = (barW * percent) / 100;
      if(fillW > 0) sprite->fillRect(barX, barY, fillW, barH, TFT_GREEN);
      // time (draw as part of sprite)
      sprite->setCursor(4, 48);
      sprite->print(posStr);
      sprite->setCursor(sprite->width()-60, 48);
      sprite->print(durStr);
      sprite->pushSprite(0, 0);
      // update caches
      lastTitle = t;
      lastPercent = percent;
      lastPosStr = posStr;
      lastDurStr = durStr;
    } else if(timeChanged){
      // only time changed: update small area directly on TFT to avoid rebuilding sprite
      if(tft){
        tft->setTextSize(2);
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        // clear time area (cover both pos and dur)
        int timeY = 48;
        int timeH = 16; // approximate height for text size 2
        tft->fillRect(4, timeY, tft->width()-8, timeH, TFT_BLACK);
        tft->setCursor(4, timeY);
        tft->print(posStr);
        tft->setCursor(tft->width()-60, timeY);
        tft->print(durStr);
      }
      lastPosStr = posStr;
      lastDurStr = durStr;
    }
  } else if(tft){
    // fallback to small-area redraw
    // fallback to small-area redraw if sprite not available
    // prepare formatted strings
    auto fmt = [](uint32_t ms)->String{ uint32_t s = ms/1000; uint32_t m = s/60; s = s%60; char buf[16]; sprintf(buf, "%u:%02u", (unsigned)m, (unsigned)s); return String(buf); };
    String posStr = fmt(pos_ms);
    String durStr = (dur_ms==0) ? String("--:--") : fmt(dur_ms);
    String t = title;
    if(t.length() > 28) t = t.substring(0, 28) + "...";

    bool titleChanged = (t != lastTitle);
    bool percentChanged = (percent != lastPercent);
    bool timeChanged = (posStr != lastPosStr) || (durStr != lastDurStr);

    if(titleChanged || percentChanged){
      tft->fillRect(0, 0, tft->width(), 72, TFT_BLACK);
      tft->setTextSize(2);
      tft->setTextColor(TFT_WHITE, TFT_BLACK);
      tft->setCursor(4, 6);
      tft->print(t);
      int barX = 4, barY = 28, barW = tft->width() - 8, barH = 12;
      tft->drawRect(barX-1, barY-1, barW+2, barH+2, TFT_WHITE);
      int fillW = (barW * percent) / 100;
      if(fillW > 0) tft->fillRect(barX, barY, fillW, barH, TFT_GREEN);
      tft->setCursor(4, 48);
      tft->print(posStr);
      tft->setCursor(tft->width()-60, 48);
      tft->print(durStr);
      lastTitle = t;
      lastPercent = percent;
      lastPosStr = posStr;
      lastDurStr = durStr;
    } else if(timeChanged){
      // only update time area
      tft->setTextSize(2);
      tft->setTextColor(TFT_WHITE, TFT_BLACK);
      int timeY = 48;
      int timeH = 16;
      tft->fillRect(4, timeY, tft->width()-8, timeH, TFT_BLACK);
      tft->setCursor(4, timeY);
      tft->print(posStr);
      tft->setCursor(tft->width()-60, timeY);
      tft->print(durStr);
      lastPosStr = posStr;
      lastDurStr = durStr;
    }
  }
#else
  // Reduced logging: avoid printing NowPlaying on every display update
#endif
}

void DisplayManager::cleanup(){
#if HAS_TFT
  if(sprite){
    sprite->deleteSprite();
    delete sprite;
    sprite = nullptr;
  }
  if(tft){
    delete tft;
    tft = nullptr;
  }
#endif
}
