#include "do_port_finder.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

#include "do_errors.h"
#include "do_error_helpers.h"
#include "do_persistence.h"

using namespace std::chrono_literals; // NOLINT(build/namespaces)

const int32_t g_maxNumPortFileReadAttempts = 3;

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

static std::string g_DiscoverDOPort()
{
    const std::string runtimeDirectory = GetRuntimeDirectory();
    if (!std::filesystem::exists(runtimeDirectory))
    {
        return std::string();
    }

    std::filesystem::path mostRecentFile("");
    auto mostRecentTime = std::filesystem::file_time_type::min();
    for (std::filesystem::directory_iterator itr(runtimeDirectory); itr != std::filesystem::directory_iterator(); ++itr)
    {
        const std::filesystem::path& dirEntry = itr->path();
        if (dirEntry.filename().string().find("restport") != std::string::npos)
        {
            auto currTime = std::filesystem::last_write_time(dirEntry);
            if (currTime > mostRecentTime)
            {
                mostRecentTime = currTime;
                mostRecentFile = dirEntry;
            }
        }
    }
    std::ifstream file(mostRecentFile.string());
    std::string port;
    std::getline(file, port);

    return port;
}

std::string CPortFinder::GetDOPort(bool launchClientFirst)
{
    std::string port;

    for (int numAttempts = 1; (numAttempts <= g_maxNumPortFileReadAttempts) && port.empty(); numAttempts++)
    {
        // If built as service, then we expect client to already be running.
        // We still wait+discover loop below in case client started just now.
        // TODO(jimson) Do attempt to start the service if launchClientFirst==true

        // Wait up to 1s for client to write the restport file
        for (int i = 1; (i <= 4) && port.empty(); ++i)
        {
            std::this_thread::sleep_for(250ms);
            port = g_DiscoverDOPort();
        }
        launchClientFirst = port.empty(); // force launch on further attempts
    }
    if (port.empty())
    {
        ThrowException(errc::no_service);
    }
    return port;
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
