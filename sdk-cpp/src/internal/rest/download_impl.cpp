
#include "download_impl.h"

#include <map>
#include <thread>

#include "do_cpprest_uri_builder.h"
#include "do_exceptions_internal.h"
#include "do_exceptions.h"
#include "do_http_client.h"

namespace msdo = microsoft::deliveryoptimization;
using namespace std::chrono_literals; // NOLINT(build/namespaces)

const int g_maxNumRetryAttempts = 3;

const char* const g_downloadUriPart = "download";

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{
CDownloadImpl::CDownloadImpl(const std::string& uri, const std::string& downloadFilePath)
{
    _codeInit = _Initialize(uri, downloadFilePath);
}

int32_t CDownloadImpl::_Initialize(const std::string& uri, const std::string& downloadFilePath)
{
    cpprest_web::uri_builder builder(g_downloadUriPart);
    builder.append_path("create");
    builder.append_query("Uri", uri);
    builder.append_query("DownloadFilePath", downloadFilePath);

    for (int retryAttempts = 0; retryAttempts < g_maxNumRetryAttempts; retryAttempts++)
    {
        try
        {
            const auto respBody = CHttpClient::GetInstance().SendRequest(HttpRequest::POST, builder.to_string());
            _id = respBody.get<std::string>("Id");
            return static_cast<int32_t>(msdo::errc:ok);
        }
        catch (const msdo::exception& e)
        {
            // Handle DOCS in Shutdown state while Create request was issued, client will restart and this loop will break
            // TODO(jimson): SDK doesn't have the capability to start do-client-lite.service if not running already. Test this
            // code path when it is implemented.
            if (e.error_code() != static_cast<int32_t>(msdo::errc::no_service))
            {
                return e.error_code();
            }
            if (retryAttempts < g_maxNumRetryAttempts - 1)
            {
                std::this_thread::sleep_for(1s);
            }
        }
    }
    return static_cast<int32_t>(msdo::errc::no_service);
}

int32_t CDownloadImpl::Start()
{
    RETURN_HR_IF_FAILED(_codeInit);
    return _DownloadOperationCall("start");
}

int32_t CDownloadImpl::Pause()
{
    RETURN_HR_IF_FAILED(_codeInit);
    return _DownloadOperationCall("pause");
}

int32_t CDownloadImpl::Resume()
{
    RETURN_HR_IF_FAILED(_codeInit);
    return _DownloadOperationCall("start");
}

int32_t CDownloadImpl::Finalize()
{
    RETURN_HR_IF_FAILED(_codeInit);
    return _DownloadOperationCall("finalize");
}

int32_t CDownloadImpl::Abort()
{
    RETURN_HR_IF_FAILED(_codeInit);
    return _DownloadOperationCall("abort");
}

int32_t CDownloadImpl::GetStatus(msdo::download_status& status)
{
    RETURN_HR_IF_FAILED(_codeInit);
    try {
        cpprest_web::uri_builder builder(g_downloadUriPart);
        builder.append_path("getstatus");
        builder.append_query("Id", _id);

        const auto respBody = CHttpClient::GetInstance().SendRequest(HttpRequest::GET, builder.to_string());

        uint64_t bytesTotal = respBody.get<uint64_t>("BytesTotal");
        uint64_t bytesTransferred = respBody.get<uint64_t>("BytesTransferred");
        int32_t errorCode = respBody.get<int32_t>("ErrorCode");
        int32_t extendedErrorCode = respBody.get<int32_t>("ExtendedErrorCode");

        static const std::map<std::string, download_state> stateMap =
        { { "Created", download_state::created },
        { "Transferring", download_state::transferring },
        { "Transferred", download_state::transferred },
        { "Finalized", download_state::finalized },
        { "Aborted", download_state::aborted },
        { "Paused", download_state::paused } };

        download_state state = download_state::created;
        auto it = stateMap.find(respBody.get<std::string>("Status"));
        if (it != stateMap.end())
        {
            state = it->second;
        }
        else
        {
            return static_cast<int32_t>(msdo::errc::unexpected);
        }

        status = msdo::download_status(bytesTotal, bytesTransferred, errorCode, extendedErrorCode, state);
    }
    catch (const exception& e)
    {
        return e.error_code();
    }

    return 0;
}

int32_t CDownloadImpl::_DownloadOperationCall(const std::string& type)
{
    try
    {
        cpprest_web::uri_builder builder(g_downloadUriPart);
        builder.append_path(type);
        builder.append_query("Id", _id);
        (void)CHttpClient::GetInstance().SendRequest(HttpRequest::POST, builder.to_string());
    }
    catch (const exception& e)
    {
        return e.error_code();
    }
    return 0;
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
