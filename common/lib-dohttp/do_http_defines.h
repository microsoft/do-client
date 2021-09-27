// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

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
    Created = 201,
    Accepted = 202,
    NonAuthInfo = 203,
    NoContent = 204,
    PartialContent = 206,

    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    UseProxy = 305,

    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    RequestEntityTooLarge = 413,
    RequestUriTooLarge = 414,
    UnsupportedMediaType = 415,

    InternalError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HttpVersionNotSupported = 505,
};

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft

#endif // _DELIVERY_OPTIMIZATION_DO_HTTP_DEFINES_H
