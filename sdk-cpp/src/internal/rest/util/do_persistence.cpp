// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <string>

#include "do_persistence.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

const std::string& GetRuntimeDirectory()
{
#ifdef DO_DEV_DEBUG
    static std::string runDirectory("/tmp/run/deliveryoptimization-agent");
#else
    static std::string runDirectory("/var/run/deliveryoptimization-agent");
#endif
    return runDirectory;
}

// TODO(shishirb): this is used only in test
const std::string& GetAdminConfigFilePath()
{
#ifdef DO_DEV_DEBUG
    static std::string configFilePath("/tmp/etc/deliveryoptimization-agent/admin-config.json");
#else
    static std::string configFilePath("/etc/deliveryoptimization-agent/admin-config.json");
#endif
    return configFilePath;
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
