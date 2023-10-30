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

#include <stdint.h>
#include <ArduinoJson.h>

#ifndef debugLog_h
#define debugLog_h

extern DynamicJsonDocument deviceSettings;
extern char* stack_start;

// debug helper
const char startup[] = "Startup";

#ifdef DEBUG_ESP_PORT
  #define DEBUG_MSG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
  #define DEBUG_LN( X ) DEBUG_ESP_PORT.println( X )
#else
  #define DEBUG_MSG(...)
  #define DEBUG_LN( X )
#endif

typedef enum loglevel_t {
  LOG_DEBUG = 0,
  LOG_INFO,
  LOG_WARN,
  LOG_ERROR
} loglevel_t;

void os_log(const char c);
void os_log_write();
void log_ls(char const* path);
void log_meminfo(const char* context);
void log_u8_P(const loglevel_t level, const char* context, const char* msg, uint8_t num);
int debugLog(const loglevel_t level, const char* context, const char* text);
int debugLog_P(const loglevel_t level, const char* context, const char* text);
void debugLogSetup();
bool debugLogClear();

#endif
