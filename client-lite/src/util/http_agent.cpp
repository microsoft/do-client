#include "do_common.h"
#include "http_agent.h"

#include <sstream>
#include <boost/algorithm/string.hpp>
#include "do_cpprest_uri_builder.h"
#include "do_cpprest_uri.h"
#include "do_curl_multi_operation.h"
#include "do_http_defines.h"
#include "safe_int.h"

// TBD version
#define DO_USER_AGENT_STR   "Microsoft-Delivery-Optimization-Lite/10.0.0.1"

namespace msdod = microsoft::deliveryoptimization::details;

HttpAgent::HttpAgent(CurlMultiOperation& curlOps, IHttpAgentEvents& callback) :
    _curlOps(curlOps),
    _callback(callback)
{
}

HttpAgent::~HttpAgent()
{
    Close();
    curl_easy_cleanup(_requestContext.curlHandle);
}

// Determine if the status code is a 4xx code
bool HttpAgent::IsClientError(UINT httpStatusCode)
{
    return (400 <= httpStatusCode) && (httpStatusCode < 500);
}

std::array<char, DO_HTTP_RANGEREQUEST_STR_LEN> HttpAgent::MakeRange(UINT64 startOffset, UINT64 lengthBytes)
{
    std::array<char, DO_HTTP_RANGEREQUEST_STR_LEN> range;
    const auto endOffset = UInt64Sub(UInt64Add(startOffset, lengthBytes), 1);
    (void)StringPrintf(range.data(), range.size(), "%llu-%llu", startOffset, endOffset);
    return range;
}

bool HttpAgent::ValidateUrl(const std::string& url)
{
    if (!msdod::cpprest_web::uri::validate(url))
    {
        return false;
    }

    msdod::cpprest_web::uri uri{url};
    if ((StringCompareCaseInsensitive(uri.scheme().data(), "http") != 0)
        && (StringCompareCaseInsensitive(uri.scheme().data(), "https") != 0))
    {
        return false;
    }
    if (uri.host().empty())
    {
        return false;
    }

    return true;
}

// IHttpAgent
HRESULT HttpAgent::SendRequest(PCSTR szUrl, PCSTR szProxyUrl, PCSTR szRange) try
{
    RETURN_IF_FAILED(_CreateClient(szUrl, szProxyUrl));
    DO_ASSERT(_requestContext.curlHandle);
    if (szRange == nullptr)
    {
        curl_slist_free_all(_requestContext.requestHeaders);
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_HTTPHEADER, nullptr);
    }
    else
    {
        std::string rangeHeader("Range: bytes=");
        rangeHeader += szRange;
        auto tempList = curl_slist_append(_requestContext.requestHeaders, rangeHeader.c_str());
        RETURN_HR_IF(E_OUTOFMEMORY, tempList == nullptr);
        _requestContext.requestHeaders = tempList;
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_HTTPHEADER, _requestContext.requestHeaders);
    }

    _requestContext.responseHeaders.clear();
    _requestContext.responseStatusCode = 0;
    _requestContext.hrTranslatedStatusCode = S_OK;
    _requestContext.responseOnHeadersAvailableInvoked = false;
    _requestContext.responseOnCompleteInvoked = false;
    _curlOps.AddHandle(_requestContext.curlHandle, s_CompleteCallback, this);
    return S_OK;
} CATCH_RETURN()

void HttpAgent::Close()
{
    if (_requestContext.curlHandle)
    {
        _curlOps.RemoveHandle(_requestContext.curlHandle);
    }
    // Clients may now make new requests if they choose
}

// The Query* functions are supposed to be called only from within the IHttpAgentEvents callbacks
// function to get back valid data.
HRESULT HttpAgent::QueryStatusCode(_Out_ UINT* pStatusCode) const
{
    *pStatusCode = static_cast<UINT>(_requestContext.responseStatusCode);
    return S_OK;
}

HRESULT HttpAgent::QueryContentLength(_Out_ UINT64* pContentLength)
{
    *pContentLength = 0;

    std::string lengthHeader;
    if (SUCCEEDED(QueryHeaders("Content-Length", lengthHeader)))
    {
        *pContentLength = std::stoull(lengthHeader);
    }
    return S_OK;
}

HRESULT HttpAgent::QueryContentLengthFromRange(_Out_ UINT64* pContentLength) try
{
    *pContentLength = 0;

    std::string rangeHeader;
    RETURN_IF_FAILED_EXPECTED(QueryHeadersByType(HttpAgentHeaders::Range, rangeHeader));

    // attempt to extract the content length from the content range format:
    // Content-Range: bytes <start>-<end>/<length>
    auto marker = strchr(rangeHeader.data(), L'/');
    RETURN_HR_IF_EXPECTED(E_NOT_SET, (marker == nullptr));
    *pContentLength = std::stoull(marker + 1);
    return S_OK;
} CATCH_RETURN()

HRESULT HttpAgent::QueryHeaders(PCSTR pszName, std::string& headers) const noexcept
{
    headers.clear();
    const auto& reponseHeaders = _requestContext.responseHeaders;
    if (pszName == nullptr)
    {
        // Accumulate all headers into the output string
        std::stringstream ss;
        if (!reponseHeaders.empty())
        {
            for (const auto& item : reponseHeaders)
            {
                ss << item.first << ':' << item.second;
                if (!boost::algorithm::ends_with(item.second, "\r\n"))
                {
                    ss << "\r\n";
                }
            }
            ss << "\r\n";
        }
        headers = ss.str();
    }
    else
    {
        auto it = reponseHeaders.find(pszName);
        RETURN_HR_IF_EXPECTED(E_NOT_SET, (it == reponseHeaders.end()));
        headers = it->second;
    }
    return S_OK;
}

HRESULT HttpAgent::QueryHeadersByType(HttpAgentHeaders type, std::string& headers) noexcept
{
    PCSTR headerName;
    switch (type)
    {
    case HttpAgentHeaders::Range:
        headerName = "Content-Range";
        break;
    default:
        return E_UNEXPECTED;
    }
    return QueryHeaders(headerName, headers);
}

HRESULT HttpAgent::_CreateClient(PCSTR szUrl, PCSTR szProxyUrl) try
{
    std::unique_lock<std::recursive_mutex> lock(_requestLock);

    if (!_requestContext.curlHandle)
    {
        RETURN_HR_IF(E_INVALIDARG, szUrl == nullptr);

        std::string url(szUrl);
        RETURN_HR_IF(INET_E_INVALID_URL, !ValidateUrl(url));

        _requestContext.curlHandle = curl_easy_init();
        RETURN_HR_IF(E_OUTOFMEMORY, _requestContext.curlHandle == nullptr);
        DoLogVerbose("New http_client for %s", szUrl);

        // Set options that are generic to all requests
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_FOLLOWLOCATION, static_cast<long>(1));
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_MAXREDIRS, static_cast<long>(10));
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_SUPPRESS_CONNECT_HEADERS, static_cast<long>(1));
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_HTTPGET, static_cast<long>(1));
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_USERAGENT, DO_USER_AGENT_STR);

        // Timeout request if download speed is less than 4KB/s for 20s (7KB/s or 56kb/s is dial-up modem speed)
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_LOW_SPEED_TIME, static_cast<long>(20));
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_LOW_SPEED_LIMIT, static_cast<long>(4000));

        // Set up callbacks
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_HEADERFUNCTION, s_HeaderCallback);
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_HEADERDATA, this);
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_WRITEFUNCTION, s_WriteCallback);
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_WRITEDATA, this);
    }

    if (szUrl != nullptr)
    {
        std::string url(szUrl);
        RETURN_HR_IF(INET_E_INVALID_URL, !ValidateUrl(url));
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_URL, szUrl);
    }

    _SetWebProxyFromProxyUrl(szProxyUrl);

    return S_OK;
} CATCH_RETURN()

HRESULT HttpAgent::_ResultFromStatusCode(unsigned int code)
{
    using status = msdod::http_status_codes;

    HRESULT hr = HTTP_E_STATUS_UNEXPECTED;
    switch (code)
    {
    case status::OK:
    case status::Created:
    case status::Accepted:
    case status::NoContent:
    case status::NonAuthInfo:
    case status::PartialContent:
        hr = S_OK;
        break;

    case status::MultipleChoices:
    case status::InternalError:
    case status::ServiceUnavailable:
        hr = HTTP_E_STATUS_SERVER_ERROR;
        break;

    case status::MovedPermanently:
    case status::Found:
    case status::SeeOther:
        hr = HTTP_E_STATUS_UNEXPECTED_REDIRECTION;
        break;

    case status::NotFound:
    case status::Gone:
        hr = HTTP_E_STATUS_NOT_FOUND;
        break;

    case status::UseProxy:
    case status::BadGateway:
    case status::GatewayTimeout:
        hr = HTTP_E_STATUS_BAD_GATEWAY;
        break;

    case status::BadRequest:
    case status::LengthRequired:
    case status::PreconditionFailed:
    case status::RequestUriTooLarge:
    case status::UnsupportedMediaType:
    case status::MethodNotAllowed:
    case status::Conflict:
        hr = HTTP_E_STATUS_BAD_REQUEST;
        break;

    case status::Unauthorized:
        hr = HTTP_E_STATUS_DENIED;
        break;

    case status::NotAcceptable:
        hr = HTTP_E_STATUS_NONE_ACCEPTABLE;
        break;

    case status::Forbidden:
        hr = HTTP_E_STATUS_FORBIDDEN;
        break;

    case status::ProxyAuthRequired:
        hr = HTTP_E_STATUS_PROXY_AUTH_REQ;
        break;

    case status::RequestTimeout:
        hr = HTTP_E_STATUS_REQUEST_TIMEOUT;
        break;

    case status::RequestEntityTooLarge:
        hr = HTTP_E_STATUS_REQUEST_TOO_LARGE;
        break;

    case status::NotImplemented:
    case status::HttpVersionNotSupported:
        hr = HTTP_E_STATUS_NOT_SUPPORTED;
        break;
    }
    return hr;
}

void HttpAgent::_SetWebProxyFromProxyUrl(_In_opt_ PCSTR szProxyUrl)
{
    if (szProxyUrl == nullptr)
    {
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_PROXY, "");
    }
    else
    {
        msdod::cpprest_web::uri_builder proxyFullAddress(szProxyUrl);
        const int port = proxyFullAddress.port();
        proxyFullAddress.set_port(-1); // easier to specify via CURLOPT_PROXYPORT
        curl_easy_setopt(_requestContext.curlHandle, CURLOPT_PROXY, proxyFullAddress.to_string().c_str());
        if (port != -1)
        {
            curl_easy_setopt(_requestContext.curlHandle, CURLOPT_PROXYPORT, static_cast<long>(port));
        }
        proxyFullAddress.set_user_info(""); // do not log credentials
        DoLogInfo("Using proxy %s:[%d]", proxyFullAddress.to_string().c_str(), port);
    }
}

size_t HttpAgent::_HeaderCallback(char* pBuffer, size_t size, size_t nItems)
{
    const auto cbBuffer = nItems * size;
    try
    {
        auto pColon = reinterpret_cast<char*>(memchr(pBuffer, ':', cbBuffer));
        if (pColon > pBuffer)
        {
            auto pBufferPastEnd = pBuffer + cbBuffer;
            auto result = _requestContext.responseHeaders.emplace(std::piecewise_construct,
                std::forward_as_tuple(pBuffer, pColon),
                std::forward_as_tuple(pColon + 1, pBufferPastEnd));
            if (!result.second)
            {
                // Duplicate header (likely following redirects), take the new one
                result.first->second.assign(pColon + 1, pBufferPastEnd);
            }
        }
    }
    catch (const std::exception& e)
    {
        DoLogWarning("Caught exception: %s", e.what());
    }
    return cbBuffer;
}

size_t HttpAgent::_WriteCallback(char* pBuffer, size_t size, size_t nMemb)
{
    _TrySetStatusCodeAndInvokeOnHeadersAvailable();

    const auto cbBuffer = nMemb * size;
    // Forward body only for success response
    if (SUCCEEDED(_requestContext.hrTranslatedStatusCode))
    {
        (void)_callback.OnData(reinterpret_cast<BYTE*>(pBuffer), cbBuffer);
    }
    return cbBuffer;
}

void HttpAgent::_CompleteCallback(int curlResult)
{
    if (_requestContext.responseOnCompleteInvoked)
    {
        return;
    }
    _requestContext.responseOnCompleteInvoked = true;

    if (curlResult == CURLE_OK)
    {
        _TrySetStatusCodeAndInvokeOnHeadersAvailable();

        _requestContext.hrTranslatedStatusCode = _ResultFromStatusCode(_requestContext.responseStatusCode);
        (void)_callback.OnComplete(_requestContext.hrTranslatedStatusCode);
    }
    else
    {
        _requestContext.hrTranslatedStatusCode = HRESULT_FROM_XPLAT_SYSERR(curlResult);
        (void)_callback.OnComplete(_requestContext.hrTranslatedStatusCode);
    }
}

void HttpAgent::_TrySetStatusCodeAndInvokeOnHeadersAvailable()
{
    if (_requestContext.responseOnHeadersAvailableInvoked)
    {
        return;
    }
    _requestContext.responseOnHeadersAvailableInvoked = true;

    long responseCode;
    auto res = curl_easy_getinfo(_requestContext.curlHandle, CURLINFO_RESPONSE_CODE, &responseCode);
    if (res == CURLE_OK)
    {
        _requestContext.responseStatusCode = static_cast<unsigned int>(responseCode);
    }

    _requestContext.hrTranslatedStatusCode = _ResultFromStatusCode(_requestContext.responseStatusCode);
    if (SUCCEEDED(_requestContext.hrTranslatedStatusCode))
    {
        (void)_callback.OnHeadersAvailable();
    }
    else
    {
        // Not interested in receiving body for failure response. Notify completion immediately.
        _requestContext.responseOnCompleteInvoked = true;
        (void)_callback.OnComplete(_requestContext.hrTranslatedStatusCode);
    }
}
