/*
ESP8266_ArtNetNode v2.1.0
Copyright (c) 2016, Matthew Tong
Copyright (c) 2023, Richard Case
https://github.com/casesolved-co-uk/ESP8266_ArtNetNode_v2

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.
If not, see http://www.gnu.org/licenses/
*/


const char scene_msg[] PROGMEM = "Not outputting, %u Scenes Recorded, %u of %u bytes used";
const char err_generic[] = "Failed to save data. Reload page and try again.";
const char err_2dmx_in[] = "Cannot have DMX Input on two channels";
const char err_no_dmxin_rdm[] = "Cannot mix DMX Input and RDM";
const char err_dmxin_artnet[] = "DMX Input only supports Artnet currently";
const char err_outofrange[] = "Value out of range";
const char err_wrongtype[] = "Wrong type";
const char* err_msg;
const char success[] = "success";
const char message[] = "message";
const char busy[] = "{\"success\":0,\"message\":\"Device busy\"}";

volatile uint8_t inAJAX = 0;

AsyncCallbackJsonWebHandler* ajaxHandler = new AsyncCallbackJsonWebHandler("/ajax", [](AsyncWebServerRequest *request, JsonVariant jsin) {
  // Gets reused as reply
  JsonObject json = jsin.as<JsonObject>();
  String reply;

  if (inAJAX++) {
    request->send(503, JSON_MIMETYPE, busy);
    return;
  }

  // Handle request to reboot into update mode
  if (json["doUpdate"]) {
    uint8_t updateStage = (uint8_t)json["doUpdate"];
    json.clear();
    json[success] = 1;
    json[message] = "Entering update mode...";
    json["doUpdate"] = 1;

    deviceSettings[doFirmwareUpdate] = 1;
    if (updateStage == 1) {
      doSave = 1;
      doReboot = 1;
    }

  // Handle reboots
  } else if (json["reboot"]) {
    json.clear();
    json[success] = 1;
    json[message] = "Device restarting...";
    
    doReboot = 1;

  // Handle load and save of data
  } else if (json.containsKey("section")) {
    uint8_t section = json["section"];
    if (ajaxSave(section, json)) {
      json.clear();
      ajaxLoad(section, json);
      if (!json.containsKey(message))
        json[message] = "Settings saved!";
    } else {
      json.clear();
      json[success] = 0;
      json[message] = err_msg;
      debugLog(LOG_ERROR, "ajaxSave", err_msg);
    }
  } else {
    json.clear();
    json[success] = 0;
    json[message] = "Invalid request";
    serializeJson(json, reply);
    request->send(400, JSON_MIMETYPE, reply);
    return;
  }

  serializeJson(json, reply);
  request->send(200, JSON_MIMETYPE, reply);
  inAJAX = 0;
});

int starts_with(const char *str, const char *prefix) {
  size_t str_len = strlen(str);
  size_t prefix_len = strlen(prefix);

  return (str_len >= prefix_len) &&
         (!memcmp(str, prefix, prefix_len));
}

int ends_with(const char *str, const char *suffix) {
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);

  return (str_len >= suffix_len) &&
         (!memcmp(str + str_len - suffix_len, suffix, suffix_len));
}

bool ajaxSave(uint8_t section, JsonObject json) {
  const char* key;
  bool ret = false;
  bool reboot = false;
  err_msg = err_generic;

  log_meminfo("before_save");

  // checks new values from the input
  for (JsonPair kv : json) {
    key = kv.key().c_str();
    if (deviceSettings.containsKey(key)) {

      // Should do a lot of input checking here!
      if (strcmp(key, portAmode) == 0 && (uint8_t)kv.value() == TYPE_DMX_IN) {
        if ((uint8_t)deviceSettings[portBmode] == TYPE_DMX_IN) {
          err_msg = err_2dmx_in;
          return false;
        } else if ((uint8_t)deviceSettings[portBmode] == TYPE_RDM_OUT) {
          err_msg = err_no_dmxin_rdm;
          return false;
        } else if (json.containsKey(portAprot) && (uint8_t)json[portAprot] != PROT_ARTNET) {
          err_msg = err_dmxin_artnet;
          return false;
        }
      } else if (strcmp(key, portBmode) == 0 && (uint8_t)kv.value() == TYPE_DMX_IN) {
        if ((uint8_t)deviceSettings[portAmode] == TYPE_DMX_IN) {
          err_msg = err_2dmx_in;
          return false;
        } else if ((uint8_t)deviceSettings[portAmode] == TYPE_RDM_OUT) {
          err_msg = err_no_dmxin_rdm;
          return false;
        } else if (json.containsKey(portBprot) && (uint8_t)json[portBprot] != PROT_ARTNET) {
          err_msg = err_dmxin_artnet;
          return false;
        }
      } else if(starts_with(key, "port")) {
        if(ends_with(key, "net") && (uint8_t)kv.value() >= 128) {
          err_msg = err_outofrange;
          return false;
        } else if(ends_with(key, "sub") && (uint8_t)kv.value() >= 16) {
          err_msg = err_outofrange;
          return false;
        } else if(ends_with(key, "uni")) {
          for (uint8_t x = 0; x < 4; x++) {
            if(ends_with(key, "sACNuni") && (kv.value()[x] <= 0 || kv.value()[x] >= 64000) ) {
              err_msg = err_outofrange;
              return false;
            } else if(ends_with(key, "uni") && kv.value()[x] >= 16 ) {
              err_msg = err_outofrange;
              return false;
            }
          }
        } else if(ends_with(key, "numPix") && (uint16_t)kv.value() > 680) {
          err_msg = err_outofrange;
          return false;
        }
      }
      // gets saved at function end
    }
  }

  switch (section) {
    case 1:     // Device Status
      // We don't need to save anything for this.  Go straight to load
      return true;
      break;

    case 2:     // Wifi
      ret = true;
      break;

    case 3:     // IP Address & Node Name
      if (!isHotspot && json.containsKey(dhcpEnable) && (uint8_t)json[dhcpEnable] != (uint8_t)deviceSettings[dhcpEnable]) {
        
        if ((bool)json[dhcpEnable]) {
          enable_dhcp();
        }
        reboot = true;
      }

      if (!isHotspot) {
        artRDM.setShortName((const char*)deviceSettings[nodeName]);
        artRDM.setLongName((const char*)deviceSettings[longName]);
      }
      ret = true;
      break;

    case 0xA:     // Port A
      {
        bool e131 = ((uint8_t)json[portAprot] == PROT_ARTNET_SACN) ? true : false;
        for (uint8_t x = 0; x < 4; x++) {
          artRDM.setE131(portA[0], portA[x+1], e131);
          artRDM.setE131Uni(portA[0], portA[x+1], json[portAsACNuni][x]);
        }

        if (deviceSettings[portAmode] != json[portAmode]) {
          reboot = true;
        }

        // Update the Artnet class
        artRDM.setNet(portA[0], json[portAnet]);
        artRDM.setSubNet(portA[0], json[portAsub]);
        artRDM.setUni(portA[0], portA[1], json[portAuni][0]);
        artRDM.setMerge(portA[0], portA[1], json[portAmerge]);

        if ((uint8_t)json[portAmode] >= LED_MODE_START && !reboot) {
          if (  (uint16_t)deviceSettings[portAnumPix] != (uint16_t)json[portAnumPix] ||
                (uint8_t)deviceSettings[portApixMode] != (uint8_t)json[portApixMode]
          ) {
            reboot = true;
          }
        }
        ret = true;
        break;
      }

    case 0xB:     // Port B
      #ifndef ONE_PORT
      {
        bool e131 = ((uint8_t)json[portBprot] == PROT_ARTNET_SACN) ? true : false;
        for (uint8_t x = 0; x < 4; x++) {
          artRDM.setE131(portB[0], portB[x+1], e131);
          artRDM.setE131Uni(portB[0], portB[x+1], json[portBsACNuni][x]);
        }

        if (deviceSettings[portBmode] != json[portBmode]) {
          reboot = true;
        }

        // Update the Artnet class
        artRDM.setNet(portB[0], (uint8_t)json[portBnet]);
        artRDM.setSubNet(portB[0], (uint8_t)json[portBsub]);
        artRDM.setUni(portB[0], portB[1], json[portBuni][0]);
        artRDM.setMerge(portB[0], portB[1], (uint8_t)json[portBmerge]);

        if ((uint8_t)json[portBmode] >= LED_MODE_START && !reboot) {
          if (  (uint16_t)deviceSettings[portBnumPix] != (uint16_t)json[portBnumPix] ||
                (uint8_t)deviceSettings[portBpixMode] != (uint8_t)json[portBpixMode]
          ) {
            reboot = true;
          }
        }
        ret = true;
        break;
      }
      #endif

    case 4:     // Scenes
      // Not yet implemented
      
      ret = true;
      break;

    case 5:     // Firmware
      // Doesn't come here
      return true;
      break;

    case 6:     // Miscellaneous
      if ((bool)json["debugLogClear"]) {
        return debugLogClear();
      }
      ret = true;
      break;

    default:
      // Catch errors
      return false;
  }

  // assigns new values from the input
  for (JsonPair kv : json) {
    key = kv.key().c_str();
    if (deviceSettings.containsKey(key)) {
      if (sameType(deviceSettings[key], kv.value())) {
        deviceSettings[key].set(kv.value());
      } else {
        err_msg = err_wrongtype;
        // Will print all errors not just first
        debugLog(LOG_ERROR, key, err_wrongtype);
        ret = false;
      }
    } // fail silently to save success, page, etc
  }

  // make sure reboot doesn't start before save
  doSave = ret;
  doReboot = reboot;

  return ret;
}

// Only reply with data not available from the settings.json
void ajaxLoad(uint8_t section, JsonObject jsonReply) {
  // Get MAC Address
  char MAC_char[30] = "";
  for (int i = 0; i < 6; ++i) {
    sprintf(&MAC_char[i * 3], "%02X", MAC_array[i]);
    if(i<5) strcat(MAC_char, ":");
  }

  jsonReply["macAddress"] = MAC_char;
  jsonReply[success] = 1;

  switch (section) {
    case 1:     // Device Status
    {
      char buf[160];
      jsonReply["isHotspot"] = isHotspot;

      FSInfo fs_info;
      SPIFFS.info(fs_info);
      sprintf_P(buf, scene_msg, 0, fs_info.usedBytes, fs_info.totalBytes);
      jsonReply["sceneStatus"] = buf;
      jsonReply["firmwareStatus"] = FIRMWARE_VERSION;
      jsonReply[message] = "Settings Loaded!";
      break;
    }

    case 5:     // Firmware
      jsonReply["firmwareStatus"] = FIRMWARE_VERSION;
      break;
  }
}

// type check ArduinoJson variant values
bool sameType(JsonVariant value1, JsonVariant value2) {
  if (  (value1.is<float>() && value2.is<float>()) ||
        (value1.is<const char*>() && value2.is<const char*>()) ||
        (value1.is<bool>() && value2.is<bool>()) ||
        (value1.is<JsonArray>() && value2.is<JsonArray>()) ||
        (value1.is<JsonObject>() && value2.is<JsonObject>()) )
  {
    return true;
  }
  return false;
}
