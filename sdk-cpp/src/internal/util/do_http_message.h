#pragma once

#include <string>
#include <vector>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/ptree.hpp>

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

    unsigned int StatusCode() const { return _statusCode; }
    boost::property_tree::ptree ExtractJsonBody();

private:
    unsigned int _statusCode { 0 };
    size_t _contentLength { 0 };
    std::stringstream _body;
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
