#pragma once

#include <memory>
#include <cpprest/http_listener.h>
#include <cpprest/http_msg.h>

class RestHttpListener
{
public:
    void AddHandler(const web::http::method& method, const std::function<void(web::http::http_request)>& handler);
    void Start(const std::string& listenUrl);
    void Stop();
    std::string Endpoint() const;
    uint16_t Port() const;

private:
    std::unique_ptr<web::http::experimental::listener::http_listener> _listener;
};
