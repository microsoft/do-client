#pragma once

#include <memory>
#include <boost/asio.hpp>
// TODO(shishirb) fwd declare the acceptor?

class RestHttpListener
{
public:
    class Message
    {
        char* method;
        boost::asio::ip::tcp::endpoint remoteEndpoint;
        std::string url;
        std::string body; // response only
    };

    using callback_t = std::function<void(Message)>;

    void AddHandler(const std::function<void(Message)>& handler);
    void Start(boost::asio::io_service& ioService);
    void Stop();
    std::string Endpoint() const;
    uint16_t Port() const;

private:
    void _BeginAccept();

    std::unique_ptr<boost::asio::ip::tcp::acceptor> _listener;
    callback_t _handler;
    boost::asio::io_service* _io { nullptr };
};
