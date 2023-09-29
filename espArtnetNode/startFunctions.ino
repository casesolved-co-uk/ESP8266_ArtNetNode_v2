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

const char wifi_connect[] PROGMEM = "Wifi connected.<br />SSID: %s";
const char wifi_hotspot[] PROGMEM = "No Wifi. Hotspot started.<br />\nHotspot SSID: %s";

void doNodeReport() {
  if (nextNodeReport > millis())
    return;
  
  char c[ARTNET_NODE_REPORT_LENGTH];

  if (nodeErrorTimeout > millis())
    nextNodeReport = millis() + 2000;
  else
    nextNodeReport = millis() + 5000;
  
  if (nodeError[0] != '\0' && !nodeErrorShowing && nodeErrorTimeout > millis()) {
    
    nodeErrorShowing = true;
    strcpy(c, nodeError);
    
  } else {
    nodeErrorShowing = false;
    
    strcpy(c, "OK: PortA:");

    switch ((uint8_t)deviceSettings[portAmode]) {
      case TYPE_DMX_OUT:
        sprintf(c, "%s DMX Out", c);
        break;
      
      case TYPE_RDM_OUT:
        sprintf(c, "%s RDM Out", c);
        break;
      
      case TYPE_DMX_IN:
        sprintf(c, "%s DMX In", c);
        break;
      
      default:
        if ((uint8_t)deviceSettings[portApixMode] == FX_MODE_12) {
          sprintf(c, "%s 12chan", c);
        } else {
          sprintf(c, "%s RGBLED %ipixels", c, (uint16_t)deviceSettings[portAnumPix]);
        }
        break;
    }
  
    #ifndef ONE_PORT
      sprintf(c, "%s. PortB:", c);
      
      switch ((uint8_t)deviceSettings[portBmode]) {
        case TYPE_DMX_OUT:
          sprintf(c, "%s DMX Out", c);
          break;
        
        case TYPE_RDM_OUT:
          sprintf(c, "%s RDM Out", c);
          break;

        case TYPE_DMX_IN:
          sprintf(c, "%s DMX In", c);
          break;

        default:
          if ((uint8_t)deviceSettings[portBpixMode] == FX_MODE_12) {
            sprintf(c, "%s 12chan", c);
          } else {
            sprintf(c, "%s RGBLED %ipixels", c, (uint16_t)deviceSettings[portBnumPix]);
          }
          break;
      }
    #endif
  }
  
  artRDM.setNodeReport(c, ARTNET_RC_POWER_OK);
}

void portSetup() {
  if ((uint8_t)deviceSettings[portAmode] == TYPE_DMX_OUT || (uint8_t)deviceSettings[portAmode] == TYPE_RDM_OUT) {
    setStatusLed(STATUS_LED_A, CRGB::Blue);

    dmxA.begin(DMX_DIR_A, artRDM.getDMX(portA[0], portA[1]));
    if ((uint8_t)deviceSettings[portAmode] == TYPE_RDM_OUT && !dmxA.rdmEnabled()) {
      dmxA.rdmEnable(ESTA_MAN, ESTA_DEV);
      dmxA.rdmSetCallBack(rdmReceivedA);
      dmxA.todSetCallBack(sendTodA);
    }
  } else if ((uint8_t)deviceSettings[portAmode] == TYPE_DMX_IN) {
    #ifndef ESP_01
      setStatusLed(STATUS_LED_A, CRGB::Cyan);
    #endif
    
    dmxA.begin(DMX_DIR_A, artRDM.getDMX(portA[0], portA[1]));
    dmxA.dmxIn(true);
    dmxA.setInputCallback(dmxIn);

    dataIn = (byte*) os_malloc(sizeof(byte) * 512);
    memset(dataIn, 0, 512);

  } else {
    #ifndef ESP_01
      setStatusLed(STATUS_LED_A, CRGB::Green);
    #endif

    led_controller_A();
  }
  
  #ifndef ONE_PORT
    if ((uint8_t)deviceSettings[portBmode] == TYPE_DMX_OUT || (uint8_t)deviceSettings[portBmode] == TYPE_RDM_OUT) {
      setStatusLed(STATUS_LED_B, CRGB::Blue);
      
      dmxB.begin(DMX_DIR_B, artRDM.getDMX(portB[0], portB[1]));
      if ((uint8_t)deviceSettings[portBmode] == TYPE_RDM_OUT && !dmxB.rdmEnabled()) {
        dmxB.rdmEnable(ESTA_MAN, ESTA_DEV);
        dmxB.rdmSetCallBack(rdmReceivedB);
        dmxB.todSetCallBack(sendTodB);
      }
    } else if ((uint8_t)deviceSettings[portBmode] == TYPE_DMX_IN) {
      #ifndef ESP_01
        setStatusLed(STATUS_LED_B, CRGB::Cyan);
      #endif

      dmxB.begin(DMX_DIR_B, artRDM.getDMX(portB[0], portB[1]));
      dmxB.dmxIn(true);
      dmxB.setInputCallback(dmxIn);

      dataIn = (byte*) os_malloc(sizeof(byte) * 512);
      memset(dataIn, 0, 512);

    } else {
      #ifndef ESP_01
        setStatusLed(STATUS_LED_B, CRGB::Green);
      #endif

      led_controller_B();
    }
  #endif

  //pixDriver.allowInterruptSingle = WS2812_ALLOW_INT_SINGLE;
  //pixDriver.allowInterruptDouble = WS2812_ALLOW_INT_DOUBLE;
}

void artStart() {
  // Initialise out ArtNet
  artRDM.init(ip, subnet, isHotspot ? true : (uint8_t)deviceSettings[dhcpEnable], (const char*)deviceSettings[nodeName], (const char*)deviceSettings[longName], ARTNET_OEM, ESTA_MAN, MAC_array);

  // Set firmware
  artRDM.setFirmwareVersion(ART_FIRM_VERSION);

  /************* PORT A *************/
  // Add Group
  portA[0] = artRDM.addGroup((uint8_t)deviceSettings[portAnet], (uint8_t)deviceSettings[portAsub]);
  
  bool e131 = ((uint8_t)deviceSettings[portAprot] == PROT_ARTNET_SACN) ? true : false;

  // LED modes advertise mode TYPE_DMX_OUT - the rest use the value assigned
  if ((uint8_t)deviceSettings[portAmode] >= LED_MODE_START)
    portA[1] = artRDM.addPort(portA[0], 0, deviceSettings[portAuni][0], TYPE_DMX_OUT, (uint8_t)deviceSettings[portAmerge]);
  else
    portA[1] = artRDM.addPort(portA[0], 0, deviceSettings[portAuni][0], (uint8_t)deviceSettings[portAmode], (uint8_t)deviceSettings[portAmerge]);

  artRDM.setE131(portA[0], portA[1], e131);
  artRDM.setE131Uni(portA[0], portA[1], deviceSettings[portAsACNuni][0]);
  
  // Add extra Artnet ports for long LED strips
  if ((uint8_t)deviceSettings[portAmode] >= LED_MODE_START && (uint8_t)deviceSettings[portApixMode] == FX_MODE_PIXEL_MAP) {
    if ((uint16_t)deviceSettings[portAnumPix] > 170) {
      portA[2] = artRDM.addPort(portA[0], 1, deviceSettings[portAuni][1], TYPE_DMX_OUT, (uint8_t)deviceSettings[portAmerge]);

      artRDM.setE131(portA[0], portA[2], e131);
      artRDM.setE131Uni(portA[0], portA[2], deviceSettings[portAsACNuni][1]);
    }
    if ((uint16_t)deviceSettings[portAnumPix] > 340) {
      portA[3] = artRDM.addPort(portA[0], 2, deviceSettings[portAuni][2], TYPE_DMX_OUT, (uint8_t)deviceSettings[portAmerge]);

      artRDM.setE131(portA[0], portA[3], e131);
      artRDM.setE131Uni(portA[0], portA[3], deviceSettings[portAsACNuni][2]);
    }
    if (deviceSettings[portAnumPix] > 510) {
      portA[4] = artRDM.addPort(portA[0], 3, deviceSettings[portAuni][3], TYPE_DMX_OUT, (uint8_t)deviceSettings[portAmerge]);

      artRDM.setE131(portA[0], portA[4], e131);
      artRDM.setE131Uni(portA[0], portA[4], deviceSettings[portAsACNuni][3]);
    }
  }


  /************* PORT B *************/
  #ifndef ONE_PORT
    // Add Group
    portB[0] = artRDM.addGroup(deviceSettings[portBnet], deviceSettings[portBsub]);
    e131 = ((uint8_t)deviceSettings[portBprot] == PROT_ARTNET_SACN) ? true : false;
    
    // LED modes advertise mode TYPE_DMX_OUT - the rest use the value assigned
    if ((uint8_t)deviceSettings[portBmode] >= LED_MODE_START)
      portB[1] = artRDM.addPort(portB[0], 0, deviceSettings[portBuni][0], TYPE_DMX_OUT, (uint8_t)deviceSettings[portBmerge]);
    else
      portB[1] = artRDM.addPort(portB[0], 0, deviceSettings[portBuni][0], (uint8_t)deviceSettings[portBmode], (uint8_t)deviceSettings[portBmerge]);

    artRDM.setE131(portB[0], portB[1], e131);
    artRDM.setE131Uni(portB[0], portB[1], deviceSettings[portBsACNuni][0]);
  
    // Add extra Artnet ports for long LED strips
    if ((uint8_t)deviceSettings[portBmode] >= LED_MODE_START && (uint8_t)deviceSettings[portBpixMode] == FX_MODE_PIXEL_MAP) {
      if (deviceSettings[portBnumPix] > 170) {
        portB[2] = artRDM.addPort(portB[0], 1, deviceSettings[portBuni][1], TYPE_DMX_OUT, (uint8_t)deviceSettings[portBmerge]);
        
        artRDM.setE131(portB[0], portB[2], e131);
        artRDM.setE131Uni(portB[0], portB[2], deviceSettings[portBsACNuni][1]);
      }
      if (deviceSettings[portBnumPix] > 340) {
        portB[3] = artRDM.addPort(portB[0], 2, deviceSettings[portBuni][2], TYPE_DMX_OUT, (uint8_t)deviceSettings[portBmerge]);
        
        artRDM.setE131(portB[0], portB[3], e131);
        artRDM.setE131Uni(portB[0], portB[3], deviceSettings[portBsACNuni][2]);
      }
      if (deviceSettings[portBnumPix] > 510) {
        portB[4] = artRDM.addPort(portB[0], 3, deviceSettings[portBuni][3], TYPE_DMX_OUT, (uint8_t)deviceSettings[portBmerge]);
        
        artRDM.setE131(portB[0], portB[4], e131);
        artRDM.setE131Uni(portB[0], portB[4], deviceSettings[portBsACNuni][3]);
      }
    }
  #endif

  // Add required RDM callback functions
  artRDM.setArtDMXCallback(dmxHandle);          // Consume DMX for local lights
  artRDM.setArtRDMCallback(rdmHandle);          // RDM to DMX routing
  artRDM.setArtSyncCallback(syncHandle);        // Local light sync
  artRDM.setArtIPCallback(ipHandle);            // DHCP
  artRDM.setArtAddressCallback(addressHandle);  // IP Settings
  artRDM.setTODRequestCallback(todRequest);     // Table of devices
  artRDM.setTODFlushCallback(todFlush);


  switch (resetInfo.reason) {
    case REASON_DEFAULT_RST:  // normal start
    case REASON_EXT_SYS_RST:
    case REASON_SOFT_RESTART:
      artRDM.setNodeReport("OK: Device started", ARTNET_RC_POWER_OK);
      nextNodeReport = millis() + 4000;
      break;
      
    case REASON_WDT_RST:
      artRDM.setNodeReport("ERROR: (HWDT) Unexpected device restart", ARTNET_RC_POWER_FAIL);
      strcpy(nodeError, "Restart error: HWDT");
      nextNodeReport = millis() + 10000;
      nodeErrorTimeout = millis() + 30000;
      break;
    case REASON_EXCEPTION_RST:
      artRDM.setNodeReport("ERROR: (EXCP) Unexpected device restart", ARTNET_RC_POWER_FAIL);
      strcpy(nodeError, "Restart error: EXCP");
      nextNodeReport = millis() + 10000;
      nodeErrorTimeout = millis() + 30000;
      break;
    case REASON_SOFT_WDT_RST:
      artRDM.setNodeReport("ERROR: (SWDT) Unexpected device restart", ARTNET_RC_POWER_FAIL);
      strcpy(nodeError, "Error on Restart: SWDT");
      nextNodeReport = millis() + 10000;
      nodeErrorTimeout = millis() + 30000;
      break;
    case REASON_DEEP_SLEEP_AWAKE:
      // not used
      break;
  }
  
  // Start artnet
  artRDM.begin();

  yield();
}

void wifiStart() {
  // If it's the default WiFi SSID, make it unique
  if (strcmp(deviceSettings[hotspotSSID].as<const char*>(), "espArtNetNode") == 0 || deviceSettings[hotspotSSID].as<const char*>()[0] == '\0') {
    char buf[20];
    sprintf(buf, "espArtNetNode_%05u", (ESP.getChipId() & 0xFF));
    deviceSettings[hotspotSSID] = buf;
  }
  
  if ((bool)deviceSettings[standAloneEnable]) {
    startHotspot();
    return;
  }
  
  if (deviceSettings[wifiSSID][0] != '\0') {
    WiFi.begin(deviceSettings[wifiSSID].as<const char*>(), deviceSettings[wifiPass].as<const char*>());
    WiFi.mode(WIFI_STA);
    WiFi.hostname(deviceSettings[nodeName].as<const char*>());
    
    unsigned long endTime = millis() + ((uint8_t)deviceSettings[hotspotDelay] * 1000);

    if ((bool)deviceSettings[dhcpEnable]) {
      while (WiFi.status() != WL_CONNECTED && endTime > millis())
        yield();

      if (millis() >= endTime)
        startHotspot();
      
      ip = WiFi.localIP();
      subnet = WiFi.subnetMask();

      if (gateway == INADDR_NONE)
        gateway = WiFi.gatewayIP();

      conversions();
    } else
      WiFi.config(ip, gateway, subnet);

    //sprintf(wifiStatus, "Wifi connected.  Signal: %ld<br />SSID: %s", WiFi.RSSI(), deviceSettings.wifiSSID);
    sprintf_P(wifiStatus, wifi_connect, deviceSettings[wifiSSID].as<const char*>());
    WiFi.macAddress(MAC_array);
    
  } else
    startHotspot();
  
  yield();
}

void startHotspot() {
  yield();

  isHotspot = true;
  conversions();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(deviceSettings[hotspotSSID].as<const char*>(), deviceSettings[hotspotPass].as<const char*>());
  WiFi.softAPConfig(ip, ip, subnet);

  sprintf_P(wifiStatus, wifi_hotspot, deviceSettings[hotspotSSID].as<const char*>());
  WiFi.macAddress(MAC_array);
  
  if ((bool)deviceSettings[standAloneEnable])
    return;

  unsigned long endTime = millis() + 30000;

  // Stay here if not in stand alone mode - no dmx or artnet
  while (endTime > millis() || wifi_softap_get_station_num() > 0) {
    yield();
  }

  ESP.restart();
  isHotspot = false;
  conversions();
}
