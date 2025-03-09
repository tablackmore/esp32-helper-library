#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <functional>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Define the WiFiNetwork struct
struct WiFiNetwork
{
    String ssid;
    int rssi;
    bool encryption;
};

class WiFiScanner
{
public:
    static WiFiScanner &getInstance();

    // Callback type for scan results
    using ScanResultCallback = std::function<void(const String &)>;

    void subscribe(ScanResultCallback callback);
    void connectToNetwork(const String &ssid, const String &password, ScanResultCallback callback);
    void tryLoadSavedNetwork(std::function<void(bool)> callback);
    void clearSavedNetwork();

    // Add these declarations
    void startScan();
    void checkScanResult();

    // Add this public method
    void handleScanResults(int numNetworks);

private:
    WiFiScanner();
    ~WiFiScanner();

    void processResults(int numNetworks);

    // New methods for handling credentials in SPIFFS
    void saveCredentialsToFile(const String &ssid, const String &password);
    void verifyCredentialsFile(const String &expectedSSID);
    bool loadCredentialsFromFile(std::function<void(bool)> callback);
    void connectToSavedNetwork(const String &ssid, const String &password, std::function<void(bool)> callback);

    std::vector<ScanResultCallback> subscribers;
    JsonDocument networkResults;

    static const unsigned long SCAN_TIMEOUT = 10000;
    static const unsigned long CACHE_DURATION = 30000;

    unsigned long lastScanTime = 0;
    bool hasCachedResults = false;
    bool isScanning = false;

    // Add these variables
    bool scanInProgress = false;
    unsigned long scanStartTime = 0;
};