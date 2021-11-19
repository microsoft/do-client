// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "do_download.h"

#include <system_error>
#include <cassert>
#include <thread>

#include "do_errors.h"
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

#if defined(DO_ENABLE_EXCEPTIONS)
download download::make(const std::string& uri, const std::string& downloadFilePath)
{
    download out;
    throw_if_fail(out._download->Init(uri, downloadFilePath));
    return out;
}

void download::start()
{
    throw_if_fail(_download->Start());
}

void download::pause()
{
    throw_if_fail(_download->Pause());
}

void download::resume()
{
    throw_if_fail(_download->Resume());
}

void download::finalize()
{
    throw_if_fail(_download->Finalize());
}

void download::abort()
{
    throw_if_fail(_download->Abort());
}

download_status download::get_status() const
{
    download_status status;
    throw_if_fail(_download->GetStatus(status));

    return status;
}

void download::start_and_wait_until_completion(std::chrono::seconds timeOut)
{
    std::atomic_bool isCancelled{ false };
    start_and_wait_until_completion(isCancelled, timeOut);
}

void download::download_url_to_path(const std::string& uri, const std::string& downloadFilePath, std::chrono::seconds timeOut)
{
    download oneShotDownload = download::make(uri, downloadFilePath);
    oneShotDownload.start_and_wait_until_completion(timeOut);
}

void download::download_url_to_path(const std::string& uri, const std::string& downloadFilePath, const std::atomic_bool& isCancelled, std::chrono::seconds timeOut)
{
    download oneShotDownload = download::make(uri, downloadFilePath);
    oneShotDownload.start_and_wait_until_completion(isCancelled, timeOut);
}

void download::start_and_wait_until_completion(const std::atomic_bool& isCancelled, std::chrono::seconds timeOut)
{
    throw_if_fail(start_and_wait_until_completion_nothrow(isCancelled, timeOut));
}

download_property_value download::get_property(download_property prop)
{
    download_property_value val;
    throw_if_fail(_download->GetProperty(prop, val));
    return val;
}

void download::set_property(download_property prop, const download_property_value& val)
{
    throw_if_fail(set_property_nothrow(prop, val));
}

#endif //DO_ENABLE_EXCEPTIONS

std::error_code download::make_nothrow(const std::string& uri, const std::string& downloadFilePath, download& out) noexcept
{
    download tmp;
    tmp._download = std::make_shared<msdod::CDownloadImpl>();
    std::error_code code = tmp._download->Init(uri, downloadFilePath);
    DO_RETURN_IF_FAILED(code);
    out = tmp;
    return DO_OK;
}

std::error_code download::start_nothrow() noexcept
{
    return _download->Start();
}

std::error_code download::pause_nothrow() noexcept
{
    return _download->Pause();
}

std::error_code download::resume_nothrow() noexcept
{
    return _download->Resume();
}

std::error_code download::finalize_nothrow() noexcept
{
    return _download->Finalize();
}

std::error_code download::abort_nothrow() noexcept
{
    return _download->Abort();
}

std::error_code download::get_status_nothrow(download_status& status) noexcept
{
    return _download->GetStatus(status);
}

std::error_code download::start_and_wait_until_completion_nothrow(std::chrono::seconds timeOut) noexcept
{
    std::atomic_bool isCancelled{ false };
    return start_and_wait_until_completion_nothrow(isCancelled, timeOut);
}

std::error_code download::start_and_wait_until_completion_nothrow(const std::atomic_bool& isCancelled, std::chrono::seconds timeOut) noexcept
{
    constexpr std::chrono::seconds maxPollTime = 5s;
    std::chrono::milliseconds pollTime = 500ms;
    const auto endTime = std::chrono::system_clock::now() + timeOut;

    DO_RETURN_IF_FAILED(start_nothrow());
    download_status status;

    DO_RETURN_IF_FAILED(get_status_nothrow(status));

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
        DO_RETURN_IF_FAILED(get_status_nothrow(status));
        timedOut = std::chrono::system_clock::now() >= endTime;
    } while ((status.state() == download_state::created || status.state() == download_state::transferring || status.is_transient_error())
        && !timedOut);

    if (status.state() == download_state::transferred)
    {
        DO_RETURN_IF_FAILED(finalize_nothrow());
    }
    else
    {
        DO_RETURN_IF_FAILED(abort_nothrow());
        if (isCancelled)
        {
            return microsoft::deliveryoptimization::make_error_code(std::errc::operation_canceled);
        }
        else if (timedOut)
        {
            return microsoft::deliveryoptimization::make_error_code(std::errc::timed_out);
        }
        else if (status.state() == download_state::paused && !status.is_transient_error())
        {
            assert(status.error_code().value() != 0);
            return status.error_code();
        }
    }
    return DO_OK;
}

std::error_code download::download_url_to_path_nothrow(const std::string& uri, const std::string& downloadFilePath, std::chrono::seconds timeOut) noexcept
{
    std::atomic_bool dummy{false};
    return download_url_to_path_nothrow(uri, downloadFilePath, dummy, timeOut);
}

std::error_code download::download_url_to_path_nothrow(const std::string& uri, const std::string& downloadFilePath, const std::atomic_bool& isCancelled, std::chrono::seconds timeOut) noexcept
{
    download oneShotDownload;
    DO_RETURN_IF_FAILED(download::make_nothrow(uri, downloadFilePath, oneShotDownload));
    return oneShotDownload.start_and_wait_until_completion_nothrow(isCancelled, timeOut);
}

std::error_code download::set_property_nothrow(download_property prop, const download_property_value& val) noexcept
{
    if (prop == download_property::callback_interface)
    {
        download_property_value::status_callback_t userCallback;
        DO_RETURN_IF_FAILED(val.as_nothrow(userCallback));

        return _download->SetCallback(userCallback, *this);
    }
    else
    {
        return _download->SetProperty(prop, val);
    }
}

std::error_code download::get_property_nothrow(download_property prop, download_property_value& val) noexcept
{
    return _download->GetProperty(prop, val);
}

} // namespace deliveryoptimization
} // namespace microsoft
