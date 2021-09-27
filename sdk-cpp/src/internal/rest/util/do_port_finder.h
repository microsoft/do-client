// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DELIVERY_OPTIMIZATION_DO_PORT_FINDER_H
#define _DELIVERY_OPTIMIZATION_DO_PORT_FINDER_H

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
#endif
