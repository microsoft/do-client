#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace microsoft::deliveryoptimization::details
{

class HttpResponseParser
{
public:
    HttpResponseParser(unsigned int& statusCodeBuf, size_t& contentLenBuf, std::stringstream& bodyBuf);

    void OnData(const char* pData, size_t cb);

    bool Done() const noexcept
    {
        return (_state == ParserState::Complete);
    }

private:
    bool _ParseBuf();
    bool _ParseNextField();
    std::vector<char>::iterator _FindCRLF(std::vector<char>::iterator itStart);

    // Parsing info
    enum class ParserState
    {
        StatusLine,
        Fields,
        Body,
        Complete
    };

    ParserState _state { ParserState::StatusLine };
    std::vector<char> _responseBuf;
    std::vector<char>::iterator _itParseFrom;

    unsigned int& _statusCode;
    size_t& _contentLength;
    std::stringstream& _body;
};

} // microsoft::deliveryoptimization::details
