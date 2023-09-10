/* racitup
 *  for storing a debug log in the SPIFFS
 */

#include <FS.h> // SPIFFS
#include "debugLog.h"

#define BUFSIZE 160
#define FORMATLEN 15

const char logfile[] = "/debug.log";
const char format[] PROGMEM = "%s-%s: %s";
const char setup_fmt[] PROGMEM = "Logfile Size: %u, SPIFFS Total: %u, SPIFFS Used %u (bytes)";

// create log if not exist & log file size and SPIFFS space remaining
void debugLogSetup() {
  size_t fsize = 0;
  char buf[BUFSIZE];
  FSInfo fs_info;
  SPIFFS.info(fs_info);

  if(SPIFFS.exists(logfile)) {
    File f = SPIFFS.open(logfile, "r");
    fsize = f.size();
    f.close();
  }
  sprintf_P(buf, setup_fmt, fsize, fs_info.totalBytes, fs_info.usedBytes);
  //debugLog(LOG_INFO, "Startup", buf);
}

// Allows FORMATLEN characters for log level, context and other format characters
bool debugLog(enum loglevel_t level, const char* context, const char* text) {
  char buf[BUFSIZE];
  bool ret = true;

  if(strlen(text) >= (BUFSIZE - FORMATLEN)) {
    sprintf_P(buf, format, loglevelstr[level], context, "String too long");
    ret = false;
  } else {
    sprintf_P(buf, format, loglevelstr[level], context, text);
  }

  File f = SPIFFS.open(logfile, "a");
  f.print(buf);
  f.write({ 0x00 }, 1); // nul termination character
  f.close();
  return ret;
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
