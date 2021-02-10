#include "do_common.h"
#include "rest_http_listener.h"

using cpprest_http_listener_t = web::http::experimental::listener::http_listener;

void RestHttpListener::AddHandler(const web::http::method& method, const std::function<void(web::http::http_request)>& handler)
{
    _listener->support(method, handler);
}

void RestHttpListener::Start(const std::string& listenUrl)
{
    web::uri_builder endpointBuilder{ web::uri(listenUrl) };

    // IANA suggests ephemeral ports can be in range [49125, 65535].
    // Linux suggests [32768, 60999] while Windows says [1025, 65535].
    // We just choose a range that lies within all three implementations.
    uint16_t restPort = 50000;
    constexpr uint16_t restPortLimit = 60999;
    std::unique_ptr<cpprest_http_listener_t> tmpListener;
    while (true)
    {
        endpointBuilder.set_port(restPort);
        tmpListener = std::make_unique<cpprest_http_listener_t>(endpointBuilder.to_uri());
        try
        {
            tmpListener->open().wait(); // wait for completion and check for exceptions
            break;                      // break because listening was successfully started
        }
        catch (const boost::system::system_error& ex)
        {
            if ((ex.code().value() != EADDRINUSE) || (restPort == restPortLimit))
            {
                throw;
            }
            ++restPort;
        }
    }
    DO_ASSERT(tmpListener);
    _listener = std::move(tmpListener);
}

void RestHttpListener::Stop()
{
    if (_listener)
    {
        _listener->close().wait();
        _listener.reset();
    }
}

std::string RestHttpListener::Endpoint() const
{
    return _listener->uri().to_string();
}

uint16_t RestHttpListener::Port() const
{
    return _listener->uri().port();
}
