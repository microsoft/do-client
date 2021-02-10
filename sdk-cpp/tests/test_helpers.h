#pragma once

#include <string>

class TestHelpers
{
public:
    static bool IsActiveProcess(std::string name);
    static int ShutdownProcess(std::string name);
    static void RestartService(const std::string& name);
    static void CreateRestPortFiles(int numFiles);
    static void DeleteRestPortFiles();
    static void CleanTestDir();
    static void CleanupWorkingDir();

    static void DisableNetwork();
    static void EnableNetwork();

    static std::string GetLocalIPv4Address();

private:
    static int _GetPidFromProcName(std::string name);
    static int _KillProcess(int pid, int signal);

    // Disallow creating an instance of this object
    TestHelpers() {}
};
