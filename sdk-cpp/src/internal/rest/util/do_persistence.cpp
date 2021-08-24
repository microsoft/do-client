#include <string>

#include "do_persistence.h"

namespace microsoft::deliveryoptimization::details
{

const std::string& GetRuntimeDirectory()
{
    static std::string runDirectory("/var/run/deliveryoptimization-agent");
    return runDirectory;
}

const std::string& GetConfigFilePath()
{
    static std::string configFilePath("/etc/deliveryoptimization-agent/sdk-config.json");
    return configFilePath;
}

// TODO(shishirb): this is used only in test
const std::string& GetAdminConfigFilePath()
{
    static std::string configFilePath("/etc/deliveryoptimization-agent/admin-config.json");
    return configFilePath;
}

} // namespace microsoft::deliveryoptimization::details
