#define ENABLE_LITTLEFS__ 1
#if ENABLE_LITTLEFS__
//#include <Arduino.h>
#include <FS.h> // SPIFFS is declared
#include <LittleFS.h> // LittleFS is declared
//#include "SDFS.h" // SDFS is declared

#define FORMAT_LITTLEFS_IF_FAILED true

#define FILE_SETTING "/settings.ini"

static char wlssid[33] = {0};
static char wlkey[20] = {0};

int load_config() {
  File fp = LittleFS.open(FILE_SETTING, "r");
  if (!fp) {
    Serial.println("Failed to load_conf");
    return -1;
  }
    
  while (fp.available()) {
    String line = fp.readStringUntil('\n');
    line.trim(); // Remove leading/trailing whitespace
        
    // Ignore comments and empty lines
    if (line.startsWith("#") || line.length() == 0) {
      continue;
    }

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
    
  // Close the file
  fp.close();
  Serial.println("FILE_SETTING loaded");
  return 0;
}

void save_config() {
  File fp = LittleFS.open(FILE_SETTING, "r");
  if (!fp) {
    Serial.println("Failed to save_config");
    return;
  }
  fp.print("wlssid="); fp.println(wlssid);
  fp.print("wlkey="); fp.println(wlkey);
  delay(10);
  fp.close();
  Serial.println("FILE_SETTING saved");
}

const char *get_wifi_config(const char*kname, const char *defval) {
  if(!strcmp(kname, "wlssid")) {
    return wlssid;
  }
  else if(!strcmp(kname, "wlkey")) {
    return wlkey;
  }
  return defval;
}

void set_wifi_config(const char*ssid, const char *key) {
  strncpy(wlssid, ssid, 32);
  strncpy(wlkey, key, 16);
}

void setup_fs() {
  File FFfile;

  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("LITTLEFS Mount failed");
    return;
  }

  if( LittleFS.exists(FILE_SETTING) ) {
    FFfile = LittleFS.open(FILE_SETTING, "r");
    if (!FFfile) {
      Serial.println("Failed to open/r FILE_SETTINGS");
      return;
    }
  }
  else {
    //LittleFS.format();
    FFfile = LittleFS.open(FILE_SETTING, "w");
    if (!FFfile) {
      Serial.println("Failed to open/w FILE_SETTINGS");
      return;
    }
  }

#if 0
  FSInfo fs_info;
  if( LittleFS.info(fs_info) ) {
    Serial.print("fs_info.totalBytes: "); Serial.println(fs_info.totalBytes);
    Serial.print("fs_info.usedBytes: "); Serial.println(fs_info.usedBytes);
    Serial.print("fs_info.blockSize: "); Serial.println(fs_info.blockSize);
    Serial.print("fs_info.pageSize: "); Serial.println(fs_info.pageSize);
  }
  else {
    Serial.println("failed to get fs_info");
  }
#endif 

  load_config();
  //This method unmounts the file system. Use this method before updating the file system using OTA.
  //LittleFS.end();
}


#endif //Endof_ENABLE_LITTLEFS__
