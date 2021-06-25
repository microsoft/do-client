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
    std::string GetHost(const std::string& originalUrl);
    void ReportHostError(HRESULT hr, UINT httpStatusCode, const std::string& mccHost, const std::string& originalUrl);
    bool IsBanned(const std::string& mccHost, const std::string& originalUrl) const;

private:
    struct MccHost
    {
    public:
        MccHost(const std::string& address);
        void Ban(std::chrono::seconds banInterval);

        const std::string& Address() const noexcept { return address; }
        bool IsBanned() const;

    private:
        std::string address;
        std::chrono::steady_clock::time_point timeOfUnban;
    };

    ConfigManager& _configManager;
    std::unordered_map<std::string, MccHost> _mccHostsByOriginalHost;
};
