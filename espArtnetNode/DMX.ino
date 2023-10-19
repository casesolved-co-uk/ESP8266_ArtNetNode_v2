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

void DMXInHandle() {
  // Handle received DMX
  if (newDmxIn) {
    uint8_t g, p, status_led;

    newDmxIn = false;

    if ((uint8_t)deviceSettings[portAmode] == TYPE_DMX_IN) {
      g = portA[0];
      p = portA[1];
      status_led = STATUS_LED_A;
    } else {
      g = portB[0];
      p = portB[1];
      status_led = STATUS_LED_B;
    }

    artRDM.sendDMX(g, p, dmxBroadcast, dataIn, 512);
    setStatusLed(status_led, CRGB::Cyan);
  }
}


void portSetup() {
  DEBUG_LN("Starting ports...");

  #ifdef DEBUG_ESP_PORT
    DEBUG_LN("Port A debug mode");
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
  
      dataIn = (byte*) os_malloc(sizeof(byte) * 512);
      memset(dataIn, 0, 512);
  
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

// DMX callback
void dmxIn(uint16_t numChans) {
  (void) numChans;
  espDMX* dmx;

  if ((uint8_t)deviceSettings[portBmode] == TYPE_DMX_IN) {
    dmx = &dmxB;
  } else {
    dmx = &dmxA;
  }

  // could be overwritten with 0
  byte* tmp = dataIn;
  dataIn = dmx->getChans();
  dmx->setBuffer(tmp);
  
  newDmxIn = true;
}
