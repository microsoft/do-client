#pragma once

#include <sstream>
#include <string>

namespace strutil
{

template <class T>
std::string MapToString(const T& map)
{
    std::stringstream ss;
    for (const auto& item : map)
    {
        ss << item.first << ": " << item.second << "\n";
    }
    return ss.str();
}

// This explicit overload prevents format-security warning
// at call sites that do not pass any format arguments.
inline std::string FormatString(const char* msg)
{
    return std::string{msg};
}

template <typename... Args>
std::string FormatString(const char* fmt, Args&&... args)
{
    char msgBuf[1024];
    snprintf(msgBuf, sizeof(msgBuf), fmt, std::forward<Args>(args)...);
    return { msgBuf };
}

std::string HexEncode(const unsigned char* src, size_t len);

inline bool StartsWith(const std::string& str, const char* s) noexcept
{
    return (str.rfind(s, 0) == 0);
}

}
