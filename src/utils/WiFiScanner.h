#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>

typedef std::function<void(const String &)> ScanResultCallback;

class WiFiScanner
{
public:
    static WiFiScanner &getInstance();
    void subscribe(ScanResultCallback callback);
    void connectToNetwork(const String &ssid, const String &password, ScanResultCallback callback);
    void checkScanResult(); // New public method for loop
    void tryLoadSavedNetwork(std::function<void(bool)> callback);
    void clearSavedNetwork();

private:
    WiFiScanner();
    ~WiFiScanner();

    void processResults(int numNetworks);

    static const unsigned long CACHE_DURATION = 30000;
    static const unsigned long SCAN_TIMEOUT = 10000;

    std::vector<ScanResultCallback> subscribers;
    JsonDocument networkResults;
    unsigned long lastScanTime = 0;
    bool hasCachedResults = false;
    bool isScanning = false;
};

#endif