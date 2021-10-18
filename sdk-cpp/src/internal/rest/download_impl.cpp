// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "download_impl.h"

#include <map>
#include <thread>

#include "do_cpprest_uri_builder.h"
#include "do_errors.h"
#include "do_exceptions_internal.h"
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
std::error_code CDownloadImpl::Init(const std::string& uri, const std::string& downloadFilePath) noexcept
{
    try
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
                return DO_OK;
            }
            catch (const msdo::exception& e)
            {
                // Handle DOCS in Shutdown state while Create request was issued, client will restart and this loop will break
                // TODO(jimson): SDK doesn't have the capability to start do-client-lite.service if not running already. Test this
                // code path when it is implemented.
                if (e.error_code().value() != static_cast<int>(msdo::errc::no_service))
                {
                    return e.error_code();
                }
                if (retryAttempts < g_maxNumRetryAttempts - 1)
                {
                    std::this_thread::sleep_for(1s);
                }
            }
        }
        return msdo::make_error_code(msdo::errc::no_service);
    }
    catch (msdo::exception& e)
    {
        return e.error_code();
    }
}

std::error_code CDownloadImpl::Start() noexcept
{
    return _DownloadOperationCall("start");
}

std::error_code CDownloadImpl::Pause() noexcept
{
    return _DownloadOperationCall("pause");
}

std::error_code CDownloadImpl::Resume() noexcept
{
    return _DownloadOperationCall("start");
}

std::error_code CDownloadImpl::Finalize() noexcept
{
    return _DownloadOperationCall("finalize");
}

std::error_code CDownloadImpl::Abort() noexcept
{
    return _DownloadOperationCall("abort");
}

std::error_code CDownloadImpl::GetStatus(msdo::download_status& outStatus) noexcept
{
    try
    {
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

        download_state status = download_state::created;
        auto it = stateMap.find(respBody.get<std::string>("Status"));
        if (it != stateMap.end())
        {
            status = it->second;
        }
        else
        {
            ThrowException(msdo::errc::unexpected);
        }

        msdo::download_status out(bytesTotal, bytesTransferred, errorCode, extendedErrorCode, status);
        outStatus = out;
        return DO_OK;
    }
    catch (msdo::exception& e)
    {
        return e.error_code();
    }
}

std::error_code CDownloadImpl::GetProperty(msdo::download_property key, msdo::download_property_value& value) noexcept
{
    return msdo::make_error_code(msdo::errc::e_not_impl);
}

std::error_code CDownloadImpl::SetProperty(msdo::download_property key, const msdo::download_property_value& val) noexcept
{
    return msdo::make_error_code(msdo::errc::e_not_impl);
}

std::error_code CDownloadImpl::SetCallback(const download_property_value::status_callback_t& callback, download& download) noexcept
{
    return msdo::make_error_code(msdo::errc::e_not_impl);
}

std::error_code CDownloadImpl::_DownloadOperationCall(const std::string& type) noexcept
{
    try 
    {
        cpprest_web::uri_builder builder(g_downloadUriPart);
        builder.append_path(type);
        builder.append_query("Id", _id);
        (void)CHttpClient::GetInstance().SendRequest(HttpRequest::POST, builder.to_string());
        return DO_OK;
    }
    catch (msdo::exception& e)
    {
        return msdo::make_error_code(e.error_code().value());
    }

}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
