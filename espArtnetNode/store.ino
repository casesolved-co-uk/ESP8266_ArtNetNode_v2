/*
 * Copyright (c) 2023, Richard Case
 * 
 * Manage deviceSettings as json using ArduinoJson v6
 */

#include "store.h"

const char settings_json[] = "/settings.json";
const char defaults_json[] = "/defaults.json";
const char files[][20] = {
  "/defaults.json",
  "/index.html",
  "/style.css",
  "/languages.js",
  "/controller.js"
};
const char store[] = "store";

void FS_start() {
  CRGB::HTMLColorCode colour = CRGB::Green;
  bool okay;
  // Start SPIFFS file system, will be formatted if it does not exist
  SPIFFS.begin();

  debugLogSetup();

  okay = validate_FS(&colour);
  //DEBUG_MSG("Status LED Colour: 0x%06X", colour);
  setStatusLed(STATUS_LED_S, colour);
  doStatusLedOutput();

  // Switch OS UART logging:
  // DMXlib does this:
  system_set_os_print(0);
  //ets_install_putc1((void *) &uart_ignore_char);
  // func needs to be in IRAM:
  ets_install_putc1((void *) &os_log);
  //os_install_putc1(os_log);

  // start with basic config for debug.log
  if (!okay) {
    webSetup();
    wifiStart();
    while(1) {
      delay(1000);
    }
  }

  //log_ls("/");
}

// check for essential files
bool validate_FS(CRGB::HTMLColorCode* colour) {
  File f;
  const char *fn;
  bool ret = true;

  for (uint8_t i = 0; i < sizeof(files) / 20; i++) {
    fn = files[i];
    if (!SPIFFS.exists(fn)) {
      *colour = CRGB::Orange;
      debugLog_P(LOG_ERROR, fn, PSTR("Not found!"));
      ret = false;
      continue;
    }
    f = SPIFFS.open(fn, "r");
    if (f) {
      if (f.size() < 400) {
        *colour = CRGB::Purple;
        debugLog_P(LOG_ERROR, fn, PSTR("Size < 400 bytes!"));
        ret = false;
      }
      f.close();
    } else {
      *colour = CRGB::Indigo;
      debugLog_P(LOG_ERROR, fn, PSTR("Did not open!"));
      ret = false;
    }
  }
  return ret;
}

/*  Whenever new values are saved, update other variables
 *  IP address can be set by Artnet RDM, DHCP or user
 */
void conversions(bool init) {
  // skip if deviceSettings not initialised properly (FS not available)
  if (!deviceSettings.containsKey(dhcpEnable)) {
    return;
  }

  if (!init && (uint8_t)deviceSettings[dhcpEnable]) {
    if (isHotspot) {
      deviceSettings[hotspotIp][0] = ip[0];
      deviceSettings[hotspotIp][1] = ip[1];
      deviceSettings[hotspotIp][2] = ip[2];
      deviceSettings[hotspotIp][3] = ip[3];
      deviceSettings[hotspotSubnet][0] = subnet[0];
      deviceSettings[hotspotSubnet][1] = subnet[1];
      deviceSettings[hotspotSubnet][2] = subnet[2];
      deviceSettings[hotspotSubnet][3] = subnet[3];
      deviceSettings[gwAddress][0] = 0;
      deviceSettings[gwAddress][1] = 0;
      deviceSettings[gwAddress][2] = 0;
      deviceSettings[gwAddress][3] = 0;
    } else {
      deviceSettings[ipAddress][0] = ip[0];
      deviceSettings[ipAddress][1] = ip[1];
      deviceSettings[ipAddress][2] = ip[2];
      deviceSettings[ipAddress][3] = ip[3];
      deviceSettings[subAddress][0] = subnet[0];
      deviceSettings[subAddress][1] = subnet[1];
      deviceSettings[subAddress][2] = subnet[2];
      deviceSettings[subAddress][3] = subnet[3];
      deviceSettings[gwAddress][0] = gateway[0];
      deviceSettings[gwAddress][1] = gateway[1];
      deviceSettings[gwAddress][2] = gateway[2];
      deviceSettings[gwAddress][3] = gateway[3];
    }
  }
  else if (isHotspot) {
    ip = IPAddress((uint8_t)deviceSettings[hotspotIp][0],(uint8_t)deviceSettings[hotspotIp][1],(uint8_t)deviceSettings[hotspotIp][2],(uint8_t)deviceSettings[hotspotIp][3]);
    subnet = IPAddress((uint8_t)deviceSettings[hotspotSubnet][0],(uint8_t)deviceSettings[hotspotSubnet][1],(uint8_t)deviceSettings[hotspotSubnet][2],(uint8_t)deviceSettings[hotspotSubnet][3]);
    // self ip is used as gateway in hotspot mode
    // INADDR_NONE is a constant IPAddress value from arduino/IPAddress.h
    gateway = INADDR_NONE;
  }
  else {
    ip = IPAddress((uint8_t)deviceSettings[ipAddress][0],(uint8_t)deviceSettings[ipAddress][1],(uint8_t)deviceSettings[ipAddress][2],(uint8_t)deviceSettings[ipAddress][3]);
    subnet = IPAddress((uint8_t)deviceSettings[subAddress][0],(uint8_t)deviceSettings[subAddress][1],(uint8_t)deviceSettings[subAddress][2],(uint8_t)deviceSettings[subAddress][3]);
    gateway = IPAddress((uint8_t)deviceSettings[gwAddress][0],(uint8_t)deviceSettings[gwAddress][1],(uint8_t)deviceSettings[gwAddress][2],(uint8_t)deviceSettings[gwAddress][3]);
  }
  broadcast = (uint32_t) ip | ~((uint32_t) subnet);
  // write calculated broadcast address back out
  deviceSettings[bcAddress][0] = broadcast[0];
  deviceSettings[bcAddress][1] = broadcast[1];
  deviceSettings[bcAddress][2] = broadcast[2];
  deviceSettings[bcAddress][3] = broadcast[3];
  dmxBroadcast = IPAddress((uint8_t)deviceSettings[dmxInBroadcast][0],(uint8_t)deviceSettings[dmxInBroadcast][1],(uint8_t)deviceSettings[dmxInBroadcast][2],(uint8_t)deviceSettings[dmxInBroadcast][3]);

  // skip the # character from the html colour code
  portAcorrect = strtoul(&((const char*)deviceSettings[portAcorrection])[1], NULL, 16);
  portBcorrect = strtoul(&((const char*)deviceSettings[portBcorrection])[1], NULL, 16);
  portAtemperature = strtoul(&((const char*)deviceSettings[portAtemp])[1], NULL, 16);
  portBtemperature = strtoul(&((const char*)deviceSettings[portBtemp])[1], NULL, 16);

  // flag that deviceSettings is valid
  settingsLoaded = true;
}

/*  Settings philosophy
 *  Every modification (write by ajax) is first written to deviceSettings object, 
 *  then saved to both EEPROM and SPIFFS settings file.
 *  Startup: settings are loaded from EEPROM and SPIFFS file is assumed the same
 */
// can't be used in callback - settingsWrite delay
void save() {
  if (doSave) {
    conversions();
    settingsWrite();
    eepromSave();
    delay(10);
    log_meminfo("after_save");
    deviceSettings.garbageCollect();
    // set last so reboot cannot happen before save completes
    doSave = false;
  }
}

// ensure settings are written to json file before calling
bool eepromSave() {
  File f = SPIFFS.open(settings_json, "r");
  uint16_t idx = 0;
  uint8_t c;
  bool success = false;

  if (f) {
    EEPROM.begin(CONFIG_SIZE);
    while (f.available()) {
      c = f.read();
      EEPROM.write(CONFIG_START + idx, c);
      //DEBUG_MSG("%c", (char)c);
      idx++;
    }
    //DEBUG_LN("");
  
    f.close();
    success = EEPROM.commit();
    EEPROM.end();
    if (success) {
      debugLog_P(LOG_INFO, store, PSTR("EEPROM Committed"));
    } else {
      debugLog_P(LOG_ERROR, store, PSTR("EEPROM Commit Failed"));
    }
  }
  return success;
}

DeserializationError deserialize(const char* buf, size_t len) {
  DeserializationError err = deserializeJson(deviceSettings, buf, len);
  if (err) {
    debugLog(LOG_ERROR, "deserialize", err.c_str());
  } else {
    // recalculate the compiled port count to override defaults
    #ifdef ONE_PORT
      deviceSettings[numPorts] = 1;
    #else
      deviceSettings[numPorts] = 2;
    #endif
  }
  return err;
}

void eepromLoad() {
  uint8_t buf[CONFIG_SIZE];
  DeserializationError err;
  bool config_valid = false;

  //DEBUG_MSG("Flash sector size: %i\n", SPI_FLASH_SEC_SIZE);

  // Reads CONFIG_SIZE into RAM
  EEPROM.begin(CONFIG_SIZE);
  EEPROM.get(CONFIG_START, buf);
  EEPROM.end();

  err = deserialize((char*)buf, CONFIG_SIZE);
  if (err) {
    DEBUG_LN((char*)buf);
  } else {
    config_valid = strcmp((const char*)deviceSettings["version"], CONFIG_VERSION) == 0;
  }

  if (config_valid) {
    conversions(true);
    debugLog_P(LOG_INFO, store, PSTR("Loaded EEPROM"));
  } else {
    loadDefaults(buf, CONFIG_SIZE);
  }
}

void settingsWrite() {
  File f = SPIFFS.open(settings_json, "w");
  serializeJson(deviceSettings, f);
  f.close();
  debugLog_P(LOG_INFO, store, PSTR("Saved settings.json"));
}

// use existing buffer
void loadDefaults(uint8_t* buf, size_t buf_len) {
  DeserializationError err;
  size_t len;
  File f = SPIFFS.open(defaults_json, "r");

  if (f) {
    len = min(buf_len, f.size());
    f.read(buf, len);
    f.close();
    err = deserialize((char*)buf, len);
  
    if (!err) {
      // equivalent of save()
      settingsWrite();
      conversions(true);
      if (eepromSave())
        debugLog_P(LOG_INFO, store, PSTR("Loaded defaults"));
    }
  } else {
    debugLog_P(LOG_ERROR, "loadDefaults", PSTR("defaults.json not found"));
    delay(100);
    ESP.deepSleep(0);
  }

  // erase the ESP wifi config to clear away any residue
  ESP.eraseConfig();
  delay(1000);
  // Give visual indication of restart
  digitalWrite(LED_BUILTIN, HIGH);
  DEBUG_LN("Sleeping...");
  delay(1000);
  ESP.deepSleep(0);
  //ESP.restart();
}
