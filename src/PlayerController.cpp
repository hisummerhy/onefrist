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
  // initialize recentActions reserve
  recentActions.clear();
  // default to random playback and start playing continuously
  randomSeed(micros());
  setLoop(3); // 3 = random
  if(!playlist.empty()){
    // pick a random start index
    currentIndex = random((int)playlist.size());
    // start playing immediately
    play(playlist[currentIndex]);
  }
}

bool PlayerController::play(const String &fname){
  // enqueue the play request so connecttoFS calls are serialized
  if(fname.length()==0) return false;
  Serial.printf("[Player] enqueue play requested: %s\n", fname.c_str());
  lastPlayRequestAt = millis();
  playQueue.push_back(fname);
  // record recent action
  String ra = String("enqueue:") + fname + ",@" + String(lastPlayRequestAt);
  recentActions.push_back(ra);
  if(recentActions.size()>32) recentActions.erase(recentActions.begin());
  return true; // accepted for processing
}

// perform the actual connect/play; returns whether connecttoFS succeeded
bool PlayerController::performPlayNow(const String &fname){
  if(fname.length()==0) return false;
  Serial.printf("[Player] play requested(now): %s\n", fname.c_str());
  String useName = fname;
  lastPlayDebug = "";
  // normalize checks: many VFS/SD implementations expect paths that start with '/'
  String probe = useName;
  if(probe.length()>0 && probe.charAt(0) != '/') probe = String("/") + probe;
  if(!SD.exists(useName.c_str()) && !SD.exists(probe.c_str())){
    Serial.println("[Player] SD.exists -> false for requested name, searching playlist for candidate");
    lastPlayDebug = "SD.exists -> false";
    // try to find a matching name from playlist (handle leading slash or minor differences)
    String candidate = "";
    for(size_t i=0;i<playlist.size();++i){
      String p = playlist[i];
      if(p.equalsIgnoreCase(useName) || (String("/")+p).equalsIgnoreCase(useName) || p.equalsIgnoreCase(String("/")+useName)){
        candidate = p; break;
      }
      if(p.indexOf(useName) >= 0 || useName.indexOf(p) >= 0){ candidate = p; break; }
    }
    if(candidate.length()>0){
      Serial.printf("[Player] trying candidate from playlist: %s\n", candidate.c_str());
      useName = candidate;
    } else {
      // try with leading slash as last resort
      String alt = String("/") + useName;
      if(SD.exists(alt.c_str())){ Serial.printf("[Player] SD.exists true for /name variant: %s\n", alt.c_str()); useName = alt; }
      else { lastPlayDebug += ", no candidate"; return false; }
    }
  }
  // final sanity: if still not exists, try leading slash variant
  if(!SD.exists(useName.c_str())){
    String alt = useName;
    if(alt.length()==0 || alt.charAt(0) != '/') alt = String("/") + useName;
    if(SD.exists(alt.c_str())){ Serial.printf("[Player] SD.exists true for /name variant (post): %s\n", alt.c_str()); useName = alt; }
  }
  Serial.println("[Player] SD.exists -> true");
  lastPlayDebug = "SD.exists -> true";
  // ensure any previous stream is stopped before connecting to new file
  audio.stopSong();
  // ensure connect path starts with '/'
  String connectPath = useName;
  if(connectPath.length()==0 || connectPath.charAt(0) != '/') connectPath = String("/") + connectPath;
  Serial.printf("[Player] connecting to FS path: %s (original: %s)\n", connectPath.c_str(), useName.c_str());
  bool ok = audio.connecttoFS(SD, connectPath.c_str());
  Serial.printf("[Player] connecttoFS returned: %d\n", ok);
  lastPlayDebug += ", connecttoFS -> "; lastPlayDebug += (ok?"true":"false");
  if(ok){
    lastPlayStartedAt = millis();
    // record estimated end time if duration known
    uint32_t dur_ms = audio.getAudioFileDuration();
    if(dur_ms > 0) estimatedEndAt = millis() + dur_ms;
    else estimatedEndAt = 0;
  }
  // find index for fname (normalize leading slash)
  currentIndex = -1;
  String canonical = useName;
  if(canonical.length()>0 && canonical.charAt(0)=='/') canonical = canonical.substring(1);
  for(size_t i=0;i<playlist.size();++i){
    if(playlist[i].equalsIgnoreCase(canonical) || playlist[i].equalsIgnoreCase(useName) || (String("/")+playlist[i]).equalsIgnoreCase(useName)){
      currentIndex = i;
      // ensure useName is the canonical playlist entry (no leading slash)
      useName = playlist[i];
      break;
    }
  }
  // append success/fail to recent actions (use canonical name if available)
  String raName = (currentIndex>=0 ? playlist[currentIndex] : useName);
  String ra = String("play:") + raName + ",ok=" + (ok?"1":"0") + ",@" + String(millis());
  recentActions.push_back(ra);
  if(recentActions.size()>32) recentActions.erase(recentActions.begin());
  return ok;
}

String PlayerController::playlistJSON(){
  String out = "[";
  for(size_t i=0;i<playlist.size();++i){
    if(i) out += ",";
    out += "\"" + playlist[i] + "\"";
  }
  out += "]";
  return out;
}

String PlayerController::getLastPlayDebug(){
  return lastPlayDebug;
}

void PlayerController::stop(){
  audio.stopSong();
}

bool PlayerController::pauseToggle(){
  bool ok = audio.pauseResume();
  String ra = String("pauseToggle:ok=") + (ok?"1":"0") + ",@" + String(millis());
  recentActions.push_back(ra);
  if(recentActions.size()>32) recentActions.erase(recentActions.begin());
  return ok;
}

bool PlayerController::seekSec(uint32_t sec){
  bool ok = audio.setAudioPlayPosition((uint16_t)sec);
  String ra = String("seek:") + String(sec) + ",ok=" + (ok?"1":"0") + ",@" + String(millis());
  recentActions.push_back(ra);
  if(recentActions.size()>32) recentActions.erase(recentActions.begin());
  return ok;
}

void PlayerController::next(){
  if(playlist.empty()) return;
  Serial.printf("[Player] next() called: loopMode=%d currentIndex=%d playlistSize=%d\n", loopMode, currentIndex, (int)playlist.size());
  if(currentIndex < 0) currentIndex = 0;
  if(loopMode == 3){
    // random mode: pick a different random index when possible
    if((int)playlist.size() == 1) currentIndex = 0;
    else {
      int nextIdx = currentIndex;
      while(nextIdx == currentIndex) nextIdx = random((int)playlist.size());
      currentIndex = nextIdx;
    }
  } else {
    currentIndex = (currentIndex + 1) % (int)playlist.size();
  }
  Serial.printf("[Player] next selected index=%d name=%s\n", currentIndex, playlist[currentIndex].c_str());
  play(playlist[currentIndex]);
}

void PlayerController::prev(){
  if(playlist.empty()) return;
  Serial.printf("[Player] prev() called: loopMode=%d currentIndex=%d playlistSize=%d\n", loopMode, currentIndex, (int)playlist.size());
  if(currentIndex < 0) currentIndex = 0;
  if(loopMode == 3){
    // random mode: pick a different random index when possible
    if((int)playlist.size() == 1) currentIndex = 0;
    else {
      int nextIdx = currentIndex;
      while(nextIdx == currentIndex) nextIdx = random((int)playlist.size());
      currentIndex = nextIdx;
    }
  } else {
    currentIndex = (currentIndex - 1 + (int)playlist.size()) % (int)playlist.size();
  }
  Serial.printf("[Player] prev selected index=%d name=%s\n", currentIndex, playlist[currentIndex].c_str());
  play(playlist[currentIndex]);
}

void PlayerController::setLoop(int mode){
  // mode: 0=sequential,1=single-loop,2=reverse,3=random
  loopMode = mode;
  // enable file-level loop only for single-loop mode
  audio.setFileLoop(mode==1);
}

int PlayerController::getLoop(){
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

bool PlayerController::playIndex(int idx){
  if(idx < 0 || idx >= (int)playlist.size()) return false;
  return play(playlist[idx]);
}

String PlayerController::statusJSON(){
  String s = "{";
  bool running = audio.isRunning();
  s += "\"playing\": " + String(running ? "true" : "false") + ",";
  s += "\"loop_mode\": " + String(loopMode) + ",";
  s += "\"volume\": " + String(getVolume()) + ",";
  uint32_t pos = audio.getAudioCurrentTime();
  uint32_t dur = audio.getAudioFileDuration();
  uint32_t percent = 0;
  if(dur > 0) percent = (pos * 100) / dur;
  s += "\"position_ms\": " + String(pos) + ",";
  s += "\"duration_ms\": " + String(dur) + ",";
  s += "\"percent\": " + String(percent) + ",";
  String cur = "";
  if(currentIndex>=0 && currentIndex < (int)playlist.size()) cur = playlist[currentIndex];
  s += "\"current\": \"" + cur + "\"";
  s += "}";
  return s;
}

void PlayerController::loop(){
  // called from main loop
  // process queued play requests serially
  if(!playBusy && !playQueue.empty()){
    String next = playQueue.front();
    playQueue.erase(playQueue.begin());
    playBusy = true;
    performPlayNow(next);
    playBusy = false;
  }
    bool running = audio.isRunning();
    // Use position/duration as a stronger end-of-track signal when available
    uint32_t pos = audio.getAudioCurrentTime();
    uint32_t dur = audio.getAudioFileDuration();
    // fallback: if we previously recorded an estimated end time, and we've passed it, advance
    if(estimatedEndAt != 0 && millis() > estimatedEndAt + 1500 && loopMode != 1){
      Serial.printf("[Player] estimated-end reached now=%lu est=%lu -> advancing\n", millis(), estimatedEndAt);
      estimatedEndAt = 0;
      if(!playlist.empty()){
        int nextIndex = currentIndex;
        switch(loopMode){
          case 2: // reverse
            if(currentIndex < 0) nextIndex = 0; else nextIndex = (currentIndex - 1 + (int)playlist.size()) % (int)playlist.size();
            break;
          case 3: // random
            if((int)playlist.size() == 1) nextIndex = 0;
            else { while(nextIndex == currentIndex) nextIndex = random((int)playlist.size()); }
            break;
          case 0: // sequential
          default:
            nextIndex = (currentIndex < 0) ? 0 : (currentIndex + 1) % (int)playlist.size();
            break;
        }
        currentIndex = nextIndex;
        Serial.printf("[Player] advancing to nextIndex=%d name=%s (loopMode=%d)\n", currentIndex, playlist[currentIndex].c_str(), loopMode);
        play(playlist[currentIndex]);
        runningFalseAt = 0;
        wasRunning = true;
        return;
      }
    }
    // if we have a valid duration and we're within 1.5s of the end, advance immediately (unless single-loop)
    if(dur > 0 && loopMode != 1){
      if(pos + 1500 >= dur){
        // avoid immediate advancement right after starting (require position >1s)
        if(pos > 1000){
          Serial.printf("[Player] position-based end detected pos=%u dur=%u -> advancing\\n", pos, dur);
          if(!playlist.empty()){
            int nextIndex = currentIndex;
            switch(loopMode){
              case 2: // reverse
                if(currentIndex < 0) nextIndex = 0; else nextIndex = (currentIndex - 1 + (int)playlist.size()) % (int)playlist.size();
                break;
              case 3: // random
                if((int)playlist.size() == 1) nextIndex = 0;
                else { while(nextIndex == currentIndex) nextIndex = random((int)playlist.size()); }
                break;
              case 0: // sequential
              default:
                nextIndex = (currentIndex < 0) ? 0 : (currentIndex + 1) % (int)playlist.size();
                break;
            }
            currentIndex = nextIndex;
            Serial.printf("[Player] advancing to nextIndex=%d name=%s (loopMode=%d)\\n", currentIndex, playlist[currentIndex].c_str(), loopMode);
            play(playlist[currentIndex]);
            // treat as running after scheduling next
            runningFalseAt = 0;
            wasRunning = true;
            return;
          }
        } else {
          // too soon after start, skip position-based advance
        }
      }
    }

  // Debounce transient false reads from audio.isRunning() to avoid spurious track changes
  if(wasRunning && !running){
    if(runningFalseAt == 0) runningFalseAt = millis();
    else if(millis() - runningFalseAt > END_DEBOUNCE_MS){
      Serial.println("[Player] confirmed track end -> advancing");
      if(!playlist.empty()){
        int nextIndex = currentIndex;
        switch(loopMode){
          case 1: // single-loop -> replay same
            nextIndex = currentIndex >= 0 ? currentIndex : 0;
            break;
          case 2: // reverse
            if(currentIndex < 0) nextIndex = 0; else nextIndex = (currentIndex - 1 + (int)playlist.size()) % (int)playlist.size();
            break;
          case 3: // random
            if((int)playlist.size() == 1) nextIndex = 0;
            else { while(nextIndex == currentIndex) nextIndex = random((int)playlist.size()); }
            break;
          case 0: // sequential (default)
          default:
            nextIndex = (currentIndex < 0) ? 0 : (currentIndex + 1) % (int)playlist.size();
            break;
        }
        currentIndex = nextIndex;
        Serial.printf("[Player] advancing to nextIndex=%d name=%s (loopMode=%d)\n", currentIndex, playlist[currentIndex].c_str(), loopMode);
        play(playlist[currentIndex]);
      }
      runningFalseAt = 0;
      wasRunning = true; // after scheduling next play, treat as running
      return;
    }
  } else if(!wasRunning && running){
    // transitioned from not running to running
    runningFalseAt = 0;
  } else if(running){
    // still running
    runningFalseAt = 0;
  }
  wasRunning = running;
}

String PlayerController::recentActionsJSON(){
  String out = "[";
  for(size_t i=0;i<recentActions.size();++i){
    if(i) out += ",";
    out += "\"" + recentActions[i] + "\"";
  }
  out += "]";
  return out;
}

int PlayerController::getPlaylistSize(){ return (int)playlist.size(); }
int PlayerController::getQueueSize(){ return (int)playQueue.size(); }
