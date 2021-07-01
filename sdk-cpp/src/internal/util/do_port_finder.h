#pragma once

#include <string>

namespace microsoft::deliveryoptimization::details
{
class CPortFinder
{
public:
    static std::string GetDOPort(bool launchClientFirst = false);
};
} // namespace microsoft::deliveryoptimization::details
