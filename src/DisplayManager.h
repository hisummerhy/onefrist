#pragma once
#include <Arduino.h>

class DisplayManager {
public:
  void begin();
  void showIP(const String &ip);
  void showNowPlaying(const String &title, uint32_t pos_ms, uint32_t dur_ms, uint8_t percent);
  void setNowPlayingEnabled(bool en);
  bool isNowPlayingEnabled();
  // Release display resources (safe to call before reboot or reinit)
  void cleanup();

private:
  bool nowPlayingEnabled = true;
  // cached values to minimize redraws
  String lastTitle = "";
  String lastPosStr = "";
  String lastDurStr = "";
  uint8_t lastPercent = 255;
};
