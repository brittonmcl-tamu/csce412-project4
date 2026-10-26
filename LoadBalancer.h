#pragma once
#include <fstream>
#include <queue>
#include <random>
#include <string>
#include <vector>
#include "WebServer.h"

/** @file LoadBalancer.h The request queue, server pool, and simulation counters. */
/** @brief Assigns waiting requests to servers and records what happens. */
class LoadBalancer {
    private:
        std::queue<Request> requestQueue;
        std::vector<WebServer> webServers;
        std::mt19937 gen;
        std::ofstream logFile;

        int clockCycle = 0;
        int startingServers = 0;
        std::size_t startingQueue = 0;
        std::size_t largestQueue = 0;
        int mostServers = 0;
        int minTaskTime = 1;
        int maxTaskTime = 5;
        unsigned int randomSeed = 42;
        bool verbose = false;
        bool useColor = false;
        bool blockIPs = false;
        unsigned long blockedStart = 0;
        unsigned long blockedEnd = 0;

        long long generatedRequests = 0;
        long long acceptedRequests = 0;
        long long blockedRequests = 0;
        long long invalidRequests = 0;
        long long assignedRequests = 0;
        long long completedRequests = 0;
        int serversAdded = 0;
        int serversRemoved = 0;
        int deferredRemovals = 0;

        /** @brief Write an event to the terminal and file in verbose mode. */
        void logEvent(const std::string& message, const std::string& color = "36");

    public:
        /** @brief Create numServers idle workers; caller supplies a positive count. */
        LoadBalancer(int numServers);
        /** @brief Set the positive, inclusive task-time range before generating work. */
        bool setTaskTimes(int minimum, int maximum);
        /** @brief Configure an inclusive IP range, a single IP, or none/0. */
        bool setBlockedRange(const std::string& range);
        /** @brief Seed the engine once before generating requests. */
        void setRandomSeed(unsigned int seed);
        /** @brief Open the event/summary file and select quiet or verbose output. */
        bool openLog(const std::string& filename, bool verboseMode, bool color);
        /** @brief Queue valid, unblocked work; return whether it was accepted. */
        bool addRequest(const Request& req);
        /** @brief Give the oldest waiting requests to idle workers. */
        void distributeRequests();
        /** @brief Advance every worker once and count newly completed requests. */
        void advanceCycle();
        /** @brief Make one random request without putting it in the queue. */
        Request generateRandomRequest();
        /** @brief Append 100 accepted requests per server; return false on retry limit. */
        bool generateRequestQueue(int numServers);
        /** @brief Add one server to the pool. */
        void addWebServer();
        /** @brief Remove one idle server, keeping at least one; report success. */
        bool removeWebServer();
        /** @brief Number of requests waiting, excluding requests being processed. */
        std::size_t getRequestQueueSize() const;
        /** @brief Current number of allocated servers. */
        int getServerCount() const;
        /** @brief Number of servers currently processing requests. */
        int getBusyServerCount() const;
        /** @brief Number of requests that finished processing. */
        long long getCompletedRequests() const;
        /** @brief Number of requests accepted, including the initial queue. */
        long long getAcceptedRequests() const;
        /** @brief Number of requests rejected by the IP blocker. */
        long long getBlockedRequests() const;
        /** @brief Print starting conditions to the terminal and log. */
        void printStart(int totalCycles);
        /** @brief Print final statistics and check that no accepted work was lost. */
        bool printSummary();
};
