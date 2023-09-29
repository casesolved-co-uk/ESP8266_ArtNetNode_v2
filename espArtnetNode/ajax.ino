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
#include <AsyncJson.h>
#include <ArduinoJson.h> // v6

const char scene_msg[] = "Not outputting<br />%u Scenes Recorded<br />%u of %u bytes used";
const char err_generic[] = "Failed to save data. Reload page and try again.";
const char err_2dmx_in[] = "Cannot have DMX Input on two channels";
const char err_no_dmxin_rdm[] = "Cannot mix DMX Input and RDM";
const char err_dmxin_artnet[] = "DMX Input only supports Artnet currently";
const char err_outofrange[] = "Value out of range";
const char* err_msg;
const char success[] = "success";
const char message[] = "message";

AsyncCallbackJsonWebHandler* ajaxHandler = new AsyncCallbackJsonWebHandler("/ajax", [](AsyncWebServerRequest *request, JsonVariant jsin) {
  // Gets reused as reply
  JsonObject json = jsin.as<JsonObject>();
  String reply;
  size_t jsonSz = json.size();

  // Handle request to reboot into update mode
  if (json["doUpdate"]) {
    json.clear();
    json[success] = 1;
    json[message] = "Entering update mode...";
    json["doUpdate"] = 1;

    artRDM.end();
    // Turn pixel strips off if they're on
    stop_LEDs();
    deviceSettings[doFirmwareUpdate] = 1;
    save();
    doReboot = true;

  // Handle reboots
  } else if (json["reboot"]) {
    json.clear();
    json[success] = 1;
    json[message] = "Device restarting...";
    
    artRDM.end();
    // Turn pixel strips off if they're on
    stop_LEDs();
    
    doReboot = true;

  // Handle load and save of data
  } else if (json["page"]) {
    uint8_t page = json["page"];

    if (jsonSz <= 2 || ajaxSave(page, json)) {
      json.clear();
      ajaxLoad(page, json);

      if (jsonSz > 2)
        json[message] = "Settings saved!";

    } else {
      json.clear();
      json[success] = 0;
      json[message] = err_msg;
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

bool ajaxSave(uint8_t page, JsonObject json) {
  const char* key;
  bool ret = false;
  err_msg = err_generic;

  log_meminfo("before_save");

  // checks new values from the input
  for (JsonPair kv : json) {
    key = kv.key().c_str();
    if (deviceSettings.containsKey(key)) {

      // Should do a lot of input checking here!

      if (strcmp(key, portAmode) == 0 && (uint8_t)json[key] == TYPE_DMX_IN) {
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
      } else if (strcmp(key, portBmode) == 0 && (uint8_t)json[key] == TYPE_DMX_IN) {
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
        if(ends_with(key, "net") && (uint8_t)json[key] >= 128) {
          err_msg = err_outofrange;
          return false;
        } else if(ends_with(key, "sub") && (uint8_t)json[key] >= 16) {
          err_msg = err_outofrange;
          return false;
        } else if(ends_with(key, "uni")) {
          for (uint8_t x = 0; x < 4; x++) {
            if(ends_with(key, "sACNuni") && (json[key][x] <= 0 || json[key][x] >= 64000) ) {
              err_msg = err_outofrange;
              return false;
            } else if(ends_with(key, "uni") && json[key][x] >= 16 ) {
              err_msg = err_outofrange;
              return false;
            }
          }
        } else if(ends_with(key, "numPix") && (uint16_t)json[key] > 680) {
          err_msg = err_outofrange;
          return false;
        }
      }
      // gets saved at function end
    }
  }

  switch (page) {
    case 1:     // Device Status
      // We don't need to save anything for this.  Go straight to load
      return true;
      break;

    case 2:     // Wifi
      ret = true;
      break;

    case 3:     // IP Address & Node Name
      if (!isHotspot && json.containsKey(dhcpEnable) && (bool)json[dhcpEnable] != (bool)deviceSettings[dhcpEnable]) {
        
        if ((bool)json[dhcpEnable]) {
          enable_dhcp();
        }
        doReboot = true;
      }

      if (!isHotspot) {
        artRDM.setShortName(deviceSettings[nodeName].as<const char*>());
        artRDM.setLongName(deviceSettings[longName].as<const char*>());
      }
      ret = true;
      break;

    case 4:     // Port A
      {
        bool e131 = ((uint8_t)json[portAprot] == PROT_ARTNET_SACN) ? true : false;
        for (uint8_t x = 0; x < 4; x++) {
          artRDM.setE131(portA[0], portA[x+1], e131);
          artRDM.setE131Uni(portA[0], portA[x+1], json[portAsACNuni][x]);
        }

        if (deviceSettings[portAmode] != json[portAmode]) {
          doReboot = true;
        }

        // Update the Artnet class
        artRDM.setNet(portA[0], json[portAnet]);
        artRDM.setSubNet(portA[0], json[portAsub]);
        artRDM.setUni(portA[0], portA[1], json[portAuni][0]);
        artRDM.setMerge(portA[0], portA[1], json[portAmerge]);

        if ((uint8_t)json[portAmode] >= LED_MODE_START && !doReboot) {
          if ((uint16_t)deviceSettings[portAnumPix] != (uint16_t)json[portAnumPix]) {
            doReboot = true;
          }
        }
        ret = true;
        break;
      }

    case 5:     // Port B
      #ifndef ONE_PORT
      {
        bool e131 = ((uint8_t)json[portBprot] == PROT_ARTNET_SACN) ? true : false;
        for (uint8_t x = 0; x < 4; x++) {
          artRDM.setE131(portB[0], portB[x+1], e131);
          artRDM.setE131Uni(portB[0], portB[x+1], json[portBsACNuni][x]);
        }

        if (deviceSettings[portBmode] != json[portBmode]) {
          doReboot = true;
        }

        // Update the Artnet class
        artRDM.setNet(portB[0], (uint8_t)json[portBnet]);
        artRDM.setSubNet(portB[0], (uint8_t)json[portBsub]);
        artRDM.setUni(portB[0], portB[1], json[portBuni][0]);
        artRDM.setMerge(portB[0], portB[1], (uint8_t)json[portBmerge]);

        if ((uint8_t)json[portBmode] >= LED_MODE_START && !doReboot) {
          if ((uint16_t)deviceSettings[portBnumPix] != (uint16_t)json[portBnumPix]) {
            doReboot = true;
          }
        }
        ret = true;
        break;
      }
      #endif

    case 6:     // Scenes
      // Not yet implemented
      
      return true;
      break;

    case 7:     // Firmware
      // Doesn't come here
      return true;
      break;

    case 8:     // Debug log
      if ((bool)json["debugLogClear"]) {
        return debugLogClear();
      }
      return true;
      break;

    default:
      // Catch errors
      return false;
  }

  // assigns new values from the input
  for (JsonPair kv : json) {
    key = kv.key().c_str();
    if (deviceSettings.containsKey(key)) {
      deviceSettings[key] = json[key];
    }
  }
  if (ret)
    save();

  return ret;
}

const char* msg_disabled = "Disabled in hotspot mode";

// Only reply with data not available from the settings.json
void ajaxLoad(uint8_t page, JsonObject jsonReply) {
  // Get MAC Address
  char MAC_char[30] = "";
  for (int i = 0; i < 6; ++i) {
    sprintf(&MAC_char[i * 3], "%02X", MAC_array[i]);
    if(i<5) strcat(MAC_char, ":");
  }

  jsonReply["macAddress"] = MAC_char;
  jsonReply[success] = 1;

  switch (page) {
    case 1:     // Device Status
    {
      char buf[160];
      String sceneStatus;
      jsonReply["isHotspot"] = isHotspot;
      if (isHotspot && !deviceSettings[standAloneEnable]) {
        jsonReply["portAStatus"] = msg_disabled;
        jsonReply["portBStatus"] = msg_disabled;
      } else {
        jsonReply["portAStatus"] = deviceSettings[portAmode];
        jsonReply["portBStatus"] = deviceSettings[portBmode];
      }

      FSInfo fs_info;
      SPIFFS.info(fs_info);
      sprintf_P(buf, scene_msg, 0, fs_info.usedBytes, fs_info.totalBytes);
      sceneStatus = buf;
      jsonReply["sceneStatus"] = sceneStatus;
      //jsonReply["sceneStatus"] = "Not outputting<br />0 Scenes Recorded<br />0 of 250KB used";
      jsonReply["firmwareStatus"] = FIRMWARE_VERSION;
      break;
    }

    case 7:     // Firmware
      jsonReply["firmwareStatus"] = FIRMWARE_VERSION;
      break;
  }
}
