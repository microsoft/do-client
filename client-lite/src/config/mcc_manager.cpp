// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "mcc_manager.h"

#include "config_defaults.h"
#include "config_manager.h"
#include "do_cpprest_uri.h"
#include "http_agent.h"

namespace msdod = microsoft::deliveryoptimization::details;

MCCManager::MCCManager(ConfigManager& sdkConfigs):
    _configManager(sdkConfigs)
{
}

boost::optional<std::chrono::seconds> MCCManager::FallbackDelay()
{
    return _configManager.CacheHostFallbackDelay();
}

std::string MCCManager::GetHost(const std::string& originalUrl)
{
    std::string mccHostName = _configManager.CacheHostServer();

    const auto originalHost = msdod::cpprest_web::uri{originalUrl}.host();
    if (!mccHostName.empty())
    {
        auto it = _mccHostsByOriginalHost.find(originalHost);
        if (it != _mccHostsByOriginalHost.end())
        {
            if (it->second.Address() != mccHostName)
            {
                // MCC to be used has changed, discard existing one
                _mccHostsByOriginalHost.erase(it);
                it = _mccHostsByOriginalHost.end();
            }
        }

        if (it == _mccHostsByOriginalHost.end())
        {
            MccHost newEntry{mccHostName};
            it = _mccHostsByOriginalHost.emplace(originalHost, std::move(newEntry)).first;
        }

        DO_ASSERT(it != _mccHostsByOriginalHost.end());
    }
    DoLogVerbose("Returning MCC host: [%s] for original host: [%s]", mccHostName.data(), originalHost.data());
    return mccHostName;
}

void MCCManager::ReportHostError(HRESULT hr, UINT httpStatusCode, const std::string& mccHost, const std::string& originalUrl)
{
    const bool isFatalError = HttpAgent::IsClientError(httpStatusCode);
    const auto originalHost = msdod::cpprest_web::uri{originalUrl}.host();
    DoLogWarningHr(hr, "ACK error from MCC host: [%s], original host: [%s], fatal error? %d",
        mccHost.data(), originalHost.data(), isFatalError);
    if (isFatalError)
    {
        auto it = _mccHostsByOriginalHost.find(originalHost);
        if (it != _mccHostsByOriginalHost.end() && (it->second.Address() == mccHost))
        {
            it->second.Ban(g_mccHostBanInterval);
        }
    }
}

bool MCCManager::IsBanned(const std::string& mccHost, const std::string& originalUrl) const
{
    const auto originalHost = msdod::cpprest_web::uri{originalUrl}.host();
    auto it = _mccHostsByOriginalHost.find(originalHost);
    if (it != _mccHostsByOriginalHost.end() && (it->second.Address() == mccHost))
    {
        return it->second.IsBanned();
    }
    return false;
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

bool MCCManager::MccHost::IsBanned() const
{
    const auto now = std::chrono::steady_clock::now();
    const bool isBanned = (_timeOfUnban > now);
    if (isBanned)
    {
        const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(_timeOfUnban - now);
        DoLogVerbose("%s will be unbanned after %ld ms", _address.data(), diff.count());
    }
    return isBanned;
}
