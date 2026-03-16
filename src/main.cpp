#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Audio.h>
#include "WiFiManager.h"
#include "DisplayManager.h"
#include "WebServerModule.h"

// I2S pins (adjust if your board wiring differs)
#define I2S_DOUT      39
#define I2S_BCLK      40
#define I2S_LRC       41

// SD pins (match your wiring)
#define SD_CS         9
#define SD_SCK        14
#define SD_MISO       17
#define SD_MOSI       10

SPIClass SD_SPI(HSPI);

Audio audio;
const int AUDIO_VOLUME = 12; // 0..21

String songs[128];
int song_count = 0;

void scanSD(){
  song_count = 0;
  File root = SD.open("/");
  if(!root){ Serial.println("SD open / failed"); return; }
  File file = root.openNextFile();
  while(file){
    if(!file.isDirectory()){
      const char *fname = file.name();
      int len = strlen(fname);
      if(len>4){
        const char *ext = &fname[len-4];
        if(strcasecmp(ext, ".mp3")==0){
          if(song_count<128) songs[song_count++] = String(fname);
          Serial.printf("Found: %s\n", fname);
        }
      }
    }
    file = root.openNextFile();
  }
}

void setup(){
  Serial.begin(115200);
  delay(200);
  Serial.println("mp3_player (independent): start");

  // 初始化模块
  WiFiManager wifi;
  DisplayManager display;
  WebServerModule web;

  display.begin();
  wifi.begin("Office", "13906831000");
  display.showIP(wifi.getIP());
  web.begin();

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SD_SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  SD_SPI.setFrequency(1000000); // start with 1MHz for compatibility

  if(!SD.begin(SD_CS, SD_SPI)){
    Serial.println("Card Mount Failed");
  } else {
    Serial.println("Card Mount Success");
    uint8_t cardType = SD.cardType();
    Serial.printf("SD card type: %d\n", cardType);
    scanSD();
  }

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(AUDIO_VOLUME);

  if(song_count>0){
    randomSeed(micros());
    int idx = random(song_count);
    const char *sel = songs[idx].c_str();
    Serial.printf("Playing SD: %s\n", sel);
    audio.connecttoFS(SD, sel);
  } else {
    Serial.println("No MP3 found on SD.");
  }
}

void loop(){
  audio.loop();
  // 让 web server 处理请求
  WebServerModule ws; ws.loop();
  delay(5);
}
