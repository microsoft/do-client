#ifndef _DELIVERY_OPTIMIZATION_DO_HTTP_PARSER_H
#define _DELIVERY_OPTIMIZATION_DO_HTTP_PARSER_H

#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "do_http_packet.h"

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

    void OnData(const char* pData, size_t cb);
    void Reset();

    bool Done() const noexcept
    {
        return (_state == ParserState::Complete);
    }

    const std::string& Method() const { return _parsedData->method; }
    const cpprest_web::uri& Url() const { return _parsedData->url; }
    unsigned int StatusCode() const { return _parsedData->statusCode; }
    std::stringstream& Body() { return _parsedData->body; }
    const std::shared_ptr<HttpPacket>& ParsedData() const { return _parsedData; }

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

    std::shared_ptr<HttpPacket> _parsedData;
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
#endif
