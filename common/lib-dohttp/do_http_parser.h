#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

// Very limited parsing abilities, just enough to support the SDK/Agent's requests and responses.
// Credit: Code takes a little inspiration from Boost.Beast. Too bad it is not available on Ubuntu 18.04.
class HttpParser
{
public:
    HttpParser();

    int OnData(const char* pData, size_t cb);

    bool Done() const noexcept
    {
        return (_state == ParserState::Complete);
    }

    const std::string& Method() const { return _method; }
    const std::string& Path() const { return _path; }
    unsigned int StatusCode() const { return _statusCode; }
    std::stringstream& Body() { return _body; }

private:
    bool _ParseBuf();
    bool _ParseNextField();
    std::vector<char>::iterator _FindCRLF(std::vector<char>::iterator itStart);

    // Parsing info
    enum class ParserState
    {
        FirstLine,
        Fields,
        Body,
        Complete
    };

    ParserState _state { ParserState::FirstLine };
    std::vector<char> _incomingDataBuf;
    std::vector<char>::iterator _itParseFrom;

    std::string _method;                // request
    std::string _path;                  // request or response
    unsigned int _statusCode { 0 };     // response
    size_t _contentLength { 0 };        // request or response
    std::stringstream _body;            // request or response
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
