#pragma once

#include <string>

class TestHelpers
{
public:
    static void CleanTestDir();
    static void CleanupWorkingDir();

#if (DO_PLATFORM_ID == DO_PLATFORM_ID_LINUX)
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
#endif // Linux

private:
#if (DO_PLATFORM_ID == DO_PLATFORM_ID_LINUX)
    static int _GetPidFromProcName(std::string name);
    static int _KillProcess(int pid, int signal);
#endif // Linux

    // Disallow creating an instance of this object
    TestHelpers() {}

};
