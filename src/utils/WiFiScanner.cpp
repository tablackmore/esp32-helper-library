#include "WiFiScanner.h"
#include <Preferences.h>
#include <vector>

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
        startScan();
    }
    else if (n >= 0)
    {
        Serial.printf("DEBUG: Using available results: %d networks\n", n);
        processResults(n);
    }
    else if (n == -1)
    {
        Serial.println("DEBUG: Scan already in progress, waiting for results");
        scanInProgress = true;
    }
}

void WiFiScanner::checkScanResult()
{
    int scanStatus = WiFi.scanComplete();

    static int lastStatus = -99;
    if (scanStatus != lastStatus)
    {
        Serial.printf("DEBUG: Scan status changed: %d\n", scanStatus);
        lastStatus = scanStatus;

        // If we detect a completed scan but weren't tracking it
        if (scanStatus >= 0 && !scanInProgress)
        {
            Serial.println("DEBUG: Detected completed scan that we weren't tracking");
            scanInProgress = true; // Set this so we'll process it
        }
    }

    if (!scanInProgress)
    {
        return;
    }

    if (millis() - scanStartTime > SCAN_TIMEOUT)
    {
        Serial.println("DEBUG: WiFi scan timeout after 10 seconds");
        scanInProgress = false;
        WiFi.scanDelete();

        JsonDocument doc;
        doc["type"] = "networkList";
        doc["networks"].to<JsonArray>(); // Create empty array with newer syntax

        String response;
        serializeJson(doc, response);

        for (auto &callback : subscribers)
        {
            if (callback)
                callback(response);
        }

        subscribers.clear();
        return;
    }

    if (scanStatus == WIFI_SCAN_RUNNING)
    {
        return;
    }

    scanInProgress = false;

    if (scanStatus < 0 && scanStatus != -2)
    {
        Serial.printf("DEBUG: WiFi scan failed with status %d\n", scanStatus);

        JsonDocument doc;
        doc["type"] = "networkList";
        doc["error"] = "Scan failed";
        doc["networks"].to<JsonArray>(); // Create empty array with newer syntax

        String response;
        serializeJson(doc, response);

        for (auto &callback : subscribers)
        {
            if (callback)
                callback(response);
        }

        subscribers.clear();
        return;
    }

    if (scanStatus >= 0)
    {
        Serial.printf("DEBUG: Scan completed with %d networks\n", scanStatus);
        processResults(scanStatus);
    }
}

void WiFiScanner::processResults(int numNetworks)
{
    Serial.printf("DEBUG: Processing %d networks\n", numNetworks);
    networkResults.clear();

    // Set the type for the response
    networkResults["type"] = "networkList";

    // Create networks array
    JsonArray networks = networkResults["networks"].to<JsonArray>();

    // First, collect all valid networks with deduplication
    std::vector<String> processedSSIDs;

    for (int i = 0; i < numNetworks; i++)
    {
        String ssid = WiFi.SSID(i);
        // Skip networks with empty SSIDs
        if (ssid.length() == 0)
        {
            Serial.println("DEBUG: Skipping network with empty SSID");
            continue;
        }

        // Check if we've already seen this SSID
        bool alreadyProcessed = false;
        for (const String &processedSSID : processedSSIDs)
        {
            if (processedSSID == ssid)
            {
                alreadyProcessed = true;
                break;
            }
        }

        if (alreadyProcessed)
        {
            continue; // Skip duplicates
        }

        // First time seeing this SSID
        processedSSIDs.push_back(ssid);

        // Find the strongest signal for this SSID
        int bestRSSI = WiFi.RSSI(i);
        bool encryption = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;

        for (int j = i + 1; j < numNetworks; j++)
        {
            if (WiFi.SSID(j) == ssid && WiFi.RSSI(j) > bestRSSI)
            {
                bestRSSI = WiFi.RSSI(j);
                encryption = WiFi.encryptionType(j) != WIFI_AUTH_OPEN;
            }
        }

        Serial.printf("DEBUG: Adding network: %s (RSSI: %d)\n", ssid.c_str(), bestRSSI);

        // Add to networks array
        JsonObject network = networks.add<JsonObject>();
        network["ssid"] = ssid;
        network["rssi"] = bestRSSI;
        network["encryption"] = encryption;
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
    Serial.printf("WiFiScanner: Connecting to network: %s\n", ssid.c_str());

    // Disconnect from any existing connection
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFiScanner: Disconnecting from current network");
        WiFi.disconnect();
        delay(500);
    }

    // Begin connection attempt
    Serial.printf("WiFiScanner: Starting connection to %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startTime = millis();
    Serial.println("WiFiScanner: Waiting for connection...");

    // Wait for connection with timeout
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < SCAN_TIMEOUT)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    // Prepare response
    JsonDocument response;
    response["type"] = "connectionStatus";

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFiScanner: Successfully connected to network");
        response["status"] = "Connected to " + ssid;
        response["ip"] = WiFi.localIP().toString();
        response["gateway"] = WiFi.gatewayIP().toString();
        response["subnet"] = WiFi.subnetMask().toString();

        // Save the network credentials
        preferences.begin("wifi-config", false);
        preferences.putString("ssid", ssid);
        preferences.putString("password", password);
        preferences.end();
        Serial.println("WiFiScanner: Network credentials saved");
    }
    else
    {
        Serial.println("WiFiScanner: Connection failed");
        response["status"] = "Connection failed";

        // Print the failure reason
        int status = WiFi.status();
        String statusStr;
        switch (status)
        {
        case WL_IDLE_STATUS:
            statusStr = "Idle";
            break;
        case WL_NO_SSID_AVAIL:
            statusStr = "No SSID available";
            break;
        case WL_SCAN_COMPLETED:
            statusStr = "Scan completed";
            break;
        case WL_CONNECT_FAILED:
            statusStr = "Connection failed";
            break;
        case WL_CONNECTION_LOST:
            statusStr = "Connection lost";
            break;
        case WL_DISCONNECTED:
            statusStr = "Disconnected";
            break;
        default:
            statusStr = "Unknown status: " + String(status);
            break;
        }
        Serial.printf("WiFiScanner: Connection status: %s\n", statusStr.c_str());
        response["error"] = statusStr;
    }

    String jsonResponse;
    serializeJson(response, jsonResponse);
    Serial.printf("WiFiScanner: Connection response: %s\n", jsonResponse.c_str());

    // Call the callback with the response
    if (callback)
    {
        Serial.println("WiFiScanner: Calling connection callback");
        callback(jsonResponse);
    }
    else
    {
        Serial.println("WiFiScanner: No callback provided");
    }
}

void WiFiScanner::tryLoadSavedNetwork(std::function<void(bool)> callback)
{
    preferences.begin("wifi-config", true);
    String savedSSID = preferences.getString("ssid", "");
    String savedPassword = preferences.getString("password", "");
    preferences.end();

    if (savedSSID.length() > 0)
    {
        // Start connection attempt
        WiFi.begin(savedSSID.c_str(), savedPassword.c_str());

        // Wait for connection (with timeout)
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
        { // 10 second timeout
            delay(500);
            Serial.print(".");
        }

        bool connected = (WiFi.status() == WL_CONNECTED);
        Serial.println();

        if (connected)
        {
            Serial.println("Successfully connected to saved network");
            callback(true);
        }
        else
        {
            Serial.println("Failed to connect to saved network");
            callback(false);
        }
    }

    callback(false);
}

void WiFiScanner::clearSavedNetwork()
{
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
}

void WiFiScanner::startScan()
{
    if (scanInProgress)
    {
        Serial.println("Scan already in progress");
        return;
    }

    Serial.println("Starting WiFi scan...");
    scanInProgress = true;
    scanStartTime = millis();

    int prevStatus = WiFi.scanComplete();
    if (prevStatus != WIFI_SCAN_RUNNING && prevStatus > 0)
    {
        WiFi.scanDelete();
    }

    bool result = WiFi.scanNetworks(true, true, false, 300);
    Serial.printf("Scan started: %s\n", result ? "true" : "false");
}

void WiFiScanner::handleScanResults(int numNetworks)
{
    if (numNetworks >= 0)
    {
        Serial.printf("DEBUG: Handling scan results: %d networks\n", numNetworks);
        processResults(numNetworks);
    }
}