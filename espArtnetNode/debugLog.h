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

For storing a debug log in the SPIFFS
*/

#ifndef debugLog_h
#define debugLog_h

// debug helper
const char got_here_u8[] PROGMEM = "Got here %d";
const char got_here_u32[] PROGMEM = "Got here %08" PRIx32;

typedef enum {
  LOG_DEBUG = 0,
  LOG_INFO,
  LOG_WARN,
  LOG_ERROR
} loglevel_t;

const char* loglevelstr[] = {
  "DEBUG", 
  "INFO",
  "WARN",
  "ERROR"
};

#endif
