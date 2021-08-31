#pragma once

#include <memory>
#include <boost/asio.hpp>
#include "do_http_parser.h"

class HttpListenerConnection;
using http_listener_callback_t = std::function<void(const std::shared_ptr<microsoft::deliveryoptimization::details::HttpPacket>&,
    HttpListenerConnection&)>;

class HttpListenerConnection : public std::enable_shared_from_this<HttpListenerConnection>
{
public:
    HttpListenerConnection(boost::asio::io_service& ioService, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    ~HttpListenerConnection();

    static std::shared_ptr<HttpListenerConnection> Make(boost::asio::io_service& ioService,
        std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    void Receive(http_listener_callback_t& callback);
    void Reply(unsigned int statusCode);
    void Reply(unsigned int statusCode, const std::string& body);

    boost::asio::ip::tcp::endpoint RemoteEndpoint() const;

private:
    void _OnData(const boost::system::error_code& ec, size_t cbRead, http_listener_callback_t& callback);

    std::shared_ptr<boost::asio::ip::tcp::socket> _socket;
    boost::asio::io_service& _io;

    std::vector<char> _recvBuf;
    microsoft::deliveryoptimization::details::HttpParser _httpParser;
};

class RestHttpListener
{
public:
    void AddHandler(const http_listener_callback_t& handler);
    void Start(boost::asio::io_service& ioService);
    void Stop();
    std::string Endpoint() const;
    uint16_t Port() const;

private:
    void _BeginAccept();
    void _ProcessConnection(const std::shared_ptr<boost::asio::ip::tcp::socket>& socket);

    std::unique_ptr<boost::asio::ip::tcp::acceptor> _listener;
    http_listener_callback_t _handler;
    boost::asio::io_service* _io { nullptr };

    std::atomic<unsigned int> _numConnections { 0 };
};
