#include "do_http_message.h"

#include <iostream>
#include <boost/asio/write.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "do_http_parser.h"

namespace net = boost::asio;        // from <boost/asio.hpp>

namespace microsoft::deliveryoptimization::details
{

HttpRequest::HttpRequest(Method method, const std::string& url) :
    _method(method),
    _url(url.data())
{
}

void HttpRequest::Serialize(boost::asio::ip::tcp::socket& socket) const
{
    std::stringstream request;
    const char* pVerb = (_method == Method::GET) ? "GET" : "POST";
    request << pVerb << ' ' << _url << ' ' << "HTTP/1.1\r\n";
    request << "Host: 127.0.0.1\r\n";
    request << "User-Agent: DO-SDK-CPP\r\n";
    request << "\r\n";

    const auto req = request.str();
    // std::cout << "Sending request:\n" << req << std::endl; // uncomment for debugging
    net::write(socket, net::buffer(req.data(), req.size()));
}

void HttpResponse::Deserialize(boost::asio::ip::tcp::socket& socket)
{
    HttpResponseParser parser{_statusCode, _contentLength, _body};
    std::vector<char> readBuf(1024);
    do
    {
        auto bytesRead = socket.read_some(net::buffer(readBuf.data(), readBuf.size()));
        parser.OnData(readBuf.data(), bytesRead);
    } while (!parser.Done());
}

boost::property_tree::ptree HttpResponse::ExtractJsonBody()
{
    boost::property_tree::ptree responseBodyJson;
    if (_body.rdbuf()->in_avail() > 0)
    {
        try
        {
            boost::property_tree::read_json(_body, responseBodyJson);
        }
        catch (...)
        {
        }
    }
    return responseBodyJson;
}

} // microsoft::deliveryoptimization::details
