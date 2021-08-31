#include "do_common.h"
#include "rest_http_listener_conn.h"

#include "do_http_defines.h"
#include "do_version.h"

using boost_tcp_t = boost::asio::ip::tcp;
namespace msdod = microsoft::deliveryoptimization::details;

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

static const char* g_HttpStatusToString(unsigned int statusCode)
{
#define RETURN_HTTP_STATUSCODE_STR(code) \
    case msdod::http_status_codes::code: return #code;

    switch (statusCode)
    {
        RETURN_HTTP_STATUSCODE_STR(OK);
        RETURN_HTTP_STATUSCODE_STR(BadRequest);
        RETURN_HTTP_STATUSCODE_STR(NotFound);
        RETURN_HTTP_STATUSCODE_STR(InternalError);
        RETURN_HTTP_STATUSCODE_STR(ServiceUnavailable);
        default: return "StatusDescription";
    }
    return "StatusDescription";
}

void HttpListenerConnection::Reply(unsigned int statusCode, const std::string& body)
{
    std::stringstream ss;
    ss << "HTTP/1.1 " << statusCode << ' ' << g_HttpStatusToString(statusCode) << "\r\n"; // caller doesn't care about description
    if (!body.empty())
    {
        ss << "Content-Length: " << body.size() << "\r\n";
    }
    ss << "Server: Delivery-Optimization-Agent/" << microsoft::deliveryoptimization::util::details::SimpleVersion() << "\r\n";
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

    // read more data or next message
    Receive(callback);
}
