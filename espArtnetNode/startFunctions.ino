/*
ESP8266_ArtNetNode v2.0.0
Copyright (c) 2016, Matthew Tong
https://github.com/mtongnz/ESP8266_ArtNetNode_v2

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.
If not, see http://www.gnu.org/licenses/
*/


const char reset_normal[] PROGMEM = "OK: Device started";
const char reset_error_short[] PROGMEM = "Restart error: %s";
const char reset_error_long[] PROGMEM = "ERROR: (%s) Unexpected device restart";

// outputs true if normal non-error reset
bool resetDecode(char* str_out, uint16_t* code_out) {
  const char* err_desc = "DFLT";

  switch (resetInfo.reason) {
    case REASON_DEFAULT_RST:  // normal start
    case REASON_EXT_SYS_RST:
    case REASON_SOFT_RESTART:
      nodeError[0] = '\0';
      if (str_out)
        sprintf_P(str_out, reset_normal);
      if (code_out)
        *code_out = ARTNET_RC_POWER_OK;
      nextNodeReport = millis() + 4000;
      nodeErrorTimeout = 0;
      return true;
      break;
      
    case REASON_WDT_RST:
      err_desc = "HWDT";
      break;
    case REASON_EXCEPTION_RST:
      err_desc = "EXCP";
      break;
    case REASON_SOFT_WDT_RST:
      err_desc = "SWDT";
      break;
    case REASON_DEEP_SLEEP_AWAKE:
      // not used
      err_desc = "AWAK";
      break;
  }
  sprintf_P(nodeError, reset_error_short, err_desc);
  if (str_out)
    sprintf_P(str_out, reset_error_long, err_desc);
  if (code_out)
    *code_out = ARTNET_RC_POWER_FAIL;
  nextNodeReport = millis() + 10000;
  nodeErrorTimeout = millis() + 30000;
  return false;
}


// configures basic defaults for if board FS is not initialised
void wifiBasicInit() {
  if (!settingsLoaded) {
    deviceSettings[hotspotSSID] = "";
    deviceSettings[hotspotPass] = "6hMbaXIqzneK";
    deviceSettings[standAloneEnable] = 1;
    ip = IPAddress(2,0,0,1);
    subnet = IPAddress(255,0,0,0);
  }
  // If it's the default WiFi SSID, make it unique
  if (strcmp((const char*)deviceSettings[hotspotSSID], "espArtNetNode") == 0 || (const char*)deviceSettings[hotspotSSID][0] == '\0') {
    char buf[20];
    sprintf(buf, "espArtNetNode_%05u", (ESP.getChipId() & 0xFFFF)); // 32-bit -> 5 digits or 99999 or <17-bits
    deviceSettings[hotspotSSID] = buf;
  }
}

// Wifi event handler wifi_handle_event_cb - deprecated. Use ESP8266WiFiGeneric.h individual handlers (e.g. onSoftAPModeStationConnected)
// for debugging see Debug level WiFi and DEBUG_ESP_PORT.setDebugOutput(true);
void wifiStart() {
  wifi_set_sleep_type(NONE_SLEEP_T);

  wifiBasicInit();

  if ((uint8_t)deviceSettings[standAloneEnable]) {
    startHotspot();
    return;
  }
  
  if ((const char*)deviceSettings[wifiSSID][0] != '\0') {
    DEBUG_LN("Starting wifi...");
    WiFi.begin((const char*)deviceSettings[wifiSSID], (const char*)deviceSettings[wifiPass]);
    WiFi.mode(WIFI_STA);
    WiFi.hostname((const char*)deviceSettings[nodeName]);

    // Try to connect for timeout, if not start hotspot
    uint32_t endTime = millis() + ((uint8_t)deviceSettings[hotspotDelay] * 1000);
    if ((uint8_t)deviceSettings[dhcpEnable]) {
      while (WiFi.status() != WL_CONNECTED && endTime > millis())
        delay(10);

      if (millis() >= endTime)
        startHotspot();
      
      ip = WiFi.localIP();
      subnet = WiFi.subnetMask();

      if (gateway == INADDR_NONE)
        gateway = WiFi.gatewayIP();

      conversions();
    } else
      WiFi.config(ip, gateway, subnet);

    WiFi.macAddress(MAC_array);
    
  } else
    startHotspot();
}

void startHotspot() {
  isHotspot = true;
  conversions();

  DEBUG_LN("Starting hotspot...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP((const char*)deviceSettings[hotspotSSID], (const char*)deviceSettings[hotspotPass]);
  // Must give MacOS a gateway (not 0.0.0.0) otherwise the wifi symbol keeps searching & doesn't look connected
  WiFi.softAPConfig(ip, ip, subnet);

  WiFi.macAddress(MAC_array);

  // don't keep rebooting if there's no wifi config
  if ((uint8_t)deviceSettings[standAloneEnable] || (const char*)deviceSettings[wifiSSID][0] == '\0')
    return;

  uint32_t endTime = millis() + 30000;

  // Stay here if not in stand alone mode - no dmx or artnet
  while (endTime > millis() || wifi_softap_get_station_num() > 0) {
    delay(100);
  }

  isHotspot = false;
  conversions();
  ESP.restart();
}
