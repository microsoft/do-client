#include "do_common.h"
#include <chrono>

using filetime_duration_t = std::chrono::duration<INT64, std::ratio<1,10000000>>;
using wall_clock_t = std::chrono::system_clock;

#define filetime_cast(d)        std::chrono::duration_cast<filetime_duration_t>(d)
#define seconds_cast(d)         std::chrono::duration_cast<std::chrono::seconds>(d)

inline std::array<char, 30> SysTimePointToUTCString(wall_clock_t::time_point timePoint)
{
    const auto tt = wall_clock_t::to_time_t(timePoint);
    struct tm st = {};
    gmtime_r(&tt, &st);

    auto ft = filetime_cast(timePoint.time_since_epoch());
    auto fractionalSeconds = ft - seconds_cast(ft);
    std::array<char, 30> timebuf = {};
    snprintf(timebuf.data(), timebuf.size(), "%04d-%02d-%02dT%02d:%02d:%02d.%07dZ",
        st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, static_cast<int>(fractionalSeconds.count()));
    return timebuf;
}
