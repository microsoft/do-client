#include "do_http_client.h"

#include <thread>
#include <boost/asio/connect.hpp>
// Debian10 uses 1.67 while Ubuntu18.04 has 1.65.1.
// Starting in 1.66, boost::asio::io_service changed to io_context and retained io_service as a typedef.
// Include this header explicitly to get it regardless of which boost version is installed.
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <gsl/gsl_util>

#include "do_errors.h"
#include "do_error_helpers.h"
#include "do_http_message.h"
#include "do_port_finder.h"

namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

class CHttpClientImpl
{
public:
    ~CHttpClientImpl()
    {
        if (_socket.is_open())
        {
            // Gracefully close the socket
            boost::system::error_code ec;
            _socket.shutdown(tcp::socket::shutdown_both, ec);
        }
    }

    boost::system::error_code Connect(ushort port)
    {
        tcp::resolver resolver{_ioc};
        const auto endpoints = resolver.resolve({ "127.0.0.1", std::to_string(port) });
        boost::system::error_code ec;
        boost::asio::connect(_socket, endpoints, ec);
        return ec;
    }

    std::pair<unsigned int, boost::property_tree::ptree> GetResponse(HttpRequest::Method method, const std::string& url)
    {
        HttpRequest request{method, url};
        request.Serialize(_socket);

        HttpResponse response;
        response.Deserialize(_socket);

        return {response.StatusCode(), response.ExtractJsonBody()};
    }

private:
    net::io_service _ioc;
    net::ip::tcp::socket _socket{_ioc};
};

CHttpClient::~CHttpClient() = default;

CHttpClient& CHttpClient::GetInstance()
{
    static CHttpClient myInstance;
    return myInstance;
}

void CHttpClient::_InitializeDOConnection(bool launchClientFirst)
{
    const auto port = std::strtoul(CPortFinder::GetDOPort(launchClientFirst).data(), nullptr, 10);
    auto spImpl = std::make_unique<CHttpClientImpl>();
    auto ec = spImpl->Connect(gsl::narrow<ushort>(port));
    if (ec)
    {
        // TODO(shishirb) Log the actual error when logging is available
        ThrowException(microsoft::deliveryoptimization::errc::no_service);
    }

    std::unique_lock<std::mutex> lock(_mutex);
    _httpClientImpl = std::move(spImpl);
}

boost::property_tree::ptree CHttpClient::SendRequest(HttpRequest::Method method, const std::string& url, bool retry)
{
    auto responseStatusCode = 0u;
    boost::property_tree::ptree responseBodyJson;
    try
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto pClient = static_cast<CHttpClientImpl*>(_httpClientImpl.get());
        std::tie(responseStatusCode, responseBodyJson) = pClient->GetResponse(method, url);
    }
    catch (const boost::system::system_error& e)
    {
        if (retry)
        {
            _InitializeDOConnection(true);
            return SendRequest(method, url, false);
        }

        ThrowException(e.code().value());
    }

    if (responseStatusCode != 200)
    {
        auto agentErrorCode = responseBodyJson.get_optional<int32_t>("ErrorCode");
        ThrowException(agentErrorCode ? *agentErrorCode : -1);
    }

    return responseBodyJson;
}

CHttpClient::CHttpClient()
{
    _InitializeDOConnection();
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
