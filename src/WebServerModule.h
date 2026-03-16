#pragma once
#include <Arduino.h>

class WebServerModule {
public:
  void begin(uint16_t port = 80);
  void loop();
private:
  void handleRoot();
  void handleMusic();
};
