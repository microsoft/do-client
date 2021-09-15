#include "do_log.h"

#ifdef DEBUG

#include <unistd.h> // getpid
#include <sys/syscall.h> // SYS_gettid
#include <fstream>
#include "do_date_time.h"
#include "do_plugin_exception.h"

void LogMessage(const std::string& msg) try
{
    static std::fstream logStream;
    if (!logStream.is_open())
    {
        logStream.open(DO_PLUGIN_APT_LOG_PATH, std::ios_base::app | std::ios_base::ate);
        if (logStream.bad())
        {
            throw DOPluginException("Couldn't open debug log file");
        }
    }

    static const pid_t pid = getpid();
    const auto tid = static_cast<int>(syscall(SYS_gettid)); // using syscall because glibc wrapper is unavailable
    const auto timeStr = SysTimePointToUTCString(wall_clock_t::now());

    // Timestamp ProcessID ThreadID msg
    std::array<char, 128> prefixBuf;
    snprintf(prefixBuf.data(), prefixBuf.size(), "%s %-5d %-5d ", timeStr.data(), pid, tid);
    logStream << prefixBuf.data() << " " << msg << "\n";
    logStream.flush();
} catch (...)
{
}

#endif // DEBUG
