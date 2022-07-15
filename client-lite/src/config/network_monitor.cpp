// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "network_monitor.h"

#include <errno.h>
#include <ifaddrs.h>

// https://www.freedesktop.org/software/systemd/man/systemd.net-naming-scheme.html
static const char* g_publicIfNames[] = { "eth", "wlan", "en", "wl", "ww" };

bool NetworkMonitor::IsConnected()
{
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1)
    {
        DoLogError("getifaddrs() failed, errno: %d", errno);
        return true;
    }

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
        {
            continue;
        }

        int family = ifa->ifa_addr->sa_family;
        if ((family != AF_INET) && (family != AF_INET6))
        {
            continue;
        }

        // TODO(shishirb): Look for a better way to find the relevant adapter. Naming conventions can change.
        for (auto ifname : g_publicIfNames)
        {
            auto foundPos = strcasestr(ifa->ifa_name, ifname);
            if ((foundPos != nullptr) && (foundPos == ifa->ifa_name))
            {
                DoLogInfo("Network connectivity detected. Interface: %s, address family: %d%s.",
                    ifa->ifa_name, family,
                    (family == AF_INET) ? " (AF_INET)" :
                    (family == AF_INET6) ? " (AF_INET6)" : "");
                freeifaddrs(ifaddr);
                return true;
            }
        }
    }

    DoLogWarning("No network connectivity detected");
    freeifaddrs(ifaddr);
    return false;
}