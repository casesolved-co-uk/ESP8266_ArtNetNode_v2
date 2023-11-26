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

DMX handlers
*/

#include <DmxRdmLib.h>
#include "debugLog.h"


// dmxA and dmxB are declared by the library because it handles both channels together
// Just don't call begin on a channel if you don't want to use it

uint16_t DmxInChans = 0;
byte* dataIn = NULL;

void DMXInHandle() {
  // Handle received DMX
  if (DmxInChans) {
    uint8_t g, p, status_led;

  #ifdef TRIGGER_DMX_IN
    digitalWrite(TRIGGER, HIGH);
  #endif

    if ((uint8_t)deviceSettings[portAmode] == TYPE_DMX_IN) {
      g = portA[0];
      p = portA[1];
      status_led = STATUS_LED_A;
    } else {
      g = portB[0];
      p = portB[1];
      status_led = STATUS_LED_B;
    }

    artRDM.sendDMX(g, p, dmxBroadcast, dataIn, DmxInChans);
    setStatusLed(status_led, CRGB::Cyan);
    DmxInChans = 0;

  #ifdef TRIGGER_DMX_IN
    digitalWrite(TRIGGER, LOW);
  #endif
  }
}


void portSetup() {
  #ifdef DEBUG_ESP_PORT
    DEBUG_LN("Starting ports...");
    DEBUG_LN("Port A debug mode");
    delay(5);
  #else
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
  
      dataIn = (byte*) os_zalloc(sizeof(byte) * 512);
  
    } else {
      #ifndef ESP_01
        setStatusLed(STATUS_LED_A, CRGB::Green);
      #endif
  
      led_controller_A();
    }
  #endif
  
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
        setStatusLed(STATUS_LED_B, CRGB::Teal);
      #endif

      dmxB.begin(DMX_DIR_B, artRDM.getDMX(portB[0], portB[1]));
      dmxB.dmxIn(true);
      dmxB.setInputCallback(dmxIn);

      dataIn = (byte*) os_zalloc(sizeof(byte) * 512);

    } else {
      #ifndef ESP_01
        setStatusLed(STATUS_LED_B, CRGB::Green);
      #endif

      led_controller_B();
    }
  #endif
}

// DMX RDM callbacks
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

// DMX In callback
void dmxIn(uint16_t numChans) {
  DmxInChans = numChans;
  espDMX* dmx;

  // PortB doesn't actually support DMXIn
  if ((uint8_t)deviceSettings[portAmode] == TYPE_DMX_IN) {
    dmx = &dmxA;
  } else {
    dmx = &dmxB;
  }

  // could be overwritten with 0
  byte* tmp = dataIn;
  dataIn = dmx->getChans();
  dmx->setBuffer(tmp);
}
