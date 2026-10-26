/** @file WebServer.cpp Request assignment and processing countdown. */
#include "WebServer.h"

WebServer::WebServer() : busy(false), remainingTime(0) {}

bool WebServer::assignRequest(Request req) {
    if (busy || req.processingTime <= 0) {
        return false;
    }
    request = req;
    busy = true;
    remainingTime = req.processingTime;
    return true;
}

bool WebServer::isAvailable() const {
    return !busy;
}

void WebServer::advanceCycle() {
    if (busy) {
        remainingTime--;
        if (remainingTime <= 0) {
            busy = false;
        }
    }
}
