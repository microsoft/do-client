// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <string>
#include "do_string_util.h"

#ifdef DEBUG

void LogDebugMessage(const std::string& msg);

template <typename... Args>
void LogDebug(const char* fmt, Args&&... args)
{
    const std::string msg = strutil::FormatString(fmt, std::forward<Args>(args)...);
    LogDebugMessage(msg);
}

#else

#define LogDebug(...)

#endif // DEBUG

void LogErrorMessage(const std::string& msg);

template <typename... Args>
void LogError(const char* fmt, Args&&... args)
{
    const std::string msg = strutil::FormatString(fmt, std::forward<Args>(args)...);
    LogErrorMessage(msg);
}
