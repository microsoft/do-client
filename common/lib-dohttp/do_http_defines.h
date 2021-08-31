#ifndef _DELIVERY_OPTIMIZATION_DO_HTTP_DEFINES_H
#define _DELIVERY_OPTIMIZATION_DO_HTTP_DEFINES_H

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

class http_methods
{
public:
    static const char* const GET;
    static const char* const POST;
};

enum http_status_codes
{
    OK = 200,
    BadRequest = 400,
    NotFound = 404,
    InternalError = 500,
    ServiceUnavailable = 503,
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_HTTP_DEFINES_H
