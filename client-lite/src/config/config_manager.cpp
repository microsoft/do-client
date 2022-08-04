// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "config_manager.h"

#include "config_defaults.h"
#include "do_persistence.h"
#include "string_ops.h"

ConfigManager::ConfigManager() :
    ConfigManager(docli::GetAdminConfigFilePath())
{
}

// Used by unit tests to override config paths
ConfigManager::ConfigManager(const std::string& adminConfigPath) :
    _adminConfigs(adminConfigPath, true)
{
}

void ConfigManager::RefreshAdminConfigs()
{
    _adminConfigs.Refresh();
}

boost::optional<std::chrono::seconds> ConfigManager::CacheHostFallbackDelay()
{
    boost::optional<std::chrono::seconds> returnValue;

    // We don't yet differentiate between background and foreground downloads, so check both configs
    boost::optional<int> delay = _adminConfigs.Get<int>(ConfigName_CacheHostFallbackDelayBgSecs);
    if (!delay)
    {
        delay = _adminConfigs.Get<int>(ConfigName_CacheHostFallbackDelayFgSecs);
    }

    if (delay)
    {
        returnValue = std::chrono::seconds(delay.get());
    }
    return returnValue;
}

std::string ConfigManager::CacheHostServer()
{
    boost::optional<std::string> cacheHostServer = _adminConfigs.Get<std::string>(ConfigName_CacheHostServer);
    return boost::get_optional_value_or(cacheHostServer, std::string{});
}

bool ConfigManager::RestControllerValidateRemoteAddr()
{
    boost::optional<bool> validateRemoteAddr = _adminConfigs.Get<bool>(ConfigName_RestControllerValidateRemoteAddr);
    return boost::get_optional_value_or(validateRemoteAddr, g_RestControllerValidateRemoteAddrDefault);
}
