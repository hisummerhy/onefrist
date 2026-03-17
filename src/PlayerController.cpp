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
  uint32_t dur_ms = audio.getAudioFileDuration();
  estimatedEndAt = (dur_ms > 0) ? millis() + dur_ms : 0;
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
  // === 1. 先处理播放队列（必须最优先）===
  if(!playBusy && !playQueue.empty()){
    String next = playQueue.front();
    playQueue.erase(playQueue.begin());
    playBusy = true;
    bool ok = performPlayNow(next);
    if(ok){
      // mark as running to avoid immediate debounce-based advancement
      wasRunning = true;
      runningFalseAt = 0;
    } else {
      Serial.printf("[Player] performPlayNow failed for %s\n", next.c_str());
    }
    playBusy = false;
  }

  // === 2. 核心：可靠结束检测（解决 dur==0 的问题）===
  bool running = audio.isRunning();
  uint32_t pos = audio.getAudioCurrentTime();
  uint32_t dur = audio.getAudioFileDuration();

  bool shouldAdvance = false;

  // A. 有 duration 时用 position 判断（最准）
  // 对于短音频，原始硬阈值 pos>1000 会导致无法识别结束，改为基于文件时长的阈值：
  // 当 dur 较大 (>2000ms) 时保持 1000ms 限制，否则使用 dur/2
  if(dur > 0){
    // 当文件有时长时，优先使用位置判断。但要避免在刚刚开始播放（pos==0）时立即误判为结束。
    bool posBased = false;
    // only consider position-based end detection when we have a non-zero position
    if(pos > 0 && pos + 1500 >= dur){
      // 允许通过的条件：要么播放已走过较长时间（pos>1000），
      // 要么自播放开始已过去一小段时间（>2000ms）且 audio 正在运行，
      // 以防 connect/解码延迟导致 pos 仍为 0
      if(pos > 1000 || (lastPlayStartedAt != 0 && millis() - lastPlayStartedAt > 2000 && audio.isRunning())){
        posBased = true;
      }
    }
    if(posBased){
      shouldAdvance = true;
      Serial.printf("[Player] position end detected (dur=%u pos=%u lastStart=%lu)\n", dur, pos, lastPlayStartedAt);
    }
  }
  // B. estimatedEndAt 超时（你原来的逻辑保留）
  else if(estimatedEndAt != 0 && millis() > estimatedEndAt + 1000){
    shouldAdvance = true;
    Serial.printf("[Player] estimated-end timeout -> advancing\n");
    estimatedEndAt = 0;
  }
  // C. 经典 debounce（加大到 800ms 更稳）+ 极端 5秒超时 fallback（防死锁）
  else if(wasRunning && !running){
    if(runningFalseAt == 0) runningFalseAt = millis();
    else if(millis() - runningFalseAt > 800 || millis() - lastPlayStartedAt > 5000){
      shouldAdvance = true;
      Serial.println("[Player] debounce confirmed track end -> advancing");
    }
  }

  // === 3. 执行下一首（所有模式都支持）===
  if(shouldAdvance && loopMode != 1 && !playlist.empty()){
    int nextIndex = currentIndex;
    switch(loopMode){
      case 0: // 顺序
        nextIndex = (currentIndex < 0) ? 0 : (currentIndex + 1) % (int)playlist.size();
        break;
      case 2: // 倒序
        nextIndex = (currentIndex < 0) ? 0 : (currentIndex - 1 + (int)playlist.size()) % (int)playlist.size();
        break;
      case 3: // 随机（默认）
        if((int)playlist.size() == 1) nextIndex = 0;
        else {
          do { nextIndex = random((int)playlist.size()); } while(nextIndex == currentIndex);
        }
        break;
    }
    currentIndex = nextIndex;
    Serial.printf("[Player] advancing to nextIndex=%d name=%s (mode=%d)\n", 
                  currentIndex, playlist[currentIndex].c_str(), loopMode);
    
    play(playlist[currentIndex]);   // 走队列，确保不卡
    runningFalseAt = 0;
    wasRunning = true;
    return;
  }

  // === 4. 更新状态（放在最后，避免 race）===
  if(!running && wasRunning){
    if(runningFalseAt == 0) runningFalseAt = millis();
  } else if(running){
    runningFalseAt = 0;
    wasRunning = true;
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
