#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

// Simple banning functionality - ban on first offence until specified time point.
// Banning using a moving window is left undone until we need to support banning
// only after repeated offences threshold and phasing out of the offence counts over time.
class CBanList
{
public:
    void Report(const std::string& name, std::chrono::milliseconds banInterval);
    bool IsBanned(const std::string& name);

private:
    std::unordered_map<std::string, std::chrono::system_clock::time_point> _banList;
};
