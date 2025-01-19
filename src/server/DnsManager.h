#pragma once

#include <DNSServer.h>
#include <ESPmDNS.h>
#include <IPAddress.h>

class DnsManager
{
public:
    static DnsManager &getInstance()
    {
        static DnsManager instance;
        return instance;
    }

    void begin(const IPAddress &apIP, const char *hostname);
    void processRequests();

private:
    DnsManager() = default;
    ~DnsManager() = default;
    DnsManager(const DnsManager &) = delete;
    DnsManager &operator=(const DnsManager &) = delete;

    bool initializeMDNS(const char *hostname);

    DNSServer dnsServer;
    bool isInitialized = false;
};