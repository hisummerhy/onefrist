#pragma once
#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
  void begin(const char* ssid, const char* password);
  String getIP();
private:
  String ipStr;
};
