#include "do_common.h"
#include "trace_src.h"

#include <cstdarg> // va_start, etc.
#include <memory>
#include "string_ops.h"
#include "trace_sink.h"

void DoTraceLoggingRegister()
{
    docli::SetResultLoggingCallback(LogRuntimeFailure);
}

void DoTraceLoggingUnregister()
{
    docli::SetResultLoggingCallback(nullptr);
}

bool IsLoggingEnabled(boost::log::trivial::severity_level level)
{
    return (level >= TraceConsumer::Level());
}

bool IsVerboseLoggingEnabled()
{
    return IsLoggingEnabled(EVENT_LEVEL_VERBOSE);
}

void LogRuntimeFailure(const docli::FailureInfo& failure) noexcept
{
    // Runtime failures, including caught exceptions, are logged here.
    // By default, pszFunction is present in debug code, not retail.
    // Similarly, pszCode (string version of macro contents) is debug only.
    // pszMessage is only present when using a _MSG macro, such as LOG_IF_FAILED_MSG.
    if (failure.pszMessage != nullptr)
    {
        LogResult(EVENT_LEVEL_ERROR, failure.pszFunction, failure.uLineNumber, failure.hr, "%s [%s, %d]", failure.pszMessage, failure.pszFile, failure.uLineNumber);
    }
    else if (failure.pszCode != nullptr)
    {
        LogResult(EVENT_LEVEL_ERROR, failure.pszFunction, failure.uLineNumber, failure.hr, "%s [%s, %d]", failure.pszCode, failure.pszFile, failure.uLineNumber);
    }
    else
    {
        LogResult(EVENT_LEVEL_ERROR, failure.pszFunction, failure.uLineNumber, failure.hr, "[%s, %d]", failure.pszFile, failure.uLineNumber);
    }
}

static HRESULT _LogMessage(_In_ PSTR pszBuffer, size_t cchBuffer, boost::log::trivial::severity_level level,
    _In_opt_ PCSTR pszFunc, UINT nLine, HRESULT hrIn, _In_ _Printf_format_string_ PCSTR pszMsg, _In_ va_list argList)
{
    HRESULT hr = S_OK;
    int cchWritten = 0;
    // Note: pszFunc may not be available. Example is logging from error_macros.cpp in release builds.
    if (hrIn != HRESULT(-1))
    {
        if (pszFunc != nullptr)
        {
            hr = StringPrintf(pszBuffer, cchBuffer, &cchWritten, "{%s} (hr:%X) ", pszFunc, hrIn);
        }
        else
        {
            hr = StringPrintf(pszBuffer, cchBuffer, &cchWritten, "(hr:%X) ", hrIn);
        }
    }
    else
    {
        if (pszFunc != nullptr)
        {
            hr = StringPrintf(pszBuffer, cchBuffer, &cchWritten, "{%s} ", pszFunc);
        }
    }

    // Append the user provided message
    if (SUCCEEDED(hr))
    {
        hr = StringPrintfV(pszBuffer + cchWritten, (cchBuffer - cchWritten), pszMsg, argList);
    }

    if (SUCCEEDED(hr))
    {
        try
        {
            BOOST_LOG_SEV(::boost::log::trivial::logger::get(), level) << pszBuffer;
        }
        catch (const std::exception& ex)
        {
#ifdef DEBUG
            printf("Logging exception: %s\n", ex.what());
#endif
        }
    }
    return hr;
}

void LogMessageV(boost::log::trivial::severity_level level, _In_ PCSTR pszFunc, UINT nLine, HRESULT hrIn,
    _In_ _Printf_format_string_ PCSTR pszMsg, _In_ va_list argList)
{
    // TBD save and restore errno?

    // Make a copy of argList to allow its reuse
    va_list argCopy;
    va_copy(argCopy, argList);

    // Build the message text
    char szMessage[200];
    size_t cchMessage = ARRAYSIZE(szMessage);

    // Try first with the stack buffer
    HRESULT hr = _LogMessage(szMessage, cchMessage, level, pszFunc, nLine, hrIn, pszMsg, argCopy);
    while (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
    {
        // Use a heap buffer
        cchMessage *= 2;

        std::unique_ptr<char[]> spMessage;
        spMessage.reset(new (std::nothrow) char[cchMessage]);
        hr = spMessage ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            va_copy(argCopy, argList);
            hr = _LogMessage(spMessage.get(), cchMessage, level, pszFunc, nLine, hrIn, pszMsg, argCopy);
        }
    }
}

void LogMessage(boost::log::trivial::severity_level level, _In_ PCSTR pszFunc, UINT nLine, _In_ _Printf_format_string_ PCSTR pszMsg, ...)
{
    if (IsLoggingEnabled(level))
    {
        va_list marker;
        va_start(marker, pszMsg);
        LogMessageV(level, pszFunc, nLine, HRESULT(-1), pszMsg, marker);
        va_end(marker);
    }
}

void LogResult(boost::log::trivial::severity_level level, _In_ PCSTR pszFunc, UINT nLine, HRESULT hr, _In_ _Printf_format_string_ PCSTR pszMsg, ...)
{
    if (IsLoggingEnabled(level))
    {
        va_list marker;
        va_start(marker, pszMsg);
        LogMessageV(level, pszFunc, nLine, hr, pszMsg, marker);
        va_end(marker);
    }
}
