// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <chrono>
#include <unordered_map>
#include <boost/optional.hpp>

class ConfigManager;

class MCCManager
{
public:
    MCCManager(ConfigManager& configManager);

    boost::optional<std::chrono::seconds> FallbackDelay();
    std::string GetHost();
    void ReportHostError(HRESULT hr, UINT httpStatusCode, const std::string& mccHost, const std::string& originalUrl);
    bool IsBanned(const std::string& mccHost, const std::string& originalUrl) const;

private:
    // MCC host can be banned for all original hosts (connection failures) or per host (not part of MCC allow list).
    // MccHost class is responsible for keeping track of this.
    class MccHost
    {
    public:
        MccHost(const std::string& address);

        void Ban(std::chrono::seconds banInterval);
        void BanForOriginalHost(const std::string& originalHost, std::chrono::seconds banInterval);

        const std::string& Address() const noexcept { return _address; }
        bool IsBanned(const std::string& originalHost) const;
        bool operator==(const std::string& otherMccAddress) const noexcept { return (_address == otherMccAddress); }

    private:
        struct OriginalHostStatus
        {
            std::string host;
            std::chrono::steady_clock::time_point timeOfUnban;

            OriginalHostStatus(const std::string& host, std::chrono::steady_clock::time_point timeOfUnban) :
                host(host),
                timeOfUnban(timeOfUnban)
            {
            }

            bool operator==(const std::string& otherHost) const noexcept { return (host == otherHost); }
        };

        std::string _address;
        std::chrono::steady_clock::time_point _timeOfUnban;
        std::vector<OriginalHostStatus> _timeOfUnbanForOriginalHosts;
    };

    ConfigManager& _configManager;
    std::vector<MccHost> _mccHosts;
};
