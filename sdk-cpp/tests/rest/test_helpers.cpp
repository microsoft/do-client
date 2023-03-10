// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "test_helpers.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <ifaddrs.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <boost/filesystem.hpp>

#include "do_persistence.h"
#include "do_port_finder.h"

#include "do_test_helpers.h"
namespace dtu = dotest::util;

#include "test_data.h"

namespace msdod = microsoft::deliveryoptimization::details;

void TestHelpers::RestartService(const std::string& name)
{
#ifndef DO_BUILD_FOR_SNAP
    // 'snap' does not support reset-failed
    const auto resetCmd = dtu::FormatString("systemctl reset-failed %s", name.c_str());
    dtu::ExecuteSystemCommand(resetCmd.data()); // avoids hitting start-limit-hit error in systemd
#endif

    const auto restartCmd = dtu::FormatString(DO_SERVICE_CONTROLLER " restart %s", name.c_str());
    dtu::ExecuteSystemCommand(restartCmd.data());
}

void TestHelpers::StartService(const std::string& name)
{
    dtu::ExecuteSystemCommand(dtu::FormatString(DO_SERVICE_CONTROLLER " start %s", name.c_str()).data());
}

void TestHelpers::StopService(const std::string& name)
{
    dtu::ExecuteSystemCommand(dtu::FormatString(DO_SERVICE_CONTROLLER " stop %s", name.c_str()).data());
}

void TestHelpers::CreateRestPortFiles(int numFiles)
{
    std::string portFile;
    for (int i = 0; i < numFiles; i++)
    {
        portFile = std::string(msdod::GetRuntimeDirectory()) + std::string("/restport.") + std::to_string(i);
        std::ofstream file(portFile);
        file << std::to_string(50000 + i).c_str() << std::endl;
    }
}

void TestHelpers::DeleteRestPortFiles()
{
    for (boost::filesystem::directory_iterator itr(msdod::GetRuntimeDirectory()); itr != boost::filesystem::directory_iterator(); ++itr)
    {
        auto& dirEntry = itr->path();
        if (dirEntry.filename().string().find("restport") != std::string::npos)
        {
            boost::filesystem::remove(dirEntry);
        }
    }
}

unsigned int TestHelpers::CountRestPortFiles()
{
    unsigned int count = 0;
    for (boost::filesystem::directory_iterator itr(msdod::GetRuntimeDirectory()); itr != boost::filesystem::directory_iterator(); ++itr)
    {
        auto& dirEntry = itr->path();
        if (dirEntry.filename().string().find("restport") != std::string::npos)
        {
            ++count;
        }
    }
    return count;
}

void TestHelpers::DisableNetwork()
{
    dtu::ExecuteSystemCommand("ifconfig eth0 down");
    std::cout << "Disabled eth0" << '\n';
}

void TestHelpers::EnableNetwork()
{
    dtu::ExecuteSystemCommand("ifconfig eth0 up");
    std::cout << "Enabled eth0" << '\n';
}

std::string TestHelpers::GetLocalIPv4Address()
{
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1)
    {
        std::cout << "getifaddrs() failed, errno: " << errno;
        return {};
    }

    std::string localIPv4;
    for (auto ifa = ifaddr; (ifa != nullptr) && localIPv4.empty(); ifa = ifa->ifa_next)
    {
        if ((ifa->ifa_addr == nullptr) || (ifa->ifa_addr->sa_family != AF_INET))
        {
            continue;
        }

        auto foundPos = strcasestr(ifa->ifa_name, "eth");
        if ((foundPos == nullptr) || (foundPos != ifa->ifa_name))
        {
            foundPos = strcasestr(ifa->ifa_name, "wlan");
            if ((foundPos == nullptr) || (foundPos != ifa->ifa_name))
            {
                continue;
            }
        }

        char ifAddrStr[32] = {};
        inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr)->sin_addr, ifAddrStr, 32);
        localIPv4 = ifAddrStr;
        std::cout << "Network interface detected: " << ifa->ifa_name << ", addr: " << localIPv4 << std::endl;
    }

    freeifaddrs(ifaddr);
    return localIPv4;
}
