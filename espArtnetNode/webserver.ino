/*
 * Copyright (c) 2023, Richard Case
 * 
 * Port to AsyncWebServer
 */

#include "debugLog.h"

// directory, size, date. name is done after
const char ll_fmt[] PROGMEM = "%s %u %s ";

// Debug: log directory listing
//void log_ls(char const* path) {
//  char buf[1024];
//  String list = ls(path);
//  list.toCharArray(buf, 1024);
//  debugLog(LOG_DEBUG, "Webserver", buf);
//}

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

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found!");
}

void webRespond(AsyncWebServerRequest *request, String json) {
  AsyncWebServerResponse *r = request->beginResponse(200, JSON_MIMETYPE, json);
  r->addHeader("Connection", "close");
  r->addHeader("Access-Control-Allow-Origin", "*");
  request->send(r);
}

void handleFirmwareStatus(AsyncWebServerRequest *request) {
  String fail = "{\"success\":0,\"message\":\"Unknown Error\"}";
  String ok = "{\"success\":1,\"message\":\"Success: Device restarting\"}";

  webRespond(request, (Update.hasError()) ? fail : ok);

  doReboot = true;
}

void handleFirmwareUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  (void) filename;
  // json response message
  String reply = "";
  if(index == 0){
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if(!Update.begin(maxSketchSpace)){//start with max available size
      reply = "{\"success\":0,\"message\":\"Insufficient space.\"}";
    }
  }
  if(reply.length() == 0 && Update.write(data, len) != len){
    reply = "{\"success\":0,\"message\":\"Failed to save\"}";
  }
  if(reply.length() == 0 && final){
    if(Update.end(true)){ //true to set the size to the current progress
      reply = "{\"success\":1,\"message\":\"Success: Device Restarting\"}";
    } else {
      reply = "{\"success\":0,\"message\":\"Unknown Error\"}";
    }
  }

  if (reply.length() > 0) {
    webRespond(request, reply);
  }
}

void handleStyleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  (void) request;
  (void) filename;
  if(index == 0){
    SPIFFS.rename("/style.css", "/style.css.old");
    fsUploadFile = SPIFFS.open("/style.css", "w");
  }

  if(fsUploadFile) fsUploadFile.write(data, len);
  if(final && fsUploadFile) fsUploadFile.close();
}

void handleStyleDelete(AsyncWebServerRequest *request){
  AsyncWebServerResponse *r;
  if (SPIFFS.exists("/style.css.old")) {
    r = request->beginResponse(200, "text/plain", "style.css deleted. The default style was restored.");
    if (SPIFFS.exists("/style.css")) SPIFFS.remove("/style.css");
    SPIFFS.rename("/style.css.old", "/style.css");
  } else {
    r = request->beginResponse(403, "text/plain", "Forbidden. No backup for style.css.");
  }
  r->addHeader("Connection", "close");
  request->send(r);
}

void webSetup() {
  // Set utf-8 as default charset
  DefaultHeaders::Instance().addHeader("charset", "utf-8");
  // Serve all files!
  webServer.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setCacheControl("max-age=600");

  webServer.addHandler(ajaxHandler);
  webServer.on("/upload", HTTP_POST, handleFirmwareStatus, handleFirmwareUpload);

  webServer.on("/style_upload", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Upload successful!");
  }, handleStyleUpload);
  webServer.on("/style_delete", HTTP_GET, handleStyleDelete);

  webServer.onNotFound(notFound);
}
