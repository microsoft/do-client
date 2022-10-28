// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <string>
#include <thread>
#ifdef DO_PLATFORM_LINUX
// TODO(shishirb) Need to install boost-asio on windows/mac build agents?
#include <boost/asio.hpp>
#endif

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

#ifdef DO_PLATFORM_LINUX
class BoostAsioWorker
{
public:
    ~BoostAsioWorker();

    std::unique_ptr<boost::asio::ip::tcp::endpoint> ResolveDnsQuery(const boost::asio::ip::tcp::resolver::query& resolverQuery,
        const boost::asio::ip::tcp::resolver::protocol_type* prot = nullptr);

    boost::asio::io_service& Service() { return _io; }

private:
    boost::asio::io_service _io;
    boost::asio::io_service::work _work { _io };
    std::thread _myThread { [this](){ _io.run(); } };
};
#endif // DO_PLATFORM_LINUX

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

template <typename TLambda>
class lambda_call
{
public:
    lambda_call(const lambda_call&) = delete;
    lambda_call& operator=(const lambda_call&) = delete;
    lambda_call& operator=(lambda_call&& other) = delete;

    explicit lambda_call(TLambda&& lambda) noexcept :
        _lambda(std::move(lambda))
    {
        static_assert(std::is_same<decltype(lambda()), void>::value, "scope_exit lambdas must not have a return value");
        static_assert(!std::is_lvalue_reference<TLambda>::value && !std::is_rvalue_reference<TLambda>::value,
            "scope_exit should only be directly used with a lambda");
    }

    lambda_call(lambda_call&& other) noexcept :
        _lambda(std::move(other._lambda)),
        _fCall(other._fCall)
    {
        other._fCall = false;
    }

    ~lambda_call() noexcept
    {
        reset();
    }

    // Ensures the scope_exit lambda will not be called
    void release() noexcept
    {
        _fCall = false;
    }

    // Executes the scope_exit lambda immediately if not yet run; ensures it will not run again
    void reset() noexcept
    {
        if (_fCall)
        {
            _fCall = false;
            _lambda();
        }
    }

    // Returns true if the scope_exit lambda is still going to be executed
    explicit operator bool() const noexcept
    {
        return _fCall;
    }

protected:
    TLambda _lambda;
    bool _fCall { true };
};

// Returns an object that executes the given lambda when destroyed.
// Capture the object with 'auto'; use reset() to execute the lambda early or release() to avoid execution.
template <typename TLambda>
inline auto scope_exit(TLambda&& lambda) noexcept
{
    return lambda_call<TLambda>(std::forward<TLambda>(lambda));
}

} // namespace util
} // namespace dotest
