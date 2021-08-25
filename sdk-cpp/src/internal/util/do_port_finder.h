#pragma once

#include <string>

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

class CPortFinder
{
public:
    static std::string GetDOPort(bool launchClientFirst = false);
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
