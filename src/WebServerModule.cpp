#include "WebServerModule.h"
#include <WebServer.h>
#include <SD.h>
#include <WiFi.h>
#include "PlayerController.h"

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
  String ip = WiFi.localIP().toString();
  String page = "<html><head><meta charset=\"utf-8\"><title>MP3 Player</title>"
                "<style>body{font-family:Arial,Helvetica,sans-serif;margin:12px} table{border-collapse:collapse;width:100%} td,th{padding:6px;border-bottom:1px solid #ddd} button{margin:2px;padding:6px}</style></head><body>";
  page += "<h2>MP3 Player</h2>";
  page += "<p>Device IP: " + ip + "</p>";
  page += "<div id=\"controls\">";
  page += "<button id=\"btnPlay\">Play</button>";
  page += "<button id=\"btnStop\">Stop</button>";
  page += "<button id=\"btnPrev\">Prev</button>";
  page += "<button id=\"btnNext\">Next</button>";
  page += " Loop:<input type=checkbox id=loopbox>";
  page += " Volume:<input id=vol type=range min=0 max=21 value=12 style=\"vertical-align:middle\">";
  page += "</div>";
  page += "<h3>Playlist</h3>";
  page += "<div id=\"playlist\">Loading...</div>";
  page += "<div id=\"status\" style=\"margin-top:8px;color:#333\"></div>";
  page += "<script>\n";
  page += "var current='';\n";
  page += "function fetchList(){\n";
  page += "  fetch('/api/list').then(r=>r.json()).then(arr=>{\n";
  page += "    var html='<table><tr><th>#</th><th>Title</th><th>Actions</th></tr>';\n";
  page += "    for(var i=0;i<arr.length;i++){\n";
  page += "      var n=escapeHtml(arr[i]);\n";
  page += "      html += '<tr'+(arr[i]==current?' style=\\\"background:#f0f8ff\\\"':'')+'>');\n";
  page += "      html += '<td>'+(i+1)+'</td>';\n";
  page += "      html += '<td>'+n+'</td>';\n";
  page += "      html += '<td>'+'<button onclick=\"playFile(\\\''+encodeURIComponent(arr[i])+'\\\')\">Play</button>'+";
  page += "              ' <a href=\"/music?file='+encodeURIComponent(arr[i])+'\" target=_blank>Stream</a>'+";
  page += "              ' <button onclick=\"playIndex('+i+')\">Play#</button>'+'</td>';\n";
  page += "      html += '</tr>';\n";
  page += "    }\n";
  page += "    html += '</table>';\n";
  page += "    document.getElementById('playlist').innerHTML = html;\n";
  page += "  });\n";
  page += "}\n";
  page += "function playFile(fn){ fetch('/api/play?file='+fn).then(r=>status()); }\n";
  page += "function playIndex(i){ fetch('/api/list').then(r=>r.json()).then(arr=>{ if(i>=0 && i<arr.length) fetch('/api/play?file='+encodeURIComponent(arr[i])).then(r=>status()); }); }\n";
  page += "function status(){ fetch('/api/status').then(r=>r.json()).then(j=>{ current=j.current; document.getElementById('status').innerText='Playing:'+j.current+' | Volume:'+j.volume+' | Loop:'+j.loop; document.getElementById('vol').value=j.volume; document.getElementById('loopbox').checked=j.loop; fetchList(); }); }\n";
  page += "function escapeHtml(s){ return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'); }\n";
  page += "document.getElementById('btnPlay').addEventListener('click', ()=>fetch('/api/play_current').then(r=>status()));\n";
  page += "document.getElementById('btnStop').addEventListener('click', ()=>fetch('/api/stop').then(r=>status()));\n";
  page += "document.getElementById('btnPrev').addEventListener('click', ()=>fetch('/api/prev').then(r=>status()));\n";
  page += "document.getElementById('btnNext').addEventListener('click', ()=>fetch('/api/next').then(r=>status()));\n";
  page += "document.getElementById('vol').addEventListener('input', e=>fetch('/api/volume?val='+e.target.value));\n";
  page += "document.getElementById('loopbox').addEventListener('change', e=>fetch('/api/toggleLoop?val='+(e.target.checked?1:0)).then(r=>status()));\n";
  page += "status(); setInterval(status,3000);\n";
  page += "</script>";
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
  server.on("/api/play_current", [](){ server.send(200, "application/json", "{\"ok\":true}"); });
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
