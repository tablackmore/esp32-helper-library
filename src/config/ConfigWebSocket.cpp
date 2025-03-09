#include "ConfigWebSocket.h"
#include "../utils/WiFiScanner.h"
#include "../utils/FileManager.h"
#include <ArduinoJson.h>

const char *ConfigWebSocket::WEBSOCKET_PATH = "/config";

ConfigWebSocket &ConfigWebSocket::getInstance()
{
    static ConfigWebSocket instance;
    return instance;
}

void ConfigWebSocket::begin(AsyncWebServer *server)
{
    Serial.println("Initializing Config WebSocket...");
    ws.onEvent([this](AsyncWebSocket *s, AsyncWebSocketClient *c,
                      AwsEventType t, void *arg, uint8_t *d, size_t l)
               { handleEvent(s, c, t, arg, d, l); });
    server->addHandler(&ws);
    Serial.println("Config WebSocket handler added");
}

void ConfigWebSocket::handleEvent(AsyncWebSocket *server,
                                  AsyncWebSocketClient *client,
                                  AwsEventType type,
                                  void *arg,
                                  uint8_t *data,
                                  size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("Config WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("Config WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketData(client, arg, data, len);
        break;
    case WS_EVT_ERROR:
        Serial.printf("Config WebSocket client #%u error(%u): %s\n", client->id(), *((uint16_t *)arg), (char *)data);
        break;
    }
}

void ConfigWebSocket::handleWebSocketData(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (!info || !data || len == 0)
    {
        Serial.println("Invalid WebSocket data received");
        return;
    }

    if (info->opcode != WS_TEXT)
    {
        Serial.println("Non-text WebSocket message received");
        return;
    }

    // Create a null-terminated string from the data
    char *message = (char *)malloc(len + 1);
    if (!message)
    {
        Serial.println("Failed to allocate memory for message");
        return;
    }
    memcpy(message, data, len);
    message[len] = '\0';

    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    free(message); // Free the allocated memory

    if (error)
    {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return;
    }

    // Process the message
    const char *type = doc["type"] | "";
    Serial.printf("Received config message type: %s\n", type);

    if (strcmp(type, "scanNetworks") == 0)
    {
        handleScanNetworksRequest(client);
    }
    else if (strcmp(type, "connectToNetwork") == 0 || strcmp(type, "connect") == 0)
    {
        const char *ssid = doc["ssid"] | "";
        const char *password = doc["password"] | "";
        Serial.printf("Connecting to network: %s with password: %s\n", ssid, password);
        handleConnectToNetworkRequest(client, ssid, password);
    }
    else if (strcmp(type, "getFiles") == 0)
    {
        handleGetFilesRequest(client);
    }
    else if (strcmp(type, "getFileContent") == 0)
    {
        const char *filename = doc["filename"] | "";
        handleGetFileContentRequest(client, filename);
    }
    else if (strcmp(type, "saveFile") == 0)
    {
        const char *filename = doc["filename"] | "";
        const char *content = doc["content"] | "";
        handleSaveFileRequest(client, filename, content);
    }
    else if (strcmp(type, "deleteFile") == 0)
    {
        const char *filename = doc["filename"] | "";
        handleDeleteFileRequest(client, filename);
    }
}

void ConfigWebSocket::handleScanNetworksRequest(AsyncWebSocketClient *client)
{
    Serial.println("Handling scan networks request");
    WiFiScanner::getInstance().subscribe([client, this](const String &result)
                                         {
        Serial.println("Scan result callback received");
        Serial.println(result);
        
        // Parse the result JSON
        JsonDocument networksDoc;
        DeserializationError error = deserializeJson(networksDoc, result);
        
        if (error) {
            Serial.print("Failed to parse network results: ");
            Serial.println(error.c_str());
            return;
        }
        
        // Debug: Print the entire document
        Serial.println("Parsed document contents:");
        String docDebug;
        serializeJson(networksDoc, docDebug);
        Serial.println(docDebug);
        
        // Create a response with the networks
        JsonDocument response;
        response["type"] = "networkList";
        
        // Check if networks field exists in the result and is not null
        if (networksDoc["networks"].is<JsonArray>()) {
            Serial.println("Networks field found in result");
            
            // Debug: Print the networks array from the source document
            String networksDebug;
            serializeJson(networksDoc["networks"], networksDebug);
            Serial.print("Networks from source: ");
            Serial.println(networksDebug);
            
            // Check the size of the networks array
            JsonArray networksArray = networksDoc["networks"].as<JsonArray>();
            Serial.printf("Networks array size: %d\n", networksArray.size());
            
            // Copy the networks array to the response
            JsonArray responseNetworks = response["networks"].to<JsonArray>();
            
            for (JsonObject network : networksArray) {
                Serial.print("Processing network: ");
                String networkDebug;
                serializeJson(network, networkDebug);
                Serial.println(networkDebug);
                
                JsonObject newNetwork = responseNetworks.add<JsonObject>();
                if (network["ssid"].is<String>()) {
                    newNetwork["ssid"] = network["ssid"].as<String>();
                } else {
                    Serial.println("WARNING: Network missing SSID!");
                }
                
                if (network["rssi"].is<int>()) {
                    newNetwork["rssi"] = network["rssi"].as<int>();
                }
                
                if (network["encryption"].is<bool>()) {
                    newNetwork["encryption"] = network["encryption"].as<bool>();
                }
            }
        } else {
            Serial.println("No networks field in result or it's null");
            response["networks"].to<JsonArray>();
        }
        
        String jsonResponse;
        serializeJson(response, jsonResponse);
        
        Serial.print("Sending response: ");
        Serial.println(jsonResponse);
        
        if (client->status() == WS_CONNECTED) {
            client->text(jsonResponse);
        } });
}

void ConfigWebSocket::handleConnectToNetworkRequest(AsyncWebSocketClient *client, const char *ssid, const char *password)
{
    Serial.printf("Handling connect to network request: %s\n", ssid);

    if (strlen(ssid) == 0)
    {
        Serial.println("ERROR: Empty SSID provided");
        JsonDocument response;
        response["type"] = "connectionStatus";
        response["status"] = "Error: Empty SSID provided";

        String jsonResponse;
        serializeJson(response, jsonResponse);

        if (client->status() == WS_CONNECTED)
        {
            client->text(jsonResponse);
        }
        return;
    }

    Serial.printf("Attempting to connect to WiFi network: %s\n", ssid);

    WiFiScanner::getInstance().connectToNetwork(ssid, password, [client](const String &result)
                                                {
        Serial.println("Connection attempt callback received");
        Serial.println(result);
        
        if (client->status() == WS_CONNECTED) {
            client->text(result);
            Serial.println("Connection status sent to client");
        } else {
            Serial.println("Client disconnected, couldn't send connection status");
        } });
}

void ConfigWebSocket::handleGetFilesRequest(AsyncWebSocketClient *client)
{
    Serial.println("Handling get files request");
    std::vector<String> files = FileManager::getInstance().listFiles();

    JsonDocument response;
    response["type"] = "fileList";
    JsonArray fileArray = response["files"].to<JsonArray>();

    for (const String &file : files)
    {
        fileArray.add(file);
    }

    String jsonResponse;
    serializeJson(response, jsonResponse);

    if (client->status() == WS_CONNECTED)
    {
        client->text(jsonResponse);
    }
}

void ConfigWebSocket::handleGetFileContentRequest(AsyncWebSocketClient *client, const char *filename)
{
    Serial.printf("Handling get file content request: %s\n", filename);
    String content = FileManager::getInstance().readFile(filename);

    JsonDocument response;
    response["type"] = "fileContent";
    response["filename"] = filename;
    response["content"] = content;

    String jsonResponse;
    serializeJson(response, jsonResponse);

    if (client->status() == WS_CONNECTED)
    {
        client->text(jsonResponse);
    }
}

void ConfigWebSocket::handleSaveFileRequest(AsyncWebSocketClient *client, const char *filename, const char *content)
{
    Serial.printf("Handling save file request: %s\n", filename);
    bool success = FileManager::getInstance().writeFile(filename, content);

    JsonDocument response;
    response["type"] = "saveFileResult";
    response["filename"] = filename;
    response["success"] = success;

    String jsonResponse;
    serializeJson(response, jsonResponse);

    if (client->status() == WS_CONNECTED)
    {
        client->text(jsonResponse);
    }
}

void ConfigWebSocket::handleDeleteFileRequest(AsyncWebSocketClient *client, const char *filename)
{
    Serial.printf("Handling delete file request: %s\n", filename);
    bool success = FileManager::getInstance().deleteFile(filename);

    JsonDocument response;
    response["type"] = "deleteFileResult";
    response["filename"] = filename;
    response["success"] = success;

    String jsonResponse;
    serializeJson(response, jsonResponse);

    if (client->status() == WS_CONNECTED)
    {
        client->text(jsonResponse);
    }
}