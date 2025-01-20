#pragma once

#include <WiFi.h>
#include <IPAddress.h>
#include <functional>
#include <vector>

class WiFiManager
{
public:
    using ConnectCallback = std::function<void(bool)>;

    static WiFiManager &getInstance()
    {
        static WiFiManager instance;
        return instance;
    }

    void beginAccessPoint(const char *ssid, const char *password);
    IPAddress getAccessPointIP() const { return WiFi.softAPIP(); }
    void setHostname(const char *hostname) { WiFi.setHostname(hostname); }

    // WiFi Station methods
    void tryLoadSavedNetwork(ConnectCallback callback);
    void startScan();
    void checkScanResult();
    void connectToNetwork(const char *ssid, const char *password, ConnectCallback callback);
    void saveNetwork(const char *ssid, const char *password);

private:
    WiFiManager();
    ~WiFiManager() = default;
    WiFiManager(const WiFiManager &) = delete;
    WiFiManager &operator=(const WiFiManager &) = delete;

    void configureAccessPoint();
    bool connectWithTimeout(const char *ssid, const char *password, unsigned long timeout);

    IPAddress apIP{192, 168, 4, 1};
    IPAddress apGateway{192, 168, 4, 1};
    IPAddress apSubnet{255, 255, 255, 0};

    ConnectCallback connectCallback = nullptr;
    bool scanning = false;
    unsigned long lastScanTime = 0;
    const unsigned long SCAN_INTERVAL = 10000; // 10 seconds
};