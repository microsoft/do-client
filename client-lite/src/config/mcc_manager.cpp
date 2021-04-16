#include "do_common.h"
#include "mcc_manager.h"

#include "config_defaults.h"
#include "config_manager.h"

static std::string GetHostNameFromIoTConnectionString(const char* connectionString)
{
    DoLogDebug("Parsing connection string: %s", connectionString);

    static const char* toFind = "GatewayHostName=";
    const char* start = strcasestr(connectionString, toFind);
    if (start == NULL)
    {
        DoLogDebug("GatewayHostName not found in %s", connectionString);
        return {};
    }

    start = start + strlen(toFind);

    std::string hostname;
    const char* end = strchr(start, ';');
    if (end == NULL)
    {
        hostname = start;
    }
    else
    {
        hostname.assign(start, start + (end - start));
    }
    return hostname;
}

MCCManager::MCCManager(ConfigManager& sdkConfigs):
    _configManager(sdkConfigs)
{
}

std::string MCCManager::NextHost()
{
    std::string mccHostName =_configManager.CacheHostServer();
    if (mccHostName.empty())
    {
        const std::string connString = _configManager.IoTConnectionString();
        if (!connString.empty())
        {
            mccHostName = GetHostNameFromIoTConnectionString(connString.data());
        }
    }

    if (!mccHostName.empty())
    {
        if (_banList.IsBanned(mccHostName))
        {
            _hosts.erase(mccHostName);
            mccHostName.clear();
        }
        else
        {
            // Record the time of when we first handed out this host
            if (_hosts.find(mccHostName) == _hosts.end())
            {
                _hosts[mccHostName] = std::chrono::steady_clock::now();
            }
        }
    }
    DoLogVerbose("Returning MCC host: [%s]", mccHostName.data());
    return mccHostName;
}

bool MCCManager::NoFallback() const
{
    return (_configManager.CacheHostFallbackDelay() == g_cacheHostFallbackDelayNoFallback);
}

// Returns true if fallback to original URL is due now, false otherwise
bool MCCManager::ReportHostError(HRESULT hr, const std::string& host)
{
    const bool fallbackDue = _IsFallbackDue(host);
    DoLogWarningHr(hr, "ACK error from MCC host: [%s], fallback due? %d", host.data(), fallbackDue);
    if (fallbackDue)
    {
        _banList.Report(host, g_mccHostBanInterval);
    }
    return fallbackDue;
}

bool MCCManager::_IsFallbackDue(const std::string& host) const
{
    auto it = _hosts.find(host);
    DO_ASSERT(it != _hosts.end());

    const auto timeFirstHandedOut = it->second;
    const auto fallbackDelay = _configManager.CacheHostFallbackDelay();
    if (fallbackDelay == g_cacheHostFallbackDelayNoFallback)
    {
        // No fallback, so don't ban this host.
        // Will have to rework this when there can be multiple MCC hosts.
        return false;
    }

    // Fallback is due if the delay interval has passed since the first time this host was handed out
    return (timeFirstHandedOut + fallbackDelay) <= std::chrono::steady_clock::now();
}
