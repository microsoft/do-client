#include "do_common.h"
#include "do_log.h"

#include <unistd.h> // getpid
#include <sys/syscall.h> // SYS_gettid
#include <cstdarg> // va_start, etc.
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include "do_date_time.h"
#include "do_file.h"
#include "string_ops.h"

namespace DOLog
{

class LogBuffer
{
private:
    std::vector<char> _buffer;
    size_t _cursor;

public:
    LogBuffer(size_t bufSize) :
        _buffer(bufSize)
    {
        Reset();
    }

    void Reset()
    {
        memset(_buffer.data(), 0, _buffer.size());
        _cursor = 0;
    }

    HRESULT Flush(DOFile& file) try
    {
        file.Append(reinterpret_cast<BYTE*>(_buffer.data()), static_cast<UINT>(_cursor));
        Reset();
        return S_OK;
    } CATCH_RETURN()

    size_t Available() const noexcept
    {
        return (_buffer.size() - _cursor);
    }

    size_t AvailablePercentage() const noexcept
    {
        return Available() * static_cast<uint64_t>(100) / _buffer.size();
    }

    HRESULT Write(const char* pszLevel, const char* pszFunc, UINT nLine, HRESULT hrIn, const char* pszFmt, va_list argList)
    {
        static const pid_t pid = getpid();
        const auto tid = static_cast<int>(syscall(SYS_gettid)); // using syscall because glibc wrapper is unavailable
        const auto timeStr = SysTimePointToUTCString(wall_clock_t::now());

        int cchWritten = 0;
        int cchTotalWritten = 0;

        char* pszBuffer = _buffer.data() + _cursor;
        const size_t cchBuffer = Available();

        // Timestamp ProcessID ThreadID severity message
        HRESULT hr = StringPrintf(pszBuffer, cchBuffer, &cchWritten, "%s %-5d %-5d %-8s ", timeStr.data(), pid, tid, pszLevel);
        if (SUCCEEDED(hr))
        {
            cchTotalWritten += cchWritten;

            // Note: pszFunc may not be available. Example is logging from error_macros.cpp in release builds.
            if (hrIn != HRESULT(-1))
            {
                if (pszFunc != nullptr)
                {
                    hr = StringPrintf(pszBuffer + cchTotalWritten, (cchBuffer - cchTotalWritten), &cchWritten, "{%s} (hr:%X) ", pszFunc, hrIn);
                }
                else
                {
                    hr = StringPrintf(pszBuffer + cchTotalWritten, (cchBuffer - cchTotalWritten), &cchWritten, "(hr:%X) ", hrIn);
                }
            }
            else
            {
                if (pszFunc != nullptr)
                {
                    hr = StringPrintf(pszBuffer + cchTotalWritten, (cchBuffer - cchTotalWritten), &cchWritten, "{%s} ", pszFunc);
                }
            }
        }
        // Append the user provided message
        if (SUCCEEDED(hr))
        {
            cchTotalWritten += cchWritten;
            hr = StringPrintfV(pszBuffer + cchTotalWritten, (cchBuffer - cchTotalWritten), &cchWritten, pszFmt, argList);
        }
        // Log the entire buffer
        if (SUCCEEDED(hr))
        {
            _cursor += cchTotalWritten;

            try
            {
                // BOOST_LOG_SEV(::boost::log::trivial::logger::get(), level) << pszBuffer;
                printf("[hello] %s\n", pszBuffer);
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
};

class LoggerImpl
{
private:
    std::string _logDir;
    Level _maxLogLevel;
    std::thread _flushThread;
    std::mutex _mutex;
    std::condition_variable _cv;
    LogBuffer _writeBuffer;

    static const char* _LevelToString(Level level)
    {
        switch (level)
        {
            case Level::Error:      return "error";
            case Level::Warning:    return "warn ";
            case Level::Info:       return "info ";
            case Level::Verbose:    return "     ";
        }
        return "";
    }

public:
    LoggerImpl(const std::string& logDir, Level maxLogLevel) :
        _logDir(logDir),
        _maxLogLevel(maxLogLevel),
        _writeBuffer(512 * 1024)
    {
    }

    ~LoggerImpl()
    {
    }

    void WriteV(Level level, const char* pszFunc, unsigned int uLine, HRESULT hrIn, const char* pszFmt, va_list argList)
    {
        if (level > _maxLogLevel)
        {
            return;
        }

        // TBD save and restore errno?

        // Make a copy of argList to allow its reuse
        va_list argCopy;
        va_copy(argCopy, argList);

        HRESULT hr = _writeBuffer.Write(_LevelToString(level), pszFunc, uLine, hrIn, pszFmt, argCopy);
        if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
        {
            // hr = _writeBuffer.Flush();
            // DO_ASSERT(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
            {
                va_copy(argCopy, argList);
                hr = _writeBuffer.Write(_LevelToString(level), pszFunc, uLine, hrIn, pszFmt, argCopy);
            }
        }

        if (FAILED(hr))
        {
            fprintf(stderr, "Log write failed with 0x%x", hr);
        }
    }
};

void g_LogRuntimeFailure(const docli::FailureInfo& failure) noexcept
{
    // Runtime failures, including caught exceptions, are logged here.
    // By default, pszFunction is present in debug code, not retail.
    // Similarly, pszCode (string version of macro contents) is debug only.
    // pszMessage is only present when using a _MSG macro, such as LOG_IF_FAILED_MSG.
    if (failure.pszMessage != nullptr)
    {
        WriteResult(Level::Error, failure.pszFunction, failure.uLineNumber, failure.hr, "%s [%s, %d]", failure.pszMessage, failure.pszFile,
            failure.uLineNumber);
    }
    else if (failure.pszCode != nullptr)
    {
        WriteResult(Level::Error, failure.pszFunction, failure.uLineNumber, failure.hr, "%s [%s, %d]", failure.pszCode, failure.pszFile,
            failure.uLineNumber);
    }
    else
    {
        WriteResult(Level::Error, failure.pszFunction, failure.uLineNumber, failure.hr, "[%s, %d]", failure.pszFile, failure.uLineNumber);
    }
}

static LoggerImpl* g_pLogger = nullptr;

void Init(const std::string& logDir, Level maxLogLevel)
{
    if (g_pLogger)
    {
        return;
    }

    g_pLogger = new (std::nothrow) LoggerImpl{logDir, maxLogLevel};
    docli::SetResultLoggingCallback(g_LogRuntimeFailure);
}

void Close()
{
    docli::SetResultLoggingCallback(nullptr);
    delete g_pLogger;
    g_pLogger = nullptr;
}

void Write(Level level, const char* pszFunc, unsigned int uLine, const char* pszFmt, ...)
{
    if (g_pLogger)
    {
        va_list marker;
        va_start(marker, pszFmt);
        g_pLogger->WriteV(level, pszFunc, uLine, (HRESULT)-1, pszFmt, marker);
        va_end(marker);
    }
}

void WriteResult(Level level, const char* pszFunc, unsigned int uLine, HRESULT hr, const char* pszFmt, ...)
{
    if (g_pLogger)
    {
        va_list marker;
        va_start(marker, pszFmt);
        g_pLogger->WriteV(level, pszFunc, uLine, hr, pszFmt, marker);
        va_end(marker);
    }
}

} // namespace DOLog
