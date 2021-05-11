#pragma once

#include <mutex>

#include <boost/property_tree/ptree.hpp>
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

    boost::property_tree::ptree SendRequest(const web::http::method& method, const utility::string_t& url, bool retry = true);

private:
    CHttpClient();
    void _InitializeDOConnection(bool launchClientFirst = false);

    mutable std::mutex _mutex;
    std::unique_ptr<web::http::client::http_client> _httpClient;
};
} // namespace microsoft::deliveryoptimization::details
