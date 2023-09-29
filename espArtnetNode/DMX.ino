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
