#pragma once

#include <Arduino.h>
#include <SPIFFS.h>
#include <vector>

class FileManager
{
public:
    static FileManager &getInstance()
    {
        static FileManager instance;
        return instance;
    }

    std::vector<String> listFiles()
    {
        std::vector<String> fileList;
        File root = SPIFFS.open("/");
        if (!root)
        {
            Serial.println("Failed to open directory");
            return fileList;
        }
        if (!root.isDirectory())
        {
            Serial.println("Not a directory");
            return fileList;
        }

        File file = root.openNextFile();
        while (file)
        {
            if (!file.isDirectory())
            {
                fileList.push_back(String(file.name()));
            }
            file = root.openNextFile();
        }
        return fileList;
    }

    String readFile(const char *path)
    {
        File file = SPIFFS.open(path, "r");
        if (!file)
        {
            Serial.println("Failed to open file for reading");
            return "";
        }

        String content = "";
        while (file.available())
        {
            content += (char)file.read();
        }
        file.close();
        return content;
    }

    bool writeFile(const char *path, const char *content)
    {
        File file = SPIFFS.open(path, "w");
        if (!file)
        {
            Serial.println("Failed to open file for writing");
            return false;
        }

        bool success = file.print(content);
        file.close();
        return success;
    }

    bool deleteFile(const char *path)
    {
        if (!SPIFFS.exists(path))
        {
            Serial.println("File doesn't exist");
            return false;
        }
        return SPIFFS.remove(path);
    }

private:
    FileManager() {}
};