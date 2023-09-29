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

void enable_dhcp() {
  // save() is done outside
  deviceSettings[dhcpEnable] = 1;
  gateway = INADDR_NONE;
}



void dmxHandle(uint8_t group, uint8_t port, uint16_t numChans, bool syncEnabled) {
  if (portA[0] == group) {
    if ((uint8_t)deviceSettings[portAmode] >= LED_MODE_START) {
      
      setStatusLed(STATUS_LED_A, CRGB::Green);
      
      if ((uint8_t)deviceSettings[portApixMode] == FX_MODE_PIXEL_MAP) {
        if (numChans > 510)
          numChans = 510;

        // Copy DMX data to the pixels buffer
        set_LEDs(STATUS_LED_A, port * 510, artRDM.getDMX(group, port), numChans);

        // Output to pixel strip
        if (!syncEnabled)
          pixDone = false;

        return;

      // FX 12 Mode
      } else if (port == portA[1]) {
        byte* a = artRDM.getDMX(group, port);
        uint16_t s = (uint8_t)deviceSettings[portApixFXstart] - 1;
        
        pixFXA.Intensity = a[s + 0];
        pixFXA.setFX(a[s + 1]);
        pixFXA.setSpeed(a[s + 2]);
        pixFXA.Pos = a[s + 3];
        pixFXA.Size = a[s + 4];
        
        pixFXA.setColour1((a[s + 5] << 16) | (a[s + 6] << 8) | a[s + 7]);
        pixFXA.setColour2((a[s + 8] << 16) | (a[s + 9] << 8) | a[s + 10]);
        pixFXA.Size1 = a[s + 11];
        //pixFXA.Fade = a[s + 12];

        pixFXA.NewData = 1;
          
      }

    // DMX modes
    } else if ((uint8_t)deviceSettings[portAmode] != TYPE_DMX_IN && port == portA[1]) {
      dmxA.chanUpdate(numChans);
      
      setStatusLed(STATUS_LED_A, CRGB::Blue);
    }
      

  #ifndef ONE_PORT
  } else if (portB[0] == group) {
    if ((uint8_t)deviceSettings[portBmode] >= LED_MODE_START) {

      setStatusLed(STATUS_LED_B, CRGB::Green);
      
      if ((uint8_t)deviceSettings[portBpixMode] == FX_MODE_PIXEL_MAP) {
        if (numChans > 510)
          numChans = 510;
        
        // Copy DMX data to the pixels buffer
        set_LEDs(STATUS_LED_B, port * 510, artRDM.getDMX(group, port), numChans);
        
        // Output to pixel strip
        if (!syncEnabled)
          pixDone = false;

        return;

      // FX 12 mode
      } else if (port == portB[1]) {
        byte* a = artRDM.getDMX(group, port);
        uint16_t s = (uint8_t)deviceSettings[portBpixFXstart] - 1;
        
        pixFXB.Intensity = a[s + 0];
        pixFXB.setFX(a[s + 1]);
        pixFXB.setSpeed(a[s + 2]);
        pixFXB.Pos = a[s + 3];
        pixFXB.Size = a[s + 4];
        pixFXB.setColour1((a[s + 5] << 16) | (a[s + 6] << 8) | a[s + 7]);
        pixFXB.setColour2((a[s + 8] << 16) | (a[s + 9] << 8) | a[s + 10]);
        pixFXB.Size1 = a[s + 11];
        //pixFXB.Fade = a[s + 12];

        pixFXB.NewData = 1;
      }
    } else if ((uint8_t)deviceSettings[portBmode] != TYPE_DMX_IN && port == portB[1]) {
      dmxB.chanUpdate(numChans);
      setStatusLed(STATUS_LED_B, CRGB::Blue);
    }
  #endif
  }
}

void syncHandle() {
  if ((uint8_t)deviceSettings[portAmode] >= LED_MODE_START) {
    rdmPause(1);
    pixDone = show_LEDs(STATUS_LED_A);
    rdmPause(0);
  } else if ((uint8_t)deviceSettings[portAmode] != TYPE_DMX_IN) {
    dmxA.unPause();
  }

  #ifndef ONE_PORT
    if ((uint8_t)deviceSettings[portBmode] >= LED_MODE_START) {
      rdmPause(1);
      pixDone = show_LEDs(STATUS_LED_B);
      rdmPause(0);
    } else if ((uint8_t)deviceSettings[portBmode] != TYPE_DMX_IN) {
      dmxB.unPause();
    }
  #endif
}

void ipHandle() {
  if (artRDM.getDHCP()) {
    enable_dhcp();
    doReboot = true;
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
    
    doReboot = true;
  }

  // Store everything to EEPROM
  save();
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
  save();
}

void rdmHandle(uint8_t group, uint8_t port, rdm_data* c) {
  if (portA[0] == group && portA[1] == port)
    dmxA.rdmSendCommand(c);

  #ifndef ONE_PORT
    else if (portB[0] == group && portB[1] == port)
      dmxB.rdmSendCommand(c);
  #endif
}

void rdmReceivedA(rdm_data* c) {
  artRDM.rdmResponse(c, portA[0], portA[1]);
}

void sendTodA() {
  artRDM.artTODData(portA[0], portA[1], dmxA.todMan(), dmxA.todDev(), dmxA.todCount(), dmxA.todStatus());
}

#ifndef ONE_PORT
void rdmReceivedB(rdm_data* c) {
  artRDM.rdmResponse(c, portB[0], portB[1]);
}

void sendTodB() {
  artRDM.artTODData(portB[0], portB[1], dmxB.todMan(), dmxB.todDev(), dmxB.todCount(), dmxB.todStatus());
}
#endif

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
