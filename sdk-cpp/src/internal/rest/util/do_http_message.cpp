// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_http_message.h"

#ifdef DO_DEBUG_REST_INTERFACE
#include <iostream>
#endif
#include <boost/asio/write.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "do_http_defines.h"

namespace net = boost::asio;        // from <boost/asio.hpp>

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

HttpRequest::HttpRequest(Method method, const std::string& url) :
    _method(method),
    _url(url.data())
{
}

void HttpRequest::Serialize(boost::asio::ip::tcp::socket& socket) const
{
    std::stringstream request;
    const char* pVerb = (_method == Method::GET) ? http_methods::GET : http_methods::POST;
    request << pVerb << ' ' << _url << ' ' << "HTTP/1.1\r\n";
    request << "Host: 127.0.0.1\r\n";
    request << "User-Agent: DO-SDK-CPP\r\n";
    request << "\r\n";

    const auto req = request.str();
#ifdef DO_DEBUG_REST_INTERFACE
    std::cout << "Sending request:\n" << req << std::endl;
#endif
    net::write(socket, net::buffer(req.data(), req.size()));
}

void HttpResponse::Deserialize(boost::asio::ip::tcp::socket& socket)
{
    std::vector<char> readBuf(1024);
    do
    {
        auto bytesRead = socket.read_some(net::buffer(readBuf.data(), readBuf.size()));
        _parser.OnData(readBuf.data(), bytesRead);
    } while (!_parser.Done());
}

boost::property_tree::ptree HttpResponse::ExtractJsonBody()
{
    boost::property_tree::ptree responseBodyJson;
    if (_parser.Body().rdbuf()->in_avail() > 0)
    {
        try
        {
            boost::property_tree::read_json(_parser.Body(), responseBodyJson);
        }
        catch (...)
        {
        }
    }
    return responseBodyJson;
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft