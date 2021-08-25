#include "do_http_parser.h"

#include <iostream>
#include <regex>
#include <gsl/gsl_util>

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

HttpParser::HttpParser()
{
    Reset();
}

void HttpParser::OnData(const char* pData, size_t cb)
{
    if ((_incomingDataBuf.size() + cb) > _incomingDataBuf.capacity())
    {
        throw std::length_error("HttpParser receiving too much data");
    }
    _incomingDataBuf.insert(_incomingDataBuf.end(), pData, pData + cb);

    while (_ParseBuf())
    {
    }
}

void HttpParser::Reset()
{
    _incomingDataBuf.clear();
    _incomingDataBuf.reserve(2048); // Request/response is always small in size
    _state = ParserState::FirstLine;
    _parsedData = std::make_shared<HttpPacket>();
}

// Returns true if more processing can be done, false if processing is done or cannot continue until more data is received.
bool HttpParser::_ParseBuf()
{
    const auto oldState = _state;
    switch (_state)
    {
    case ParserState::FirstLine:
    {
        auto itCR = _FindCRLF(_incomingDataBuf.begin());
        if (itCR != _incomingDataBuf.end())
        {
            std::string firstLine{_incomingDataBuf.begin(), itCR};
            // std::cout << "Request/Status line: " << firstLine << std::endl;

            static const std::regex rxRequestLine{"([a-zA-Z]+) ([a-zA-Z0-9\\-_\\.!~\\*'\\(\\)%:@&=\\+$,/?]+) [hHtTpP/1\\.]+"};
            static const std::regex rxStatusLine{"[hHtTpP/1\\.]+ (\\d+) [a-zA-Z0-9 ]+"};
            std::cmatch matches;
            if (std::regex_match(firstLine.data(), matches, rxStatusLine))
            {
                _parsedData->statusCode = static_cast<unsigned int>(std::strtoul(matches[1].str().data(), nullptr, 10));
                std::cout << "Result: " << _parsedData->statusCode << std::endl;
            }
            else if (std::regex_match(firstLine.data(), matches, rxRequestLine))
            {
                _parsedData->method = matches[1].str();
                _parsedData->url = matches[2].str();
                std::cout << "Method: " << _parsedData->method << std::endl;
                std::cout << "Url: " << _parsedData->url << std::endl;
            }
            else
            {
                throw std::invalid_argument("HttpParser received malformed first line");
            }

            _state = ParserState::Fields;
            _itParseFrom = itCR + 2;
        }
        break;
    }

    case ParserState::Fields:
    {
        while (_ParseNextField())
        {
        }
        break;
    }

    case ParserState::Body:
    {
        if (_parsedData->contentLength == 0)
        {
            _state = ParserState::Complete;
        }
        else
        {
            const auto availableBodySize = gsl::narrow<size_t>(std::distance(_itParseFrom, _incomingDataBuf.end()));
            // Agent response is a JSON body, couple hundred bytes at max. Read everything at once.
            if (availableBodySize == _parsedData->contentLength)
            {
                _parsedData->body.write(&(*_itParseFrom), _parsedData->contentLength);
                std::cout << "Body: " << _parsedData->body.str() << std::endl;
                _state = ParserState::Complete;
                _itParseFrom = _incomingDataBuf.end();
            }
        }
        break;
    }

    case ParserState::Complete:
        break;
    }

    return (oldState != _state);
}

// Returns true if there are more fields to process, false otherwise
bool HttpParser::_ParseNextField()
{
    // Find \r\n, look for Content-Length.
    auto itCR = _FindCRLF(_itParseFrom);
    if (itCR == _incomingDataBuf.end())
    {
        return false; // need more data
    }

    if (_itParseFrom == itCR)
    {
        _state = ParserState::Body; // empty field == end of headers
        _itParseFrom = itCR + 2;
        return false;
    }

    std::string field{_itParseFrom, itCR};
    // std::cout << "Field: " << field << std::endl;
    if (field.find("Content-Length") != std::string::npos)
    {
        std::regex rxContentLength{".*:[ ]*(\\d+).*"};
        std::cmatch matches;
        if (!std::regex_match(field.data(), matches, rxContentLength))
        {
            throw std::invalid_argument("HttpParser received malformed Content-Length");
        }

        _parsedData->contentLength = static_cast<size_t>(std::strtoul(matches[1].str().data(), nullptr, 10));
        // std::cout << "Body size: " << _parsedData->contentLength << std::endl;
    }
    // else, field not interesting
    _itParseFrom = itCR + 2;
    return true;
}

std::vector<char>::iterator HttpParser::_FindCRLF(std::vector<char>::iterator itStart)
{
    auto itCR = std::find(itStart, _incomingDataBuf.end(), '\r');
    if (itCR == _incomingDataBuf.end() || (itCR + 1) == _incomingDataBuf.end())
    {
        return _incomingDataBuf.end(); // need more data
    }
    if (*(itCR + 1) != '\n')
    {
        throw std::invalid_argument("HttpParser received malformed message (CRLF)");
    }
    return itCR;
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
