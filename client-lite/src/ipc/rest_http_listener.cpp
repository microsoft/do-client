#include "do_common.h"
#include "rest_http_listener.h"

using boost_tcp_t = boost::asio::ip::tcp;

void RestHttpListener::AddHandler(const std::function<void(Message)>& handler)
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

static std::vector<char>::iterator g_FindCRLF(std::vector<char>& buf, std::vector<char>::iterator itStart)
{
    auto itCR = std::find(itStart, buf.end(), '\r');
    if (itCR == buf.end() || (itCR + 1) == buf.end())
    {
        return buf.end(); // need more data
    }
    if (*(itCR + 1) != '\n')
    {
        throw std::exception();
    }
    return itCR;
}

void RestHttpListener::_BeginAccept()
{
    auto acceptSocket = std::make_shared<boost_tcp_t::socket>(*_io);
    _listener->async_accept(*acceptSocket, [this, acceptSocket](const boost::system::error_code& ec) mutable
        {
            if (!ec)
            {
                try
                {
                    std::stringstream ss;
                    ss << acceptSocket->remote_endpoint();
                    DoLogVerbose("Received connection from %s", ss.str().data());
                    acceptSocket->shutdown(boost_tcp_t::socket::shutdown_both);
                    acceptSocket->close();
                } CATCH_LOG()
                _BeginAccept(); // accept the next connection
            }
            else
            {
                DoLogWarning("Async accept failed, error = %d", ec.value());
            }
        });
}
