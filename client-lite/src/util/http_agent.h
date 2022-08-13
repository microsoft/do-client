// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <curl/curl.h>
#include "http_agent_interface.h"

#define DO_HTTP_RANGEREQUEST_STR_LEN    48              // two 64bit numbers plus a '-' character (20 digits in UINT64)

class CurlRequests;

class HttpAgent : public IHttpAgent
{
public:
    HttpAgent(CurlRequests& curlOps, IHttpAgentEvents& callback);
    ~HttpAgent();

    static bool IsClientError(UINT httpStatusCode);
    static std::array<char, DO_HTTP_RANGEREQUEST_STR_LEN> MakeRange(UINT64 startOffset, UINT64 lengthBytes);
    static bool ValidateUrl(const std::string& url);

    // IHttpAgent

    HRESULT SendRequest(PCSTR szUrl = nullptr, PCSTR szProxyUrl = nullptr, PCSTR szRange = nullptr, UINT connectTimeoutSecs = 0) override;
    void Close() override;

    // The Query* functions are supposed to be called only from within the IHttpAgentEvents callbacks
    // function because the httpContext (which is the request handle) must be valid.
    HRESULT QueryStatusCode(_Out_ UINT* pStatusCode) const override;
    HRESULT QueryContentLength(_Out_ UINT64* pContentLength) override;
    HRESULT QueryContentLengthFromRange(_Out_ UINT64* pContentLength) override;
    HRESULT QueryHeaders(_In_opt_z_ PCSTR pszName, std::string& headers) const noexcept override;
    HRESULT QueryHeadersByType(HttpAgentHeaders type, std::string& headers) noexcept override;

private:
    mutable std::recursive_mutex _requestLock;

    CurlRequests& _curlOps;
    IHttpAgentEvents& _callback;
    UINT64 _callbackContext { 0 };

    // Current usage pattern is to create only one request at a time.
    // Holding a single request context is sufficient.
    struct RequestContext
    {
        CURL* curlHandle;
        struct curl_slist* requestHeaders;

        unsigned int responseStatusCode;
        HRESULT hrTranslatedStatusCode;
        std::unordered_map<std::string, std::string> responseHeaders;
        bool responseOnHeadersAvailableInvoked;
        bool responseOnCompleteInvoked;

        ~RequestContext()
        {
            curl_slist_free_all(requestHeaders);
        }
    };

    RequestContext _requestContext {};

private:
    HRESULT _CreateClient(PCSTR szUrl = nullptr, PCSTR szProxyUrl = nullptr, UINT connectTimeoutSecs = 0);
    static HRESULT _ResultFromStatusCode(unsigned int code);
    void _SetWebProxyFromProxyUrl(_In_opt_ PCSTR szProxyUrl);

    size_t _HeaderCallback(char* pBuffer, size_t size, size_t nItems);
    size_t _WriteCallback(char* pBuffer, size_t size, size_t nMemb);
    void _CompleteCallback(int curlResult);
    void _TrySetStatusCodeAndInvokeOnHeadersAvailable();

    // libcurl callbacks
    static size_t s_HeaderCallback(char* pBuffer, size_t size, size_t nItems, void* pUserData)
    {
        return reinterpret_cast<HttpAgent*>(pUserData)->_HeaderCallback(pBuffer, size, nItems);
    }

    static size_t s_WriteCallback(char* pBuffer, size_t size, size_t nMemb, void* pUserData)
    {
        return reinterpret_cast<HttpAgent*>(pUserData)->_WriteCallback(pBuffer, size, nMemb);
    }

    // CurlRequests callback
    static void s_CompleteCallback(int curlResult, void* pUserData)
    {
        return reinterpret_cast<HttpAgent*>(pUserData)->_CompleteCallback(curlResult);
    }
};
