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
