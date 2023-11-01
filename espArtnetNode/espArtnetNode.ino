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

/* INFO
 *  A full DMX frame takes ~23msec to transmit, max refresh = 44Hz. Use partial frames for faster refresh
 *  250kbits/s, 2 stop bits, no parity
 *  Port A uses UART0: IO1 & IO3 OR IO15 & IO13 (Serial.swap() or system_uart_swap/system_uart_de_swap - bootloader not affected) OR IO2 & IO3
 *  Port B uses UART1: IO2 & IO8 but IO8 (RX2) is normally used for flash
 *  Wemos has 470R resistor on TX output preventing in-circuit programming and logging - bridge
 *  ESP8266 boot print baud rate: 26MHz clock = 74880, 40MHz clock = 115200
 *  Wemos and ESP-12 series have 26MHz clock
 *  
 *  Busy times:
 *  DMX output - start frame takes 2.5ms, thereafter works on UART interrupts
 *  STATUS LED - 200usec
 *  WS2812 144 LED strip - 4.6ms
 */

/* casesolved additions:
  ESP8266 BSP URL: https://arduino.esp8266.com/stable/package_esp8266com_index.json
  BSP v2.5.2 is latest supported on MACOS 10.11, the testing platform
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
    Support ESP8266 BSP v2.5.2
    Update to ArduinoJson v6
    [TODO] Update store to EEPROM_Rotate
    Use json file as settings store instead of struct for sharing with website
    [TODO] Import other peoples' fixes
    Fix compile warnings
    Redirect debug/exception logging to debug log & remove disable from DMX lib
    [TODO] Add port disable option

    # Next version:
    [TODO] Static scene saving
    [TODO] Chase scene recording and playback, hopefully with a fast RLE for compression
    [TODO] Restyle website with bootstrap
    [TODO] HW: Support clocked LED strip types over single SPI bus - PortC
    [TODO] HW: Move port A to IO15 & IO13 to avoid bootloader output
    ADC pin fix

    # Major hardware upgrade
    [TODO] HW Upgrade hardware to Wemos S2 mini / ESP32 S2

  Artnet 2CH DMX board supported ESP8266 boards: Wemos D1 mini (User supplied) or Generic ESP8266 module (ESP-12S pre-populated)
*/

// required by ESPAsyncWebServer for ArduinoJson v6 compatibility
// from arduinojson.org/v6/assistant
//#define DYNAMIC_JSON_DOCUMENT_SIZE 3072
#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1
#define ARDUINOJSON_USE_LONG_LONG 0
#include <ArduinoJson.h> // v6

#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
#include <ESPAsyncTCP.h>
#include <AsyncJson.h>
#include <ESPAsyncWebSrv.h>
// TODO: Switch to EEPROM_Rotate library to extend life of EEPROM
#include <EEPROM.h>
#include <FastLED.h>
#include "espDMX_RDM.h"
#include "espArtNetRDM.h"
#include "store.h"
#include "wsFX.h"
#include "debugLog.h"

extern "C" {
#include "user_interface.h"
  extern struct rst_info resetInfo;
}
extern char nodeError[];

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
// Testing oscilloscope trigger pin
#define TRIGGER 13 // must use this with any of below
//#define TRIGGER_DMX
#define TRIGGER_LEDS
//#define TRIGGER_STATUS

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
uint8_t portA[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t portB[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t MAC_array[6];
IPAddress ip;
IPAddress subnet;
IPAddress gateway;
IPAddress broadcast;
IPAddress dmxBroadcast;


esp8266ArtNetRDM artRDM;
AsyncWebServer webServer(80);
// from monitoring memoryUsage()
DynamicJsonDocument deviceSettings(2560);
File fsUploadFile;

uint32_t portAcorrect, portBcorrect, portAtemperature, portBtemperature;

volatile bool pixADone = true;
volatile bool pixBDone = true;
volatile bool isHotspot = false;
volatile bool doReboot = false;
volatile bool doSave = false;
volatile bool settingsLoaded = false;


void setup(void) {
  // for meminfo
  char stack;
  stack_start = &stack;
  bool resetDefaults = false;

#ifdef DEBUG_ESP_PORT
  // debugging: configure serial
  DEBUG_ESP_PORT.begin(74880);
#endif

  // testing
#ifdef TRIGGER
  pinMode(TRIGGER, OUTPUT);
  digitalWrite(TRIGGER, LOW);
#endif

  // DIR HIGH = Opto LED off = logic HIGH on MAX485 Data Enable = TX mode
  // Make direction input to avoid boot garbage being sent out and avoid DMX shorts
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

  // Init status LED
  LEDSetup();

  // FS Should be initialised using ESP8266 Sketch Data Upload tool
  // Also inits the debug log and os logging to file
  FS_start();
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
  yield();

  // Start wifi, sets Hotspot mode
  wifiStart();
  yield();

  // Don't start our Artnet or DMX in firmware update mode
  if (!(uint8_t)deviceSettings[doFirmwareUpdate]) {
    // Setup Artnet Ports & Callbacks
    artStart();
    yield();

    // DMX port setup - uses Serial debug port
    portSetup();
    yield();
  }

  deviceSettings[doFirmwareUpdate] = 0;

  // save after either connecting to an AP or staying in hotspot mode
  doSave = true;

  // Don't open any ports for a bit to let the ESP spill it's garbage to serial
  //while (millis() < 3000)
  //  delay(100);

#ifdef DEBUG_ESP_PORT
  //WiFi.printDiag(DEBUG_ESP_PORT);
  DEBUG_ESP_PORT.setDebugOutput(true);
  // builtin LED on GPIO2 also UART1 TX
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
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
#ifdef TRIGGER_DMX
  digitalWrite(TRIGGER, HIGH);
#endif
  dmxA.handler();
  #ifndef ONE_PORT
    dmxB.handler();
  #endif
#ifdef TRIGGER_DMX
  digitalWrite(TRIGGER, LOW);
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
    delay(500);

    ESP.restart();
  }
}
