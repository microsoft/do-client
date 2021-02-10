#pragma once

#include <string>
#include "do_string_util.h"

void LogMessage(const std::string& msg);

// This explicit overload prevents format-security warning
// for callers who log a simple string without any format arguments.
inline void LogDebug(const char* msg)
{
    LogMessage(std::string{msg});
}

template <typename... Args>
void LogDebug(const char* fmt, Args&&... args) try
{
    const std::string msg = strutil::FormatString(fmt, std::forward<Args>(args)...);
    LogMessage(msg);
} catch (...)
{
}
