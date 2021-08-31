#pragma once

#include <memory>
#include <boost/asio.hpp>
#include "rest_http_listener_conn.h"

class RestHttpListener
{
public:
    void Start(boost::asio::io_service& ioService, const http_listener_callback_t& requestHandler);
    void Stop();
    std::string Endpoint() const;
    uint16_t Port() const;

private:
    void _BeginAccept();
    void _ProcessConnection(const std::shared_ptr<boost::asio::ip::tcp::socket>& socket);

    std::unique_ptr<boost::asio::ip::tcp::acceptor> _listener;
    http_listener_callback_t _requestHandler;
    boost::asio::io_service* _io { nullptr };

    std::atomic<unsigned int> _numConnections { 0 };
};
