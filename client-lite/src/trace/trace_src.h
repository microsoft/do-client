#pragma once

void DoTraceLoggingRegister();
void DoTraceLoggingUnregister();

bool IsLoggingEnabled(boost::log::trivial::severity_level level);
bool IsVerboseLoggingEnabled();

void LogRuntimeFailure(const docli::FailureInfo& failure) noexcept;

#define EVENT_LEVEL_ERROR       boost::log::trivial::error
#define EVENT_LEVEL_WARNING     boost::log::trivial::warning
#define EVENT_LEVEL_INFO        boost::log::trivial::info
#define EVENT_LEVEL_VERBOSE     boost::log::trivial::trace

// '##' is required before __VA_ARGS__ to allow an empty arg list to the variadic macro.
// MSVC supports this by default but GCC requires '##'. C++2a has added VA_OPT macro
// to officially support this behavior.

#define DoLogMessage(level, msg, ...)   ::LogMessage((level), __FUNCTION__, __LINE__, (msg), ##__VA_ARGS__)
#define DoLogResult(lvl, hr, msg, ...)  ::LogResult((lvl), __FUNCTION__, __LINE__, (hr), (msg), ##__VA_ARGS__)

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

void LogMessageV(boost::log::trivial::severity_level level, _In_ PCSTR pszFunc, UINT nLine, HRESULT hrIn,
    _In_ _Printf_format_string_ PCSTR pszMsg, _In_ va_list argList);

void LogMessage(boost::log::trivial::severity_level level, _In_ PCSTR pszFunc, UINT uLine,
    _In_ _Printf_format_string_ PCSTR pszMsg, ...);

void LogResult(boost::log::trivial::severity_level level, _In_ PCSTR pszFunc, UINT uLine, HRESULT hr,
    _In_ _Printf_format_string_ PCSTR pszMsg, ...);
