#ifndef _DELIVERY_OPTIMIZATION_TEST_HELPERS_H
#define _DELIVERY_OPTIMIZATION_TEST_HELPERS_H

#include <string>
#include <thread>

#include "tests_common.h"

#include "do_download.h"
#include "do_download_status.h"

namespace msdo = microsoft::deliveryoptimization;
using namespace std::chrono_literals; // NOLINT(build/namespaces)

class TestHelpers
{
public:
    static void CleanTestDir();
    static void CleanupWorkingDir();

    // On Windows, operations are async - there may be some delay setting a state internally
    static void WaitForState(msdo::download& download, msdo::download_state expectedState, std::chrono::seconds waitTimeSecs = 10s)
    {
        msdo::download_status status = download.get_status();
        const auto endtime = std::chrono::steady_clock::now() + waitTimeSecs;
        while ((status.state() != expectedState) && (std::chrono::steady_clock::now() < endtime))
        {
            std::this_thread::sleep_for(1s);
            status = download.get_status();
            std::cout << "Transferred " << status.bytes_transferred() << " / " << status.bytes_total() << "\n";
        }

        ASSERT_EQ(status.state(), expectedState) << "Download must have reached expected state before timeout";
    }

#if defined(DO_INTERFACE_REST)
    static bool IsActiveProcess(std::string name);
    static int ShutdownProcess(std::string name);
    static void RestartService(const std::string& name);
    static void StartService(const std::string& name);
    static void StopService(const std::string& name);
    static void CreateRestPortFiles(int numFiles);
    static void DeleteRestPortFiles();
    static unsigned int CountRestPortFiles();
    static void DisableNetwork();
    static void EnableNetwork();

    static std::string GetLocalIPv4Address();
#endif // Rest

private:
#if defined(DO_INTERFACE_REST)
    static int _GetPidFromProcName(std::string name);
    static int _KillProcess(int pid, int signal);
#endif // Rest

    // Disallow creating an instance of this object
    TestHelpers() {}

};

#endif
