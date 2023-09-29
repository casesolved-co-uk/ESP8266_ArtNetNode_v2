/*
 * Copyright (c) 2023, Richard Case
 * 
 * Manage deviceSettings as json using ArduinoJson v6
 */

#include <ArduinoJson.h> // v6
#include "store.h"

const char settings_json[] = "/settings.json";
const char defaults_json[] = "/defaults.json";


/*  Whenever new values are saved, update other variables
 *  IP address can be set by Artnet RDM, DHCP or user
 */
void conversions() {
  if ((uint8_t)deviceSettings[dhcpEnable]) {
    if (isHotspot) {
      deviceSettings[hotspotIp][0] = ip[0];
      deviceSettings[hotspotIp][1] = ip[1];
      deviceSettings[hotspotIp][2] = ip[2];
      deviceSettings[hotspotIp][3] = ip[3];
      deviceSettings[hotspotSubnet][0] = subnet[0];
      deviceSettings[hotspotSubnet][1] = subnet[1];
      deviceSettings[hotspotSubnet][2] = subnet[2];
      deviceSettings[hotspotSubnet][3] = subnet[3];
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
    ip = IPAddress(deviceSettings[hotspotIp][0],deviceSettings[hotspotIp][1],deviceSettings[hotspotIp][2],deviceSettings[hotspotIp][3]);
    subnet = IPAddress(deviceSettings[hotspotSubnet][0],deviceSettings[hotspotSubnet][1],deviceSettings[hotspotSubnet][2],deviceSettings[hotspotSubnet][3]);
    // self ip is used as gateway in hotspot mode
    gateway = INADDR_NONE;
  }
  else {
    ip = IPAddress(deviceSettings[ipAddress][0],deviceSettings[ipAddress][1],deviceSettings[ipAddress][2],deviceSettings[ipAddress][3]);
    subnet = IPAddress(deviceSettings[subAddress][0],deviceSettings[subAddress][1],deviceSettings[subAddress][2],deviceSettings[subAddress][3]);
    gateway = IPAddress(deviceSettings[gwAddress][0],deviceSettings[gwAddress][1],deviceSettings[gwAddress][2],deviceSettings[gwAddress][3]);
  }
  broadcast = (uint32_t) ip | ~((uint32_t) subnet);
  // write calculated broadcast address back out
  deviceSettings[bcAddress][0] = broadcast[0];
  deviceSettings[bcAddress][1] = broadcast[1];
  deviceSettings[bcAddress][2] = broadcast[2];
  deviceSettings[bcAddress][3] = broadcast[3];
  dmxBroadcast = IPAddress(deviceSettings[dmxInBroadcast][0],deviceSettings[dmxInBroadcast][1],deviceSettings[dmxInBroadcast][2],deviceSettings[dmxInBroadcast][3]);

  // skip the # character from the html colour code
  portAcorrect = strtol(&deviceSettings[portAcorrection].as<const char*>()[1], NULL, 16);
  portBcorrect = strtol(&deviceSettings[portBcorrection].as<const char*>()[1], NULL, 16);
  portAtemperature = strtol(&deviceSettings[portAtemp].as<const char*>()[1], NULL, 16);
  portBtemperature = strtol(&deviceSettings[portBtemp].as<const char*>()[1], NULL, 16);

  STATUS_BRIGHTNESS = deviceSettings[statusBright];
  MAX_BRIGHTNESS = deviceSettings[maxBright];
}

void save() {
  uint8_t buf[CONFIG_SIZE];
  conversions();
  settingsWrite(buf, CONFIG_SIZE);
  eepromSave();
  log_meminfo("after_save");
}

// ensure settings are written to json file before calling
void eepromSave() {
  File f = SPIFFS.open(settings_json, "r");
  size_t len = f.size();
  for (uint16_t t = 0; t < len; t++)
    EEPROM.write(CONFIG_START + t, f.read());

  f.close();
  EEPROM.commit();
}

DeserializationError deserialize(const uint8_t* buf, size_t len) {
  DeserializationError err = deserializeJson(deviceSettings, buf, len);
  if (err) {
    debugLog(LOG_ERROR, "store", err.c_str());
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
  uint16_t t = 0;
  bool end_found = false;
  DeserializationError err;

  while(t < CONFIG_SIZE) {
    buf[t] = EEPROM.read(CONFIG_START + t);
    if (buf[t] == '}') {
      end_found = true;
      break;
    }
    t++;
  }

  if (end_found) {
    // true if error
    if (deserialize(buf, t+1)) {
      end_found = false;
    }
  }

  if (end_found && strcmp(deviceSettings["version"], CONFIG_VERSION) == 0) {
    settingsWrite(buf, t+1);
  } else {
    loadDefaults(buf, CONFIG_SIZE);
  }
}

// use existing buffer
void settingsWrite(uint8_t* buf, size_t buf_len) {
  size_t len = serializeJson(deviceSettings, buf, buf_len);
  File f = SPIFFS.open(settings_json, "w");
  f.write(buf, len);
  f.close();
}

// use existing buffer
void loadDefaults(uint8_t* buf, size_t buf_len) {
  File f = SPIFFS.open(defaults_json, "r");
  size_t len = min(buf_len, f.size());
  f.read(buf, len);
  f.close();

  deserialize(buf, len);

  settingsWrite(buf, buf_len);

  eepromSave();
  delay(500);

  // erase the ESP wifi config to clear away any residue
  ESP.eraseConfig();
  while(1);
}
