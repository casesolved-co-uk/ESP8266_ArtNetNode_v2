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
If debug level core is enabled, SPIFFS debug info will be output to Serial
SPIFFS_read rc=-10011  <-- means duplicate file found with same name
*/

#include <FS.h> // SPIFFS
#include <string.h>
#include "debugLog.h"

#define BUFSIZE 512

const char logfile[] = "/debug.log";
const char format[] = "%s-%s: ";
const char setup_fmt[] PROGMEM = "Logfile Size: %u, SPIFFS Total: %u, Used %u, Remaining: %u (bytes). VCC: %u mV";
const char too_long[] PROGMEM = "String too long";
const char meminfo_fmt[] PROGMEM = "Heap Size: %u, Stack Size: %u, Free Heap: %u, Free Stack: %u, json: %u (bytes)";
// directory, size, date. name is done after
const char ll_fmt[] PROGMEM = "%s %u %s ";
char* stack_start;

const char* loglevelstr[] = {
  "DEBUG", 
  "INFO",
  "WARN",
  "ERROR"
};

char os_logbuffer[128] = "";
// write out every 100ms
uint32_t os_log_write_timer = 0;
void os_log(const char c) {
  uint8_t len = strlen(os_logbuffer);
  // overflow - write out immediately, exception?
  if (len > 120) {
    // force output
    os_log_write_timer = 0;
  }
  if (len == 127) {
    os_log_write();
    len = 0;
  }
  os_logbuffer[len++] = c;
  os_logbuffer[len] = '\0';
}

// call in main loop
void os_log_write() {
  if (os_logbuffer[0] != '\0' && os_log_write_timer < millis()) {
    File f = SPIFFS.open(logfile, "a");
    if (f) {
      f.print(os_logbuffer);
      f.flush();
      f.close();
      os_logbuffer[0] = '\0';
    } else {
      DEBUG_LN("ERROR: os_log_write");
    }
    os_log_write_timer = millis() + 100;
  }
}

String ls(char const* path) {
  char buf[256];
  String ls;
  Dir dir = SPIFFS.openDir(path);
  while (dir.next()) {
    if(ls.length() == 0) {
      ls += path;
      ls += " listing:\n";
    } else {
      ls += "\n";
    }
    // SPIFFS has no isFile() or fileTime()
    sprintf_P(buf, ll_fmt, "-", dir.fileSize(), "01 Jan 1970");
    ls += buf;
    // fileName is a String, unable to pass to sprintf
    ls += dir.fileName();
  }
  return ls;
}

// Debug: log directory listing
void log_ls(char const* path) {
  char buf[1024];
  String list = ls(path);
  list.toCharArray(buf, 1024);
  debugLog(LOG_DEBUG, "SPIFFS", buf);
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

uint32_t stackSize() {
  char stack;
  return stack_start - &stack;
}

// Standard DRAM: 98kB
// Stack size: 4kB
#define DRAM_SIZE 98 * 1024
#define STACK_SIZE 4 * 1024
void log_meminfo(const char* context) {
  uint32_t freehp = ESP.getFreeHeap();
  uint32_t stack = stackSize();
  uint32_t freestk = STACK_SIZE - stack;
  uint32_t heap = DRAM_SIZE - freehp - STACK_SIZE;
  char buf[160];

  sprintf_P(buf, meminfo_fmt, heap, stack, freehp, freestk, deviceSettings.memoryUsage());
  debugLog(LOG_INFO, context, buf);
}

// create log if not exist & log file size and SPIFFS space remaining
void debugLogSetup() {
  size_t fsize = 0;
  char buf[160];
  File f = SPIFFS.open(logfile, "r");

  FSInfo fs_info;
  SPIFFS.info(fs_info);

  if(f) {
    fsize = f.size();
    f.close();
  }
  sprintf_P(buf, setup_fmt, fsize,
    fs_info.totalBytes, fs_info.usedBytes, fs_info.totalBytes - fs_info.usedBytes,
    ESP.getVcc()
  );
  debugLog(LOG_INFO, startup, buf);
  log_meminfo(startup);
}

void debugLogWrite(const char* buf) {
  File f = SPIFFS.open(logfile, "a");
  if (f) {
    f.println(buf);
    f.close();
  }
  DEBUG_LN(buf);
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

// page 8 of ajax; truncate
bool debugLogClear() {
  String list = ls("/");
  File f = SPIFFS.open(logfile, "w");
  f.println(list);
  f.close();
  return true; // for ajax return value
}
