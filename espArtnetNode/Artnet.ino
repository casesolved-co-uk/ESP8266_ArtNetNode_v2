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

Artnet handlers
*/

#include <DmxRdmLib.h>

extern pixPatterns* pixFXA;
extern pixPatterns* pixFXB;

uint32_t nextNodeReport = 0;
char nodeError[ARTNET_NODE_REPORT_LENGTH] = "";
uint32_t nodeErrorTimeout = 0;
volatile bool nodeErrorShowing = true;

void artStart() {
  char buf[ARTNET_NODE_REPORT_LENGTH];
  uint16_t art_code;
  bool e131;

  DEBUG_LN("Starting Artnet...");
  // Initialise out ArtNet
  artRDM.init(ip, subnet, isHotspot ? true : (uint8_t)deviceSettings[dhcpEnable], (const char*)deviceSettings[nodeName], (const char*)deviceSettings[longName], ARTNET_OEM, ESTA_MAN, MAC_array);

  // Set firmware
  artRDM.setFirmwareVersion(ART_FIRM_VERSION);

  /************* PORT A *************/
  #ifndef DEBUG_ESP_PORT
  // Add Group
  portA[0] = artRDM.addGroup((uint8_t)deviceSettings[portAnet], (uint8_t)deviceSettings[portAsub]);
  
  e131 = ((uint8_t)deviceSettings[portAprot] == PROT_ARTNET_SACN) ? true : false;

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
  #endif

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

  resetDecode(buf, &art_code);
  artRDM.setNodeReport(buf, art_code);

  // Start artnet
  artRDM.begin();
}

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


void enable_dhcp() {
  // save() is done outside
  deviceSettings[dhcpEnable] = 1;
  gateway = INADDR_NONE;
}

void dmxHandle(uint8_t group, uint8_t port, uint16_t numChans, bool syncEnabled) {
  #ifdef DEBUG_ESP_PORT
  static uint32_t _count = 0;
  if (_count % 100 == 0) {
    DEBUG_MSG("ArtDMX grp: %u port: %u num: %u sync: %u\n", group, port, numChans, syncEnabled);
  }
  _count++;
  #endif

  if (portA[0] == group) {
    if ((uint8_t)deviceSettings[portAmode] >= LED_MODE_START) {
      
      if ((uint8_t)deviceSettings[portApixMode] == FX_MODE_PIXEL_MAP) {
        if (numChans > 510)
          numChans = 510;

        // Copy DMX data to the pixels buffer
        set_LEDs(STATUS_LED_A, port * 510, artRDM.getDMX(group, port), numChans);

        // Output to pixel strip
        if (!syncEnabled)
          pixADone = false;

      // FX 14 Mode
      } else if (port == portA[1]) {
        byte* a = artRDM.getDMX(group, port);
        uint16_t s = (uint8_t)deviceSettings[portApixFXstart] - 1;
        pixFXA->DMXSet(&a[s]);
      }

    // DMX modes
    } else if ((uint8_t)deviceSettings[portAmode] != TYPE_DMX_IN && port == portA[1]) {
      dmxA.chanUpdate(numChans);
    }
      

  #ifndef ONE_PORT
  } else if (portB[0] == group) {
    if ((uint8_t)deviceSettings[portBmode] >= LED_MODE_START) {
      
      if ((uint8_t)deviceSettings[portBpixMode] == FX_MODE_PIXEL_MAP) {
        if (numChans > 510)
          numChans = 510;
        
        // Copy DMX data to the pixels buffer
        set_LEDs(STATUS_LED_B, port * 510, artRDM.getDMX(group, port), numChans);
        
        // Output to pixel strip
        if (!syncEnabled)
          pixBDone = false;

      // FX 14 mode
      } else if (port == portB[1]) {
        byte* a = artRDM.getDMX(group, port);
        uint16_t s = (uint8_t)deviceSettings[portBpixFXstart] - 1;
        pixFXB->DMXSet(&a[s]);
      }
    } else if ((uint8_t)deviceSettings[portBmode] != TYPE_DMX_IN && port == portB[1]) {
      dmxB.chanUpdate(numChans);
    }
  #endif
  }
}

void syncHandle() {
  if ((uint8_t)deviceSettings[portAmode] >= LED_MODE_START) {
    rdmPause(1);
    pixADone = show_LEDs(STATUS_LED_A);
    rdmPause(0);
  } else if ((uint8_t)deviceSettings[portAmode] != TYPE_DMX_IN) {
    dmxA.unPause();
  }

  #ifndef ONE_PORT
    if ((uint8_t)deviceSettings[portBmode] >= LED_MODE_START) {
      rdmPause(1);
      pixBDone = show_LEDs(STATUS_LED_B);
      rdmPause(0);
    } else if ((uint8_t)deviceSettings[portBmode] != TYPE_DMX_IN) {
      dmxB.unPause();
    }
  #endif
}

void ipHandle() {
  if (artRDM.getDHCP()) {
    enable_dhcp();
  } else {
    // fool conversions into saving the new info
    deviceSettings[dhcpEnable] = 1;
    ip = artRDM.getIP();
    subnet = artRDM.getSubnetMask();
    // seems strange ?
    gateway = ip;
    gateway[3] = 1;
    conversions();
    deviceSettings[dhcpEnable] = 0;
  }

  // Store everything to EEPROM
  doSave = 1;
  doReboot = 1;
}

void addressHandle() {
  deviceSettings[nodeName] = artRDM.getShortName();
  deviceSettings[longName] = artRDM.getLongName();
  
  deviceSettings[portAnet] = artRDM.getNet(portA[0]);
  deviceSettings[portAsub] = artRDM.getSubNet(portA[0]);
  deviceSettings[portAuni][0] = artRDM.getUni(portA[0], portA[1]);
  deviceSettings[portAmerge] = artRDM.getMerge(portA[0], portA[1]);

  if (artRDM.getE131(portA[0], portA[1]))
    deviceSettings[portAprot] = PROT_ARTNET_SACN;
  else
    deviceSettings[portAprot] = PROT_ARTNET;


  #ifndef ONE_PORT
    deviceSettings[portBnet] = artRDM.getNet(portB[0]);
    deviceSettings[portBsub] = artRDM.getSubNet(portB[0]);
    deviceSettings[portBuni][0] = artRDM.getUni(portB[0], portB[1]);
    deviceSettings[portBmerge] = artRDM.getMerge(portB[0], portB[1]);
    
    if (artRDM.getE131(portB[0], portB[1]))
      deviceSettings[portBprot] = PROT_ARTNET_SACN;
    else
      deviceSettings[portBprot] = PROT_ARTNET;
  #endif
  
  // Store everything to EEPROM
  doSave = true;
}

void rdmHandle(uint8_t group, uint8_t port, rdm_data* c) {
  if (portA[0] == group && portA[1] == port)
    dmxA.rdmSendCommand(c);

  #ifndef ONE_PORT
    else if (portB[0] == group && portB[1] == port)
      dmxB.rdmSendCommand(c);
  #endif
}

void todRequest(uint8_t group, uint8_t port) {
  if (portA[0] == group && portA[1] == port)
    sendTodA();

  #ifndef ONE_PORT
    else if (portB[0] == group && portB[1] == port)
      sendTodB();
  #endif
}

void todFlush(uint8_t group, uint8_t port) {
  if (portA[0] == group && portA[1] == port)
    dmxA.rdmDiscovery();

  #ifndef ONE_PORT
    else if (portB[0] == group && portB[1] == port)
      dmxB.rdmDiscovery();
  #endif
}
