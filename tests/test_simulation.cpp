/** @file test_simulation.cpp Check processing, firewall admission, and safe removal. */
#include "LoadBalancer.h"
#include <cassert>
#include <iostream>

int main() {
    WebServer server;
    assert(server.isAvailable());
    assert(!server.assignRequest({"1.2.3.4", "5.6.7.8", 0, 'P'}));
    assert(server.assignRequest({"1.2.3.4", "5.6.7.8", 3, 'P'}));
    assert(!server.assignRequest({"1.2.3.4", "5.6.7.8", 1, 'S'}));
    server.advanceCycle(); server.advanceCycle();
    assert(!server.isAvailable());
    server.advanceCycle(); assert(server.isAvailable());
    server.advanceCycle(); assert(server.isAvailable());

    LoadBalancer loadBalancer(2);
    assert(loadBalancer.setTaskTimes(3, 3));
    assert(!loadBalancer.setTaskTimes(0, 3));
    assert(loadBalancer.setBlockedRange("192.168.1.0-192.168.1.255"));
    assert(!loadBalancer.setBlockedRange("2.0.0.0-1.0.0.0"));
    assert(!loadBalancer.setBlockedRange("0.0.0.0-255.255.255.255"));
    assert(!loadBalancer.setBlockedRange("1.2.3.4."));
    assert(loadBalancer.generateRequestQueue(2));
    assert(loadBalancer.getRequestQueueSize() == 200);
    long long accepted = loadBalancer.getAcceptedRequests();
    long long blocked = loadBalancer.getBlockedRequests();
    assert(!loadBalancer.addRequest({"192.168.1.0", "1.2.3.4", 1, 'P'}));
    assert(!loadBalancer.addRequest({"192.168.1.255", "1.2.3.4", 1, 'S'}));
    assert(loadBalancer.getBlockedRequests() == blocked + 2);
    for (const std::string ip : {"1.2.3", "1.2.3.4.", "1.2.3.256", "1.2.-1.4", "+1.2.3.4", "1..2.3"}) {
        assert(!loadBalancer.addRequest({ip, "5.6.7.8", 1, 'P'}));
    }
    assert(!loadBalancer.addRequest({"1.2.3.4", "5.6.7.8", -1, 'P'}));
    assert(loadBalancer.getAcceptedRequests() == accepted);
    loadBalancer.distributeRequests();
    assert(loadBalancer.getRequestQueueSize() == 198);
    assert(loadBalancer.getBusyServerCount() == 2);
    assert(!loadBalancer.removeWebServer());
    loadBalancer.addWebServer();
    assert(loadBalancer.getServerCount() == 3);
    assert(loadBalancer.removeWebServer());
    assert(loadBalancer.getBusyServerCount() == 2);
    loadBalancer.advanceCycle(); loadBalancer.advanceCycle();
    assert(loadBalancer.getCompletedRequests() == 0);
    loadBalancer.advanceCycle();
    assert(loadBalancer.getCompletedRequests() == 2);
    while (loadBalancer.getRequestQueueSize() || loadBalancer.getBusyServerCount()) {
        loadBalancer.distributeRequests(); loadBalancer.advanceCycle();
    }
    assert(loadBalancer.getCompletedRequests() == accepted);
    assert(loadBalancer.removeWebServer());
    assert(loadBalancer.getServerCount() == 1);
    assert(!loadBalancer.removeWebServer());

    LoadBalancer first(1), second(1);
    first.setRandomSeed(123); second.setRandomSeed(123);
    assert(first.setTaskTimes(2, 7)); assert(second.setTaskTimes(2, 7));
    for (int i = 0; i < 100; i++) {
        Request a = first.generateRandomRequest(), b = second.generateRandomRequest();
        assert(a.ipIn == b.ipIn && a.ipOut == b.ipOut);
        assert(a.processingTime == b.processingTime && a.jobType == b.jobType);
        assert(a.processingTime >= 2 && a.processingTime <= 7);
        assert(a.jobType == 'P' || a.jobType == 'S');
        assert(first.addRequest(a));
    }
    assert(first.getRequestQueueSize() == 100);
    std::cout << "Simulation checks passed\n";
}
