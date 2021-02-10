#pragma once

#include <string>
#include "do_string_util.h"

class DOPluginException
{
private:
    std::string _msg;

public:
    DOPluginException(std::string msg) :
        _msg(std::move(msg))
    {
    }

    // This explicit overload prevents format-security warning
    // for callers who log a simple string without any format arguments.
    explicit DOPluginException(const char* msg) :
        _msg(msg)
    {
    }

    template <typename... Args>
    explicit DOPluginException(const char* fmt, Args&&... args)
    {
        _msg = strutil::FormatString(fmt, std::forward<Args>(args)...);
    }

    const char* what() const { return _msg.c_str(); }
};
