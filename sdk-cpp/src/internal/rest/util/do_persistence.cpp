// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <string>
#include "do_persistence.h"

#include "do_errors.h"
#include "do_error_helpers.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

#ifdef DO_BUILD_FOR_SNAP
static std::string ConstructPath(const char* suffix)
{
    const char* snapDataRoot = std::getenv("SNAP_DATA");
    if ((snapDataRoot == nullptr) || (*snapDataRoot == '\0'))
    {
        ThrowException(microsoft::deliveryoptimization::errc::unexpected);
    }
    std::string outputPath = snapDataRoot;
    if (outputPath.back() != '/')
    {
        outputPath.push_back('/');
    }
    if (*suffix == '/')
    {
        ++suffix;
    }
    outputPath += suffix;
    return outputPath;
}
#endif

const std::string& GetRuntimeDirectory()
{
#ifdef DO_BUILD_FOR_SNAP
    static std::string runDirectory(ConstructPath("do-port-numbers"));
#elif defined(DO_DEV_DEBUG)
    static std::string runDirectory("/tmp/run/deliveryoptimization-agent");
#else
    static std::string runDirectory("/var/run/deliveryoptimization-agent");
#endif
    return runDirectory;
}

const std::string& GetConfigFilePath()
{
#ifdef DO_BUILD_FOR_SNAP
    static std::string configFilePath(ConstructPath("do-configs/sdk-config.json"));
#elif defined(DO_DEV_DEBUG)
    static std::string configFilePath("/tmp/etc/deliveryoptimization-agent/sdk-config.json");
#else
    static std::string configFilePath("/etc/deliveryoptimization-agent/sdk-config.json");
#endif
    return configFilePath;
}

// TODO(shishirb): this is used only in test
const std::string& GetAdminConfigFilePath()
{
#ifdef DO_BUILD_FOR_SNAP
    static std::string configFilePath(ConstructPath("do-configs/admin-config.json"));
#elif defined(DO_DEV_DEBUG)
    static std::string configFilePath("/tmp/etc/deliveryoptimization-agent/admin-config.json");
#else
    static std::string configFilePath("/etc/deliveryoptimization-agent/admin-config.json");
#endif
    return configFilePath;
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
