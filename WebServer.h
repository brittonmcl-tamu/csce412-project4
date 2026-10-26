#pragma once
#include "Request.h"
/** @file WebServer.h Single-request simulated worker. */
/** @brief Processes at most one request at a time. */
class WebServer {
private:
    Request request{};
    bool busy = false;
    int remainingTime = 0;
public:
    /** @brief Start idle with zero remaining work. */
    WebServer();
    /** @brief Accept positive-duration work if idle; return success. */
    bool assignRequest(Request req);
    /** @brief Return whether the server can accept a request. */
    bool isAvailable() const;
    /** @brief Consume one processing cycle, becoming idle on completion. */
    void advanceCycle();
};
