#pragma once
#include <Arduino.h>
#include <vector>

class PlayerController {
public:
  void begin();
  bool play(const String &fname); // enqueue play request
  String getLastPlayDebug();
  String getCurrentName();
  void stop();
  void next();
  void prev();
  // loopMode: 0=sequential, 1=single-loop, 2=reverse, 3=random
  void setLoop(int mode);
  int getLoop();
  void setVolume(uint8_t v);
  uint8_t getVolume();
  bool playCurrent();
  bool playIndex(int idx);
  String playlistJSON();
  String statusJSON();
  bool pauseToggle();
  bool seekSec(uint32_t sec);
  void loop();
  // recent action logs for diagnostics
  String recentActionsJSON();
  int getPlaylistSize();
  int getQueueSize();
private:
  std::vector<String> playlist;
  int currentIndex = -1;
  int loopMode = 3;
  bool wasRunning = false;
  unsigned long runningFalseAt = 0;
  unsigned long lastPlayStartedAt = 0; // millis when current track started
  unsigned long estimatedEndAt = 0; // millis estimated end time based on reported duration
  static const unsigned long END_DEBOUNCE_MS = 500; // ms to confirm track end
  void buildPlaylist();
  // queue and serialization
  std::vector<String> playQueue;
  bool playBusy = false;
  unsigned long lastPlayRequestAt = 0;
  // recent action log (ring buffer)
  std::vector<String> recentActions;
  String lastPlayDebug;
  bool performPlayNow(const String &fname);
};

// single global instance (defined in main.cpp)
extern PlayerController playerController;
