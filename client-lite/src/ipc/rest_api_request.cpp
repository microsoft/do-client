#include "do_common.h"
#include "rest_api_request.h"

#include "do_error.h"
#include "do_guid.h"
#include "download.h"
#include "download_manager.h"
#include "string_ops.h"

namespace strconv = docli::string_conversions;

static std::string GetUri(RestApiParser& parser)
{
    return parser.GetStringParam(RestApiParameters::Uri);
}

static std::string GetDownloadFilePath(RestApiParser& parser)
{
    return parser.GetStringParam(RestApiParameters::DownloadFilePath);
}

static std::string GetDownloadId(RestApiParser& parser)
{
    const std::string* str = parser.QueryStringParam(RestApiParameters::Id);
    if (str != nullptr)
    {
        return *str;
    }
    return {};
}

static PCSTR DownloadStateToString(DownloadState state)
{
#define RETURN_DOWNLOAD_STATE_STR(state) \
    case DownloadState::state: return #state;

    switch (state)
    {
    RETURN_DOWNLOAD_STATE_STR(Created);
    RETURN_DOWNLOAD_STATE_STR(Transferring);
    RETURN_DOWNLOAD_STATE_STR(Transferred);
    RETURN_DOWNLOAD_STATE_STR(Finalized);
    RETURN_DOWNLOAD_STATE_STR(Aborted);
    RETURN_DOWNLOAD_STATE_STR(Paused);

    default:
        DO_ASSERT(false);
        return nullptr;
    }
}

// Entry point for request parsing. The only parsing that is done here is to determine the request method type.
// Other parsing, like the query string and request JSON body is done when the Process() method is invoked.
// Process() is called asynchronously to unblock the HTTP listener thread.
RestApiRequestBase::RestApiRequestBase(web::http::http_request clientRequest) :
    _parser(std::move(clientRequest))
{
    switch (_parser.Method())
    {
    case RestApiMethods::Create:    _apiRequest = std::make_unique<RestApiCreateRequest>(); break;
    case RestApiMethods::Enumerate: _apiRequest = std::make_unique<RestApiEnumerateRequest>(); break;

    case RestApiMethods::Start:
    case RestApiMethods::Pause:
    case RestApiMethods::Finalize:
    case RestApiMethods::Abort:
        _apiRequest = std::make_unique<RestApiDownloadStateChangeRequest>();
        break;

    case RestApiMethods::GetStatus:     _apiRequest = std::make_unique<RestApiGetStatusRequest>(); break;
    case RestApiMethods::GetProperty:   _apiRequest = std::make_unique<RestApiGetPropertyRequest>(); break;
    case RestApiMethods::SetProperty:   _apiRequest = std::make_unique<RestApiSetPropertyRequest>(); break;

    default:
        DO_ASSERT(false);
        THROW_HR(E_UNEXPECTED);
    }
}

HRESULT RestApiRequestBase::Process(DownloadManager& downloadManager, web::json::value& responseBody) try
{
    return _apiRequest->ParseAndProcess(downloadManager, _parser, responseBody);
} CATCH_RETURN()

HRESULT RestApiCreateRequest::ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, web::json::value& responseBody)
{
    std::string uri = GetUri(parser);
    std::string filePath = GetDownloadFilePath(parser);
    auto downloadId = downloadManager.CreateDownload(uri, filePath);
    responseBody[RestApiParser::ParamToString(RestApiParameters::Id)] = web::json::value(downloadId);
    return S_OK;
}

HRESULT RestApiEnumerateRequest::ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser,
    web::json::value& responseBody)
{
    std::string filePath = GetDownloadFilePath(parser);
    std::string uri = GetUri(parser);
    DoLogInfo("%s, %s", filePath.data(), uri.data());
    return E_NOTIMPL;
}

HRESULT RestApiDownloadStateChangeRequest::ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser,
    web::json::value& responseBody)
{
    DoLogInfo("Download state change: %d", static_cast<int>(parser.Method()));

    auto downloadId = GetDownloadId(parser);
    switch(parser.Method())
    {
    case RestApiMethods::Start:
        downloadManager.StartDownload(downloadId);
        break;

    case RestApiMethods::Pause:
        downloadManager.PauseDownload(downloadId);
        break;

    case RestApiMethods::Finalize:
        downloadManager.FinalizeDownload(downloadId);
        break;

    case RestApiMethods::Abort:
        downloadManager.AbortDownload(downloadId);
        break;

    default:
        DO_ASSERT(false);
        return E_NOTIMPL;
    }
    return S_OK;
}

HRESULT RestApiGetStatusRequest::ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser,
    web::json::value& responseBody)
{
    DoLogInfo("");
    auto status = downloadManager.GetDownloadStatus(GetDownloadId(parser));
    responseBody["Status"] = web::json::value::string(DownloadStateToString(status.State));
    responseBody["BytesTotal"] = static_cast<uint64_t>(status.BytesTotal);
    responseBody["BytesTransferred"] = static_cast<uint64_t>(status.BytesTransferred);
    responseBody["ErrorCode"] = static_cast<int32_t>(status.Error);
    responseBody["ExtendedErrorCode"] = static_cast<int32_t>(status.ExtendedError);
    return S_OK;
}

HRESULT RestApiGetPropertyRequest::ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser,
    web::json::value& responseBody)
{
    const std::string* propKey = parser.QueryStringParam(RestApiParameters::PropertyKey);
    RETURN_HR_IF_EXPECTED(E_INVALIDARG, (propKey == nullptr) || (propKey->empty()));

    const RestApiParam* param = RestApiParam::Lookup(propKey->data());
    RETURN_HR_IF(DO_E_UNKNOWN_PROPERTY_ID, (param == nullptr) || param->IsUnknownDownloadPropertyId());

    const auto downloadId = GetDownloadId(parser);
    std::string val = downloadManager.GetDownloadProperty(downloadId, param->downloadPropertyId);

    web::json::value propVal;
    switch (param->type)
    {
    case RestApiParamTypes::UInt:
        propVal = web::json::value(strconv::ToUInt(val));
        break;
    case RestApiParamTypes::String:
        propVal = web::json::value(val);
        break;
    default:
        DO_ASSERT(false);
        break;
    }

    responseBody[*propKey] = std::move(propVal);
    return S_OK;
}

HRESULT RestApiSetPropertyRequest::ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser,
    web::json::value& responseBody)
{
    auto downloadId = GetDownloadId(parser);

    std::vector<std::pair<DownloadProperty, std::string>> propertiesToSet;
    const auto& requestBody = parser.Body();
    for (const auto& item : requestBody)
    {
        const RestApiParam* param = item.first;
        RETURN_HR_IF(DO_E_UNKNOWN_PROPERTY_ID, param->IsUnknownDownloadPropertyId());

        const web::json::value& propVal = item.second;

        std::string propValue;
        switch (param->type)
        {
        case RestApiParamTypes::UInt:
            RETURN_HR_IF(E_INVALIDARG, !propVal.as_number().is_uint32());
            propValue = std::to_string(propVal.as_number().to_uint32());
            break;
        case RestApiParamTypes::String:
            propValue = propVal.as_string();
            break;
        default:
            DO_ASSERT(false);
            break;
        }

        DO_ASSERT(!propValue.empty());
        propertiesToSet.emplace_back(param->downloadPropertyId, std::move(propValue));
    }

    for (const auto& prop : propertiesToSet)
    {
        downloadManager.SetDownloadProperty(downloadId, prop.first, prop.second);
    }

    return S_OK;
}
