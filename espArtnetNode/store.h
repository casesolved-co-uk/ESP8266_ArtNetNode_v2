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

Constants
*/

#ifndef store_h
#define store_h

// Change this if the settings structure changes, should match FIRMWARE_VERSION
#define CONFIG_VERSION "210"

// Dont change this
#define CONFIG_START 0

// from defaults.json file
#define CONFIG_SIZE 1200

enum fx_mode {
  FX_MODE_PIXEL_MAP = 0,
  FX_MODE_12 = 1
};

enum p_type {
  TYPE_DMX_OUT = 0,
  TYPE_RDM_OUT = 1,
  TYPE_DMX_IN = 2,
  // see raw_modes in main.js
  LED_MODE_START = 30
};

enum p_protocol {
  PROT_ARTNET = 0,
  PROT_ARTNET_SACN = 1
};

enum p_merge {
  MERGE_LTP = 0,
  MERGE_HTP = 1
};


// Settings Keywords
const char ipAddress[] = "ipAddress";
const char subAddress[] = "subAddress";
const char gwAddress[] = "gwAddress";
const char bcAddress[] = "bcAddress";
const char hotspotIp[] = "hotspotIp";
const char hotspotSubnet[] = "hotspotSubnet";
const char dmxInBroadcast[] = "dmxInBroadcast";

const char dhcpEnable[] = "dhcpEnable";
const char standAloneEnable[] = "standAloneEnable";
const char nodeName[] = "nodeName";
const char longName[] = "longName";
const char wifiSSID[] = "wifiSSID";
const char wifiPass[] = "wifiPass";
const char hotspotSSID[] = "hotspotSSID";
const char hotspotPass[] = "hotspotPass";
const char hotspotDelay[] = "hotspotDelay";

const char portAmode[] = "portAmode";
const char portAprot[] = "portAprot";
const char portAmerge[] = "portAmerge";
const char portAnet[] = "portAnet";
const char portAsub[] = "portAsub";
const char portAnumPix[] = "portAnumPix";
const char portAcorrection[] = "portAcorrection";
const char portAtemp[] = "portAtemp";
const char portApixMode[] = "portApixMode";
const char portApixFXstart[] = "portApixFXstart";
const char portAuni[] = "portAuni";
const char portAsACNuni[] = "portAsACNuni";

const char portBmode[] = "portBmode";
const char portBprot[] = "portBprot";
const char portBmerge[] = "portBmerge";
const char portBnet[] = "portBnet";
const char portBsub[] = "portBsub";
const char portBnumPix[] = "portBnumPix";
const char portBcorrection[] = "portBcorrection";
const char portBtemp[] = "portBtemp";
const char portBpixMode[] = "portBpixMode";
const char portBpixFXstart[] = "portBpixFXstart";
const char portBuni[] = "portBuni";
const char portBsACNuni[] = "portBsACNuni";

const char doFirmwareUpdate[] = "doFirmwareUpdate";
const char resetCounter[] = "resetCounter";
const char wdtCounter[] = "wdtCounter";
const char numPorts[] = "numPorts";
const char statusBright[] = "statusBright";
const char maxBright[] = "maxBright";

#endif
