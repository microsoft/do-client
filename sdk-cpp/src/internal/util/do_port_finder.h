#pragma once

#include <string>

namespace microsoft::deliveryoptimization::details
{
class CPortFinder
{
public:
    static std::string ConstructLocalUrl(const std::string& port);
    static std::string DiscoverDOPort();
    static std::string GetDOBaseUrl(bool launchDocs = false);
};
} // namespace microsoft::deliveryoptimization::details
