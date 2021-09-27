// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_log.h"

#include <unistd.h> // getpid
#include <sys/syscall.h> // SYS_gettid
#include <iostream>
#include <fstream>
#include "do_date_time.h"
#include "do_plugin_exception.h"

static std::array<char, 128> g_GetLogLinePrefix()
{
    static const pid_t pid = getpid();
    const auto tid = static_cast<int>(syscall(SYS_gettid)); // using syscall because glibc wrapper is unavailable
    const auto timeStr = SysTimePointToUTCString(wall_clock_t::now());

    // Timestamp ProcessID ThreadID msg
    std::array<char, 128> prefixBuf;
    snprintf(prefixBuf.data(), prefixBuf.size(), "%s %-5d %-5d ", timeStr.data(), pid, tid);
    return prefixBuf;
}

#ifdef DEBUG

void LogDebugMessage(const std::string& msg)
{
    static std::fstream logStream;
    if (!logStream.is_open())
    {
        logStream.open(DO_PLUGIN_APT_LOG_PATH, std::ios_base::app | std::ios_base::ate);
        if (logStream.bad())
        {
            std::cerr << "Couldn't open debug log file at " << DO_PLUGIN_APT_LOG_PATH << std::endl;
            return;
        }
    }

    auto prefixBuf = g_GetLogLinePrefix();
    logStream << prefixBuf.data() << " " << msg << "\n";
    logStream.flush();
}

#endif // DEBUG

void LogErrorMessage(const std::string& msg)
{
    auto prefixBuf = g_GetLogLinePrefix();
    std::cerr << prefixBuf.data() << " " << msg << "\n";
    std::cerr.flush();

#ifdef DEBUG
    // Also write to debug log
    LogDebugMessage(msg);
#endif
}
