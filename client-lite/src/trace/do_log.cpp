#include "do_common.h"
#include "do_log.h"

#include <dirent.h>
#include <unistd.h> // getpid
#include <sys/syscall.h> // SYS_gettid
#include <cstdarg> // va_start, etc.
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include "do_date_time.h"
#include "string_ops.h"

namespace DOLog
{

const char* const g_logFileNamePrefix = "do-agent.";
constexpr uint32_t g_maxLogFileSizeBytes = 256 * 1024;
constexpr uint32_t g_maxLogFiles = 3;
constexpr uint32_t g_logBufferSizeBytes = 64 * 1024;

class LogBuffer
{
public:
    struct Stats
    {
        uint32_t numResets;
        uint32_t numFlushes;
        uint32_t numWritesSucceeded;
        uint32_t numWritesFailed;
    };

private:
    std::vector<char> _buffer;
    size_t _cursor;
    Stats _stats {};

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
        ++_stats.numResets;
    }

    HRESULT Flush(std::ofstream& fileStream) try
    {
        if (fileStream.is_open() && BytesBuffered())
        {
            fileStream.write(_buffer.data(), _cursor);
            fileStream.flush();
            Reset();
            ++_stats.numFlushes;
        }
        return S_OK;
    } CATCH_RETURN()

    size_t BytesBuffered() const noexcept
    {
        return _cursor;
    }

    size_t BytesFree() const noexcept
    {
        return (_buffer.size() - _cursor);
    }

    size_t BytesFreePercentage() const noexcept
    {
        return BytesFree() * static_cast<uint64_t>(100) / _buffer.size();
    }

    const Stats& PerfStats() const noexcept
    {
        return _stats;
    }

    HRESULT Write(const char* pszLevel, const char* pszFunc, uint32_t nLine, HRESULT hrIn, const char* pszFmt, va_list argList)
    {
        static const pid_t pid = getpid();
        const auto tid = static_cast<int>(syscall(SYS_gettid)); // using syscall because glibc wrapper is unavailable
        const auto timeStr = SysTimePointToUTCString(wall_clock_t::now());

        int cchWritten = 0;
        int cchTotalWritten = 0;

        char* pszBuffer = _buffer.data() + _cursor;
        const size_t cchBuffer = BytesFree();

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
        if (SUCCEEDED(hr))
        {
            cchTotalWritten += cchWritten;

            if ((cchBuffer - cchTotalWritten) > 1)
            {
                *(pszBuffer + cchTotalWritten) = '\n';
                ++cchTotalWritten;
                *(pszBuffer + cchTotalWritten) = '\0';
            }
            else
            {
                hr = STRSAFE_E_INSUFFICIENT_BUFFER;
            }
        }
        // Log the entire buffer
        if (SUCCEEDED(hr))
        {
            _cursor += cchTotalWritten;
#ifdef DEBUG
            fprintf(stdout, "%s", pszBuffer);
#endif
            ++_stats.numWritesSucceeded;
        }
        else
        {
#ifdef DEBUG
            fprintf(stderr, "Log write failed with 0x%x, bytes_free: %zu, written: %d, total_written: %d\n",
                hr, cchBuffer, cchWritten, cchTotalWritten);
#endif
            ++_stats.numWritesFailed;
        }
        return hr;
    }
};

class LoggerImpl
{
private:
    struct Stats
    {
        uint32_t numFilesDeleted;
        uint32_t numFileCreationAttempts;
        uint32_t numFilesCreated;
        uint64_t numFlushThreadLoops;
        uint64_t numFlushWrites;
        uint32_t numWritesFailed;
    };

    std::string _logDir;
    Level _maxLogLevel;
    std::thread _flushThread;
    std::mutex _mutex;
    std::condition_variable _cv;

    LogBuffer _writeBuffer;
    std::ofstream _logFile;

    Stats _stats {};

    bool _fRunning { true };

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

    static int s_LogFilesFilter(const struct dirent* logfile)
    {
        return (logfile->d_type == DT_REG) && (strstr(logfile->d_name, g_logFileNamePrefix) != NULL);
    }

    void _ClearOlderLogFilesIfNeeded()
    {
        // From: https://github.com/Azure/iot-hub-device-update/blob/main/src/logging/zlog/src/zlog.c

        // List the files specified by file_select in alphabetical order
        struct dirent** logfiles;
        const int nFiles = scandir(_logDir.c_str(), &logfiles, s_LogFilesFilter, alphasort);
        if (nFiles == -1)
        {
            return;
        }

        if (static_cast<uint32_t>(nFiles) > g_maxLogFiles)
        {
            for (uint32_t i = 0; i < (static_cast<uint32_t>(nFiles) - g_maxLogFiles); ++i)
            {
                char filepath[512];
                if (SUCCEEDED(StringPrintf(filepath, ARRAYSIZE(filepath), "%s/%s", _logDir.c_str(), logfiles[i]->d_name)))
                {
                    remove(filepath);
                    ++_stats.numFilesDeleted;
                }
            }
        }

        // Free memory allocated by scandir.
        for (int i = 0; i < nFiles; ++i)
        {
            free(logfiles[i]);
        }
        free(logfiles);
    }

    void _CreateLogFile()
    {
        ++_stats.numFileCreationAttempts;

        const auto timestamp = SysTimePointToFileNameString(wall_clock_t::now());
        char filePath[512];
        const auto hr = StringPrintf(filePath, ARRAYSIZE(filePath), "%s/%s%s.log", _logDir.c_str(), g_logFileNamePrefix, timestamp.data());
        DO_ASSERT(SUCCEEDED(hr));

        _logFile.open(filePath, std::fstream::out | std::fstream::app);
        if (_logFile.fail())
        {
            fprintf(stderr, "Failed to create log file at %s", filePath);
        }
        else
        {
            ++_stats.numFilesCreated;
        }
    }

    void _RotateLogFilesIfNeeded()
    {
        const bool fLogWasOpen = _logFile.is_open();
        if (fLogWasOpen && (_logFile.tellp() >= static_cast<std::streampos>(g_maxLogFileSizeBytes)))
        {
            _logFile.close();
        }
        _ClearOlderLogFilesIfNeeded();
        if (fLogWasOpen && !_logFile.is_open())
        {
            _CreateLogFile();
        }
    }

    void _FlushThreadProc()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait_for(lock, std::chrono::minutes(3), [this](){ return !_fRunning; });
            if (!_fRunning)
            {
                break;
            }

            _writeBuffer.Flush(_logFile);
            _RotateLogFilesIfNeeded();
            ++_stats.numFlushThreadLoops;
        }
    }

public:
    LoggerImpl(const std::string& logDir, Level maxLogLevel) :
        _logDir(logDir),
        _maxLogLevel(maxLogLevel),
        _writeBuffer(g_logBufferSizeBytes)
    {
        _RotateLogFilesIfNeeded();
        _CreateLogFile();

        _flushThread = std::thread{[this]()
            {
                _FlushThreadProc();
            }};
    }

    ~LoggerImpl()
    {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _fRunning = false;
            _cv.notify_all();
        }
        _flushThread.join();

        const auto& bufferStats = _writeBuffer.PerfStats();
        DoLogInfo("Logger stats: [creation_attempts: %u, created: %u, deleted: %u, threadproc: %lu, flush_writes: %lu, writes_f: %u], "
            "buffer: [resets: %u, flushes: %u, writes_s: %u, writes_f: %u]",
            _stats.numFileCreationAttempts, _stats.numFilesCreated, _stats.numFilesDeleted, _stats.numFlushThreadLoops,
            _stats.numFlushWrites, _stats.numWritesFailed,
            bufferStats.numResets, bufferStats.numFlushes, bufferStats.numWritesSucceeded, bufferStats.numWritesFailed);

        std::unique_lock<std::mutex> lock(_mutex);
        if (_logFile.is_open())
        {
            _writeBuffer.Flush(_logFile);
            _logFile.close();
        }
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

        std::unique_lock<std::mutex> lock(_mutex);

        HRESULT hr = _writeBuffer.Write(_LevelToString(level), pszFunc, uLine, hrIn, pszFmt, argCopy);
        if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
        {
            hr = _writeBuffer.Flush(_logFile);
            DO_ASSERT(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
            {
                _RotateLogFilesIfNeeded();

                va_copy(argCopy, argList);
                hr = _writeBuffer.Write(_LevelToString(level), pszFunc, uLine, hrIn, pszFmt, argCopy);
            }
            ++_stats.numFlushWrites;
        }

        if (FAILED(hr))
        {
            ++_stats.numWritesFailed;
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
