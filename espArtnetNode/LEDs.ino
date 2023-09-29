/*
ESP8266_ArtNetNode v2.1.0
Copyright (c) 2023, Richard Case
https://github.com/casesolved-co-uk/ESP8266_ArtNetNode_v2

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.
If not, see http://www.gnu.org/licenses/

FastLED support
*/

#include <FastLED.h>

const char type_err[] PROGMEM = "Invalid LED type number: %d";
const char buf_err[] PROGMEM = "No LED data buffer: %d";
const char ctl_err[] PROGMEM = "No LED controller: %d";
const char toobig_err[] PROGMEM = "Too much LED data: %d";

// Status, channel A & B
CLEDController* led_controllers[NUM_STATUS_LEDS];
uint8_t STATUS_BRIGHTNESS = 64;
uint8_t MAX_BRIGHTNESS = 255;

#ifdef STATUS_LED_MODE
  CRGB status_leds[NUM_STATUS_LEDS];
#endif


void LEDSetup() {
  memset(led_controllers, 0, sizeof(CLEDController*) * NUM_STATUS_LEDS);
  
#ifdef STATUS_LED_MODE
  // Status LEDs
  led_controllers[STATUS_LED_S] = &FastLED.addLeds<STATUS_LED_MODE, STATUS_LED_PIN, BGR>(status_leds, NUM_STATUS_LEDS);
  // see color.h
  led_controllers[STATUS_LED_S]->setCorrection(0xFFFFF0);

  setStatusLed(STATUS_LED_S, CRGB::Pink);
  doStatusLedOutput();
#endif

  //  led_controllers started in portSetup
}

void stop_LEDs() {
  if (led_controllers[STATUS_LED_A])
    led_controllers[STATUS_LED_A]->clearLedData();
  setStatusLed(STATUS_LED_A, CRGB::Red);

  if (led_controllers[STATUS_LED_B])
    led_controllers[STATUS_LED_B]->clearLedData();
  setStatusLed(STATUS_LED_B, CRGB::Red);
}

/*
 * Inits the correct LED controller type for channel A and B
 * Matches raw_modes in main.js
 * reconfiguration caused by reboot hence free not required
 * Cannot be parameterised due to pin number being required at compile time in addLeds template
 * Each addLeds line requires program space so keep to realistic minimum
 */
// TODO: input, save and set colour correction & temperature
void led_controller_A() {
  digitalWrite(DMX_DIR_A, HIGH);
  // Dynamically allocate LED data array
  CRGB* mem = (CRGB*)malloc(sizeof(CRGB) * deviceSettings[portAnumPix]);

  // init controller
  switch((uint8_t)deviceSettings[portAmode]) {
    case 36: { led_controllers[STATUS_LED_A] = &FastLED.addLeds<WS2811, DMX_TX_A, RGB>(mem, deviceSettings[portAnumPix]); break; }
    case 40: { led_controllers[STATUS_LED_A] = &FastLED.addLeds<WS2812, DMX_TX_A, RGB>(mem, deviceSettings[portAnumPix]); break; }
    case 48: { led_controllers[STATUS_LED_A] = &FastLED.addLeds<WS2813, DMX_TX_A, RGB>(mem, deviceSettings[portAnumPix]); break; }
    default: { log_u8_P(LOG_ERROR, "LEDA", type_err, deviceSettings[portAmode]); return; }
  }
  led_controllers[STATUS_LED_A]->setCorrection(portAcorrect);
  led_controllers[STATUS_LED_A]->setTemperature(portAtemperature);
}

void led_controller_B() {
  digitalWrite(DMX_DIR_B, HIGH);
  // Dynamically allocate LED data array
  CRGB* mem = (CRGB*)malloc(sizeof(CRGB) * deviceSettings[portBnumPix]);

  // init controller
  switch((uint8_t)deviceSettings[portBmode]) {
    case 36: { led_controllers[STATUS_LED_B] = &FastLED.addLeds<WS2811, DMX_TX_B, RGB>(mem, deviceSettings[portBnumPix]); break; }
    case 40: { led_controllers[STATUS_LED_B] = &FastLED.addLeds<WS2812, DMX_TX_B, RGB>(mem, deviceSettings[portBnumPix]); break; }
    case 48: { led_controllers[STATUS_LED_B] = &FastLED.addLeds<WS2813, DMX_TX_B, RGB>(mem, deviceSettings[portBnumPix]); break; }
    default: { log_u8_P(LOG_ERROR, "LEDB", type_err, deviceSettings[portBmode]); return; }
  }
  led_controllers[STATUS_LED_B]->setCorrection(portBcorrect);
  led_controllers[STATUS_LED_B]->setTemperature(portBtemperature);
}

// LED strip could be more than a DMX universe long, so may need to index into the buffer for multiple writes
void set_LEDs(uint8_t controller_idx, uint16_t start_idx, uint8_t* data, uint16_t len) {
  uint8_t* leds = (uint8_t*) led_controllers[controller_idx]->leds();
  uint16_t numPix;
  const char ctx[] = "set_LEDs";

  if (!leds) {
    log_u8_P(LOG_ERROR, ctx, buf_err, controller_idx);
    setStatusLed(controller_idx, CRGB::Yellow);
    return;
  }

  if (controller_idx == STATUS_LED_A)
    numPix = deviceSettings[portAnumPix];
  else
    numPix = deviceSettings[portBnumPix];

  // 3 channels per RGB LED
  if (start_idx + len > numPix * sizeof(struct CRGB)) {
    log_u8_P(LOG_ERROR, ctx, toobig_err, controller_idx);
    setStatusLed(controller_idx, CRGB::Red);
    return;
  }
  memcpy(&leds[start_idx], data, len);
}

bool show_LEDs() {
  return show_LEDs(STATUS_LED_A) && show_LEDs(STATUS_LED_B);
}

bool show_LEDs(uint8_t controller_idx) {
  CLEDController* controller = led_controllers[controller_idx];

  if (!controller) {
    log_u8_P(LOG_ERROR, "show_LEDs", ctl_err, controller_idx);
    setStatusLed(controller_idx, CRGB::Yellow);
    return false;
  }

  controller->showLeds(MAX_BRIGHTNESS);
  return true;
}

// colours in pixeltypes.h
void setStatusLed(uint8_t num, CRGB::HTMLColorCode col) {
#ifdef STATUS_LED_MODE
  status_leds[num] = col;
#endif
}

void doStatusLedOutput() {
#ifdef STATUS_LED_MODE
  led_controllers[STATUS_LED_S]->showLeds(STATUS_BRIGHTNESS);
#endif
}
