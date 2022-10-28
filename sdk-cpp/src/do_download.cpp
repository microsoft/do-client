// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_download.h"

#include <system_error>
#include <cassert>
#include <thread>

#include "do_errors.h"
#include "do_error_helpers.h"
#include "download_interface.h"
#include "download_impl.h"

namespace msdod = microsoft::deliveryoptimization::details;
using namespace std::chrono_literals; // NOLINT(build/namespaces)

namespace microsoft
{
namespace deliveryoptimization
{

download::download()
{
    _download = std::make_shared<msdod::CDownloadImpl>();
}

download::~download() = default;

std::error_code download::make(const std::string& uri, const std::string& downloadFilePath, std::unique_ptr<download>& out) noexcept
{
    out.reset();
    // using 'new' to access non-public constructor
    std::unique_ptr<download> tmp(new download());
    tmp->_download = std::make_shared<msdod::CDownloadImpl>();
    std::error_code code = tmp->_download->Init(uri, downloadFilePath);
    DO_RETURN_IF_FAILED(code);
    out = std::move(tmp);
    return DO_OK;
}

std::error_code download::start() noexcept
{
    return _download->Start();
}

std::error_code download::pause() noexcept
{
    return _download->Pause();
}

std::error_code download::resume() noexcept
{
    return _download->Resume();
}

std::error_code download::finalize() noexcept
{
    return _download->Finalize();
}

std::error_code download::abort() noexcept
{
    return _download->Abort();
}

std::error_code download::get_status(download_status& status) noexcept
{
    return _download->GetStatus(status);
}

std::error_code download::start_and_wait_until_completion(std::chrono::seconds timeOut) noexcept
{
    std::atomic_bool isCancelled{ false };
    return start_and_wait_until_completion(isCancelled, timeOut);
}

std::error_code download::start_and_wait_until_completion(const std::atomic_bool& isCancelled, std::chrono::seconds timeOut) noexcept
{
    constexpr std::chrono::seconds maxPollTime = 5s;
    std::chrono::milliseconds pollTime = 500ms;
    const auto endTime = std::chrono::system_clock::now() + timeOut;

    DO_RETURN_IF_FAILED(start());
    download_status status;

    DO_RETURN_IF_FAILED(get_status(status));

    bool timedOut = false;
    do
    {
        if (isCancelled)
        {
            break;
        }
        std::this_thread::sleep_for(pollTime);
        if (pollTime < maxPollTime)
        {
            pollTime += 500ms;
        }
        DO_RETURN_IF_FAILED(get_status(status));
        timedOut = std::chrono::system_clock::now() >= endTime;
    } while ((status.state() == download_state::created || status.state() == download_state::transferring || status.is_transient_error())
        && !timedOut);

    if (status.state() == download_state::transferred)
    {
        DO_RETURN_IF_FAILED(finalize());
    }
    else
    {
        DO_RETURN_IF_FAILED(abort());
        if (isCancelled)
        {
            return details::make_error_code(std::errc::operation_canceled);
        }
        else if (timedOut)
        {
            return details::make_error_code(std::errc::timed_out);
        }
        else if (status.state() == download_state::paused && !status.is_transient_error())
        {
            assert(status.error_code().value() != 0);
            return status.error_code();
        }
    }
    return DO_OK;
}

std::error_code download::download_url_to_path(const std::string& uri, const std::string& downloadFilePath, std::chrono::seconds timeOut) noexcept
{
    std::atomic_bool dummy{false};
    return download_url_to_path(uri, downloadFilePath, dummy, timeOut);
}

std::error_code download::download_url_to_path(const std::string& uri, const std::string& downloadFilePath, const std::atomic_bool& isCancelled, std::chrono::seconds timeOut) noexcept
{
    std::unique_ptr<download> oneShotDownload;
    DO_RETURN_IF_FAILED(download::make(uri, downloadFilePath, oneShotDownload));
    return oneShotDownload->start_and_wait_until_completion(isCancelled, timeOut);
}

static std::error_code g_TryOverrideDownlevelOsSetPropertyError(download_property prop, std::error_code ec)
{
    // Temporary backward-compatibility for MSEdge.
    // These properties were not supported in IDODownload interface until build 19041.
    if ((ec.value() == static_cast<int>(errc::do_e_unknown_property_id)) &&
        ((prop == download_property::correlation_vector) || (prop == download_property::integrity_check_info)))
    {
        return DO_OK;
    }
    else
    {
        return ec;
    }
}

std::error_code download::set_property(download_property prop, const download_property_value& val) noexcept
{
    if (prop == download_property::callback_interface)
    {
        download_property_value::status_callback_t userCallback;
        DO_RETURN_IF_FAILED(val.as(userCallback));

        return _download->SetCallback(userCallback, *this);
    }
    else
    {
        auto ec = _download->SetProperty(prop, val);
        return g_TryOverrideDownlevelOsSetPropertyError(prop, ec);
    }
}

std::error_code download::get_property(download_property prop, download_property_value& val) noexcept
{
    return _download->GetProperty(prop, val);
}

} // namespace deliveryoptimization
} // namespace microsoft
