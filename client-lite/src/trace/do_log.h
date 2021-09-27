// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <string>

namespace DOLog
{

enum class Level
{
    Error,
    Warning,
    Info,
    Verbose,
};

void Init(const std::string& logDir, Level maxLogLevel);
void Close();
void Write(Level level, const char* pszFunc, unsigned int uLine, const char* pszFmt, ...);
void WriteResult(Level level, const char* pszFunc, unsigned int uLine, HRESULT hr, const char* pszFmt, ...);

} // namespace DOLog

#define EVENT_LEVEL_ERROR       DOLog::Level::Error
#define EVENT_LEVEL_WARNING     DOLog::Level::Warning
#define EVENT_LEVEL_INFO        DOLog::Level::Info
#define EVENT_LEVEL_VERBOSE     DOLog::Level::Verbose

// '##' is required before __VA_ARGS__ to allow an empty arg list to the variadic macro.
// MSVC supports this by default but GCC requires '##'. C++2a has added VA_OPT macro
// to officially support this behavior.

#define DoLogMessage(level, msg, ...)   DOLog::Write((level), __FUNCTION__, __LINE__, (msg), ##__VA_ARGS__)
#define DoLogResult(level, hr, msg, ...)  DOLog::WriteResult((level), __FUNCTION__, __LINE__, (hr), (msg), ##__VA_ARGS__)

#define DoLogError(msg, ...)            DoLogMessage(EVENT_LEVEL_ERROR,    (msg), ##__VA_ARGS__)
#define DoLogWarning(msg, ...)          DoLogMessage(EVENT_LEVEL_WARNING,  (msg), ##__VA_ARGS__)
#define DoLogInfo(msg, ...)             DoLogMessage(EVENT_LEVEL_INFO,     (msg), ##__VA_ARGS__)
#define DoLogVerbose(msg, ...)          DoLogMessage(EVENT_LEVEL_VERBOSE,  (msg), ##__VA_ARGS__)

#ifdef DEBUG
#define DoLogDebug(msg, ...)            DoLogMessage(EVENT_LEVEL_VERBOSE,  (msg), ##__VA_ARGS__)
#else
#define DoLogDebug(msg, ...)
#endif

#define DoLogErrorHr(hr, msg, ...)      DoLogResult(EVENT_LEVEL_ERROR,    (hr), (msg), ##__VA_ARGS__)
#define DoLogWarningHr(hr, msg, ...)    DoLogResult(EVENT_LEVEL_WARNING,  (hr), (msg), ##__VA_ARGS__)
#define DoLogInfoHr(hr, msg, ...)       DoLogResult(EVENT_LEVEL_INFO,     (hr), (msg), ##__VA_ARGS__)
#define DoLogVerboseHr(hr, msg, ...)    DoLogResult(EVENT_LEVEL_VERBOSE,  (hr), (msg), ##__VA_ARGS__)
