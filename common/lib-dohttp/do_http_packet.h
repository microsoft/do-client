#ifndef _DELIVERY_OPTIMIZATION_DO_HTTP_PACKET_H
#define _DELIVERY_OPTIMIZATION_DO_HTTP_PACKET_H

#include <sstream>
#include <string>
#include "do_cpprest_uri.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

struct HttpPacket
{
    std::string method;             // request
    cpprest_web::uri url;           // request or response
    unsigned int statusCode { 0 };  // response
    size_t contentLength { 0 };     // request or response
    std::stringstream body;         // request or response
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_HTTP_PACKET_H
