#include "do_common.h"
#include "rest_http_listener.h"

using boost_tcp_t = boost::asio::ip::tcp;

void RestHttpListener::AddHandler(const http_listener_callback_t& handler)
{
    _handler = handler;
}

void RestHttpListener::Start(boost::asio::io_service& ioService)
{
    _io = &ioService;

    // IANA suggests ephemeral ports can be in range [49125, 65535].
    // Linux suggests [32768, 60999] while Windows says [1025, 65535].
    // We just choose a range that lies within all three implementations.
    uint16_t restPort = 50000;
    constexpr uint16_t restPortLimit = 60999;
    const auto addr = boost::asio::ip::address_v4::from_string("127.0.0.1");
    boost_tcp_t::endpoint endpoint(addr, restPort);
    boost_tcp_t::acceptor tmpListener{ioService, endpoint.protocol()};
    while (true)
    {
        endpoint.port(restPort);
        try
        {
            tmpListener.bind(endpoint); // wait for completion and check for exceptions
            tmpListener.listen();
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
    DO_ASSERT(tmpListener.is_open());
    _listener = std::make_unique<boost_tcp_t::acceptor>(std::move(tmpListener));
    _BeginAccept();
}

void RestHttpListener::Stop()
{
    if (_listener && _listener->is_open())
    {
        _listener->close();
        _listener.reset();
    }
}

std::string RestHttpListener::Endpoint() const
{
    std::stringstream ss;
    ss << _listener->local_endpoint();
    return ss.str();
}

uint16_t RestHttpListener::Port() const
{
    return _listener->local_endpoint().port();
}

void RestHttpListener::_BeginAccept()
{
    auto acceptSocket = std::make_shared<boost_tcp_t::socket>(*_io);
    _listener->async_accept(*acceptSocket, [this, acceptSocket](const boost::system::error_code& ec) mutable
        {
            if (ec)
            {
                DoLogError("Async accept failed, error = %d", ec.value());
                // TODO(shishirb) terminate process by throwing exception?
                return;
            }

            try
            {
                auto request = HttpListenerConnection::Make(*_io, acceptSocket);
                request->Receive(_handler);
            } CATCH_LOG()

            _BeginAccept(); // accept the next connection
        });
}

HttpListenerConnection::HttpListenerConnection(boost::asio::io_service& ioService, std::shared_ptr<boost::asio::ip::tcp::socket> socket) :
    _socket(std::move(socket)),
    _io(ioService)
{
    _recvBuf.resize(2048);
}

HttpListenerConnection::~HttpListenerConnection()
{
    if (_socket->is_open())
    {
        std::stringstream ss;
        ss << _socket->remote_endpoint();
        DoLogDebug("Socket closing: was connected to %s", ss.str().c_str());
        _socket->shutdown(boost_tcp_t::socket::shutdown_both);
        _socket->close();
    }
}

std::shared_ptr<HttpListenerConnection> HttpListenerConnection::Make(boost::asio::io_service& ioService,
    std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    return std::make_shared<HttpListenerConnection>(ioService, std::move(socket));
}

void HttpListenerConnection::Receive(http_listener_callback_t& callback)
{
    _socket->async_read_some(boost::asio::buffer(_recvBuf.data(), _recvBuf.size()),
        [this, lifetime = shared_from_this(), &callback](const boost::system::error_code& ec, size_t cbRead)
            {
                _OnData(ec, cbRead, callback);
            });
}

void HttpListenerConnection::Reply(unsigned int statusCode)
{
    Reply(statusCode, std::string{});
}

void HttpListenerConnection::Reply(unsigned int statusCode, const std::string& body)
{
    std::stringstream ss;
    ss << statusCode << ' ' << "Description" << "\r\n"; // caller doesn't care about description
    if (!body.empty())
    {
        ss << "Content-Length: " << body.size() << "\r\n";
    }
    ss << "Server: DO-Agent" << "\r\n";
    ss << "\r\n";
    if (!body.empty())
    {
        ss << body;
    }

    auto replyHttpMessage = std::make_shared<std::string>(ss.str());
    DoLogDebug("Sending response: %s\n", replyHttpMessage->c_str());
    boost::asio::async_write(*_socket, boost::asio::buffer(replyHttpMessage->data(), replyHttpMessage->size()),
        [this, lifetime = shared_from_this(), msgLifetime = replyHttpMessage](const boost::system::error_code& ec, size_t cbSent)
            {
                if (ec)
                {
                    DoLogWarning("Socket send error: %d, %s", ec.value(), ec.message().c_str());
                }
                else
                {
                    DoLogDebug("Socket sent %zu bytes", cbSent);
                }
            });
}

boost::asio::ip::tcp::endpoint HttpListenerConnection::RemoteEndpoint() const
{
    boost::system::error_code ec;
    auto endpoint = _socket->remote_endpoint(ec);
    return endpoint;
}

void HttpListenerConnection::_OnData(const boost::system::error_code& ec, size_t cbRead, http_listener_callback_t& callback)
{
    if (ec)
    {
        DoLogWarning("Socket receive error: %d, %s", ec.value(), ec.message().c_str());
        return;
    }

    if (cbRead == 0)
    {
        DoLogDebug("Socket graceful close from remote endpoint");
        return;
    }

    try
    {
        _httpParser.OnData(_recvBuf.data(), cbRead);
    }
    catch (const std::exception&)
    {
        Reply(400);
        return;
    }

    if (_httpParser.Done())
    {
        _io.post([this, lifetime = shared_from_this(), parsedData = _httpParser.ParsedData(), &callback]()
            {
                callback(parsedData, *this);
            });
        _httpParser.Reset(); // get ready for the next message
    }
    else
    {
        // read more data
        Receive(callback);
    }
}
