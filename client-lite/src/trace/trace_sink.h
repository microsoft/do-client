#pragma once

#include <shared_mutex>
#include <boost/log/sinks.hpp>
#include "do_noncopyable.h"

class TraceConsumer : DONonCopyable
{
public:
    // Returns a static instance of this class
    static TraceConsumer& getInstance();

    static boost::log::trivial::severity_level Level();

    HRESULT Initialize() noexcept;  // Start logging sink
    void Finalize();                // Flush and close logging sink
    void Flush() noexcept;

protected:
    TraceConsumer() = default;

    using textfile_sink = boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend>;
    boost::shared_ptr<textfile_sink> _spSink;

    std::shared_timed_mutex _traceSessionLock;
};
