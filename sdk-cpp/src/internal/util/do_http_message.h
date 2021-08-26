#pragma once

#include <string>
#include <vector>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/ptree.hpp>
#include "do_http_parser.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

// Credit: Code takes a little inspiration from Boost.Beast. Too bad it is not available on Ubuntu 18.04.
class HttpRequest
{
public:
    enum Method
    {
        GET,
        POST
    };

    HttpRequest(Method method, const std::string& url);
    void Serialize(boost::asio::ip::tcp::socket& socket) const;

private:
    Method _method;
    const char* _url;
};

class HttpResponse
{
public:
    void Deserialize(boost::asio::ip::tcp::socket& socket);

    unsigned int StatusCode() const { return _parser.StatusCode(); }
    boost::property_tree::ptree ExtractJsonBody();

private:
    HttpParser _parser;
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
