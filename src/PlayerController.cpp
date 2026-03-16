#include "PlayerController.h"
#include <SD.h>
#include "Audio.h"

extern Audio audio; // defined in main.cpp

void PlayerController::buildPlaylist(){
  playlist.clear();
  File root = SD.open("/");
  if(!root) return;
  File f = root.openNextFile();
  while(f){
    if(!f.isDirectory()){
      String name = String(f.name());
      int len = name.length();
      if(len>4 && name.substring(len-4).equalsIgnoreCase(".mp3")){
        playlist.push_back(name);
      }
    }
    f = root.openNextFile();
  }
}

void PlayerController::begin(){
  buildPlaylist();
}

bool PlayerController::play(const String &fname){
  if(fname.length()==0) return false;
  Serial.printf("[Player] play requested: %s\n", fname.c_str());
  if(!SD.exists(fname.c_str())){ Serial.println("[Player] SD.exists -> false"); return false; }
  Serial.println("[Player] SD.exists -> true");
  bool ok = audio.connecttoFS(SD, fname.c_str());
  Serial.printf("[Player] connecttoFS returned: %d\n", ok);
  if(ok){
    // find index for fname
    currentIndex = -1;
    for(size_t i=0;i<playlist.size();++i) if(playlist[i]==fname) { currentIndex = i; break; }
  }
  return ok;
}

void PlayerController::stop(){
  audio.stopSong();
}

void PlayerController::next(){
  if(playlist.empty()) return;
  if(currentIndex < 0) currentIndex = 0;
  else currentIndex = (currentIndex + 1) % (int)playlist.size();
  play(playlist[currentIndex]);
}

void PlayerController::prev(){
  if(playlist.empty()) return;
  if(currentIndex < 0) currentIndex = 0;
  else currentIndex = (currentIndex - 1 + (int)playlist.size()) % (int)playlist.size();
  play(playlist[currentIndex]);
}

void PlayerController::setLoop(bool v){
  loopMode = v;
  audio.setFileLoop(v);
}

bool PlayerController::getLoop(){
  return loopMode;
}

void PlayerController::setVolume(uint8_t v){
  audio.setVolume(v);
}

uint8_t PlayerController::getVolume(){
  return audio.getVolume();
}
bool PlayerController::playCurrent(){
  if(playlist.empty()) return false;
  if(currentIndex < 0) currentIndex = 0;
  return play(playlist[currentIndex]);
}

String PlayerController::statusJSON(){
  String s = "{";
  s += "\"playing\": " + String(audio.isRunning() ? "true" : "false") + ",";
  s += "\"loop\": " + String(loopMode ? "true" : "false") + ",";
  s += "\"volume\": " + String(getVolume()) + ",";
  String cur = "";
  if(currentIndex>=0 && currentIndex < (int)playlist.size()) cur = playlist[currentIndex];
  s += "\"current\": \"" + cur + "\"";
  s += "}";
  return s;
}

void PlayerController::loop(){
  // called from main loop
  bool running = audio.isRunning();
  if(wasRunning && !running){
    // track ended
    if(loopMode){
      // replay same
      if(currentIndex>=0 && currentIndex < (int)playlist.size()){
        play(playlist[currentIndex]);
      }
    } else {
      // auto next
      if(!playlist.empty()){
        currentIndex = (currentIndex + 1) % (int)playlist.size();
        play(playlist[currentIndex]);
      }
    }
  }
  wasRunning = running;
}
