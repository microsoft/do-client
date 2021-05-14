#include "do_http_message.h"

#include <iostream>
#include <regex>
#include <boost/asio/write.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <gsl/gsl_util>
#include "do_exceptions.h"
#include "do_exceptions_internal.h"

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
    std::cout << "Sending request:\n" << req << std::endl; // uncomment for debugging
    net::write(socket, net::buffer(req.data(), req.size()));
}

HttpResponse::HttpResponse()
{
    _responseBuf.reserve(2048);
}

void HttpResponse::Deserialize(boost::asio::ip::tcp::socket& socket)
{
    // Agent response is always with a JSON body, couple hundred bytes at max.
    std::vector<char> readBuf(1024);
    do
    {
        auto bytesRead = socket.read_some(net::buffer(readBuf.data(), readBuf.size()));
        if ((_responseBuf.size() + bytesRead) > _responseBuf.capacity())
        {
            ThrowException(microsoft::deliveryoptimization::errc::unexpected);
        }
        _responseBuf.insert(_responseBuf.end(), readBuf.data(), readBuf.data() + bytesRead);

        switch (_state)
        {
        case ParserState::StatusLine:
        {
            // Find \r\n
            auto it = std::find(_responseBuf.begin(), _responseBuf.end(), '\r');
            if (it != _responseBuf.end())
            {
                if ((it + 1) == _responseBuf.end())
                {
                    break; // need more data
                }

                if (*(it + 1) != '\n')
                {
                    ThrowException(microsoft::deliveryoptimization::errc::unexpected);
                }

                std::string statusLine{_responseBuf.begin(), it};
                std::cout << "Status line: " << statusLine << std::endl;

                std::regex rxStatusLine{"[hHtTpP/1\\.]+ (\\d+) [a-zA-Z0-9 ]+"};
                std::cmatch matches;
                if (!std::regex_match(statusLine.data(), matches, rxStatusLine))
                {
                    ThrowException(microsoft::deliveryoptimization::errc::unexpected);
                }

                _statusCode = static_cast<unsigned int>(std::strtoul(matches[1].str().data(), nullptr, 10));
                std::cout << "Result: " << _statusCode << std::endl;

                _state = ParserState::Fields;
                _skip = std::distance(_responseBuf.begin(), it + 2);
                // fallthrough
            }
            else
            {
                break;
            }
        }

        case ParserState::Fields:
        {
            bool needMoreData = (_responseBuf.size() <= _skip);
            if (!needMoreData)
            {
                // Find \r\n, look for Content-Length.
                // Empty field indicates end of fields.
                while (true)
                {
                    auto itStart = _responseBuf.begin() + _skip;
                    auto itEnd = std::find(itStart, _responseBuf.end(), '\r');
                    if (itEnd != _responseBuf.end())
                    {
                        if ((itEnd + 1) == _responseBuf.end())
                        {
                            needMoreData = true;
                            break; // need more data
                        }

                        if (*(itEnd + 1) != '\n')
                        {
                            ThrowException(microsoft::deliveryoptimization::errc::unexpected);
                        }

                        _skip = std::distance(_responseBuf.begin(), itEnd + 2);
                        if (itStart == itEnd)
                        {
                            _state = ParserState::Body; // empty field == end of headers
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

                            _contentLength = static_cast<size_t>(std::strtoul(matches[1].str().data(), nullptr, 10));
                            std::cout << "Body size: " << _contentLength << std::endl;
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
            assert(_state == ParserState::Body);
        }

        case ParserState::Body:
        {
            if (_contentLength == 0)
            {
                _state = ParserState::Complete;
                break;
            }
            const auto availableBodySize = gsl::narrow<size_t>(std::distance(_responseBuf.begin() + _skip, _responseBuf.end()));
            if (availableBodySize == _contentLength)
            {
                _body.write(&(*(_responseBuf.begin() + _skip)), _contentLength);
                std::cout << "Body: " << _body.str() << std::endl;
                _state = ParserState::Complete;
            }
            // else need more data
            break;
        }

        case ParserState::Complete:
            break;
        }

    } while (_state != ParserState::Complete);
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
