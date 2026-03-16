#include "WebServerModule.h"
#include <WebServer.h>
#include <SD.h>
#include <WiFi.h>
#include "PlayerController.h"
#include "web/web_root.h"

static PlayerController player;

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
  String out = "[";
  bool first = true;
  File root = SD.open("/");
  if(root){
    File file = root.openNextFile();
    while(file){
      if(!file.isDirectory()){
        String name = String(file.name());
        int len = name.length();
        if(len>4 && name.substring(len-4).equalsIgnoreCase(".mp3")){
          if(!first) out += ",";
          out += "\"" + name + "\"";
          first = false;
        }
      }
      file = root.openNextFile();
    }
  }
  out += "]";
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
  File f = SD.open(fname.c_str());
  if(!f){
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(f, "audio/mpeg");
  f.close();
}

void WebServerModule::begin(uint16_t port){
  player.begin();
  server.on("/", [](){ WebServerModule ws; ws.handleRoot(); });
  server.on("/music", [](){ WebServerModule ws; ws.handleMusic(); });
  server.on("/api/play", [](){
    if(server.hasArg("file")){
      String f = server.arg("file");
      bool ok = player.play(f);
      server.send(200, "application/json", String("{\"ok\":") + (ok?"true":"false") + "}");
    } else server.send(400, "text/plain", "missing file");
  });
  server.on("/api/play_current", [](){ bool ok = player.playCurrent(); server.send(200, "application/json", String("{\"ok\":") + (ok?"true":"false") + "}"); });
  server.on("/api/list", [](){ server.send(200, "application/json", listSongsJson()); });
  server.on("/api/stop", [](){ player.stop(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/next", [](){ player.next(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/prev", [](){ player.prev(); server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/toggleLoop", [](){
    bool val = server.hasArg("val") && server.arg("val")=="1";
    player.setLoop(val);
    server.send(200, "application/json", String("{\"loop\":") + (val?"true":"false") + "}");
  });
  server.on("/api/volume", [](){ if(server.hasArg("val")){ uint8_t v = server.arg("val").toInt(); player.setVolume(v);} server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/status", [](){ server.send(200, "application/json", player.statusJSON()); });
  server.begin();
  Serial.println("Web server started on port 80");
}

void WebServerModule::loop(){
  server.handleClient();
}
