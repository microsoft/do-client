
#include "download_impl.h"

#include <thread>

#include <cpprest/details/basic_types.h>

#include "do_exceptions_internal.h"
#include "do_exceptions.h"
#include "do_http_client.h"

namespace msdo = microsoft::deliveryoptimization;
namespace cpprest_util_conv = utility::conversions;
using namespace std::chrono_literals; // NOLINT(build/namespaces)

const int g_maxNumRetryAttempts = 3;

namespace microsoft::deliveryoptimization::details
{
CDownloadImpl::CDownloadImpl(const std::string& uri, const std::string& downloadFilePath)
{
    web::uri_builder builder(g_downloadUriPart);
    builder.append_path(U("create"));

    builder.append_query(U("Uri"), cpprest_util_conv::to_string_t(uri));
    builder.append_query(U("DownloadFilePath"), cpprest_util_conv::to_string_t(downloadFilePath));

    for (int retryAttempts = 0; retryAttempts < g_maxNumRetryAttempts; retryAttempts++)
    {
        web::http::http_response resp = CHttpClient::GetInstance().SendRequest(web::http::methods::POST, builder.to_string());
        try
        {
            CHttpClient::HTTPErrorCheck(resp);
            web::json::object respBody = resp.extract_json().get().as_object();
            _id = cpprest_util_conv::to_utf8string(respBody.at(U("Id")).as_string());
            return;
        }
        catch (const msdo::exception& e)
        {
            // Handle DOCS in Shutdown state while Create request was issued, client will restart and this loop will break
            // TODO(jimson): SDK doesn't have the capability to start do-client-lite.service if not running already. Test this
            // code path when it is implemented.
            if (e.error_code() != static_cast<int32_t>(msdo::errc::no_service))
            {
                throw;
            }
            if (retryAttempts < g_maxNumRetryAttempts - 1)
            {
                std::this_thread::sleep_for(1s);
            }
        }
    }
    ThrowException(msdo::errc::no_service);
}

void CDownloadImpl::Start()
{
    _DownloadOperationCall("start");
}

void CDownloadImpl::Pause()
{
    _DownloadOperationCall("pause");
}

void CDownloadImpl::Resume()
{
    _DownloadOperationCall("start");
}

void CDownloadImpl::Finalize()
{
    _DownloadOperationCall("finalize");
}

void CDownloadImpl::Abort()
{
    _DownloadOperationCall("abort");
}

msdo::download_status CDownloadImpl::GetStatus()
{
    web::uri_builder builder(g_downloadUriPart);
    builder.append_path(U("getstatus"));
    builder.append_query(U("Id"), cpprest_util_conv::to_string_t(_id));

    web::http::http_response resp = CHttpClient::GetInstance().SendRequest(web::http::methods::GET, builder.to_string());
    CHttpClient::HTTPErrorCheck(resp);

    const web::json::object respBody = resp.extract_json().get().as_object();

    uint64_t bytesTotal = respBody.at(U("BytesTotal")).as_number().to_uint64();
    uint64_t bytesTransferred = respBody.at(U("BytesTransferred")).as_number().to_uint64();
    int32_t errorCode = respBody.at(U("ErrorCode")).as_number().to_int32();
    int32_t extendedErrorCode = respBody.at(U("ExtendedErrorCode")).as_number().to_int32();

    static const std::map<utility::string_t, download_state> stateMap =
        {{ U("Created"), download_state::created },
        { U("Transferring"), download_state::transferring },
        { U("Transferred"), download_state::transferred },
        { U("Finalized"), download_state::finalized },
        { U("Aborted"), download_state::aborted },
        { U("Paused"), download_state::paused }};

    download_state status = download_state::created;
    auto it = stateMap.find(respBody.at(U("Status")).as_string());
    if (it != stateMap.end())
    {
        status = it->second;
    }
    else
    {
        ThrowException(msdo::errc::unexpected);
    }

    msdo::download_status out(bytesTotal, bytesTransferred, errorCode, extendedErrorCode, status);
    return out;
}

void CDownloadImpl::_DownloadOperationCall(const std::string& type)
{
    web::uri_builder builder(g_downloadUriPart);
    builder.append_path(cpprest_util_conv::to_string_t(type));
    builder.append_query(U("Id"), cpprest_util_conv::to_string_t(_id));

    web::http::http_response resp = CHttpClient::GetInstance().SendRequest(web::http::methods::POST, builder.to_string());

    CHttpClient::HTTPErrorCheck(resp);
}

} // namespace microsoft::deliveryoptimization::details
