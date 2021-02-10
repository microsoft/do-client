#pragma once

#include "do_json_parser.h"

class ConfigManager
{
public:
    ConfigManager();
    ConfigManager(const std::string& adminConfigPath, const std::string& sdkConfigPath);

    std::chrono::seconds CacheHostFallbackDelay();
    std::string CacheHostServer();
    std::string IoTConnectionString();
    bool RestControllerValidateRemoteAddr();

private:
    JsonParser _adminConfigs;
    JsonParser _sdkConfigs;
};
