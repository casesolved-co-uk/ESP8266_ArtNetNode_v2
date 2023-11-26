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
const char reset_logmsg[] PROGMEM = "Exception (%u) %s %s\nexcvaddr=0x%08X depc=0x%08X\n>>>stack>>>\n0x%08X:\n0x%08X:\n0x%08X:\n";

// outputs true if normal non-error reset
bool resetDecode(char* str_out, uint16_t* code_out) {
  const char* err_desc = "DFLT";
  struct rst_info* resetInfo = system_get_rst_info();
  char buf[160];

  switch (resetInfo->reason) {
    case REASON_DEFAULT_RST:  // normal startup by power on
    case REASON_EXT_SYS_RST:  // external system reset
    case REASON_SOFT_RESTART: // software restart, system_restart, GPIO status won’t change
    case REASON_DEEP_SLEEP_AWAKE: // wake up from deep-sleep
      nodeError[0] = '\0';
      if (str_out)
        sprintf_P(str_out, reset_normal);
      if (code_out)
        *code_out = ARTNET_RC_POWER_OK;
      nextNodeReport = millis() + 4000;
      nodeErrorTimeout = 0;
      return true;
      break;
      
    case REASON_WDT_RST: // hardware watchdog reset
      err_desc = "HWDT";
      break;
    case REASON_EXCEPTION_RST: // exception reset, GPIO status won’t change
      err_desc = "EXCP";
      break;
    case REASON_SOFT_WDT_RST: // software watchdog reset, GPIO status won’t change
      err_desc = "SWDT";
      break;
  }

  // We know there has been an error because success returns early
  setStatusLed(STATUS_LED_S, CRGB::Red);
  exccause(resetInfo->exccause, &buf[100]);
  sprintf_P(buf, reset_logmsg, resetInfo->exccause, err_desc, &buf[100], resetInfo->excvaddr, resetInfo->depc, resetInfo->epc1, resetInfo->epc2, resetInfo->epc3);
  debugLog(LOG_ERROR, "resetDecode", buf);

  sprintf_P(nodeError, reset_error_short, err_desc);
  if (str_out)
    sprintf_P(str_out, reset_error_long, err_desc);
  if (code_out)
    *code_out = ARTNET_RC_POWER_FAIL;
  nextNodeReport = millis() + 10000;
  nodeErrorTimeout = millis() + 30000;
  return false;
}


void exccause(uint32_t num, char* buf) {
  const char* msg;
  switch(num) {
    case 0:
      msg = PSTR("IllegalInstructionCause");
      break;
    case 1:
      msg = PSTR("SyscallCause");
      break;
    case 2:
      msg = PSTR("InstructionFetchErrorCause");
      break;
    case 3:
      msg = PSTR("LoadStoreErrorCause");
      break;
    case 4:
      msg = PSTR("Level1InterruptCause");
      break;
    case 5:
      msg = PSTR("AllocaCause");
      break;
    case 6:
      msg = PSTR("IntegerDivideByZeroCause");
      break;
    case 8:
      msg = PSTR("PrivilegedCause");
      break;
    case 9:
      msg = PSTR("LoadStoreAlignmentCause");
      break;
    case 12:
      msg = PSTR("InstrPIFDateErrorCause");
      break;
    case 13:
      msg = PSTR("LoadStorePIFDataErrorCause");
      break;
    case 14:
      msg = PSTR("InstrPIFAddrErrorCause");
      break;
    case 15:
      msg = PSTR("LoadStorePIFAddrErrorCause");
      break;
    case 16:
      msg = PSTR("InstTLBMissCause");
      break;
    case 17:
      msg = PSTR("InstTLBMultiHitCause");
      break;
    case 18:
      msg = PSTR("InstFetchPrivilegeCause");
      break;
    case 20:
      msg = PSTR("InstFetchProhibitedCause");
      break;
    case 24:
      msg = PSTR("LoadStoreTLBMissCause");
      break;
    case 25:
      msg = PSTR("LoadStoreTLBMultiHitCause");
      break;
    case 26:
      msg = PSTR("LoadStorePrivilegeCause");
      break;
    case 28:
      msg = PSTR("LoadProhibitedCause");
      break;
    case 29:
      msg = PSTR("StoreProhibitedCause");
      break;
    default:
      msg = PSTR("Other");
  }
  sprintf_P(buf, msg);
}


// configures basic defaults for if board FS is not initialised
void wifiBasicInit() {
  if (!settingsLoaded) {
    deviceSettings[hotspotSSID] = "";
    deviceSettings[hotspotPass] = "6hMbaXIqzneK";
    deviceSettings[standAloneEnable] = 1;
    deviceSettings[nodeName] = espArtNetNode;
    ip = IPAddress(2,0,0,1);
    subnet = IPAddress(255,0,0,0);
  }
  // If it's the default WiFi SSID, make it unique
  if (strcmp((const char*)deviceSettings[hotspotSSID], espArtNetNode) == 0 || (const char*)deviceSettings[hotspotSSID][0] == '\0') {
    char buf[20];
    sprintf(buf, "espArtNetNode_%05u", (ESP.getChipId() & 0xFFFF)); // 32-bit -> 5 digits or 99999 or <17-bits
    deviceSettings[hotspotSSID] = buf;
  }
  WiFi.persistent(false);    // prevent excessive writing to flash
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.hostname((const char*)deviceSettings[nodeName]);
  WiFi.macAddress(MAC_array);
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
    DEBUG_MSG("Starting wifi %s as %s", (const char*)deviceSettings[wifiSSID], (const char*)deviceSettings[nodeName]);
    WiFi.mode(WIFI_STA);
    WiFi.begin((const char*)deviceSettings[wifiSSID], (const char*)deviceSettings[wifiPass]);

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
    
  } else
    startHotspot();
}

void startHotspot() {
  isHotspot = true;
  conversions();

  DEBUG_MSG("Starting hotspot %s as %s", (const char*)deviceSettings[hotspotSSID], (const char*)deviceSettings[nodeName]);
  WiFi.mode(WIFI_AP);
  WiFi.softAP((const char*)deviceSettings[hotspotSSID], (const char*)deviceSettings[hotspotPass]);
  // Must give MacOS a gateway (not 0.0.0.0) otherwise the wifi symbol keeps searching & doesn't look connected
  WiFi.softAPConfig(ip, ip, subnet);

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
