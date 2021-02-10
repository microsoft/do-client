#include "do_common.h"
#include "trace_sink.h"

#include <unistd.h> // getpid
#include <sys/syscall.h> // SYS_gettid
#include <boost/log/expressions.hpp>
#include "config_defaults.h"
#include "do_date_time.h"
#include "do_persistence.h"

namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;

static void DOLogFormatter(const logging::record_view& rec, logging::formatting_ostream& stream)
{
    static const pid_t pid = getpid();
    const auto tid = static_cast<int>(syscall(SYS_gettid)); // using syscall because glibc wrapper is unavailable
    const char* sev = boost::log::trivial::to_string(rec[logging::trivial::severity].get());
    if (sev == nullptr)
    {
        DO_ASSERT(false);
        sev = "invalid";
    }
    const auto timeStr = SysTimePointToUTCString(wall_clock_t::now());

    // Timestamp ProcessID ThreadID severity message
    std::array<char, 128> prefixBuf;
    snprintf(prefixBuf.data(), prefixBuf.size(), "%s %-5d %-5d %-8s ", timeStr.data(), pid, tid, sev);

    const auto msg = rec[expr::smessage];
    stream << prefixBuf.data() << msg;

#ifdef DEBUG
    // Log to console. Better do this here than in trace_src.cpp in order to get the fully formatted log line.
    // If a sink is not registered with the boost.log core, then it will by default dump all logs to the console,
    // without coming through here.
    printf("%s %s\n", prefixBuf.data(), msg.get().data());
#endif
}

TraceConsumer& TraceConsumer::getInstance()
{
    static TraceConsumer myInstance;
    return myInstance;
}

logging::trivial::severity_level TraceConsumer::Level()
{
    return DEF_TRACE_LEVEL;
}

HRESULT TraceConsumer::Initialize() noexcept try
{
    std::unique_lock<std::shared_timed_mutex> lock(_traceSessionLock);
    RETURN_HR_IF(S_OK, _spSink);

    const std::string logPath = docli::GetPersistenceDirectory() + "/log";
    const std::string logNameFormat = logPath + "/do-agent.%Y%m%d_%H%M%S.log";
    auto spSink = boost::make_shared<textfile_sink>(
        keywords::file_name = logNameFormat,
        keywords::rotation_size = DEF_TRACE_FILE_MAXSIZE_BYTES);

    spSink->set_formatter(&DOLogFormatter);

    auto level = Level();
    logging::core::get()->set_filter(logging::trivial::severity >= level);

    // set_file_collector creates the target dir, recursively, if not exists
    spSink->locked_backend()->set_file_collector(sinks::file::make_collector(
        keywords::target = logPath.data(),
        keywords::max_size = DEF_TRACE_FOLDER_MAXSIZE_BYTES));

    // Scan the directory for previously used files so the collector knows about them
    spSink->locked_backend()->scan_for_files();

    // Register the sink in the logging core
    logging::core::get()->add_sink(spSink);
    _spSink = spSink;

    return S_OK;
} CATCH_RETURN()

void TraceConsumer::Finalize()
{
    std::unique_lock<std::shared_timed_mutex> lock(_traceSessionLock);
    if (_spSink)
    {
        _spSink->flush();
        logging::core::get()->remove_sink(_spSink);
        _spSink.reset();
    }
}

void TraceConsumer::Flush() noexcept try
{
    std::unique_lock<std::shared_timed_mutex> lock(_traceSessionLock);
    if (_spSink)
    {
        _spSink->flush();
    }
} CATCH_LOG()
