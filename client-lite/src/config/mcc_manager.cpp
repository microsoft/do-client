// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "mcc_manager.h"

#include "config_defaults.h"
#include "config_manager.h"
#include "do_cpprest_uri.h"
#include "http_agent.h"

namespace msdod = microsoft::deliveryoptimization::details;

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

boost::optional<std::chrono::seconds> MCCManager::FallbackDelay()
{
    return _configManager.CacheHostFallbackDelay();
}

std::string MCCManager::GetHost()
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

    DoLogVerbose("Returning MCC host: [%s]", mccHostName.c_str());
    return mccHostName;
}

void MCCManager::ReportHostError(HRESULT hr, UINT httpStatusCode, const std::string& mccHost, const std::string& originalUrl)
{
    // Client error (HTTP 4xx codes) indicates that MCC is responsive but does not support this
    // original URL/host. The other errors indicate an unresponsive MCC.
    const bool perHostFatalError = HttpAgent::IsClientError(httpStatusCode);
    const bool generalFatalError = (hr == WININET_E_TIMEOUT)
        || (hr == HRESULT_FROM_WIN32(ERROR_WINHTTP_NAME_NOT_RESOLVED))
        || (hr == HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CONNECT));
    const auto originalHost = msdod::cpprest_web::uri{originalUrl}.host();
    DoLogWarningHr(hr, "ACK error from MCC host: [%s], original host: [%s], fatal error? %d",
        mccHost.data(), originalHost.data(), perHostFatalError || generalFatalError);
    if (perHostFatalError || generalFatalError)
    {
        auto it = std::find(_mccHosts.begin(), _mccHosts.end(), mccHost);
        if (it == _mccHosts.end())
        {
            it = _mccHosts.emplace(_mccHosts.end(), mccHost);
        }

        if (perHostFatalError)
        {
            it->BanForOriginalHost(originalHost, g_mccHostBanInterval);
        }
        else
        {
            it->Ban(g_mccHostBanInterval);
        }
    }
}

bool MCCManager::IsBanned(const std::string& mccHost, const std::string& originalUrl) const
{
    auto it = std::find(_mccHosts.begin(), _mccHosts.end(), mccHost);
    if (it == _mccHosts.end())
    {
        return false;
    }
    else
    {
        const auto originalHost = msdod::cpprest_web::uri{originalUrl}.host();
        return it->IsBanned(originalHost);
    }
}

MCCManager::MccHost::MccHost(const std::string& address) :
    _address(address),
    _timeOfUnban(std::chrono::steady_clock::time_point::min())
{
}

void MCCManager::MccHost::Ban(std::chrono::seconds banInterval)
{
    _timeOfUnban = std::chrono::steady_clock::now() + banInterval;
    DoLogInfo("%s banned for %ld s", _address.data(), banInterval.count());
}

void MCCManager::MccHost::BanForOriginalHost(const std::string& originalHost, std::chrono::seconds banInterval)
{
    auto timeOfUnban = std::chrono::steady_clock::now() + banInterval;
    DoLogInfo("%s banned for %ld s for %s", _address.c_str(), banInterval.count(), originalHost.c_str());

    auto banForOriginalHost = std::find(_timeOfUnbanForOriginalHosts.begin(), _timeOfUnbanForOriginalHosts.end(), originalHost);
    if (banForOriginalHost == _timeOfUnbanForOriginalHosts.end())
    {
        _timeOfUnbanForOriginalHosts.emplace_back(originalHost, timeOfUnban);
    }
    else
    {
        banForOriginalHost->timeOfUnban = timeOfUnban;
    }
}

bool MCCManager::MccHost::IsBanned(const std::string& originalHost) const
{
    const auto now = std::chrono::steady_clock::now();
    bool isBanned = (now < _timeOfUnban);
    if (isBanned)
    {
        const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(_timeOfUnban - now);
        DoLogVerbose("%s will be unbanned after %ld ms", _address.data(), diff.count());
    }
    else
    {
        auto banForOriginalHost = std::find(_timeOfUnbanForOriginalHosts.begin(), _timeOfUnbanForOriginalHosts.end(), originalHost);
        if (banForOriginalHost != _timeOfUnbanForOriginalHosts.end())
        {
            isBanned = (now < banForOriginalHost->timeOfUnban);
            if (isBanned)
            {
                const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(banForOriginalHost->timeOfUnban - now);
                DoLogVerbose("%s will be unbanned after %ld ms for host %s", _address.data(), diff.count(), originalHost.c_str());
            }
        }
    }
    return isBanned;
}
