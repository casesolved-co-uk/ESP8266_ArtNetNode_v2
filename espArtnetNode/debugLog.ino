/*
 * Copyright (c) 2023, Richard Case
 * 
 * For storing a debug log in the SPIFFS
 */

#include <FS.h> // SPIFFS
#include "debugLog.h"

#define BUFSIZE 1024
#define FORMATLEN 15

const char logfile[] = "/debug.log";
const char format[] PROGMEM = "%s-%s: %s";
const char setup_fmt[] PROGMEM = "Logfile Size: %u, SPIFFS Total: %u, Used %u, Remaining: %u (bytes)";

void os_log(char c) {
  File f = SPIFFS.open(logfile, "a");
  f.write(c);
  f.close();
}

// create log if not exist & log file size and SPIFFS space remaining
void debugLogSetup() {
  size_t fsize = 0;
  char buf[160];
  FSInfo fs_info;
  SPIFFS.info(fs_info);

  if(SPIFFS.exists(logfile)) {
    File f = SPIFFS.open(logfile, "r");
    fsize = f.size();
    f.close();
  }
  sprintf_P(buf, setup_fmt, fsize, fs_info.totalBytes, fs_info.usedBytes, fs_info.totalBytes - fs_info.usedBytes);
  debugLog(LOG_INFO, "Startup", buf);
  // overrides OS UART logging:
  os_install_putc1((void*)os_log);
}

// Allows FORMATLEN characters for log level, context and other format characters
int debugLog(enum loglevel_t level, const char* context, const char* text) {
  char buf[BUFSIZE];
  int len = strlen(text);

  if(len >= (BUFSIZE - FORMATLEN)) {
    sprintf_P(buf, format, loglevelstr[level], context, "String too long");
    len = -1;
  } else {
    sprintf_P(buf, format, loglevelstr[level], context, text);
  }

  File f = SPIFFS.open(logfile, "a");
  f.println(buf);
  f.close();
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
