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

#include <FS.h> // SPIFFS
#include "debugLog.h"

#define BUFSIZE 512

const char logfile[] = "/debug.log";
const char format[] = "%s-%s: ";
const char setup_fmt[] PROGMEM = "Logfile Size: %u, SPIFFS Total: %u, Used %u, Remaining: %u (bytes)";
const char too_long[] PROGMEM = "String too long";
const char meminfo_fmt[] PROGMEM = "Heap Size: %u, Stack Size: %u (bytes)";

void os_log(char c) {
  File f = SPIFFS.open(logfile, "a");
  f.write(&c, 1);
  f.close();
}

void log_u8_P(const loglevel_t level, const char* context, const char* msg, uint8_t num) {
  char buf[50];
  sprintf_P(buf, msg, num);
  debugLog(level, context, buf);
}

void log_u32_P(const loglevel_t level, const char* context, const char* msg, uint32_t num) {
  char buf[50];
  sprintf_P(buf, msg, num);
  debugLog(level, context, buf);
}

void log_meminfo(const char* context) {
  uint32_t heap = system_get_free_heap_size();
  uint32_t stack = stackSize();
  char buf[50];

  sprintf_P(buf, meminfo_fmt, heap, stack);
  debugLog(LOG_INFO, context, buf);
}

// create log if not exist & log file size and SPIFFS space remaining
void debugLogSetup() {
  size_t fsize = 0;
  char buf[160];
  char startup[] = "Startup";
  FSInfo fs_info;
  SPIFFS.info(fs_info);

  if(SPIFFS.exists(logfile)) {
    File f = SPIFFS.open(logfile, "r");
    fsize = f.size();
    f.close();
  }
  sprintf_P(buf, setup_fmt, fsize, fs_info.totalBytes, fs_info.usedBytes, fs_info.totalBytes - fs_info.usedBytes);
  debugLog(LOG_INFO, startup, buf);
  log_meminfo(startup);

  // overrides OS UART logging:
  os_install_putc1((void*)os_log);
}

uint32_t stackSize() {
  char stack;
  return stack_start - &stack;
}

void debugLogWrite(const char* buf) {
  File f = SPIFFS.open(logfile, "a");
  f.println(buf);
  f.close();
}

// Allows FORMATLEN characters for log level, context and other format characters
int debugLog(const loglevel_t level, const char* context, const char* text) {
  char buf[BUFSIZE];
  int len = strlen(text);
  int headlen = sprintf(buf, format, loglevelstr[level], context);

  if(len >= (BUFSIZE - headlen)) {
    strcat_P(buf, too_long);
    len = -1;
  } else {
    strcat(buf, text);
    len = strlen(buf);
  }

  debugLogWrite(buf);
  return len;
}

int debugLog_P(const loglevel_t level, const char* context, const char* text) {
  char buf[BUFSIZE];
  int len = strlen_P(text);
  int headlen = sprintf(buf, format, loglevelstr[level], context);

  if(len >= (BUFSIZE - headlen)) {
    strcat_P(buf, too_long);
    len = -1;
  } else {
    strcat_P(buf, text);
    len = strlen(buf);
  }

  debugLogWrite(buf);
  return len;
}

String debugLogString() {
  File f = SPIFFS.open(logfile, "r");
  String s = f.readString();
  f.close();
  return s;
}

// page 8 of ajax; truncate
bool debugLogClear() {
  File f = SPIFFS.open(logfile, "w");
  f.close();
  return true; // for ajax return value
}
