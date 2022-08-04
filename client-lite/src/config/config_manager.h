// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <boost/optional.hpp>
#include "do_json_parser.h"

class ConfigManager
{
public:
    ConfigManager();
    ConfigManager(const std::string& adminConfigPath);

    void RefreshAdminConfigs();

    boost::optional<std::chrono::seconds> CacheHostFallbackDelay();
    std::string CacheHostServer();
    bool RestControllerValidateRemoteAddr();

private:
    JsonParser _adminConfigs;
};
