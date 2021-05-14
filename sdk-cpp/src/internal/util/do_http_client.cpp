#include "do_http_client.h"

#include <iostream>
#include <regex>
#include <thread>
#include <boost/asio/connect.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <gsl/gsl_util>

#include "do_exceptions_internal.h"
#include "do_exceptions.h"
#include "do_port_finder.h"

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
        const auto endpoints = resolver.resolve({ "127.0.0.1", std::to_string(port) });
        boost::system::error_code ec;
        boost::asio::connect(_socket, endpoints, ec);
        return ec;
    }

    std::pair<unsigned int, boost::property_tree::ptree> GetResponse(const char* method, const std::string& url)
    {
        std::stringstream request;
        request << method << ' ' << url << ' ' << "HTTP/1.1\r\n";
        request << "Host: 127.0.0.1\r\n";
        request << "User-Agent: DO-SDK-CPP\r\n";
        request << "\r\n";

        const auto req = request.str();
        std::cout << "Sending request:\n" << req << std::endl; // uncomment for debugging
        net::write(_socket, net::buffer(req.data(), req.size()));

        // Agent response is a deterministic, fairly small, size.
        // Read everything and then parse as HTTP message.
        std::vector<char> response;
        std::vector<char> readBuf(4 * 1024);
        size_t bytesRead = 0;
        enum class ParserState
        {
            StatusLine,
            Fields,
            Body,
            Complete
        };
        auto state = ParserState::StatusLine;
        size_t skip = 0;
        unsigned int statusCode = 0;
        size_t contentLength = 0;
        std::stringstream responseBodyStream;
        do
        {
            bytesRead = _socket.read_some(net::buffer(readBuf.data(), readBuf.size()));
            if ((response.size() + bytesRead) > 8192)
            {
                ThrowException(microsoft::deliveryoptimization::errc::unexpected);
            }
            response.insert(response.end(), readBuf.data(), readBuf.data() + bytesRead);

            switch (state)
            {
            case ParserState::StatusLine:
            {
                // Find \r\n
                auto it = std::find(response.begin(), response.end(), '\r');
                if (it != response.end())
                {
                    if ((it + 1) == response.end())
                    {
                        break; // need more data
                    }

                    if (*(it + 1) != '\n')
                    {
                        ThrowException(microsoft::deliveryoptimization::errc::unexpected);
                    }

                    std::string statusLine{response.begin(), it};
                    std::cout << "Status line: " << statusLine << std::endl;

                    std::regex rxStatusLine{"[hHtTpP/1\\.]+ (\\d+) [a-zA-Z0-9 ]+"};
                    std::cmatch matches;
                    if (!std::regex_match(statusLine.data(), matches, rxStatusLine))
                    {
                        ThrowException(microsoft::deliveryoptimization::errc::unexpected);
                    }

                    statusCode = static_cast<unsigned int>(std::strtoul(matches[1].str().data(), nullptr, 10));
                    std::cout << "Result: " << statusCode << std::endl;

                    state = ParserState::Fields;
                    skip = std::distance(response.begin(), it + 2);
                    // fallthrough
                }
                else
                {
                    break;
                }
            }

            case ParserState::Fields:
            {
                bool needMoreData = (response.size() <= skip);
                if (!needMoreData)
                {
                    // Find \r\n, look for Content-Length.
                    // Empty field indicates end of fields.
                    while (true)
                    {
                        auto itStart = response.begin() + skip;
                        auto itEnd = std::find(itStart, response.end(), '\r');
                        if (itEnd != response.end())
                        {
                            if ((itEnd + 1) == response.end())
                            {
                                needMoreData = true;
                                break; // need more data
                            }

                            if (*(itEnd + 1) != '\n')
                            {
                                ThrowException(microsoft::deliveryoptimization::errc::unexpected);
                            }

                            skip = std::distance(response.begin(), itEnd + 2);
                            if (itStart == itEnd)
                            {
                                state = ParserState::Body; // empty field == end of headers
                                break;
                            }

                            std::string field{itStart, itEnd};
                            std::cout << "Field: " << field << std::endl;
                            if (field.find("Content-Length") != std::string::npos)
                            {
                                std::regex rxContentLength{".*:[ ]*(\\d+).*"};
                                std::cmatch matches;
                                if (!std::regex_match(field.data(), matches, rxContentLength))
                                {
                                    ThrowException(microsoft::deliveryoptimization::errc::unexpected);
                                }

                                contentLength = static_cast<size_t>(std::strtoul(matches[1].str().data(), nullptr, 10));
                                std::cout << "Body size: " << contentLength << std::endl;
                            }
                        }
                        else
                        {
                            needMoreData = true;
                            break; // need more data
                        }
                    }
                }

                if (needMoreData)
                {
                    break;
                }
                // else fallthrough
                assert(state == ParserState::Body);
            }

            case ParserState::Body:
            {
                if (contentLength == 0)
                {
                    state = ParserState::Complete;
                    break;
                }
                const auto availableBodySize = gsl::narrow<size_t>(std::distance(response.begin() + skip, response.end()));
                if (availableBodySize == contentLength)
                {
                    responseBodyStream.write(&(*(response.begin() + skip)), contentLength);
                    std::cout << "Body: " << responseBodyStream.str() << std::endl;
                    state = ParserState::Complete;
                }
                // else need more data
                break;
            }

            case ParserState::Complete:
                break;
            }

        } while (state != ParserState::Complete);

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

        return {statusCode, responseBodyJson};
        // return {404, responseBodyJson};
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

boost::property_tree::ptree CHttpClient::SendRequest(Method method, const std::string& url, bool retry)
{
    auto responseStatusCode = 0u;
    boost::property_tree::ptree responseBodyJson;
    try
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto pClient = static_cast<CHttpClientImpl*>(_httpClientImpl.get());
        std::tie(responseStatusCode, responseBodyJson) = pClient->GetResponse(
            method == Method::GET ? "GET" : "POST", url);
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

} // namespace microsoft::deliveryoptimization::details
