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
    return parser.GetStringParam(RestApiParameters::Id);
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
RestApiRequestBase::RestApiRequestBase(const std::shared_ptr<microsoft::deliveryoptimization::details::HttpPacket>& clientRequest) :
    _parser(clientRequest)
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

HRESULT RestApiRequestBase::Process(DownloadManager& downloadManager, boost::property_tree::ptree& responseBody) try
{
    return _apiRequest->ParseAndProcess(downloadManager, _parser, responseBody);
} CATCH_RETURN()

HRESULT RestApiCreateRequest::ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser, boost::property_tree::ptree& responseBody)
{
    std::string uri = GetUri(parser);
    std::string filePath = GetDownloadFilePath(parser);
    auto downloadId = downloadManager.CreateDownload(uri, filePath);
    responseBody.put(RestApiParser::ParamToString(RestApiParameters::Id), downloadId);
    return S_OK;
}

HRESULT RestApiEnumerateRequest::ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser,
    boost::property_tree::ptree& responseBody)
{
    std::string filePath = GetDownloadFilePath(parser);
    std::string uri = GetUri(parser);
    DoLogInfo("%s, %s", filePath.data(), uri.data());
    return E_NOTIMPL;
}

HRESULT RestApiDownloadStateChangeRequest::ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser,
    boost::property_tree::ptree& responseBody)
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
    boost::property_tree::ptree& responseBody)
{
    auto status = downloadManager.GetDownloadStatus(GetDownloadId(parser));
    responseBody.put("Status", DownloadStateToString(status.State));
    responseBody.put("BytesTotal", std::to_string(status.BytesTotal));
    responseBody.put("BytesTransferred", std::to_string(status.BytesTransferred));
    responseBody.put("ErrorCode", std::to_string(status.Error));
    responseBody.put("ExtendedErrorCode", std::to_string(status.ExtendedError));
    return S_OK;
}

HRESULT RestApiGetPropertyRequest::ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser,
    boost::property_tree::ptree& responseBody)
{
    const std::string* propKey = parser.QueryStringParam(RestApiParameters::PropertyKey);
    RETURN_HR_IF_EXPECTED(E_INVALIDARG, (propKey == nullptr) || (propKey->empty()));

    const RestApiParam* param = RestApiParam::Lookup(propKey->data());
    RETURN_HR_IF(DO_E_UNKNOWN_PROPERTY_ID, (param == nullptr) || param->IsUnknownDownloadPropertyId());

    const auto downloadId = GetDownloadId(parser);
    std::string val = downloadManager.GetDownloadProperty(downloadId, param->downloadPropertyId);
    responseBody.put(*propKey, std::move(val));
    return S_OK;
}

HRESULT RestApiSetPropertyRequest::ParseAndProcess(DownloadManager& downloadManager, RestApiParser& parser,
    boost::property_tree::ptree& responseBody)
{
    auto downloadId = GetDownloadId(parser);

    std::vector<std::pair<DownloadProperty, std::string>> propertiesToSet;
    const auto& requestQueryParams = parser.Query();
    for (const auto& item : requestQueryParams)
    {
        const RestApiParam* param = item.first;
        RETURN_HR_IF(DO_E_UNKNOWN_PROPERTY_ID, param->IsUnknownDownloadPropertyId());
        propertiesToSet.emplace_back(param->downloadPropertyId, item.second);
    }

    for (const auto& prop : propertiesToSet)
    {
        downloadManager.SetDownloadProperty(downloadId, prop.first, prop.second);
    }

    return S_OK;
}
