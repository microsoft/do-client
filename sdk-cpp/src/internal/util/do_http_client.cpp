
#include "do_http_client.h"

#include <cpprest/details/basic_types.h>
#include <cpprest/filestream.h>
#include <cpprest/http_client.h>

#include "do_exceptions_internal.h"
#include "do_exceptions.h"
#include "do_port_finder.h"

const utility::string_t g_downloadUriPart(U("/download"));

namespace microsoft::deliveryoptimization::details
{

CHttpClient& CHttpClient::GetInstance()
{
    static CHttpClient myInstance;
    return myInstance;
}

void CHttpClient::_InitializeDOConnection(bool launchClientFirst)
{
    std::unique_lock<std::mutex> lock(_mutex);
    const auto port = CPortFinder::GetDOPort(launchClientFirst);
    const auto url = "http://127.0.0.1:" + port + "/";
    _httpClient = std::make_unique<web::http::client::http_client>(url);
}

void CHttpClient::HTTPErrorCheck(const web::http::http_response& resp)
{
    if (resp.status_code() != 200)
    {
        web::json::object respBody = resp.extract_json().get().as_object();

        int32_t ErrorCode = respBody[U("ErrorCode")].as_integer();

        ThrowException(ErrorCode);
    }
}

web::http::http_response CHttpClient::SendRequest(const web::http::method& method, const utility::string_t& builderAsString, bool retry)
{
    try
    {
        return _httpClient->request(method, builderAsString).get();
    }
    catch (const web::http::http_exception& e)
    {
        if (retry)
        {
            _InitializeDOConnection(true);
            return SendRequest(method, builderAsString, false);
        }

        ThrowException(e.error_code());
    }

    // Control flow should never get here.
    // Adding empty return to make the compiler happy.
    return {};
}

web::http::http_response CHttpClient::SendRequest(const web::http::method& method, const utility::string_t& builderAsString,
    const web::json::value& body, bool retry)
{
    try
    {
        return _httpClient->request(method, builderAsString, body).get();
    }
    catch (const web::http::http_exception& e)
    {
        if (retry)
        {
            _InitializeDOConnection(true);
            return SendRequest(method, builderAsString, body, false);
        }

        ThrowException(e.error_code());
    }

    // Control flow should never get here.
    // Adding empty return to make the compiler happy.
    return {};
}

CHttpClient::CHttpClient()
{
    _InitializeDOConnection();
}

} // namespace microsoft::deliveryoptimization::details
