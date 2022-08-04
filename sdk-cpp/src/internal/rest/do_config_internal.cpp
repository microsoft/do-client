#include "do_config_internal.h"

#include <cstdio> // popen
#include <cstdlib> // calloc
#if defined(DO_CLIENT_AGENT)
#include <cstring> // strncpy
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "do_persistence.h"
#include "do_version.h"
#elif defined(DO_CLIENT_DOSVC)
#include "do_errors.h" // msdo::errc::e_not_impl
#endif

namespace msdo = microsoft::deliveryoptimization;

#if defined(DO_CLIENT_AGENT)
namespace msdoutil = microsoft::deliveryoptimization::util::details;

const char* const g_dosvcBinName = DOSVC_BIN_NAME;
const char* const g_doPluginAptBinName = DO_PLUGIN_APT_BIN_NAME;

static std::string GetSdkVersion()
{
    return msdoutil::ComponentVersion(false);
}

static std::string GetBinaryVersion(const boost::filesystem::path& binaryFilePath)
{
    std::string version;
    if (boost::filesystem::exists(binaryFilePath))
    {
        FILE* fp = nullptr;
        try
        {
            const std::string command = binaryFilePath.string() + " --version";
            fp = popen(command.c_str(), "r");
            if (fp == nullptr)
            {
                throw std::exception();
            }

            char readBuffer[256];
            constexpr auto cchBuffer = sizeof(readBuffer) / sizeof(readBuffer[0]);
            while (fgets(readBuffer, cchBuffer, fp) != nullptr)
            {
                readBuffer[cchBuffer - 1] = '\0';
                version += readBuffer;
            }

            boost::erase_all(version, "\n");
        }
        catch (const std::exception&)
        {
        }

        if (fp != nullptr)
        {
            (void)pclose(fp);
            fp = nullptr;
        }
    }
    return version;
}

static void AppendBinaryVersion(const char* binFileName, std::stringstream& allVersions)
{
    // Our binaries are normally installed to /usr/local/bin. However, check /usr/bin also
    // to cover special cases (for example, installed not via our deb/rpm packages).
    // We do not use bp::search_path() in order to avoid executing a malicious program with the
    // same name and which appears earlier in the PATH environment variable.
    boost::filesystem::path binFilePath("/usr/local/bin");
    binFilePath /= binFileName;
    std::string version = GetBinaryVersion(binFilePath);
    if (version.empty())
    {
        binFilePath = "/usr/bin";
        binFilePath /= binFileName;
        version = GetBinaryVersion(binFilePath);
    }

    if (!version.empty())
    {
        allVersions << ',' << version;
    }
}

static char* GetAllVersions()
{
    std::string allVersions;
    try
    {
        std::stringstream ss;
        ss << GetSdkVersion();
        if (*g_dosvcBinName != '\0')
        {
            AppendBinaryVersion(g_dosvcBinName, ss);
        }
        if (*g_doPluginAptBinName != '\0')
        {
            AppendBinaryVersion(g_doPluginAptBinName, ss);
        }
        allVersions = ss.str();
        // Caller knows they are getting DO versions, erase the redundant DO prefix
        boost::erase_all(allVersions, "deliveryoptimization-");
    }
    catch (const std::exception&)
    {
    }

    const size_t bufSize = (allVersions.size() + 1) * sizeof(std::string::value_type);
    auto pBuffer = reinterpret_cast<char*>(calloc(1, bufSize));
    if (pBuffer != nullptr)
    {
        strncpy(pBuffer, allVersions.c_str(), bufSize / sizeof(*pBuffer));
        pBuffer[bufSize - 1] = '\0';
    }
    return pBuffer;
}

char* internal_get_components_version()
{
    return GetAllVersions();
}

void internal_free_version_buf(char** ppBuffer)
{
    if (*ppBuffer)
    {
        free(*ppBuffer);
        *ppBuffer = NULL;
    }
}

#else // End DO_CLIENT_AGENT

char* internal_get_components_version()
{
    return nullptr;
}

void internal_free_version_buf(char** ppBuffer)
{
    if (*ppBuffer)
    {
        free(*ppBuffer);
        *ppBuffer = NULL;
    }
}


#endif // End !DO_CLIENT_AGENT

