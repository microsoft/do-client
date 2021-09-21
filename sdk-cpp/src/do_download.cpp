#include "do_download.h"

#include <system_error>
#include <cassert>
#include <thread>

#include "do_exceptions.h"
#include "do_exceptions_internal.h"
#include "download_interface.h"
#include "download_impl.h"

namespace msdod = microsoft::deliveryoptimization::details;
using namespace std::chrono_literals; // NOLINT(build/namespaces)

namespace microsoft
{
namespace deliveryoptimization
{

download::download(const std::string& uri, const std::string& downloadFilePath)
{
    _download = std::make_shared<msdod::CDownloadImpl>(uri, downloadFilePath);
}

download::~download() = default;

#if (!DO_DISABLE_EXCEPTIONS)
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
    download oneShotDownload(uri, downloadFilePath);
    oneShotDownload.start_and_wait_until_completion(timeOut);
}

void download::download_url_to_path(const std::string& uri, const std::string& downloadFilePath, const std::atomic_bool& isCancelled, std::chrono::seconds timeOut)
{
    download oneShotDownload(uri, downloadFilePath);
    oneShotDownload.start_and_wait_until_completion(isCancelled, timeOut);
}

void download::start_and_wait_until_completion(const std::atomic_bool& isCancelled, std::chrono::seconds timeOut)
{
    const auto hr = start_and_wait_until_completion_nothrow(isCancelled, timeOut);
    throw_if_fail(hr);
    //throw_if_fail(start_and_wait_until_completion_nothrow(isCancelled, timeOut));
}

// TODO: Implement all headers
#if defined(DO_INTERFACE_COM)
download_property_value download::get_property(download_property prop)
{
    download_property_value value;
    throw_if_fail(_download->GetProperty(prop, value));
    return value;
}
#endif // DO_INTERFACE_COM
#endif //!DO_DISABLE_EXCEPTIONS

int32_t download::start_nothrow()
{
    return _download->Start();
}

int32_t download::pause_nothrow()
{
    return _download->Pause();
}

int32_t download::resume_nothrow()
{
    return _download->Resume();
}

int32_t download::finalize_nothrow()
{
    return _download->Finalize();
}

int32_t download::abort_nothrow()
{
    return _download->Abort();
}

int32_t download::get_status_nothrow(download_status& status)
{
    return _download->GetStatus(status);
}

int32_t download::start_and_wait_until_completion_nothrow(std::chrono::seconds timeOut)
{
    std::atomic_bool isCancelled{ false };
    return start_and_wait_until_completion_nothrow(isCancelled, timeOut);
}

int32_t download::start_and_wait_until_completion_nothrow(const std::atomic_bool& isCancelled, std::chrono::seconds timeOut)
{
    constexpr std::chrono::seconds maxPollTime = 5s;
    std::chrono::milliseconds pollTime = 500ms;
    const auto endTime = std::chrono::system_clock::now() + timeOut;

    RETURN_HR_IF_FAILED(start_nothrow());
    download_status status;

    RETURN_HR_IF_FAILED(get_status_nothrow(status));

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
        RETURN_HR_IF_FAILED(get_status_nothrow(status));
        timedOut = std::chrono::system_clock::now() >= endTime;
    } while ((status.state() == download_state::created || status.state() == download_state::transferring || status.is_transient_error())
        && !timedOut);

    if (status.state() == download_state::transferred)
    {
        RETURN_HR_IF_FAILED(finalize_nothrow());
    }
    else
    {
        RETURN_HR_IF_FAILED(abort_nothrow());
        if (isCancelled)
        {
            return HRESULT_FROM_SYSTEM_ERROR(static_cast<int32_t>(std::errc::operation_canceled));
        }
        else if (timedOut)
        {
            return HRESULT_FROM_SYSTEM_ERROR(static_cast<int32_t>(std::errc::timed_out));
        }
        else if (status.state() == download_state::paused && !status.is_transient_error())
        {
            assert(status.error_code() != 0);
            return status.error_code();
        }
    }
    return 0;
}


int32_t download::download_url_to_path_nothrow(const std::string& uri, const std::string& downloadFilePath, std::chrono::seconds timeOut)
{
    download oneShotDownload(uri, downloadFilePath);
    return oneShotDownload.start_and_wait_until_completion_nothrow(timeOut);
}

int32_t download::download_url_to_path_nothrow(const std::string& uri, const std::string& downloadFilePath, const std::atomic_bool& isCancelled, std::chrono::seconds timeOut)
{
    download oneShotDownload(uri, downloadFilePath);
    return oneShotDownload.start_and_wait_until_completion_nothrow(timeOut);
}

#if defined(DO_INTERFACE_COM)
#if (!DO_DISABLE_EXCEPTIONS)
void download::set_property(download_property prop, const download_property_value& val)
{
    if (prop == download_property::callback_interface)
    {
        download_property_value::status_callback_t userCallback;
        val.as(userCallback);

        throw_if_fail(_download->SetCallback(userCallback, *this));
    }
    else
    {
        throw_if_fail(_download->SetProperty(prop, val));
    }
}
#endif

int32_t download::set_property_nothrow(download_property prop, const download_property_value& val) noexcept
{
    if (prop == download_property::callback_interface)
    {
        download_property_value::status_callback_t userCallback;
        val.as(userCallback);

        return _download->SetCallback(userCallback, *this);
    }
    else
    {
        return _download->SetProperty(prop, val);
    }
}
#endif // COM

} // namespace deliveryoptimization
} // namespace microsoft
