#include "do_config.h"

#include <cstdio> // popen
#include <cstdlib> // calloc
#include <cstring> // strncpy
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "do_persistence.h"
#include "do_version.h"
namespace msdoutil = microsoft::deliveryoptimization::util::details;

const char* const g_dosvcBinName = DOSVC_BIN_NAME;
const char* const g_doPluginAptBinName = DO_PLUGIN_APT_BIN_NAME;

// Deprecated, but keeping here to not break API surface
static int WriteIoTConnectionStringToConfigFile(const char* value) noexcept
{
    try
    {
        boost::property_tree::ptree configTree;
        configTree.put("ADUC_IoTConnectionString", value);

        // No other configs are used at this time so overwrite the whole file here
        const std::string& filePath = microsoft::deliveryoptimization::details::GetConfigFilePath();
        boost::property_tree::write_json(filePath, configTree);

        return 0;
    }
    catch (const std::exception&)
    {
    }
    // property_tree doesn't seem to offer an exception type that provides an error code.
    // We do not (yet) log anything from the SDK either, so stick with returning -1 for all failure cases.
    return -1;
}

static int WriteDOCacheHost(const char* value) noexcept
{
    try
    {
        boost::property_tree::ptree configTree;
        configTree.put("DOCacheHost", value);

        // No other configs are used at this time so overwrite the whole file here
        const std::string& filePath = microsoft::deliveryoptimization::details::GetConfigFilePath();
        boost::property_tree::write_json(filePath, configTree);

        return 0;
    }
    catch (const std::exception&)
    {
    }
    // property_tree doesn't seem to offer an exception type that provides an error code.
    // We do not (yet) log anything from the SDK either, so stick with returning -1 for all failure cases.
    return -1;
}

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

extern "C" int deliveryoptimization_set_cache_host(const char* value)
{
    return WriteDOCacheHost(value);
}

extern "C" int deliveryoptimization_set_iot_connection_string(const char* value)
{
    return WriteIoTConnectionStringToConfigFile(value);
}

extern "C" char* deliveryoptimization_get_components_version()
{
    return GetAllVersions();
}

extern "C" void deliveryoptimization_free_version_buf(char** ppBuffer)
{
    if (*ppBuffer)
    {
        free(*ppBuffer);
        *ppBuffer = NULL;
    }
}
