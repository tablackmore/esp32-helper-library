#include "WiFiManager.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

WiFiManager::WiFiManager()
{
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP_STA);
}

void WiFiManager::beginAccessPoint(const char *ssid, const char *password)
{
    configureAccessPoint();

    if (WiFi.softAP(ssid, password))
    {
        delay(100); // Brief delay for AP setup
        Serial.println("Access Point Created");
        Serial.print("AP IP address: ");
        Serial.println(WiFi.softAPIP());
        Serial.print("AP MAC address: ");
        Serial.println(WiFi.softAPmacAddress());
        Serial.print("STA MAC address: ");
        Serial.println(WiFi.macAddress());
        Serial.print("STA IP address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("Failed to create Access Point");
    }
}

void WiFiManager::configureAccessPoint()
{
    if (!WiFi.softAPConfig(apIP, apGateway, apSubnet))
    {
        Serial.println("AP Config Failed");
        return;
    }
    Serial.println("AP Config Success");
}

void WiFiManager::tryLoadSavedNetwork(ConnectCallback callback)
{
    if (!SPIFFS.exists("/wifi_config.json"))
    {
        if (callback)
            callback(false);
        return;
    }

    File configFile = SPIFFS.open("/wifi_config.json", "r");
    if (!configFile)
    {
        if (callback)
            callback(false);
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error)
    {
        Serial.println("Failed to parse wifi config file");
        if (callback)
            callback(false);
        return;
    }

    const char *ssid = doc["ssid"];
    const char *password = doc["password"];

    connectToNetwork(ssid, password, callback);
}

void WiFiManager::startScan()
{
    if (!scanning && millis() - lastScanTime >= SCAN_INTERVAL)
    {
        Serial.println("Starting WiFi scan...");
        WiFi.scanNetworks(true);
        scanning = true;
        lastScanTime = millis();
    }
}

void WiFiManager::checkScanResult()
{
    if (scanning)
    {
        int result = WiFi.scanComplete();
        if (result >= 0)
        {
            scanning = false;
            Serial.printf("%d networks found\n", result);
            WiFi.scanDelete();
        }
        else if (result == WIFI_SCAN_FAILED)
        {
            Serial.println("WiFi scan failed");
            scanning = false;
            WiFi.scanDelete();
        }
    }
}

void WiFiManager::connectToNetwork(const char *ssid, const char *password, ConnectCallback callback)
{
    connectCallback = callback;

    if (connectWithTimeout(ssid, password, 10000))
    {
        Serial.println("Connected to WiFi");
        if (connectCallback)
            connectCallback(true);
    }
    else
    {
        Serial.println("Failed to connect to WiFi");
        if (connectCallback)
            connectCallback(false);
    }
}

void WiFiManager::saveNetwork(const char *ssid, const char *password)
{
    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["password"] = password;

    File configFile = SPIFFS.open("/wifi_config.json", "w");
    if (!configFile)
    {
        Serial.println("Failed to open config file for writing");
        return;
    }

    serializeJson(doc, configFile);
    configFile.close();
}

bool WiFiManager::connectWithTimeout(const char *ssid, const char *password, unsigned long timeout)
{
    Serial.printf("Connecting to %s\n", ssid);

    WiFi.begin(ssid, password);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    return WiFi.status() == WL_CONNECTED;
}