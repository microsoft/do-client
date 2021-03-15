#pragma once

#include <chrono>
#include <unordered_map>
#include "ban_list.h"
#include "config_manager.h"

class MCCManager
{
public:
    MCCManager(ConfigManager& configManager);

    std::string NextHost();
    bool NoFallback() const;
    bool ReportHostError(HRESULT hr, const std::string& host);

private:
    bool _IsFallbackDue(const std::string& host) const;

    ConfigManager& _configManager;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> _hosts;
    CBanList _banList;
};
