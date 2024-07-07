#define ENABLE_LITTLEFS__ 1
#if ENABLE_LITTLEFS__
//#include <Arduino.h>
#include <FS.h> // SPIFFS is declared
#include <LittleFS.h> // LittleFS is declared
//#include "SDFS.h" // SDFS is declared

#define FORMAT_LITTLEFS_IF_FAILED true
#define FS_SETTINGS_TEST 0
#define FILE_SETTING "/settings.ini"

static char wlssid[33] = {0};
static char wlkey[20] = {0};

void writeFile(String filename, String message){
  File file = LittleFS.open(filename, "w");
  if(!file){
    Serial.println("writeFile -> failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

String readFile(String filename){
  File file = LittleFS.open(filename);
  if(!file){
    Serial.println("Failed to open file for reading");
    return "";
  }
  
  String fileText = "";
  while(file.available()){
    fileText = file.readString();
  }
  file.close();
  return fileText;
}

int load_config() {
  String confile = readFile(FILE_SETTING);
  Serial.println("Config filesz: " + String(confile.length()));

  int sp=0, ep=0;
  String line="";
  while(sp < confile.length()) {
    line == "";
    //find stringEnd
    ep = confile.indexOf('\n', sp);
    if(ep >= 0) {
      line = confile.substring(sp, ep);
      sp = ep+1;
    }
    else {
      line = confile.substring(sp);
      sp = confile.length();
    }
    // Ignore comments and empty lines
    if (line.startsWith("#") || line.length() == 0) {
      continue;
    }
    Serial.print("load_config:"); Serial.println(line);
     
    // Find the position of the '=' character
    int separatorIndex = line.indexOf('=');
    if (separatorIndex == -1) {
      Serial.print("Error Line: ");
      Serial.println(line);
      continue; // Invalid line format
    }
        
    // Extract key and value
    String key = line.substring(0, separatorIndex);
    String value = line.substring(separatorIndex + 1);
        
    key.trim();
    value.trim();
    Serial.print("Key: ");
    Serial.print(key);
    Serial.print(", Value: ");
    Serial.println(value);

    if(key == "wlssid"){
      strncpy(wlssid, value.c_str(), 32);
    }
    else if(key == "wlkey"){
      strncpy(wlkey, value.c_str(), 16);
    }
  }

  Serial.println("FILE_SETTING loaded");
  return 0;
}

void save_config(int type) {
  
  File fp = LittleFS.open(FILE_SETTING, "w");
  if (!fp) {
    Serial.println("Failed to save_config");
    return;
  }

  if(type == 911) {
    fp.println("");
    fp.close();
    LittleFS.format();
    LittleFS.end();
    Serial.println("fs Formatted");
    return;  
  }
  
  fp.print("wlssid="); fp.println(wlssid);
  fp.print("wlkey="); fp.println(wlkey);
  delay(10);
  fp.close();
  Serial.println("FILE_SETTING saved");

  if(type == 9) {
    //This method unmounts the file system. Use this method before updating the file system using OTA.
    LittleFS.end();
  }
}

const char *get_wifi_config(const char*kname, const char *defval) {
  if(!strcmp(kname, "wlssid")) {
    return wlssid[0]!= 0? wlssid:defval;
  }
  else if(!strcmp(kname, "wlkey")) {
    return wlkey[0]!=0? wlkey:defval;
  }
  return defval;
}

void set_wifi_config(const char*ssid, const char *key) {
  Serial.print("wifi_save:");
  Serial.println(ssid);
  Serial.println(key);
  strncpy(wlssid, ssid, 32);
  strncpy(wlkey, key, 16);
}

void setup_fs() {
  File FFfile;

  if (!LittleFS.begin(false)) {
    Serial.println("LITTLEFS Mount failed starting format");
    if (!LittleFS.begin(true)) {
      Serial.println("LITTLEFS mount failed, formating error");
      return;
    } else {
      Serial.println("LITTLEFS Formatted");
    }
  }

  if( LittleFS.exists(FILE_SETTING) ) {
    Serial.println("setup_fs: FILE_SETTING found");
    FFfile = LittleFS.open(FILE_SETTING, "r");
    if (!FFfile) {
      Serial.println("Failed to open/r FILE_SETTINGS");
      return;
    }
    FFfile.close();
    load_config();
  }
  else {
    //LittleFS.format();
    Serial.println("setup_fs: FILE_SETTING not found");
    FFfile = LittleFS.open(FILE_SETTING, "w");
    if (!FFfile) {
      Serial.println("Failed to open/w FILE_SETTINGS");
      return;
    }
    FFfile.close();
#if FS_SETTINGS_TEST
    Serial.println("setup_fs: FILE_SETTING testing");
    set_wifi_config("fsTestAp", "12345678");
    save_config(0);
#endif
  }

#if 1
    Serial.print("LittleFS.totalBytes: "); Serial.println(LittleFS.totalBytes());
    Serial.print("LittleFS.usedBytes: "); Serial.println(LittleFS.usedBytes());
#endif

}

#endif //Endof_ENABLE_LITTLEFS__
