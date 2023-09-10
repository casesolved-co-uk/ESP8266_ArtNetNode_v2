

#ifndef debugLog_h
#define debugLog_h

enum loglevel_t {
  LOG_DEBUG = 0,
  LOG_INFO,
  LOG_WARN,
  LOG_ERROR
};

const char* loglevelstr[] = {
  "DEBUG", 
  "INFO",
  "WARN",
  "ERROR"
};

#endif
