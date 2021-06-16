#include "do_common.h"
#include "do_persistence.h"

namespace docli
{

const std::string& GetLogDirectory()
{
    static std::string logDirectory(DO_AGENT_LOG_DIRECTORY_PATH);
    return logDirectory;
}

const std::string& GetRuntimeDirectory()
{
    static std::string runDirectory(DO_RUN_DIRECTORY_PATH);
    return runDirectory;
}

const std::string& GetConfigDirectory()
{
    static std::string configDirectory(DO_CONFIG_DIRECTORY_PATH);
    return configDirectory;
}

const std::string& GetSDKConfigFilePath()
{
    static std::string configFilePath(DO_CONFIG_DIRECTORY_PATH "/sdk-config.json");
    return configFilePath;
}

const std::string& GetAdminConfigFilePath()
{
    static std::string configFilePath(DO_CONFIG_DIRECTORY_PATH "/admin-config.json");
    return configFilePath;
}

} // namespace docli
