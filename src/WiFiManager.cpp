#include "WiFiManager.h"

void WiFiManager::begin(const char* ssid, const char* password){
  Serial.printf("Connecting to WiFi '%s'...\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 20000) {
      Serial.println("WiFi connect timeout");
      break;
    }
    delay(500);
    Serial.print('.');
  }
  if (WiFi.status() == WL_CONNECTED) {
    ipStr = WiFi.localIP().toString();
    Serial.printf("WiFi connected, IP: %s\n", ipStr.c_str());
  } else {
    ipStr = "0.0.0.0";
    Serial.println("WiFi not connected");
  }
}

String WiFiManager::getIP(){
  return ipStr;
}
