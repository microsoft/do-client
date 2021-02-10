#pragma once

#include <memory>
#include <cpprest/http_msg.h>
#include <cpprest/rawptrstream.h>
#include <pplx/pplxtasks.h>
#include "do_event.h"
#include "http_agent_interface.h"
#include "waitable_counter.h"

#define DO_HTTP_REQ_BUF_SIZE            (128 * 1024)    // default request buffer size per request
#define DO_HTTP_RANGEREQUEST_STR_LEN    48              // two 64bit numbers plus a '-' character (20 digits in UINT64)

namespace web::http::client
{
class http_client;
class http_client_config;
}

class HttpAgent : public IHttpAgent
{
public:
    HttpAgent(IHttpAgentEvents& callback);
    ~HttpAgent();

    static bool IsClientError(UINT httpStatusCode);
    static std::array<char, DO_HTTP_RANGEREQUEST_STR_LEN> MakeRange(UINT64 startOffset, UINT64 lengthBytes);
    static bool ValidateUrl(const std::string& url);

    // IHttpAgent

    HRESULT SendRequest(PCSTR szUrl = nullptr, PCSTR szProxyUrl = nullptr, PCSTR szPostData = nullptr, PCSTR szRange = nullptr,
        UINT64 callerContext = 0) override;
    void Close() override;

    // The Query* functions are supposed to be called only from within the IHttpAgentEvents callbacks
    // function because the httpContext (which is the request handle) must be valid.
    HRESULT QueryStatusCode(UINT64 httpContext, _Out_ UINT* pStatusCode) const override;
    HRESULT QueryContentLength(UINT64 httpContext, _Out_ UINT64* pContentLength) override;
    HRESULT QueryContentLengthFromRange(UINT64 httpContext, _Out_ UINT64* pContentLength) override;
    HRESULT QueryHeaders(UINT64 httpContext, _In_opt_z_ PCSTR pszName, std::string& headers) const noexcept override;
    HRESULT QueryHeadersByType(UINT64 httpContext, HttpAgentHeaders type, std::string& headers) noexcept override;

private:
    struct ReadDataBuffer
    {
        std::vector<BYTE> storage {};
        Concurrency::streams::rawptr_buffer<BYTE> streambuf;

        ReadDataBuffer() :
            storage(DO_HTTP_REQ_BUF_SIZE),
            streambuf(storage.data(), storage.size())
        {
        }
    };

    std::unique_ptr<web::http::client::http_client> _client;
    pplx::cancellation_token_source _cts;
    mutable std::recursive_mutex _requestLock;

    IHttpAgentEvents& _callback;
    WaitableCounter _callTracker;

private:
    HRESULT _CreateClient(PCSTR szUrl = nullptr, PCSTR szProxyUrl = nullptr);
    static web::http::http_request _CreateRequest(PCSTR szPostData, PCSTR szRange);
    pplx::task<void> _SubmitRequestTask(const web::http::http_request& request, UINT64 callerContext);
    pplx::task<void> _DoReadBodyData(Concurrency::streams::istream bodyStream, const std::shared_ptr<ReadDataBuffer>& bodyStorage,
        pplx::cancellation_token cancellationToken, const std::shared_ptr<web::http::http_response>& response, UINT64 callerContext);
    static HRESULT _ResultFromStatusCode(web::http::status_code code);
    static void _SetWebProxyFromProxyUrl(web::http::client::http_client_config& config, _In_opt_ PCSTR szProxyUrl);
};
