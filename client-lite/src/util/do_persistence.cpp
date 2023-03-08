// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "do_persistence.h"

#include <cstdlib>  // std::getenv

namespace docli
{

static std::string ConstructPath(const char* suffix)
{
    std::string outputPath;
#ifdef DO_BUILD_FOR_SNAP
    const char* snapDataRoot = std::getenv("SNAP_DATA");
    THROW_HR_IF(E_UNEXPECTED, (snapDataRoot == nullptr) || (*snapDataRoot == '\0'));
    outputPath = snapDataRoot;
    if (outputPath.back() != '/')
    {
        outputPath.push_back('/');
    }
    if (*suffix == '/')
    {
        ++suffix;
    }
    outputPath += suffix;
#else
    outputPath = suffix;
#endif
    return outputPath;
}

const std::string& GetLogDirectory()
{
    static std::string logDirectory(ConstructPath(DO_AGENT_LOG_DIRECTORY_PATH));
    return logDirectory;
}

const std::string& GetRuntimeDirectory()
{
    static std::string runDirectory(ConstructPath(DO_RUN_DIRECTORY_PATH));
    return runDirectory;
}

const std::string& GetConfigDirectory()
{
    static std::string configDirectory(ConstructPath(DO_CONFIG_DIRECTORY_PATH));
    return configDirectory;
}

const std::string& GetSDKConfigFilePath()
{
    static std::string configFilePath(ConstructPath(DO_CONFIG_DIRECTORY_PATH "/sdk-config.json"));
    return configFilePath;
}

const std::string& GetAdminConfigFilePath()
{
    static std::string configFilePath(ConstructPath(DO_CONFIG_DIRECTORY_PATH "/admin-config.json"));
    return configFilePath;
}

} // namespace docli
