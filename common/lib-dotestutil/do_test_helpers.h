#pragma once

#include <string>
#include <thread>
#include <boost/asio.hpp>

namespace dotest
{
namespace util
{

// Free functions

void ExecuteSystemCommand(const char* command);

template <typename... Args>
std::string FormatString(const char* fmt, Args&&... args)
{
    char msgBuf[1024];
    snprintf(msgBuf, sizeof(msgBuf), fmt, std::forward<Args>(args)...);
    return { msgBuf };
}

// Helper classes

class BoostAsioWorker
{
public:
    ~BoostAsioWorker();

    std::unique_ptr<boost::asio::ip::tcp::endpoint> ResolveDnsQuery(const boost::asio::ip::tcp::resolver::query& resolverQuery);

    boost::asio::io_service& Service() { return _io; }

private:
    boost::asio::io_service _io;
    boost::asio::io_service::work _work { _io };
    std::thread _myThread { [this](){ _io.run(); } };
};

class DOTestException : public std::exception
{
private:
    std::string _msg;

public:
    DOTestException(std::string msg) :
        _msg(std::move(msg))
    {
    }

    // This explicit overload prevents format-security warning
    // for callers who log a simple string without any format arguments.
    explicit DOTestException(const char* msg) :
        _msg(msg)
    {
    }

    template <typename... Args>
    explicit DOTestException(const char* fmt, Args&&... args)
    {
        _msg = FormatString(fmt, std::forward<Args>(args)...);
    }

    const char* what() const noexcept override { return _msg.c_str(); }
};

} // namespace util
} // namespace dotest
