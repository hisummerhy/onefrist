#include "WebServerModule.h"
#include <WebServer.h>
#include <SD.h>
#include <WiFi.h>
#include "PlayerController.h"
#include "web/web_root.h"

static WebServer server(80);

static String listSongsHtml(){
  String html = "<ul>";
  File root = SD.open("/");
  if(root){
    File file = root.openNextFile();
    while(file){
      if(!file.isDirectory()){
        String name = String(file.name());
        int len = name.length();
        if(len>4 && name.substring(len-4).equalsIgnoreCase(".mp3")){
          html += "<li><a href=\"/music?file=" + name + "\">" + name + "</a></li>";
        }
      }
      file = root.openNextFile();
    }
  }
  html += "</ul>";
  return html;
}

static String listSongsJson(){
  // use PlayerController's playlist snapshot to avoid concurrent SD access
  return playerController.playlistJSON();
}

static String urlDecode(const String &s){
  String out = "";
  int len = s.length();
  for(int i=0;i<len;i++){
    char c = s[i];
    if(c=='+') out += ' ';
    else if(c=='%' && i+2 < len){
      char hi = s[i+1];
      char lo = s[i+2];
      int v = 0;
      if(hi>='0'&&hi<='9') v = (hi-'0')<<4; else if(hi>='A'&&hi<='F') v = (hi-'A'+10)<<4; else if(hi>='a'&&hi<='f') v = (hi-'a'+10)<<4;
      if(lo>='0'&&lo<='9') v |= (lo-'0'); else if(lo>='A'&&lo<='F') v |= (lo-'A'+10); else if(lo>='a'&&lo<='f') v |= (lo-'a'+10);
      out += (char)v; i+=2;
    } else out += c;
  }
  return out;
}

void WebServerModule::handleRoot(){
  // Serve embedded HTML page (kept in src/web/web_root.h)
  server.send_P(200, "text/html", WEB_ROOT_INDEX);
}

void WebServerModule::handleMusic(){
  if(!server.hasArg("file")){
    server.send(400, "text/plain", "Missing file parameter");
    return;
  }
  String fname = server.arg("file");
  // normalize to ensure leading slash for VFS
  if(fname.length()>0 && fname.charAt(0) != '/'){
    Serial.printf("[Web] handleMusic normalizing file path: %s -> /%s\n", fname.c_str(), fname.c_str());
    fname = String("/") + fname;
  }
  File f = SD.open(fname.c_str());
  if(!f){
    Serial.printf("[Web] handleMusic - file not found: %s\n", fname.c_str());
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(f, "audio/mpeg");
  f.close();
}

void WebServerModule::begin(uint16_t port){
  // PlayerController is initialized in main.cpp; do not reinitialize here.
  static unsigned long lastPlayReqMillis = 0;
  const unsigned long PLAY_RATE_MS = 300; // simple rate limit
  server.on("/", [](){ WebServerModule ws; ws.handleRoot(); });
  server.on("/music", [](){ WebServerModule ws; ws.handleMusic(); });
  server.on("/api/play", [](){
    if(server.hasArg("file")){
      if(millis() - lastPlayReqMillis < PLAY_RATE_MS){ server.send(200, "application/json", String("{\"ok\":false,\"reason\":\"rate_limit\"}")); return; }
      String f = server.arg("file");
      String fname = urlDecode(f);
      lastPlayReqMillis = millis();
      Serial.printf("[Web] /api/play requested (raw): %s\n", f.c_str());
      Serial.printf("[Web] /api/play requested (decoded): %s\n", fname.c_str());
      bool ok = playerController.play(fname);
      server.send(200, "application/json", String("{\"ok\":") + (ok?"true":"false") + "}");
    } else server.send(400, "text/plain", "missing file");
  });
  server.on("/api/play_index", [](){
    if(server.hasArg("i")){
      if(millis() - lastPlayReqMillis < PLAY_RATE_MS){ server.send(200, "application/json", String("{\"ok\":false,\"reason\":\"rate_limit\"}")); return; }
      lastPlayReqMillis = millis();
      int idx = server.arg("i").toInt();
      Serial.printf("[Web] /api/play_index requested: %d\n", idx);
      bool ok = playerController.playIndex(idx);
      server.send(200, "application/json", String("{\"ok\":") + (ok?"true":"false") + "}");
    } else server.send(400, "text/plain", "missing i");
  });
  server.on("/api/play_current", [](){ Serial.println("[Web] /api/play_current"); bool ok = playerController.playCurrent(); server.send(200, "application/json", String("{\"ok\":") + (ok?"true":"false") + "}"); });
  server.on("/api/list", [](){ server.send(200, "application/json", listSongsJson()); });
  server.on("/api/stop", [](){ playerController.stop(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/next", [](){ playerController.next(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/prev", [](){ playerController.prev(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/toggleLoop", [](){
    // backward-compatible: val=1 -> single-loop, val=0 -> sequential
    int mode = 0;
    if(server.hasArg("val") && server.arg("val")=="1") mode = 1;
    playerController.setLoop(mode);
    server.send(200, "application/json", String("{\"loop_mode\":") + String(mode) + "}");
  });
  server.on("/api/loop", [](){
    // set explicit loop mode: mode=0..3
    if(server.hasArg("mode")){
      int m = server.arg("mode").toInt();
      if(m < 0) m = 0; if(m > 3) m = 0;
      playerController.setLoop(m);
      server.send(200, "application/json", String("{\"loop_mode\":") + String(m) + "}");
    } else server.send(400, "text/plain", "missing mode");
  });
  server.on("/api/volume", [](){ if(server.hasArg("val")){ uint8_t v = server.arg("val").toInt(); playerController.setVolume(v);} server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/status", [](){ server.send(200, "application/json", playerController.statusJSON()); });
  server.on("/api/last_play", [](){ server.send(200, "text/plain", playerController.getLastPlayDebug()); });
  server.on("/api/recent_actions", [](){ server.send(200, "application/json", playerController.recentActionsJSON()); });
  server.on("/api/pause", [](){ bool ok = playerController.pauseToggle(); server.send(200, "application/json", String("{\"ok\":") + (ok?"true":"false") + "}"); });
  server.on("/api/seek", [](){ if(server.hasArg("sec")){ uint32_t s = (uint32_t)server.arg("sec").toInt(); bool ok = playerController.seekSec(s); server.send(200, "application/json", String("{\"ok\":") + (ok?"true":"false") + "}"); } else server.send(400, "text/plain", "missing sec"); });
  // log unknown routes to help diagnose client requests
  server.onNotFound([](){
    Serial.printf("[Web] NotFound: %s\n", server.uri().c_str());
    server.send(404, "text/plain", "not found");
  });
  server.begin();
  Serial.println("Web server started on port 80");
}

void WebServerModule::loop(){
  server.handleClient();
}
