#include "WebServerModule.h"
#include <WebServer.h>
#include <SD.h>
#include <WiFi.h>

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

void WebServerModule::handleRoot(){
  String ip = WiFi.localIP().toString();
  String page = "<html><head><meta charset=\"utf-8\"><title>MP3 Player</title></head><body>";
  page += "<h2>MP3 Player</h2>";
  page += "<p>Device IP: " + ip + "</p>";
  page += "<h3>Songs</h3>" + listSongsHtml();
  page += "<hr><p>Click a song to play in browser (stream)</p>";
  page += "</body></html>";
  server.send(200, "text/html", page);
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
  server.on("/", [](){
    WebServerModule ws; ws.handleRoot();
  });
  server.on("/music", [](){
    WebServerModule ws; ws.handleMusic();
  });
  server.begin();
  Serial.println("Web server started on port 80");
}

void WebServerModule::loop(){
  server.handleClient();
}
