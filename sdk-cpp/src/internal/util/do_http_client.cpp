#include "do_http_client.h"

#include <thread>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <gsl/gsl_util>

#include "do_exceptions_internal.h"
#include "do_exceptions.h"
#include "do_port_finder.h"

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

namespace microsoft::deliveryoptimization::details
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
        const auto endpoints = resolver.resolve("127.0.0.1", std::to_string(port));
        boost::system::error_code ec;
        boost::asio::connect(_socket, endpoints, ec);
        return ec;
    }

    std::pair<http::status, boost::property_tree::ptree> GetResponse(http::verb method, const std::string& url)
    {
        http::request<http::empty_body> req{method, url, 11};
        req.set(http::field::host, "127.0.0.1");
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        // std::cout << "Sending request:\n" << req << std::endl; // uncomment for debugging
        http::write(_socket, req);

        boost::beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(_socket, buffer, res);
        // std::cout << "Received response:\n" << res << std::endl; // uncomment for debugging

        std::stringstream responseBodyStream;
        responseBodyStream << res.body();

        boost::property_tree::ptree responseBodyJson;
        if (responseBodyStream.rdbuf()->in_avail() > 0)
        {
            try
            {
                boost::property_tree::read_json(responseBodyStream, responseBodyJson);
            }
            catch (...)
            {
            }
        }

        return {res.result(), responseBodyJson};
    }

private:
    boost::asio::io_context _ioc;
    boost::asio::ip::tcp::socket _socket{_ioc};
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

boost::property_tree::ptree CHttpClient::SendRequest(Method method, const std::string& url, bool retry)
{
    auto responseStatusCode = http::status::unknown;
    boost::property_tree::ptree responseBodyJson;
    try
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto pClient = static_cast<CHttpClientImpl*>(_httpClientImpl.get());
        std::tie(responseStatusCode, responseBodyJson) = pClient->GetResponse(
            method == Method::GET ? http::verb::get : http::verb::post, url);
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

    if (responseStatusCode != http::status::ok)
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

} // namespace microsoft::deliveryoptimization::details
