#include "WiFiScanner.h"
#include <Preferences.h>

Preferences preferences;

WiFiScanner::WiFiScanner() {}
WiFiScanner::~WiFiScanner() {}

WiFiScanner &WiFiScanner::getInstance()
{
    static WiFiScanner instance;
    return instance;
}

void WiFiScanner::subscribe(ScanResultCallback callback)
{
    if (hasCachedResults && (millis() - lastScanTime < CACHE_DURATION))
    {
        Serial.println("DEBUG: Using cached results from " + String(millis() - lastScanTime) + "ms ago");
        String response;
        serializeJson(networkResults, response);
        callback(response);
        return;
    }

    Serial.println("DEBUG: Cache miss or expired, adding subscriber");
    subscribers.push_back(callback);

    int n = WiFi.scanComplete();
    Serial.printf("DEBUG: Current scan status: %d\n", n);

    if (n == -2)
    {
        Serial.println("DEBUG: Starting new async scan");
        WiFi.scanNetworks(true);
        isScanning = true;
    }
    else if (n >= 0)
    {
        Serial.printf("DEBUG: Using available results: %d networks\n", n);
        processResults(n);
    }
}

void WiFiScanner::checkScanResult()
{
    if (!isScanning)
    {
        return;
    }

    int scanStatus = WiFi.scanComplete();
    Serial.printf("DEBUG: Scan status: %d\n", scanStatus);

    if (scanStatus == -1)
    {
        Serial.println("DEBUG: Scan still in progress");
    }
    else if (scanStatus >= 0)
    {
        Serial.printf("DEBUG: Scan complete, found %d networks\n", scanStatus);
        processResults(scanStatus);
        isScanning = false;
    }
}

void WiFiScanner::processResults(int numNetworks)
{
    Serial.printf("DEBUG: Processing %d networks\n", numNetworks);
    networkResults.clear();
    JsonArray networks = networkResults.to<JsonArray>();

    for (int i = 0; i < numNetworks; i++)
    {
        Serial.printf("DEBUG: Processing network %d: %s\n", i, WiFi.SSID(i).c_str());
        JsonObject network = networks.add<JsonObject>();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["bssid"] = WiFi.BSSIDstr(i);
        network["channel"] = WiFi.channel(i);
        network["encryption"] = WiFi.encryptionType(i);
    }

    String response;
    serializeJson(networkResults, response);
    Serial.printf("DEBUG: JSON response: %s\n", response.c_str());

    Serial.printf("DEBUG: Notifying %d subscribers\n", subscribers.size());
    for (auto &callback : subscribers)
    {
        if (callback)
            callback(response);
    }

    subscribers.clear();
    lastScanTime = millis();
    hasCachedResults = true;
    Serial.println("DEBUG: Results cached and subscribers notified");

    WiFi.scanDelete();
    Serial.println("DEBUG: Scan results cleared from WiFi");
}

void WiFiScanner::connectToNetwork(const String &ssid, const String &password, ScanResultCallback callback)
{
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < SCAN_TIMEOUT)
    {
        delay(500);
    }

    JsonDocument response;
    response["type"] = "connectionStatus";

    if (WiFi.status() == WL_CONNECTED)
    {
        response["status"] = "Connected to " + ssid;
        response["ip"] = WiFi.localIP().toString();
        response["gateway"] = WiFi.gatewayIP().toString();
        response["subnet"] = WiFi.subnetMask().toString();
        preferences.begin("wifi-config", false);
        preferences.putString("ssid", ssid);
        preferences.putString("password", password);
        preferences.end();
    }
    else
    {
        response["status"] = "Connection failed";
    }

    String jsonResponse;
    serializeJson(response, jsonResponse);
    Serial.printf("DEBUG: Connection response: %s\n", jsonResponse.c_str());
    callback(jsonResponse);
}

bool WiFiScanner::tryLoadSavedNetwork(std::function<void(bool)> callback) {
    preferences.begin("wifi-config", true);
    String savedSSID = preferences.getString("ssid", "");
    String savedPassword = preferences.getString("password", "");
    preferences.end();
    
    if (savedSSID.length() > 0) {
        connectToNetwork(savedSSID.c_str(), savedPassword.c_str(), callback);
        return true;
    }
    return false;
}

void WiFiScanner::clearSavedNetwork() {
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
}