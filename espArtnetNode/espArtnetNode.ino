/*
  ESP8266_ArtNetNode v2.0.0
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

  Note:
  This is a pre-release version of this software.  It is not yet ready for prime time and contains bugs (known and unknown).
  Please submit any bugs or code changes so they can be included into the next release.

  Prizes up for grabs:
  I am giving away a few of my first batch of prototype PCBs.  They will be fully populated - valued at over $30 just for parts.
  In order to recieve one, please complete one of the following tasks.  You can "win" multiple boards.
  1 - Fix the WDT reset issue (https://github.com/mtongnz/ESP8266_ArtNetNode_v2/issues/41)
  2 - Implement stored scenes function.  I want it to allow for static scenes or for chases to run.
  3 - Most bug fixes, code improvements, feature additions & helpful submissions.
    eg. Fixing the flickering WS2812 (https://github.com/mtongnz/ESP8266_ArtNetNode_v2/issues/36)
        Adding other pixel strips (https://github.com/mtongnz/ESP8266_ArtNetNode_v2/issues/42)
        Creating new web UI theme (https://github.com/mtongnz/ESP8266_ArtNetNode_v2/issues/22)
*/

/* racitup additions:
  ESP8266 BSP URL: https://arduino.esp8266.com/stable/package_esp8266com_index.json
  Due to code problems, ESP8266 BSP Version 2.5.0 is required which installs an older espressif8266 platform version. BSP v2.5.2 is latest supported on MACOS 10.11
  Copy github repo libs to your sketchbook library and restart
  Copy ESP8266 SPIFFS filesystem uploader to sketchbook tools folder & upload the SPIFFS webserver files (1M size plenty for development)
  Copy source to a sketch folder named espArtnetNode

    Debug log visible from website
    Memory usage logging
    Website code stored as SPIFFS static files and served by ESPAsyncWebServer
    Board config for single DMX port (ONE_PORT) over ajax for dynamic web page
    Website: Dynamically remove Port B, add status and max LED brightnesses
    FASTLED library for all LED driving including options for supported non-clocked LED types
    [TODO] Reimplement FX library for FastLED
    Support BSP v2.5.2
    Update to ArduinoJson v6
    [TODO] Update store to EEPROM_Rotate
    Use json file as settings store instead of struct for sharing with website
    [TODO] Import other peoples' fixes
    Fix compile warnings
    Redirect debug/exception logging to debug log - doesn't seem to work?

    # Next version:
    [TODO] Static scene saving
    [TODO] Chase scene recording and playback, hopefully with a fast RLE for compression
    [TODO] Restyle with bootstrap
    [TODO] Support clocked LED strip types over single SPI bus

  Artnet 2CH DMX board supported ESP8266 boards: Wemos D1 mini (User supplied) or Generic ESP8266 module (ESP-12S pre-populated)
*/
// required by ESPAsyncWebServer for ArduinoJson v6 compatibility
// from arduinojson.org/v6/assistant
#define DYNAMIC_JSON_DOCUMENT_SIZE 3072
//#define ARDUINOJSON_ENABLE_STD_STRING 1
//#define ARDUINOJSON_ENABLE_ARDUINO_STRING 0

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <AsyncJson.h>
#include <ArduinoJson.h> // v6
#include <FS.h>
#include <FastLED.h>
#include "store.h"
#include "espDMX_RDM.h"
#include "espArtNetRDM.h"
#include "ws2812Driver.h"
#include "wsFX.h"
#include "debugLog.h"

extern "C" {
#include "user_interface.h"
  extern struct rst_info resetInfo;
}

// Set up for reading VCC
// May not work if ADC line is connected to voltage divider, e.g. Wemos D1 mini
ADC_MODE(ADC_VCC);

const char* FIRMWARE_VERSION = "v2.1.0";
#define ART_FIRM_VERSION 0x0210   // Firmware given over Artnet (2 bytes)


//#define ESP_01              // Un comment for ESP_01 board settings
//#define NO_RESET            // Un comment to disable the reset button

// Wemos boards use 4M (3M SPIFFS) compiler option


#define ARTNET_OEM 0x0123    // Artnet OEM Code
#define ESTA_MAN 0x08DD      // ESTA Manufacturer Code
#define ESTA_DEV 0xEE000000  // RDM Device ID (used with Man Code to make 48bit UID)

#ifdef ESP_01
#define DMX_DIR_A 2   // Same pin as TX1
#define DMX_TX_A 1
#define ONE_PORT
#define NO_RESET

#define WS2812_ALLOW_INT_SINGLE false
#define WS2812_ALLOW_INT_DOUBLE false

#else
#define DMX_DIR_A 5   // D1
#define DMX_DIR_B 16  // D0
#define DMX_TX_A 1
#define DMX_TX_B 2

//#define ONE_PORT  // Testing

#define STATUS_LED_PIN 12
#define STATUS_LED_MODE WS2812B
//  #define STATUS_LED_MODE APA106
#define STATUS_LED_A 0  // Physical wiring order for status LEDs
#define STATUS_LED_B 1
#define STATUS_LED_S 2
#define NUM_STATUS_LEDS 3

#define WS2812_ALLOW_INT_SINGLE false
#define WS2812_ALLOW_INT_DOUBLE false
#endif

#ifndef NO_RESET
#define SETTINGS_RESET 14
#endif

//#define STATUS_DIM 0x0F

// used to store artRDM values in startFunctions:
uint8_t portA[5], portB[5];
uint8_t MAC_array[6];
IPAddress ip;
IPAddress subnet;
IPAddress gateway;
IPAddress broadcast;
IPAddress dmxBroadcast;

espDMX dmxA(0);
espDMX dmxB(1);

uint8_t dmxInSeqID = 0;

esp8266ArtNetRDM artRDM;
AsyncWebServer webServer(80);
// see store.h
DynamicJsonDocument deviceSettings(DYNAMIC_JSON_DOCUMENT_SIZE);
File fsUploadFile;

uint32_t portAcorrect, portBcorrect, portAtemperature, portBtemperature;

// #### REMOVE ?
//bool statusLedsDim = true;
//bool statusLedsOff = false;
ws2812Driver pixDriver;
pixPatterns pixFXA(0, &pixDriver);
pixPatterns pixFXB(1, &pixDriver);

volatile bool pixDone = 1;  // signals output of LEDs
volatile bool isHotspot = 0;
volatile bool newDmxIn = 0;
volatile bool doReboot = false;
volatile bool doSave = 0;
volatile bool nodeErrorShowing = 1;

uint32_t nextNodeReport = 0;
char nodeError[ARTNET_NODE_REPORT_LENGTH] = "";
uint32_t nodeErrorTimeout = 0;
byte* dataIn;

void setup(void) {
  // for meminfo
  char stack;
  stack_start = &stack;
  bool resetDefaults = false;

  // debugging: configure serial and switch builtin LED ON
  Serial.begin(74880);
  delay(3000); // allow USB serial to catch up and slow boot loops
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Make direction output to avoid boot garbage being sent out
  pinMode(DMX_DIR_A, OUTPUT);
  digitalWrite(DMX_DIR_A, LOW);
#ifndef ONE_PORT
  pinMode(DMX_DIR_B, OUTPUT);
  digitalWrite(DMX_DIR_B, LOW);
#endif

#ifdef SETTINGS_RESET
  pinMode(SETTINGS_RESET, INPUT);

  delay(5);
  // button pressed = low reading
  if (!digitalRead(SETTINGS_RESET)) {
    delay(50);
    if (!digitalRead(SETTINGS_RESET))
      resetDefaults = true;
  }
#endif

  // Start SPIFFS file system, will be formatted if it does not exist
  // Should be initialised using ESP8266 Sketch Data Upload tool
  SPIFFS.begin();
  debugLogSetup();
  //log_ls("/");
  DEBUG_MSG("millis size: %u", sizeof(
  // ########## Most initialisation after debug log setup && SPIFFS ##########

  // Load our saved values or store defaults
  if (resetDefaults) {
    uint8_t buf[CONFIG_SIZE];
    loadDefaults(buf, CONFIG_SIZE);
  } else
    eepromLoad();

  // Store our counters for resetting defaults
  if (resetDecode(NULL, NULL))
    deviceSettings[resetCounter] = (uint32_t)deviceSettings[resetCounter] + 1;
  else {
    deviceSettings[wdtCounter] = (uint32_t)deviceSettings[wdtCounter] + 1;
    debugLog(LOG_ERROR, startup, nodeError);
  }

  // Webserver (ajax) and wifi (settings) both require config loaded
  // Start web server before wifi since if no client connection and wifi network configured, will restart
  webSetup();
  webServer.begin();

  // Start wifi, sets Hotspot mode
  wifiStart();

  // Init status LED after core system components
  LEDSetup();

  // Don't start our Artnet or DMX in firmware update mode
  if (!(uint8_t)deviceSettings[doFirmwareUpdate]) {

    // Setup Artnet Ports & Callbacks
    artStart();

    // Switch OS UART logging:
    os_install_putc1((void*)os_log);
    // Don't open any ports for a bit to let the ESP spill it's garbage to serial
    delay(500);

    // DMX port setup - uses Serial debug port
    portSetup();
  }

  deviceSettings[doFirmwareUpdate] = 0;

  // save after either connecting to an AP or staying in hotspot mode
  doSave = true;

#ifdef DEBUG_ESP_PORT
  //WiFi.printDiag(DEBUG_ESP_PORT);
  DEBUG_ESP_PORT.setDebugOutput(true);
#endif
}

void loop(void) {
  // writes os data to logfile every few ms
  os_log_write();
  // controlled by doSave
  save();

  // Get the node details and handle Artnet
  doNodeReport();
  artRDM.handler();

  // DMX handlers
  dmxA.handler();
  #ifndef ONE_PORT
    dmxB.handler();
  #endif
  DMXInHandle();

  LEDhandler();
  statusLED();

  handleReboot();
}


void handleReboot() {
  // don't reboot until save is complete
  if (doReboot && !doSave) {
    char c[ARTNET_NODE_REPORT_LENGTH] = "Device rebooting...";
    artRDM.setNodeReport(c, ARTNET_RC_POWER_OK);
    artRDM.artPollReply();
    artRDM.end();

    // Turn pixel strips off if they're on
    stop_LEDs();

    // Ensure all web data is sent before we reboot
    uint32_t n = millis() + 1000;
    while (millis() < n)
      delay(10);

    ESP.restart();
  }
}
