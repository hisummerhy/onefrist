#pragma once
#include <Arduino.h>

class DisplayManager {
public:
  void begin();
  void showIP(const String &ip);
};
