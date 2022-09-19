// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_common.h"
#include "network_monitor.h"

#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>

bool NetworkMonitor::HasViableInterface()
{
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1)
    {
        DoLogError("getifaddrs() failed, errno: %d", errno);
        return true;
    }

    // Assume network connectivity is available if there is at least one network interface
    // that has an IPv4/IPv6 address, is running and not a loopback interface.
    // TODO(shishirb): Look into NetworkManager dbus API if needed (ability to distinguish among
    //      local/portal/internet connectivity, or in case of false detections with current logic).
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

        if ((ifa->ifa_flags & IFF_RUNNING)
            && !(ifa->ifa_flags & IFF_LOOPBACK))
        {
            DoLogInfo("Viable network interface detected: %s, family: %d%s, flags: 0x%x.",
                ifa->ifa_name, family,
                (family == AF_INET) ? " (AF_INET)" :
                (family == AF_INET6) ? " (AF_INET6)" : "",
                ifa->ifa_flags);
            freeifaddrs(ifaddr);
            return true;
        }
    }

    DoLogWarning("No viable network interface");
    freeifaddrs(ifaddr);
    return false;
}
