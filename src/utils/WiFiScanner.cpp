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
    Serial.printf("WiFiScanner: SSID length: %d, Password length: %d\n", ssid.length(), password.length());

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

        // Save the network credentials to SPIFFS instead of Preferences for longer strings
        saveCredentialsToFile(ssid, password);
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

void WiFiScanner::saveCredentialsToFile(const String &ssid, const String &password)
{
    Serial.println("WiFiScanner: === SAVING CREDENTIALS TO FILE ===");
    Serial.printf("WiFiScanner: SSID: '%s' (length: %d)\n", ssid.c_str(), ssid.length());
    Serial.printf("WiFiScanner: Password length: %d\n", password.length());

    // Debug SPIFFS status
    Serial.printf("WiFiScanner: SPIFFS total bytes: %d, used bytes: %d\n",
                  SPIFFS.totalBytes(), SPIFFS.usedBytes());

    // Create a JSON document to store credentials
    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["password"] = password;

    // Debug JSON document
    String jsonDebug;
    serializeJson(doc, jsonDebug);
    Serial.printf("WiFiScanner: JSON to save: %s (length: %d)\n", jsonDebug.c_str(), jsonDebug.length());

    // Open file for writing
    File file = SPIFFS.open("/wifi_credentials.json", "w");
    if (!file)
    {
        Serial.println("WiFiScanner: ERROR - Failed to open credentials file for writing");
        // Try to diagnose the issue
        if (!SPIFFS.begin(true))
        {
            Serial.println("WiFiScanner: ERROR - SPIFFS mount failed");
        }
        else
        {
            Serial.println("WiFiScanner: SPIFFS is mounted");
        }
        return;
    }

    // Write JSON to file
    size_t bytesWritten = serializeJson(doc, file);
    file.close();

    if (bytesWritten == 0)
    {
        Serial.println("WiFiScanner: ERROR - Failed to write credentials to file (0 bytes written)");
    }
    else
    {
        Serial.printf("WiFiScanner: Credentials saved to file successfully (%d bytes written)\n", bytesWritten);
    }

    // Verify the file was written correctly
    verifyCredentialsFile(ssid);
}

void WiFiScanner::verifyCredentialsFile(const String &expectedSSID)
{
    Serial.println("WiFiScanner: === VERIFYING CREDENTIALS FILE ===");

    // Check if file exists
    if (!SPIFFS.exists("/wifi_credentials.json"))
    {
        Serial.println("WiFiScanner: ERROR - Credentials file not found after saving");

        // List all files in SPIFFS for debugging
        Serial.println("WiFiScanner: Files in SPIFFS:");
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while (file)
        {
            Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
            file = root.openNextFile();
        }
        return;
    }

    // Open file for reading
    File file = SPIFFS.open("/wifi_credentials.json", "r");
    if (!file)
    {
        Serial.println("WiFiScanner: ERROR - Failed to open credentials file for verification");
        return;
    }

    // Read file content
    String content = file.readString();
    file.close();

    Serial.printf("WiFiScanner: Credentials file content: '%s'\n", content.c_str());
    Serial.printf("WiFiScanner: Credentials file content length: %d\n", content.length());

    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);

    if (error)
    {
        Serial.printf("WiFiScanner: ERROR - Failed to parse credentials file: %s\n", error.c_str());
        return;
    }

    // Verify SSID and password exist - using newer ArduinoJson methods
    if (!doc["ssid"].is<String>())
    {
        Serial.println("WiFiScanner: ERROR - SSID key missing from credentials file");
        return;
    }

    if (!doc["password"].is<String>())
    {
        Serial.println("WiFiScanner: ERROR - Password key missing from credentials file");
        return;
    }

    // Verify SSID
    String savedSSID = doc["ssid"].as<String>();
    String savedPassword = doc["password"].as<String>();

    Serial.printf("WiFiScanner: Saved SSID: '%s' (length: %d)\n", savedSSID.c_str(), savedSSID.length());
    Serial.printf("WiFiScanner: Saved password length: %d\n", savedPassword.length());

    if (savedSSID == expectedSSID)
    {
        Serial.println("WiFiScanner: Credentials verified successfully");
    }
    else
    {
        Serial.printf("WiFiScanner: ERROR - Verification failed - Expected SSID: '%s', Found: '%s'\n",
                      expectedSSID.c_str(), savedSSID.c_str());
    }
}

void WiFiScanner::tryLoadSavedNetwork(std::function<void(bool)> callback)
{
    Serial.println("WiFiScanner: Trying to load saved network credentials");

    // Try to load from SPIFFS first
    if (loadCredentialsFromFile(callback))
    {
        return; // Successfully loaded from file
    }

    // Fall back to Preferences if file loading failed
    Serial.println("WiFiScanner: Falling back to Preferences for credentials");

    // Open preferences in read-only mode
    preferences.begin("wifi-config", true);

    // Check if we have saved credentials
    if (!preferences.isKey("ssid") || !preferences.isKey("password"))
    {
        Serial.println("WiFiScanner: No saved credentials found in Preferences");
        preferences.end();
        callback(false);
        return;
    }

    // Retrieve saved credentials
    String savedSSID = preferences.getString("ssid", "");
    String savedPassword = preferences.getString("password", "");
    preferences.end();

    if (savedSSID.length() == 0)
    {
        Serial.println("WiFiScanner: Saved SSID is empty");
        callback(false);
        return;
    }

    connectToSavedNetwork(savedSSID, savedPassword, callback);
}

bool WiFiScanner::loadCredentialsFromFile(std::function<void(bool)> callback)
{
    Serial.println("WiFiScanner: === LOADING CREDENTIALS FROM FILE ===");

    // Debug SPIFFS status
    Serial.printf("WiFiScanner: SPIFFS total bytes: %d, used bytes: %d\n",
                  SPIFFS.totalBytes(), SPIFFS.usedBytes());

    // Check if credentials file exists
    if (!SPIFFS.exists("/wifi_credentials.json"))
    {
        Serial.println("WiFiScanner: No credentials file found");

        // List all files in SPIFFS for debugging
        Serial.println("WiFiScanner: Files in SPIFFS:");
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while (file)
        {
            Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
            file = root.openNextFile();
        }
        return false;
    }

    // Open file for reading
    File file = SPIFFS.open("/wifi_credentials.json", "r");
    if (!file)
    {
        Serial.println("WiFiScanner: ERROR - Failed to open credentials file for reading");
        return false;
    }

    // Read file content
    String content = file.readString();
    file.close();

    Serial.printf("WiFiScanner: Read file content: '%s'\n", content.c_str());
    Serial.printf("WiFiScanner: File content length: %d\n", content.length());

    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);

    if (error)
    {
        Serial.printf("WiFiScanner: ERROR - Failed to parse credentials file: %s\n", error.c_str());
        return false;
    }

    // Check if required fields exist - using newer ArduinoJson methods
    if (!doc["ssid"].is<String>() || !doc["password"].is<String>())
    {
        Serial.println("WiFiScanner: ERROR - Required fields missing from credentials file");
        return false;
    }

    // Extract credentials
    String savedSSID = doc["ssid"].as<String>();
    String savedPassword = doc["password"].as<String>();

    if (savedSSID.length() == 0)
    {
        Serial.println("WiFiScanner: ERROR - SSID from file is empty");
        return false;
    }

    Serial.printf("WiFiScanner: Loaded credentials from file - SSID: '%s' (length: %d)\n",
                  savedSSID.c_str(), savedSSID.length());
    Serial.printf("WiFiScanner: Loaded password length: %d\n", savedPassword.length());

    connectToSavedNetwork(savedSSID, savedPassword, callback);
    return true;
}

void WiFiScanner::connectToSavedNetwork(const String &ssid, const String &password, std::function<void(bool)> callback)
{
    Serial.println("WiFiScanner: === CONNECTING TO SAVED NETWORK ===");
    Serial.printf("WiFiScanner: Connecting to saved network: '%s'\n", ssid.c_str());
    Serial.printf("WiFiScanner: SSID length: %d, Password length: %d\n", ssid.length(), password.length());

    // Debug current WiFi status
    int currentStatus = WiFi.status();
    Serial.printf("WiFiScanner: Current WiFi status: %d\n", currentStatus);

    // Disconnect from any existing connection
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFiScanner: Disconnecting from current network");
        WiFi.disconnect();
        delay(500);
        Serial.printf("WiFiScanner: WiFi status after disconnect: %d\n", WiFi.status());
    }

    // Set WiFi mode explicitly
    WiFi.mode(WIFI_AP_STA);
    Serial.printf("WiFiScanner: WiFi mode set to WIFI_AP_STA\n");

    // Start connection attempt
    Serial.printf("WiFiScanner: Calling WiFi.begin with SSID: '%s'\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait for connection (with timeout)
    unsigned long startTime = millis();
    Serial.println("WiFiScanner: Waiting for connection...");

    int lastStatus = WiFi.status();
    Serial.printf("WiFiScanner: Initial status: %d\n", lastStatus);

    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
    { // 10 second timeout
        delay(500);
        Serial.print(".");

        // Print status if it changes
        int currentStatus = WiFi.status();
        if (currentStatus != lastStatus)
        {
            Serial.printf("\nWiFiScanner: Status changed: %d -> %d\n", lastStatus, currentStatus);
            lastStatus = currentStatus;
        }
    }
    Serial.println();

    bool connected = (WiFi.status() == WL_CONNECTED);

    if (connected)
    {
        Serial.printf("WiFiScanner: Successfully connected to saved network. IP: %s\n",
                      WiFi.localIP().toString().c_str());
        Serial.printf("WiFiScanner: Gateway: %s, Subnet: %s\n",
                      WiFi.gatewayIP().toString().c_str(), WiFi.subnetMask().toString().c_str());
        callback(true);
    }
    else
    {
        Serial.println("WiFiScanner: Failed to connect to saved network");

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
        Serial.printf("WiFiScanner: Connection status: %s (%d)\n", statusStr.c_str(), status);

        callback(false);
    }
}

void WiFiScanner::clearSavedNetwork()
{
    Serial.println("WiFiScanner: === CLEARING SAVED NETWORK CREDENTIALS ===");

    // Debug SPIFFS status before clearing
    Serial.printf("WiFiScanner: SPIFFS total bytes: %d, used bytes: %d\n",
                  SPIFFS.totalBytes(), SPIFFS.usedBytes());

    // List files before clearing
    Serial.println("WiFiScanner: Files before clearing:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file)
    {
        Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
        file = root.openNextFile();
    }

    // Clear from SPIFFS
    if (SPIFFS.exists("/wifi_credentials.json"))
    {
        Serial.println("WiFiScanner: Removing credentials file...");
        if (SPIFFS.remove("/wifi_credentials.json"))
        {
            Serial.println("WiFiScanner: Credentials file removed successfully");
        }
        else
        {
            Serial.println("WiFiScanner: ERROR - Failed to remove credentials file");
        }
    }
    else
    {
        Serial.println("WiFiScanner: No credentials file found to remove");
    }

    // Also clear from Preferences for backward compatibility
    Serial.println("WiFiScanner: Clearing Preferences...");
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();

    // Verify that credentials were cleared
    bool fileExists = SPIFFS.exists("/wifi_credentials.json");
    Serial.printf("WiFiScanner: Credentials file exists after clearing: %s\n", fileExists ? "YES" : "NO");

    preferences.begin("wifi-config", true);
    bool prefsExist = preferences.isKey("ssid");
    preferences.end();
    Serial.printf("WiFiScanner: Preferences exist after clearing: %s\n", prefsExist ? "YES" : "NO");

    if (!fileExists && !prefsExist)
    {
        Serial.println("WiFiScanner: Network credentials cleared successfully");
    }
    else
    {
        Serial.println("WiFiScanner: WARNING - Failed to clear all network credentials");
        if (fileExists)
            Serial.println("WiFiScanner: Credentials file still exists");
        if (prefsExist)
            Serial.println("WiFiScanner: Preferences still exist");
    }

    // List files after clearing
    Serial.println("WiFiScanner: Files after clearing:");
    root = SPIFFS.open("/");
    file = root.openNextFile();
    while (file)
    {
        Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
        file = root.openNextFile();
    }
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