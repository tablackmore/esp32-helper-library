#include "DnsManager.h"
#include <Arduino.h>

void DnsManager::begin(const IPAddress &apIP, const char *hostname)
{
    // Start DNS Server
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", apIP);
    isInitialized = true;
    delay(100); // Allow DNS to start
    Serial.println("DNS Server started");

    // Initialize mDNS
    if (initializeMDNS(hostname))
    {
        Serial.println("mDNS responder started at http://esp32.local");
    }
    else
    {
        Serial.println("Error starting mDNS");
    }
}

bool DnsManager::initializeMDNS(const char *hostname)
{
    if (!MDNS.begin(hostname))
    {
        return false;
    }

    MDNS.addService("http", "tcp", 80);
    return true;
}

void DnsManager::processRequests()
{
    if (!isInitialized)
        return;

    try
    {
        dnsServer.processNextRequest();
    }
    catch (...)
    {
        delay(1);
    }
}