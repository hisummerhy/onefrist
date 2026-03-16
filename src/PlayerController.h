#pragma once
#include <Arduino.h>
#include <vector>

class PlayerController {
public:
  void begin();
  bool play(const String &fname);
  void stop();
  void next();
  void prev();
  void setLoop(bool v);
  bool getLoop();
  void setVolume(uint8_t v);
  uint8_t getVolume();
  bool playCurrent();
  String statusJSON();
  void loop();
private:
  std::vector<String> playlist;
  int currentIndex = -1;
  bool loopMode = false;
  bool wasRunning = false;
  void buildPlaylist();
};
