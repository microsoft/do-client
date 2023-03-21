#include "do_config_internal.h"

#include <cstdio> // popen
#include <cstdlib> // calloc
#if defined(DO_CLIENT_AGENT)
#include <cstring> // strncpy
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <filesystem>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "do_persistence.h"
#include "do_version.h"
#elif defined(DO_CLIENT_DOSVC)
#include "do_errors.h" // msdo::errc::not_impl
#endif

namespace msdo = microsoft::deliveryoptimization;

#if defined(DO_CLIENT_AGENT)
namespace msdoutil = microsoft::deliveryoptimization::util::details;

const char* const g_dosvcBinName = DOSVC_BIN_NAME;
const char* const g_doPluginAptBinName = DO_PLUGIN_APT_BIN_NAME;

static int WriteIoTConnectionStringToConfigFile(const char* value) noexcept
{
    int returnValue = 0;
    // ptree's exceptions do not provide an error code, and SDK has no logging of its own.
    // Return specific errors as a workaround.
    std::filesystem::path filePath{microsoft::deliveryoptimization::details::GetConfigFilePath()};
    std::error_code ec;
    if (std::filesystem::exists(filePath.parent_path(), ec))
    {
        try
        {
            boost::property_tree::ptree configTree;
            configTree.put("ADUC_IoTConnectionString", value);

            // No other configs are used at this time so overwrite the whole file here
            boost::property_tree::write_json(filePath.string(), configTree);

            return 0;
        }
        catch (const boost::property_tree::ptree_bad_data&)
        {
            returnValue = -1;
        }
        catch (const boost::property_tree::ptree_bad_path&)
        {
            returnValue = -2;
        }
        catch (const boost::property_tree::ptree_error& pe)
        {
            returnValue = -3;
        }
        catch (const std::exception&)
        {
            returnValue = -4;
        }
    }
    else
    {
        returnValue = ec.value();
    }
    return returnValue;
}

static std::string GetSdkVersion()
{
    return msdoutil::ComponentVersion(false);
}

static std::string GetBinaryVersion(const std::filesystem::path& binaryFilePath)
{
    std::string version;
    if (std::filesystem::exists(binaryFilePath))
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
    std::filesystem::path binFilePath("/usr/local/bin");
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

int internal_set_iot_connection_string(const char* value)
{
    return WriteIoTConnectionStringToConfigFile(value);
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

int internal_set_iot_connection_string(const char* value)
{
    return msdo::errc::not_impl;
}

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

