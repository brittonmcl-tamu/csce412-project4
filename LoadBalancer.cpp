/** @file LoadBalancer.cpp Request generation, assignment, processing, and reporting. */
#include "LoadBalancer.h"
#include <iostream>
#include <sstream>

/** @brief Read four decimal octets and convert them to one comparable number. */
static bool convertIP(const std::string& ip, unsigned long& number) {
    if (ip.empty() || ip.find_first_not_of("0123456789.") != std::string::npos) return false;
    std::istringstream input(ip);
    int a, b, c, d;
    char dot1, dot2, dot3;
    std::string extra;
    if (!(input >> a >> dot1 >> b >> dot2 >> c >> dot3 >> d) || (input >> extra)) {
        return false;
    }
    if (dot1 != '.' || dot2 != '.' || dot3 != '.' ||
        a < 0 || a > 255 || b < 0 || b > 255 ||
        c < 0 || c > 255 || d < 0 || d > 255) {
        return false;
    }
    // Each octet is a digit in base 256, like digits in a decimal number.
    number = ((static_cast<unsigned long>(a) * 256 + b) * 256 + c) * 256 + d;
    return true;
}

/** @brief Generate an IPv4 string using the caller's existing engine. */
static std::string generateRandomIP(std::mt19937& gen) {
    std::uniform_int_distribution<int> octetDist(0, 255);
    return std::to_string(octetDist(gen)) + "." +
           std::to_string(octetDist(gen)) + "." +
           std::to_string(octetDist(gen)) + "." +
           std::to_string(octetDist(gen));
}

LoadBalancer::LoadBalancer(int numServers) : gen(42) {
    webServers.resize(numServers);
    startingServers = numServers;
    mostServers = numServers;
}

bool LoadBalancer::setTaskTimes(int minimum, int maximum) {
    if (minimum < 1 || maximum < minimum) return false;
    minTaskTime = minimum;
    maxTaskTime = maximum;
    return true;
}

bool LoadBalancer::setBlockedRange(const std::string& range) {
    if (range == "none" || range == "0") {
        blockIPs = false;
        return true;
    }
    std::size_t dash = range.find('-');
    unsigned long first, last;
    if (!convertIP(range.substr(0, dash), first)) return false;
    if (dash == std::string::npos) last = first;
    else if (!convertIP(range.substr(dash + 1), last)) return false;
    if (first > last || (first == 0 && last == 4294967295UL)) return false;
    blockedStart = first;
    blockedEnd = last;
    blockIPs = true;
    return true;
}

void LoadBalancer::setRandomSeed(unsigned int seed) {
    randomSeed = seed;
    gen.seed(seed);
}

bool LoadBalancer::openLog(const std::string& filename, bool verboseMode, bool color) {
    verbose = verboseMode;
    useColor = color;
    logFile.open(filename);
    return logFile.is_open();
}

void LoadBalancer::logEvent(const std::string& message, const std::string& color) {
    if (!verbose) return;
    std::string line = "[cycle " + std::to_string(clockCycle) + "] " + message;
    if (useColor) std::cout << "\033[" << color << "m";
    std::cout << line;
    if (useColor) std::cout << "\033[0m";
    std::cout << '\n';
    logFile << line << '\n';
}

Request LoadBalancer::generateRandomRequest() {
    Request req;
    std::uniform_int_distribution<int> jobDist(0, 1);
    std::uniform_int_distribution<int> timeDist(minTaskTime, maxTaskTime);
    req.ipIn = generateRandomIP(gen);
    req.ipOut = generateRandomIP(gen);
    req.processingTime = timeDist(gen);
    req.jobType = jobDist(gen) == 0 ? 'P' : 'S';
    return req;
}

bool LoadBalancer::addRequest(const Request& req) {
    generatedRequests++;
    unsigned long source, destination;
    if (!convertIP(req.ipIn, source) || !convertIP(req.ipOut, destination) ||
        req.processingTime < 1 || (req.jobType != 'P' && req.jobType != 'S')) {
        invalidRequests++;
        logEvent("REJECTED invalid request", "31");
        return false;
    }
    if (blockIPs && source >= blockedStart && source <= blockedEnd) {
        blockedRequests++;
        logEvent("BLOCKED source=" + req.ipIn, "31");
        return false;
    }
    requestQueue.push(req);
    acceptedRequests++;
    if (requestQueue.size() > largestQueue) largestQueue = requestQueue.size();
    logEvent("QUEUED source=" + req.ipIn + " type=" + req.jobType +
             " duration=" + std::to_string(req.processingTime), "34");
    return true;
}

bool LoadBalancer::generateRequestQueue(int numServers) {
    if (numServers < 1 || numServers > 10000) return false;
    int accepted = 0;
    int attempts = 0;
    while (accepted < 100 * numServers) {
        if (++attempts > 10000 * numServers + 1000) return false;
        Request req = generateRandomRequest();
        if (addRequest(req)) accepted++;
    }
    startingQueue = requestQueue.size();
    return true;
}

void LoadBalancer::distributeRequests() {
    int serverNumber = 0;
    for (auto& server : webServers) {
        if (!requestQueue.empty() && server.isAvailable()) {
            Request req = requestQueue.front();
            if (server.assignRequest(req)) {
                requestQueue.pop();
                assignedRequests++;
                logEvent("ASSIGNED server=" + std::to_string(serverNumber) +
                         " duration=" + std::to_string(req.processingTime));
            }
        }
        serverNumber++;
    }
}

void LoadBalancer::advanceCycle() {
    clockCycle++;
    int serverNumber = 0;
    for (auto& server : webServers) {
        bool wasBusy = !server.isAvailable();
        server.advanceCycle();
        if (wasBusy && server.isAvailable()) {
            completedRequests++;
            logEvent("COMPLETED server=" + std::to_string(serverNumber), "32");
        }
        serverNumber++;
    }
}

void LoadBalancer::addWebServer() {
    webServers.emplace_back();
    serversAdded++;
    if (getServerCount() > mostServers) mostServers = getServerCount();
    logEvent("SCALE UP servers=" + std::to_string(getServerCount()), "33");
}

bool LoadBalancer::removeWebServer() {
    if (webServers.size() <= 1) return false;
    for (std::size_t i = 0; i < webServers.size(); i++) {
        if (webServers[i].isAvailable()) {
            webServers.erase(webServers.begin() + i);
            serversRemoved++;
            logEvent("SCALE DOWN servers=" + std::to_string(getServerCount()), "33");
            return true;
        }
    }
    deferredRemovals++;
    logEvent("SCALE DOWN deferred: all servers busy", "33");
    return false;
}

std::size_t LoadBalancer::getRequestQueueSize() const { return requestQueue.size(); }
int LoadBalancer::getServerCount() const { return static_cast<int>(webServers.size()); }
long long LoadBalancer::getCompletedRequests() const { return completedRequests; }
long long LoadBalancer::getAcceptedRequests() const { return acceptedRequests; }
long long LoadBalancer::getBlockedRequests() const { return blockedRequests; }

int LoadBalancer::getBusyServerCount() const {
    int busyServers = 0;
    for (const auto& server : webServers) {
        if (!server.isAvailable()) busyServers++;
    }
    return busyServers;
}

void LoadBalancer::printStart(int totalCycles) {
    std::ostringstream output;
    output << "START\nStarting servers: " << startingServers
           << "\nStarting queue size: " << startingQueue
           << "\nRequested cycles: " << totalCycles
           << "\nTask-time range: " << minTaskTime << '-' << maxTaskTime
           << "\nRandom seed: " << randomSeed
           << "\nMode: " << (verbose ? "verbose" : "quiet") << "\n\n";
    std::cout << output.str();
    logFile << output.str();
}

bool LoadBalancer::printSummary() {
    int busyServers = getBusyServerCount();
    bool balanced = acceptedRequests == completedRequests +
                    static_cast<long long>(requestQueue.size()) + busyServers;
    balanced = balanced && generatedRequests == acceptedRequests + blockedRequests + invalidRequests;
    balanced = balanced && assignedRequests == completedRequests + busyServers;
    std::ostringstream output;
    output << "END\nElapsed cycles: " << clockCycle
           << "\nStarting queue size: " << startingQueue
           << "\nEnding queue size: " << requestQueue.size()
           << "\nTask-time range: " << minTaskTime << '-' << maxTaskTime
           << "\nGenerated requests (including initial attempts): " << generatedRequests
           << "\nAccepted requests: " << acceptedRequests
           << "\nBlocked requests: " << blockedRequests
           << "\nInvalid requests: " << invalidRequests
           << "\nAssigned requests: " << assignedRequests
           << "\nCompleted requests: " << completedRequests
           << "\nRequests in progress: " << busyServers
           << "\nEnding servers: " << webServers.size()
           << "\nBusy servers: " << busyServers
           << "\nIdle servers: " << getServerCount() - busyServers
           << "\nServers added: " << serversAdded
           << "\nServers removed: " << serversRemoved
           << "\nDeferred removals: " << deferredRemovals
           << "\nPeak queue size: " << largestQueue
           << "\nPeak servers: " << mostServers
           << "\nWork conservation: " << (balanced ? "PASS" : "FAIL") << '\n';
    std::cout << output.str();
    logFile << output.str();
    logFile.flush();
    return balanced && logFile.good();
}
