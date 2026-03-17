#pragma once
#include <Arduino.h>

class DisplayManager {
public:
  void begin();
  void showIP(const String &ip);
  void showNowPlaying(const String &title, uint32_t pos_ms, uint32_t dur_ms, uint8_t percent);
};
