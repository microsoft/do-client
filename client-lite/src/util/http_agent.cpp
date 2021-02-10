#include "do_common.h"
#include "http_agent.h"

#include <sstream>

#include <cpprest/http_client.h>
#include <cpprest/http_msg.h>
#include <cpprest/uri.h>

#include <gsl/gsl_util>
#include "safe_int.h"

// TBD version
#define DO_USER_AGENT_STR   "Microsoft-Delivery-Optimization-Lite/10.0.0.1"

HttpAgent::HttpAgent(IHttpAgentEvents& callback) :
    _callback(callback)
{
}

HttpAgent::~HttpAgent()
{
    Close();
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
    if (!web::uri::validate(url))
    {
        return false;
    }

    web::uri uri{url};
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
HRESULT HttpAgent::SendRequest(PCSTR szUrl, PCSTR szProxyUrl, PCSTR szPostData, PCSTR szRange, UINT64 callerContext) try
{
    RETURN_IF_FAILED(_CreateClient(szUrl, szProxyUrl));
    (void)_SubmitRequestTask(_CreateRequest(szPostData, szRange), callerContext);
    return S_OK;
} CATCH_RETURN()

void HttpAgent::Close()
{
    // Cancelling the token notifies any pending/running pplx tasks.
    // Call tracker will then wait for the tasks acknowledge the cancel and/or complete.
    // Too bad the task_group concept from PPL isn't supported in the cpprestsdk's version.
    _cts.cancel();
    _callTracker.Wait();

    // Clients may now make new requests if they choose
    std::unique_lock<std::recursive_mutex> lock(_requestLock);
    _client.reset();
    _cts = pplx::cancellation_token_source();
}

// The Query* functions are supposed to be called only from within the IHttpAgentEvents callbacks
// function because the httpContext (which is the request handle) must be valid.
HRESULT HttpAgent::QueryStatusCode(UINT64 httpContext, _Out_ UINT* pStatusCode) const
{
    auto pResponse = reinterpret_cast<web::http::http_response*>(httpContext);
    *pStatusCode = pResponse->status_code();
    return S_OK;
}

HRESULT HttpAgent::QueryContentLength(UINT64 httpContext, _Out_ UINT64* pContentLength)
{
    auto pResponse = reinterpret_cast<web::http::http_response*>(httpContext);
    *pContentLength = pResponse->headers().content_length();
    return S_OK;
}

HRESULT HttpAgent::QueryContentLengthFromRange(UINT64 httpContext, _Out_ UINT64* pContentLength) try
{
    *pContentLength = 0;

    std::string rangeHeader;
    RETURN_IF_FAILED_EXPECTED(QueryHeadersByType(httpContext, HttpAgentHeaders::Range, rangeHeader));

    // attempt to extract the content length from the content range format:
    // Content-Range: bytes <start>-<end>/<length>
    auto marker = strchr(rangeHeader.data(), L'/');
    RETURN_HR_IF_EXPECTED(E_NOT_SET, (marker == nullptr));
    *pContentLength = std::stoull(marker + 1);
    return S_OK;
} CATCH_RETURN()

HRESULT HttpAgent::QueryHeaders(UINT64 httpContext, PCSTR pszName, std::string& headers) const noexcept
{
    headers.clear();
    auto pResponse = reinterpret_cast<web::http::http_response*>(httpContext);
    const auto& reponseHeaders = pResponse->headers();
    if (pszName == nullptr)
    {
        // Accumulate all headers into the output string
        std::stringstream ss;
        if (!reponseHeaders.empty())
        {
            for (const auto& item : reponseHeaders)
            {
                ss << item.first << ':' << item.second << "\r\n";
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

HRESULT HttpAgent::QueryHeadersByType(UINT64 httpContext, HttpAgentHeaders type, std::string& headers) noexcept
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
    return QueryHeaders(httpContext, headerName, headers);
}

HRESULT HttpAgent::_CreateClient(PCSTR szUrl, PCSTR szProxyUrl) try
{
    std::unique_lock<std::recursive_mutex> lock(_requestLock);

    // We must recreate the client if either the url or the proxy url has changed
    if ((szUrl != nullptr) && _client)
    {
        const std::string currentUrl = _client->base_uri().to_string();
        if (StringCompareCaseInsensitive(currentUrl.data(), szUrl) != 0)
        {
            _client.reset();
        }
    }
    else if ((szProxyUrl != nullptr) && _client)
    {
        const std::string currentProxy = _client->client_config().proxy().address().to_string();
        if (StringCompareCaseInsensitive(currentProxy.data(), szProxyUrl) != 0)
        {
            _client.reset();
        }
    }

    if (!_client)
    {
        RETURN_HR_IF(E_INVALIDARG, (szUrl == nullptr));

        std::string url(szUrl);
        RETURN_HR_IF(INET_E_INVALID_URL, !ValidateUrl(url));

        web::http::client::http_client_config clientConfig;
        _SetWebProxyFromProxyUrl(clientConfig, szProxyUrl);

        web::http::uri remoteUri(url);
        _client = std::make_unique<web::http::client::http_client>(remoteUri, clientConfig);
        DoLogVerbose("New http_client for %s", remoteUri.to_string().data());
    }
    return S_OK;
} CATCH_RETURN()

web::http::http_request HttpAgent::_CreateRequest(_In_opt_ PCSTR szPostData, _In_opt_ PCSTR szRange)
{
    web::http::http_request request;
    if  (szPostData != nullptr)
    {
        request.set_method(web::http::methods::POST);
        request.set_body(utf8string(szPostData));
    }

    web::http::http_headers& headers = request.headers();
    headers["User-Agent"] = DO_USER_AGENT_STR;
    if (szRange != nullptr)
    {
        std::string rangeHeader("bytes=");
        rangeHeader += szRange;
        headers["Range"] = rangeHeader;
    }
    return request;
}

pplx::task<void> HttpAgent::_SubmitRequestTask(const web::http::http_request& request, UINT64 callerContext)
{
    // Note: the tracker is moved to only the last task-based lambda because it
    // will always get executed whereas a value-based lambda can be skipped due
    // to exceptions or cancellations.
    auto cancellationToken = _cts.get_token();
    auto tracker = _callTracker.Enter();
    auto responseHolder = std::make_shared<web::http::http_response>();
    return _client->request(request, cancellationToken).then(
        [this, callerContext, cancellationToken, responseHolder](web::http::http_response response)
        {
            *responseHolder = std::move(response);

            HRESULT hrRequest = _ResultFromStatusCode(responseHolder->status_code());
            if (SUCCEEDED(hrRequest))
            {
                hrRequest = _callback.OnHeadersAvailable(reinterpret_cast<uint64_t>(responseHolder.get()), callerContext);
                if (hrRequest == S_FALSE)
                {
                    hrRequest = E_ABORT;
                }
            }
            THROW_IF_FAILED(hrRequest);

            // Start async loop to read incoming data corresponding to the request
            return _DoReadBodyData(responseHolder->body(), std::make_shared<ReadDataBuffer>(),
                cancellationToken, responseHolder, callerContext);

        }).then([this, callerContext, responseHolder, tracker = std::move(tracker)](pplx::task<void> previousTask)
        {
            HRESULT hr = S_OK;
            try
            {
                previousTask.get(); // check for exceptions
            }
            catch (...)
            {
                hr = LOG_CAUGHT_EXCEPTION();
                DoLogWarningHr(hr, "Url: %s, host: %s", _client->base_uri().to_string().data(), _client->base_uri().host().data());
            }

            // Report success and failure. Ignore cancellations.
            if (hr != E_ABORT)
            {
                (void)_callback.OnComplete(hr, reinterpret_cast<UINT64>(responseHolder.get()), callerContext);
            }
        });
}

pplx::task<void> HttpAgent::_DoReadBodyData(Concurrency::streams::istream bodyStream, const std::shared_ptr<ReadDataBuffer>& bodyStorage,
    pplx::cancellation_token cancellationToken, const std::shared_ptr<web::http::http_response>& response, UINT64 callerContext)
{
    if (cancellationToken.is_canceled())
    {
        pplx::cancel_current_task();
    }

    // Rewind the stream to the beginning for the next read operation
    // TODO(shishirb) check return value and throw
    (void)bodyStorage->streambuf.seekoff(0, std::ios_base::beg, std::ios_base::in|std::ios_base::out);

    return bodyStream.read(bodyStorage->streambuf, bodyStorage->storage.size()).then(
        [this, bodyStream, bodyStorage, cancellationToken, response, callerContext](size_t bytesRead) mutable
        {
            if (bytesRead == 0)
            {
                return pplx::create_task([](){});
            }

            HRESULT hr = _callback.OnData(bodyStorage->storage.data(), gsl::narrow<UINT>(bytesRead), reinterpret_cast<UINT64>(response.get()), callerContext);
            if (hr == S_FALSE)
            {
                hr = E_ABORT;
            }
            THROW_IF_FAILED(hr);

            return _DoReadBodyData(bodyStream, bodyStorage, cancellationToken, response, callerContext);
        });
}

HRESULT HttpAgent::_ResultFromStatusCode(web::http::status_code code)
{
    using status = web::http::status_codes;

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

void HttpAgent::_SetWebProxyFromProxyUrl(web::http::client::http_client_config& config, _In_opt_ PCSTR szProxyUrl)
{
    if (szProxyUrl == nullptr)
    {
        return;
    }

    web::uri_builder proxyFullAddress(szProxyUrl);
    web::credentials creds = [&]()
    {
        // User info will be of the form <user>:<password>.
        // TODO(jimson): Ensure this works even when running as the 'do' user after DropPermissions() is in play.
        auto credStrings = StringPartition(proxyFullAddress.user_info(), ':');
        return (credStrings.size() == 2) ? web::credentials{credStrings[0], credStrings[1]} : web::credentials{};
    }();

    // cpprest does not make use of creds embedded in proxy address. Must set it via web_proxy::set_credentials.
    proxyFullAddress.set_user_info("");

    web::web_proxy proxy(proxyFullAddress.to_uri());
    proxy.set_credentials(std::move(creds));
    config.set_proxy(proxy);
    DoLogInfo("Using proxy %s", config.proxy().address().to_string().data()); // do not log credentials
}
