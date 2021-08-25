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
    // Request/response is always small in size
    _incomingDataBuf.reserve(2048);
}

int HttpParser::OnData(const char* pData, size_t cb)
{
    if ((_incomingDataBuf.size() + cb) > _incomingDataBuf.capacity())
    {
        // ThrowException(microsoft::deliveryoptimization::errc::unexpected); TODO
        return -1;
    }
    _incomingDataBuf.insert(_incomingDataBuf.end(), pData, pData + cb);

    while (_ParseBuf())
    {
    }

    return 0;
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
            std::cout << "Request/Status line: " << firstLine << std::endl;

            static const std::regex rxRequestLine{"([a-zA-Z]+) ([a-zA-Z0-9\\-_\\.!~\\*'\\(\\)%:@&=\\+$,]+) [hHtTpP/1\\.]+"};
            static const std::regex rxStatusLine{"[hHtTpP/1\\.]+ (\\d+) [a-zA-Z0-9 ]+"};
            std::cmatch matches;
            if (std::regex_match(firstLine.data(), matches, rxStatusLine))
            {
                _statusCode = static_cast<unsigned int>(std::strtoul(matches[1].str().data(), nullptr, 10));
                // std::cout << "Result: " << _statusCode << std::endl;
            }
            else if (std::regex_match(firstLine.data(), matches, rxRequestLine))
            {
                _method = matches[1].str();
                _path = matches[2].str();
                std::cout << "Method: " << _method << std::endl;
                std::cout << "Path: " << _path << std::endl;
            }
            else
            {
                // ThrowException(microsoft::deliveryoptimization::errc::unexpected); TODO
                throw std::exception();
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
        if (_contentLength == 0)
        {
            _state = ParserState::Complete;
        }
        else
        {
            const auto availableBodySize = gsl::narrow<size_t>(std::distance(_itParseFrom, _incomingDataBuf.end()));
            // Agent response is a JSON body, couple hundred bytes at max. Read everything at once.
            if (availableBodySize == _contentLength)
            {
                _body.write(&(*_itParseFrom), _contentLength);
                // std::cout << "Body: " << _body.str() << std::endl;
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
            // ThrowException(microsoft::deliveryoptimization::errc::unexpected); TODO
            throw std::exception();
        }

        _contentLength = static_cast<size_t>(std::strtoul(matches[1].str().data(), nullptr, 10));
        // std::cout << "Body size: " << _contentLength << std::endl;
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
        // ThrowException(microsoft::deliveryoptimization::errc::unexpected);
        throw std::exception();
    }
    return itCR;
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
