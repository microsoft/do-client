// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_version.h"
#include <cstdio>
#include <cstring>
#include <sstream>

const char* const g_appBuilder = DO_VER_BUILDER_IDENTIFIER;
const char* const g_appName = DO_VER_COMPONENT_NAME;
const char* const g_appVersion = DO_VER_COMPONENT_VERSION;
const char* const g_appBuildTime = DO_VER_BUILD_TIME;
const char* const g_gitHeadRev = DO_VER_GIT_HEAD_REVISION;
const char* const g_gitHeadName = DO_VER_GIT_HEAD_NAME;

namespace microsoft
{
namespace deliveryoptimization
{
namespace util
{
namespace details
{

const char* SimpleVersion()
{
    return g_appVersion;
}

// Example version string returned: deliveryoptimization-plugin-apt/v0.1.1+20200819.000911.70fc72d
// Format is: <component name>/v<3 part version>+<build date>.<build time>[.<git head commit>]
std::string ComponentVersion(bool fIncludeExtras)
{
    std::stringstream ss;
    if (*g_appBuilder != '\0')
    {
        ss << g_appBuilder << ";";
    }
    ss << g_appName << "/v" << g_appVersion;
    if (*g_appBuildTime != '\0')
    {
        ss << '+' << g_appBuildTime;
    }
    if (*g_gitHeadRev != '\0')
    {
        ss << ((*g_appBuildTime != '\0') ? '.' : '+') << g_gitHeadRev;
    }
    if (fIncludeExtras && (*g_gitHeadName != '\0'))
    {
        ss << " (" << g_gitHeadName << ')';
    }
    return ss.str();
}

bool OutputVersionIfNeeded(int argc, char** argv)
{
    if (argc == 2)
    {
        const bool fVersionEx = (strcmp(argv[1], "--version-extra") == 0);
        if (fVersionEx || ((strcmp(argv[1], "--version") == 0) || (strcmp(argv[1], "-v") == 0)))
        {
            const auto ver = ComponentVersion(fVersionEx);
            printf("%s\n", ver.c_str());
            return true;
        }
    }
    return false;
}

} // namespace details
} // namespace util
} // namespace deliveryoptimization
} // namespace microsoft
