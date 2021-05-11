#pragma once

#include <mutex>

#include <cpprest/details/basic_types.h>
#include <cpprest/http_msg.h>

#include "do_noncopyable.h"

namespace web::http::client
{
class http_client;
}

extern const utility::string_t g_downloadUriPart;

namespace microsoft::deliveryoptimization::details
{
class CHttpClient : CDONoncopyable
{
public:
    static CHttpClient& GetInstance();

    static void HTTPErrorCheck(const web::http::http_response& resp);
    web::http::http_response SendRequest(const web::http::method& method, const utility::string_t& builderAsString, bool retry = true);

private:
    CHttpClient();
    void _InitializeDOConnection(bool launchClientFirst = false);

    mutable std::mutex _mutex;
    std::unique_ptr<web::http::client::http_client> _httpClient;
};
} // namespace microsoft::deliveryoptimization::details
