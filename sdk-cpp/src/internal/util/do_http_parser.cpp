#include "do_http_parser.h"

#include <iostream>
#include <regex>
#include <gsl/gsl_util>
#include "do_exceptions.h"
#include "do_exceptions_internal.h"

namespace microsoft::deliveryoptimization::details
{

HttpResponseParser::HttpResponseParser(unsigned int& statusCodeBuf, size_t& contentLenBuf, std::stringstream& bodyBuf) :
    _statusCode(statusCodeBuf),
    _contentLength(contentLenBuf),
    _body(bodyBuf)
{
    // Agent response is always with a JSON body, small in size.
    _responseBuf.reserve(2048);
}

void HttpResponseParser::OnData(const char* pData, size_t cb)
{
    if ((_responseBuf.size() + cb) > _responseBuf.capacity())
    {
        ThrowException(microsoft::deliveryoptimization::errc::unexpected);
    }
    _responseBuf.insert(_responseBuf.end(), pData, pData + cb);

    while (_ParseBuf())
    {
    }
}

// Returns true if more processing can be done, false if processing is done or cannot continue until more data is received
bool HttpResponseParser::_ParseBuf()
{
    const auto oldState = _state;
    switch (_state)
    {
    case ParserState::StatusLine:
    {
        auto itCR = _FindCRLF(_responseBuf.begin());
        if (itCR != _responseBuf.end())
        {
            std::string statusLine{_responseBuf.begin(), itCR};
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
            const auto availableBodySize = gsl::narrow<size_t>(std::distance(_itParseFrom, _responseBuf.end()));
            // Agent response is a JSON body, couple hundred bytes at max. Read everything at once.
            if (availableBodySize == _contentLength)
            {
                _body.write(&(*_itParseFrom), _contentLength);
                std::cout << "Body: " << _body.str() << std::endl;
                _state = ParserState::Complete;
                _itParseFrom = _responseBuf.end();
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
bool HttpResponseParser::_ParseNextField()
{
    // Find \r\n, look for Content-Length.
    auto itCR = _FindCRLF(_itParseFrom);
    if (itCR == _responseBuf.end())
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
    // else, field not interesting
    _itParseFrom = itCR + 2;
    return true;
}

std::vector<char>::iterator HttpResponseParser::_FindCRLF(std::vector<char>::iterator itStart)
{
    auto itCR = std::find(itStart, _responseBuf.end(), '\r');
    if (itCR == _responseBuf.end() || (itCR + 1) == _responseBuf.end())
    {
        return _responseBuf.end(); // need more data
    }
    if (*(itCR + 1) != '\n')
    {
        ThrowException(microsoft::deliveryoptimization::errc::unexpected);
    }
    return itCR;
}

} // microsoft::deliveryoptimization::details
